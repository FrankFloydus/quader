/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
#include "crimson/gpu/gpu_overlay_renderer.hpp"

#include "crimson/gpu/gpu_handles.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string>

#include <bgfx/bgfx.h>
#include <bx/math.h>

namespace crimson::gpu {
namespace {

struct GridVertex {
	float x;
	float y;
	float z;
};

struct OverlayPositionVertex {
	float x;
	float y;
	float z;
};

struct OverlayCameraBasis {
	quader::math::Vec3 right{ -1.0F, 0.0F, 0.0F };
	quader::math::Vec3 up{ 0.0F, 1.0F, 0.0F };
	quader::math::Vec3 view{ 0.0F, 0.0F, -1.0F };
};

const GridVertex kGridVertices[] = {
	{ -0.5F, 0.0F, -0.5F },
	{ 0.5F, 0.0F, -0.5F },
	{ 0.5F, 0.0F, 0.5F },
	{ -0.5F, 0.0F, 0.5F },
};

const std::uint16_t kGridIndices[] = {
	0,
	2,
	1,
	0,
	3,
	2,
};

constexpr float kOverlayEpsilon = 0.000001F;
constexpr float kPi = 3.14159265358979323846F;
constexpr std::uint32_t kVerticesPerQuad = 4U;
constexpr std::uint32_t kIndicesPerQuad = 6U;
constexpr std::uint32_t kMaxTransientQuads16 =
		std::numeric_limits<std::uint16_t>::max() / kVerticesPerQuad;

void push_overlay_resource_error(RendererStatus &status, std::string resource_name) {
	status.diagnostics.push_back(RendererDiagnostic{
			.severity = RendererDiagnosticSeverity::Error,
			.code = RendererDiagnosticCode::ResourceCreationFailed,
			.message = "Crimson failed to create overlay GPU resources.",
			.subsystem = "gpu.overlay",
			.resource_name = std::move(resource_name),
	});
}

[[nodiscard]] quader::math::Vec3 normalized_or(
		quader::math::Vec3 value,
		quader::math::Vec3 fallback) noexcept {
	const quader::math::Vec3 kNormalized = quader::math::normalized(value);
	return quader::math::length_squared(kNormalized) <= kOverlayEpsilon * kOverlayEpsilon
			? fallback
			: kNormalized;
}

[[nodiscard]] float degrees_to_radians(float degrees) noexcept {
	return degrees * (kPi / 180.0F);
}

[[nodiscard]] OverlayCameraBasis camera_basis(const RenderCamera &camera) noexcept {
	const quader::math::Vec3 kFallbackForward =
			normalized_or(camera.forward, { 0.0F, 0.0F, -1.0F });
	const quader::math::Vec3 kView = normalized_or(camera.target - camera.eye, kFallbackForward);
	const quader::math::Vec3 kRight = normalized_or(
			quader::math::cross(camera.up, kView),
			{ -1.0F, 0.0F, 0.0F });
	return OverlayCameraBasis{
		.right = kRight,
		.up = normalized_or(quader::math::cross(kView, kRight), { 0.0F, 1.0F, 0.0F }),
		.view = kView,
	};
}

[[nodiscard]] float world_units_per_pixel(
		const RenderView &view,
		const OverlayCameraBasis &basis,
		quader::math::Vec3 world_position) noexcept {
	const float kViewportHeightPx = std::max(1.0F, static_cast<float>(view.rect.height));
	if (view.camera.projection == CameraProjection::Orthographic) {
		return std::max(0.01F, view.camera.orthographic_height_m) / kViewportHeightPx;
	}

	const float kForwardDistance = quader::math::dot(world_position - view.camera.eye, basis.view);
	const float kDistance = std::max(std::max(0.001F, view.camera.near_plane_m), kForwardDistance);
	const float kFovRadians = degrees_to_radians(std::clamp(view.camera.vertical_fov_degrees, 1.0F, 179.0F));
	return (2.0F * std::tan(kFovRadians * 0.5F) * kDistance) / kViewportHeightPx;
}

[[nodiscard]] bool point_needs_component_depth_bias(
		OverlaySemanticRole role,
		OverlaySourceKind source_kind) noexcept {
	const bool kSelectedOrHoverVertex = role == OverlaySemanticRole::SelectedVertex ||
			role == OverlaySemanticRole::HoverVertex;
	const bool kComponentSource = source_kind == OverlaySourceKind::ComponentSelection ||
			source_kind == OverlaySourceKind::ComponentHover;
	return kSelectedOrHoverVertex && kComponentSource;
}

[[nodiscard]] std::uint32_t available_quad_count(
		std::size_t requested_quad_count,
		const bgfx::VertexLayout &layout) noexcept {
	const std::uint32_t kRequested = static_cast<std::uint32_t>(std::min<std::size_t>(
			requested_quad_count,
			kMaxTransientQuads16));
	if (kRequested == 0U) {
		return 0;
	}

	const std::uint32_t kRequestedVertices = kRequested * kVerticesPerQuad;
	const std::uint32_t kRequestedIndices = kRequested * kIndicesPerQuad;
	const std::uint32_t kAvailableVertices =
			bgfx::getAvailTransientVertexBuffer(kRequestedVertices, layout);
	const std::uint32_t kAvailableIndices =
			bgfx::getAvailTransientIndexBuffer(kRequestedIndices);
	return std::min({
			kRequested,
			kAvailableVertices / kVerticesPerQuad,
			kAvailableIndices / kIndicesPerQuad,
	});
}

void append_position_vertex(
		OverlayPositionVertex *vertices,
		std::uint32_t &vertex_index,
		quader::math::Vec3 position) noexcept {
	vertices[vertex_index++] = OverlayPositionVertex{ position.x, position.y, position.z };
}

void append_quad(
		OverlayPositionVertex *vertices,
		std::uint16_t *indices,
		std::uint32_t &vertex_index,
		std::uint32_t &index_index,
		quader::math::Vec3 a,
		quader::math::Vec3 b,
		quader::math::Vec3 c,
		quader::math::Vec3 d) noexcept {
	const auto kBase = static_cast<std::uint16_t>(vertex_index);
	append_position_vertex(vertices, vertex_index, a);
	append_position_vertex(vertices, vertex_index, b);
	append_position_vertex(vertices, vertex_index, c);
	append_position_vertex(vertices, vertex_index, d);

	indices[index_index++] = kBase;
	indices[index_index++] = static_cast<std::uint16_t>(kBase + 1U);
	indices[index_index++] = static_cast<std::uint16_t>(kBase + 2U);
	indices[index_index++] = kBase;
	indices[index_index++] = static_cast<std::uint16_t>(kBase + 2U);
	indices[index_index++] = static_cast<std::uint16_t>(kBase + 3U);
}

[[nodiscard]] std::uint64_t depth_test_state_for(
		const PreparedOverlayRenderState &render_state) noexcept {
	if (!render_state.depth_test_enabled) {
		return 0;
	}

	if (render_state.equal_depth_test_enabled) {
		return BGFX_STATE_DEPTH_TEST_EQUAL;
	}

	return BGFX_STATE_DEPTH_TEST_LEQUAL;
}

[[nodiscard]] std::uint64_t state_for_overlay_render_state(
		const PreparedOverlayRenderState &render_state,
		std::uint64_t primitive_state = 0) noexcept {
	std::uint64_t state = primitive_state | BGFX_STATE_MSAA;
	if (render_state.color_write_enabled) {
		state |= BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A;
		if (render_state.pass_kind != PreparedOverlayPassKind::DepthStamp) {
			state |= BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_INV_SRC_ALPHA);
		}
	}
	if (render_state.depth_write_enabled) {
		state |= BGFX_STATE_WRITE_Z;
	}
	state |= depth_test_state_for(render_state);
	return state;
}

[[nodiscard]] std::uint32_t clamped_vertex_count(std::size_t count) noexcept {
	return static_cast<std::uint32_t>(std::min<std::size_t>(
			count,
			std::numeric_limits<std::uint32_t>::max()));
}

} // namespace

