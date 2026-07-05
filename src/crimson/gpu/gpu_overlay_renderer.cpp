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

struct LineVertex {
	float x;
	float y;
	float z;
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

void push_overlay_resource_error(RendererStatus &status, std::string resource_name) {
	status.diagnostics.push_back(RendererDiagnostic{
			.severity = RendererDiagnosticSeverity::Error,
			.code = RendererDiagnosticCode::ResourceCreationFailed,
			.message = "Crimson failed to create overlay GPU resources.",
			.subsystem = "gpu.overlay",
			.resource_name = std::move(resource_name),
	});
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
	bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_DEPTH_TEST_LESS | BGFX_STATE_MSAA | BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_ONE, BGFX_STATE_BLEND_INV_SRC_ALPHA));
	bgfx::submit(view_id, program);
	return 1;
}

std::uint32_t GpuOverlayRenderer::submit_lines(
		bgfx::ViewId view_id,
		const PreparedLineOverlayCommand &lines,
		bgfx::ProgramHandle program) const noexcept {
	if (!impl_->ready || !bgfx::isValid(program) || lines.segments.empty()) {
		return 0;
	}

	const std::uint32_t kVertexCount = static_cast<std::uint32_t>(std::min<std::size_t>(
			lines.segments.size() * 2U,
			std::numeric_limits<std::uint32_t>::max()));
	if (kVertexCount < 2U || bgfx::getAvailTransientVertexBuffer(kVertexCount, impl_->line_vertex_layout) < kVertexCount) {
		return 0;
	}

	bgfx::TransientVertexBuffer vertices{};
	bgfx::allocTransientVertexBuffer(&vertices, kVertexCount, impl_->line_vertex_layout);
	auto *data = reinterpret_cast<LineVertex *>(vertices.data);
	std::uint32_t vertex_index = 0;
	for (const LineOverlaySegment &segment : lines.segments) {
		if (vertex_index + 1U >= kVertexCount) {
			break;
		}
		data[vertex_index++] = LineVertex{ segment.start.x, segment.start.y, segment.start.z };
		data[vertex_index++] = LineVertex{ segment.end.x, segment.end.y, segment.end.z };
	}

	float transform[16];
	bx::mtxIdentity(transform);
	std::uint64_t state = BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_PT_LINES | BGFX_STATE_MSAA | BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_INV_SRC_ALPHA);
	if (lines.command.depth_mode == OverlayDepthMode::DepthTested) {
		state |= BGFX_STATE_DEPTH_TEST_LESS;
	} else if (lines.command.depth_mode == OverlayDepthMode::XRay) {
		state |= BGFX_STATE_DEPTH_TEST_LEQUAL;
	}

	bgfx::setTransform(transform);
	bgfx::setUniform(impl_->line_color_uniform.get(), lines.color_linear_sdr.data());
	bgfx::setVertexBuffer(0, &vertices, 0, vertex_index);
	bgfx::setState(state);
	bgfx::submit(view_id, program);
	return 1;
}

} // namespace crimson::gpu