struct GpuOverlayRenderer::Impl {
	bgfx::VertexLayout grid_vertex_layout{};
	bgfx::VertexLayout line_vertex_layout{};
	UniqueVertexBufferHandle grid_vertex_buffer;
	UniqueIndexBufferHandle grid_index_buffer;
	UniqueUniformHandle line_color_uniform;
	UniqueUniformHandle grid_plane_size_uniform;
	UniqueUniformHandle grid_color_uniform;
	UniqueUniformHandle major_grid_color_uniform;
	UniqueUniformHandle origin_u_color_uniform;
	UniqueUniformHandle origin_v_color_uniform;
	UniqueUniformHandle camera_position_uniform;
	UniqueUniformHandle grid_u_axis_uniform;
	UniqueUniformHandle grid_v_axis_uniform;
	UniqueUniformHandle grid_params0_uniform;
	UniqueUniformHandle grid_params1_uniform;
	UniqueUniformHandle grid_params2_uniform;
	bool ready = false;
};

GpuOverlayRenderer::GpuOverlayRenderer() : impl_(std::make_unique<Impl>()) {
}

GpuOverlayRenderer::~GpuOverlayRenderer() = default;

bool GpuOverlayRenderer::initialize(RendererStatus &status) {
	shutdown();

	impl_->grid_vertex_layout
			.begin()
			.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
			.end();
	impl_->line_vertex_layout
			.begin()
			.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
			.end();
	impl_->grid_vertex_buffer.reset(bgfx::createVertexBuffer(
			bgfx::makeRef(kGridVertices, sizeof(kGridVertices)),
			impl_->grid_vertex_layout));
	impl_->grid_index_buffer.reset(bgfx::createIndexBuffer(bgfx::makeRef(kGridIndices, sizeof(kGridIndices))));
	impl_->line_color_uniform.reset(bgfx::createUniform("u_lineColor", bgfx::UniformType::Vec4));
	impl_->grid_plane_size_uniform.reset(bgfx::createUniform("u_gridPlaneSize", bgfx::UniformType::Vec4));
	impl_->grid_color_uniform.reset(bgfx::createUniform("u_gridColor", bgfx::UniformType::Vec4));
	impl_->major_grid_color_uniform.reset(bgfx::createUniform("u_majorGridColor", bgfx::UniformType::Vec4));
	impl_->origin_u_color_uniform.reset(bgfx::createUniform("u_originUColor", bgfx::UniformType::Vec4));
	impl_->origin_v_color_uniform.reset(bgfx::createUniform("u_originVColor", bgfx::UniformType::Vec4));
	impl_->camera_position_uniform.reset(bgfx::createUniform("u_cameraPosition", bgfx::UniformType::Vec4));
	impl_->grid_u_axis_uniform.reset(bgfx::createUniform("u_gridUAxis", bgfx::UniformType::Vec4));
	impl_->grid_v_axis_uniform.reset(bgfx::createUniform("u_gridVAxis", bgfx::UniformType::Vec4));
	impl_->grid_params0_uniform.reset(bgfx::createUniform("u_gridParams0", bgfx::UniformType::Vec4));
	impl_->grid_params1_uniform.reset(bgfx::createUniform("u_gridParams1", bgfx::UniformType::Vec4));
	impl_->grid_params2_uniform.reset(bgfx::createUniform("u_gridParams2", bgfx::UniformType::Vec4));

	impl_->ready = impl_->grid_vertex_buffer.valid() && impl_->grid_index_buffer.valid() && impl_->line_color_uniform.valid() && impl_->grid_plane_size_uniform.valid() && impl_->grid_color_uniform.valid() && impl_->major_grid_color_uniform.valid() && impl_->origin_u_color_uniform.valid() && impl_->origin_v_color_uniform.valid() && impl_->camera_position_uniform.valid() && impl_->grid_u_axis_uniform.valid() && impl_->grid_v_axis_uniform.valid() && impl_->grid_params0_uniform.valid() && impl_->grid_params1_uniform.valid() && impl_->grid_params2_uniform.valid();
	if (!impl_->ready) {
		push_overlay_resource_error(status, "GpuOverlayRenderer");
		shutdown();
		return false;
	}

	return true;
}

void GpuOverlayRenderer::shutdown() noexcept {
	impl_->grid_vertex_buffer.reset();
	impl_->grid_index_buffer.reset();
	impl_->line_color_uniform.reset();
	impl_->grid_plane_size_uniform.reset();
	impl_->grid_color_uniform.reset();
	impl_->major_grid_color_uniform.reset();
	impl_->origin_u_color_uniform.reset();
	impl_->origin_v_color_uniform.reset();
	impl_->camera_position_uniform.reset();
	impl_->grid_u_axis_uniform.reset();
	impl_->grid_v_axis_uniform.reset();
	impl_->grid_params0_uniform.reset();
	impl_->grid_params1_uniform.reset();
	impl_->grid_params2_uniform.reset();
	impl_->ready = false;
}

bool GpuOverlayRenderer::ready() const noexcept {
	return impl_->ready;
}

std::uint32_t GpuOverlayRenderer::submit_grid(
		bgfx::ViewId view_id,
		const RenderView &view,
		const PreparedGridOverlayCommand &prepared,
		bgfx::ProgramHandle program) const noexcept {
	if (!impl_->ready || !bgfx::isValid(program)) {
		return 0;
	}

	const GridOverlayCommand &grid = prepared.grid;
	float transform[16];
	bx::mtxSRT(
			transform,
			grid.plane_width,
			1.0F,
			grid.plane_height,
			grid.rotation_x,
			grid.rotation_y,
			grid.rotation_z,
			grid.origin.x,
			grid.origin.y,
			grid.origin.z);

	const float kPlaneSize[4] = { grid.plane_width, grid.plane_height, 0.0F, 0.0F };
	const float kCameraPosition[4] = { view.camera.eye.x, view.camera.eye.y, view.camera.eye.z, 0.0F };
	const float kGridUAxis[4] = { grid.u_axis.x, grid.u_axis.y, grid.u_axis.z, 0.0F };
	const float kGridVAxis[4] = { grid.v_axis.x, grid.v_axis.y, grid.v_axis.z, 0.0F };
	const float kParams0[4] = {
		grid.minor_spacing_m,
		std::max(grid.major_spacing_m, grid.minor_spacing_m),
		grid.minor_line_scale,
		grid.major_line_scale,
	};
	const float kParams1[4] = {
		grid.axis_line_scale,
		grid.fade_start_m,
		grid.fade_end_m,
		std::max(grid.plane_width, grid.plane_height) * 0.5F,
	};
	const float kParams2[4] = {
		std::max(0.0001F, grid.orthographic_height_m),
		std::max(1.0F, grid.viewport_height_px),
		grid.minor_spacing_m,
		grid.edge_softness_m,
	};

	bgfx::setTransform(transform);
	bgfx::setUniform(impl_->grid_plane_size_uniform.get(), kPlaneSize);
	bgfx::setUniform(impl_->grid_color_uniform.get(), prepared.minor_color_linear_sdr.data());
	bgfx::setUniform(impl_->major_grid_color_uniform.get(), prepared.major_color_linear_sdr.data());
	bgfx::setUniform(impl_->origin_u_color_uniform.get(), prepared.axis_u_color_linear_sdr.data());
	bgfx::setUniform(impl_->origin_v_color_uniform.get(), prepared.axis_v_color_linear_sdr.data());
	bgfx::setUniform(impl_->camera_position_uniform.get(), kCameraPosition);
	bgfx::setUniform(impl_->grid_u_axis_uniform.get(), kGridUAxis);
	bgfx::setUniform(impl_->grid_v_axis_uniform.get(), kGridVAxis);
	bgfx::setUniform(impl_->grid_params0_uniform.get(), kParams0);
	bgfx::setUniform(impl_->grid_params1_uniform.get(), kParams1);
	bgfx::setUniform(impl_->grid_params2_uniform.get(), kParams2);
	bgfx::setVertexBuffer(0, impl_->grid_vertex_buffer.get());
	bgfx::setIndexBuffer(impl_->grid_index_buffer.get());
	std::uint64_t state = BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_MSAA | BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_ONE, BGFX_STATE_BLEND_INV_SRC_ALPHA);
	if (prepared.command.depth_mode != OverlayDepthMode::AlwaysOnTop) {
		state |= BGFX_STATE_DEPTH_TEST_LESS;
	}
	bgfx::setState(state);
	bgfx::submit(view_id, program);
	return 1;
}

std::uint32_t GpuOverlayRenderer::submit_lines(
		bgfx::ViewId view_id,
		const RenderView &view,
		const PreparedLineOverlayCommand &lines,
		bgfx::ProgramHandle program) const noexcept {
	if (!impl_->ready || !bgfx::isValid(program) || lines.segments.empty()) {
		return 0;
	}

	const std::uint32_t kQuadCount = available_quad_count(lines.segments.size(), impl_->line_vertex_layout);
	if (kQuadCount == 0U) {
		return 0;
	}

	const std::uint32_t kVertexCount = kQuadCount * kVerticesPerQuad;
	const std::uint32_t kIndexCount = kQuadCount * kIndicesPerQuad;
	bgfx::TransientVertexBuffer vertices{};
	bgfx::TransientIndexBuffer indices{};
	bgfx::allocTransientVertexBuffer(&vertices, kVertexCount, impl_->line_vertex_layout);
	bgfx::allocTransientIndexBuffer(&indices, kIndexCount);
	auto *data = reinterpret_cast<OverlayPositionVertex *>(vertices.data);
	auto *index_data = reinterpret_cast<std::uint16_t *>(indices.data);
	std::uint32_t vertex_index = 0;
	std::uint32_t index_index = 0;
	const OverlayCameraBasis kBasis = camera_basis(view.camera);
	const float kThicknessPx = std::max(1.0F, lines.command.thickness_px);
	for (const LineOverlaySegment &segment : lines.segments) {
		if (vertex_index + kVerticesPerQuad > kVertexCount ||
				index_index + kIndicesPerQuad > kIndexCount) {
			break;
		}

		const quader::math::Vec3 kSegment = segment.end - segment.start;
		if (quader::math::length_squared(kSegment) <= kOverlayEpsilon * kOverlayEpsilon) {
			continue;
		}

		const quader::math::Vec3 kDirection = normalized_or(kSegment, kBasis.right);
		const quader::math::Vec3 kOffsetDirection =
				normalized_or(quader::math::cross(kBasis.view, kDirection), kBasis.up);
		const quader::math::Vec3 kMidpoint = (segment.start + segment.end) * 0.5F;
		const float kHalfWidthWorld =
				kThicknessPx * world_units_per_pixel(view, kBasis, kMidpoint) * 0.5F;
		const quader::math::Vec3 kOffset = kOffsetDirection * kHalfWidthWorld;
		append_quad(data,
				index_data,
				vertex_index,
				index_index,
				segment.start - kOffset,
				segment.start + kOffset,
				segment.end + kOffset,
				segment.end - kOffset);
	}

	if (vertex_index == 0U || index_index == 0U) {
		return 0;
	}

	float transform[16];
	bx::mtxIdentity(transform);
	const std::uint64_t kState = state_for_overlay_render_state(lines.render_state);

	bgfx::setTransform(transform);
	bgfx::setUniform(impl_->line_color_uniform.get(), lines.color_linear_sdr.data());
	bgfx::setVertexBuffer(0, &vertices, 0, vertex_index);
	bgfx::setIndexBuffer(&indices, 0, index_index);
	bgfx::setState(kState);
	bgfx::submit(view_id, program);
	return 1;
}

std::uint32_t GpuOverlayRenderer::submit_triangles(
		bgfx::ViewId view_id,
		const PreparedTriangleOverlayCommand &triangles,
		bgfx::ProgramHandle program) const noexcept {
	if (!impl_->ready || !bgfx::isValid(program) || triangles.triangles.empty()) {
		return 0;
	}

	const std::uint32_t kVertexCount = clamped_vertex_count(triangles.triangles.size() * 3U);
	if (kVertexCount < 3U || bgfx::getAvailTransientVertexBuffer(kVertexCount, impl_->line_vertex_layout) < kVertexCount) {
		return 0;
	}

	bgfx::TransientVertexBuffer vertices{};
	bgfx::allocTransientVertexBuffer(&vertices, kVertexCount, impl_->line_vertex_layout);
	auto *data = reinterpret_cast<OverlayPositionVertex *>(vertices.data);
	std::uint32_t vertex_index = 0;
	for (const TriangleOverlayPrimitive &triangle : triangles.triangles) {
		if (vertex_index + 2U >= kVertexCount) {
			break;
		}
		data[vertex_index++] = OverlayPositionVertex{ triangle.a.x, triangle.a.y, triangle.a.z };
		data[vertex_index++] = OverlayPositionVertex{ triangle.b.x, triangle.b.y, triangle.b.z };
		data[vertex_index++] = OverlayPositionVertex{ triangle.c.x, triangle.c.y, triangle.c.z };
	}

	float transform[16];
	bx::mtxIdentity(transform);
	const std::uint64_t kState = state_for_overlay_render_state(triangles.render_state);

	bgfx::setTransform(transform);
	bgfx::setUniform(impl_->line_color_uniform.get(), triangles.color_linear_sdr.data());
	bgfx::setVertexBuffer(0, &vertices, 0, vertex_index);
	bgfx::setState(kState);
	bgfx::submit(view_id, program);
	return 1;
}

std::uint32_t GpuOverlayRenderer::submit_points(
		bgfx::ViewId view_id,
		const RenderView &view,
		const PreparedPointOverlayCommand &points,
		bgfx::ProgramHandle program) const noexcept {
	if (!impl_->ready || !bgfx::isValid(program) || points.points.empty()) {
		return 0;
	}

	const std::uint32_t kQuadCount = available_quad_count(points.points.size(), impl_->line_vertex_layout);
	if (kQuadCount == 0U) {
		return 0;
	}

	const std::uint32_t kVertexCount = kQuadCount * kVerticesPerQuad;
	const std::uint32_t kIndexCount = kQuadCount * kIndicesPerQuad;
	bgfx::TransientVertexBuffer vertices{};
	bgfx::TransientIndexBuffer indices{};
	bgfx::allocTransientVertexBuffer(&vertices, kVertexCount, impl_->line_vertex_layout);
	bgfx::allocTransientIndexBuffer(&indices, kIndexCount);
	auto *data = reinterpret_cast<OverlayPositionVertex *>(vertices.data);
	auto *index_data = reinterpret_cast<std::uint16_t *>(indices.data);
	std::uint32_t vertex_index = 0;
	std::uint32_t index_index = 0;
	const OverlayCameraBasis kBasis = camera_basis(view.camera);
	for (const PointOverlayPrimitive &point : points.points) {
		if (vertex_index + kVerticesPerQuad > kVertexCount ||
				index_index + kIndicesPerQuad > kIndexCount) {
			break;
		}

		const float kSizePx = std::max(1.0F, point.size_px > 0.0F ? point.size_px : points.size_px);
		const OverlaySemanticRole kPointRole = point.semantic_role == OverlaySemanticRole::Generic
				? points.semantic_role
				: point.semantic_role;
		const OverlaySourceKind kPointSource = point.source_kind == OverlaySourceKind::Unknown
				? points.source_kind
				: point.source_kind;
		const float kHalfSizeWorld =
				kSizePx * world_units_per_pixel(view, kBasis, point.position) * 0.5F;
		const float kDepthBiasWorld = point_needs_component_depth_bias(kPointRole, kPointSource)
				? world_units_per_pixel(view, kBasis, point.position) * 1.5F
				: 0.0F;
		const quader::math::Vec3 kPosition = point.position - kBasis.view * kDepthBiasWorld;
		const quader::math::Vec3 kRight = kBasis.right * kHalfSizeWorld;
		const quader::math::Vec3 kUp = kBasis.up * kHalfSizeWorld;
		append_quad(data,
				index_data,
				vertex_index,
				index_index,
				kPosition - kRight - kUp,
				kPosition + kRight - kUp,
				kPosition + kRight + kUp,
				kPosition - kRight + kUp);
	}

	if (vertex_index == 0U || index_index == 0U) {
		return 0;
	}

	float transform[16];
	bx::mtxIdentity(transform);
	const std::uint64_t kState = state_for_overlay_render_state(points.render_state);

	bgfx::setTransform(transform);
	bgfx::setUniform(impl_->line_color_uniform.get(), points.color_linear_sdr.data());
	bgfx::setVertexBuffer(0, &vertices, 0, vertex_index);
	bgfx::setIndexBuffer(&indices, 0, index_index);
	bgfx::setState(kState);
	bgfx::submit(view_id, program);
	return 1;
}

} // namespace crimson::gpu
