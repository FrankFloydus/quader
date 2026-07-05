/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
#include "ui/viewport/crimson_viewport_render_host.hpp"

#include "crimson/frame/frame_builder.hpp"
#include "crimson/renderer.hpp"
#include "document/document.hpp"
#include "geometry/normals.hpp"
#include "math/aabb.hpp"
#include "math/scalar.hpp"
#include "mesh/core/mesh_traversal.hpp"
#include "tools/tool_manager.hpp"
#include "tools/tool_preview.hpp"
#include "ui/viewport/tool_preview_overlay_adapter.hpp"

#include <QCoreApplication>

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

namespace quader::ui {
namespace {

[[nodiscard]] std::uint32_t positive_u32(int value) noexcept {
	return static_cast<std::uint32_t>(std::max(1, value));
}

[[nodiscard]] std::uint16_t positive_u16(int value) noexcept {
	return static_cast<std::uint16_t>(std::clamp(value, 1, 65535));
}

[[nodiscard]] std::uint16_t coordinate_u16(int value) noexcept {
	return static_cast<std::uint16_t>(std::clamp(value, 0, 65535));
}

[[nodiscard]] const char *view_name_for_pane(const ViewportPane &pane) noexcept {
	switch (pane.camera_index) {
		case 0:
			return "Perspective";
		case 1:
			return "Top";
		case 2:
			return "Front";
		case 3:
			return "Right";
		default:
			return "Viewport";
	}
}

[[nodiscard]] crimson::PrototypeCameraProjection projection_from(CameraProjection projection) noexcept {
	return projection == CameraProjection::Orthographic
			? crimson::PrototypeCameraProjection::Orthographic
			: crimson::PrototypeCameraProjection::Perspective;
}

[[nodiscard]] ViewportPickElementKind viewport_kind_from(crimson::PickingElementKind kind) noexcept {
	switch (kind) {
		case crimson::PickingElementKind::None:
			return ViewportPickElementKind::None;
		case crimson::PickingElementKind::Object:
			return ViewportPickElementKind::Object;
		case crimson::PickingElementKind::Submesh:
			return ViewportPickElementKind::Submesh;
		case crimson::PickingElementKind::Face:
			return ViewportPickElementKind::Face;
		case crimson::PickingElementKind::Edge:
			return ViewportPickElementKind::Edge;
		case crimson::PickingElementKind::Vertex:
			return ViewportPickElementKind::Vertex;
	}
	return ViewportPickElementKind::None;
}

[[nodiscard]] QString qstring_from_ascii(std::string_view value) {
	return QString::fromUtf8(value.data(), static_cast<qsizetype>(value.size()));
}

[[nodiscard]] int bounded_int(std::uint32_t value) noexcept {
	return value > static_cast<std::uint32_t>(std::numeric_limits<int>::max())
			? std::numeric_limits<int>::max()
			: static_cast<int>(value);
}

constexpr float kPi = 3.14159265358979323846F;

[[nodiscard]] float radians_from_degrees(float degrees) noexcept {
	return degrees * kPi / 180.0F;
}

[[nodiscard]] quader::math::Vec3 rotate_x(quader::math::Vec3 value, float radians) noexcept {
	const float kSin = std::sin(radians);
	const float kCos = std::cos(radians);
	return { value.x, value.y * kCos - value.z * kSin, value.y * kSin + value.z * kCos };
}

[[nodiscard]] quader::math::Vec3 rotate_y(quader::math::Vec3 value, float radians) noexcept {
	const float kSin = std::sin(radians);
	const float kCos = std::cos(radians);
	return { value.x * kCos + value.z * kSin, value.y, -value.x * kSin + value.z * kCos };
}

[[nodiscard]] quader::math::Vec3 rotate_z(quader::math::Vec3 value, float radians) noexcept {
	const float kSin = std::sin(radians);
	const float kCos = std::cos(radians);
	return { value.x * kCos - value.y * kSin, value.x * kSin + value.y * kCos, value.z };
}

[[nodiscard]] quader::math::Vec3 rotate_euler(quader::math::Vec3 value,
		quader::math::Vec3 degrees) noexcept {
	value = rotate_x(value, radians_from_degrees(degrees.x));
	value = rotate_y(value, radians_from_degrees(degrees.y));
	return rotate_z(value, radians_from_degrees(degrees.z));
}

[[nodiscard]] quader::math::Vec3 transform_point(quader::math::Vec3 point,
		quader::document::Transform transform) noexcept {
	point = quader::math::Vec3{
		point.x * transform.scale.x,
		point.y * transform.scale.y,
		point.z * transform.scale.z,
	};
	return rotate_euler(point, transform.rotation_euler) + transform.translation;
}

[[nodiscard]] std::array<float, 16> render_transform_from(
		quader::document::Transform transform) noexcept {
	const quader::math::Vec3 kColumn0 = rotate_euler({ transform.scale.x, 0.0F, 0.0F }, transform.rotation_euler);
	const quader::math::Vec3 kColumn1 = rotate_euler({ 0.0F, transform.scale.y, 0.0F }, transform.rotation_euler);
	const quader::math::Vec3 kColumn2 = rotate_euler({ 0.0F, 0.0F, transform.scale.z }, transform.rotation_euler);
	return {
		kColumn0.x,
		kColumn0.y,
		kColumn0.z,
		0.0F,
		kColumn1.x,
		kColumn1.y,
		kColumn1.z,
		0.0F,
		kColumn2.x,
		kColumn2.y,
		kColumn2.z,
		0.0F,
		transform.translation.x,
		transform.translation.y,
		transform.translation.z,
		1.0F,
	};
}

[[nodiscard]] crimson::RenderMeshHandle render_mesh_handle_for(
		quader::document::ObjectId id) noexcept {
	return crimson::RenderMeshHandle{ id.index() + 1U, id.generation() };
}

[[nodiscard]] crimson::RenderObjectId render_object_id_for(
		quader::document::ObjectId id) noexcept {
	return (static_cast<crimson::RenderObjectId>(id.generation()) << 32U) | static_cast<crimson::RenderObjectId>(id.index());
}

[[nodiscard]] std::uint64_t hash_combine(std::uint64_t seed, std::uint64_t value) noexcept {
	return seed ^ (value + 0x9e3779b97f4a7c15ULL + (seed << 6U) + (seed >> 2U));
}

[[nodiscard]] std::uint64_t hash_float(float value) noexcept {
	return std::bit_cast<std::uint32_t>(value);
}

void append_revision_point(std::uint64_t &hash, quader::math::Vec3 point) noexcept {
	hash = hash_combine(hash, hash_float(point.x));
	hash = hash_combine(hash, hash_float(point.y));
	hash = hash_combine(hash, hash_float(point.z));
}

void append_upload_vertex(crimson::RenderMeshUpload &upload,
		quader::math::Vec3 position,
		quader::math::Vec3 normal) {
	upload.position_normal_interleaved.push_back(position.x);
	upload.position_normal_interleaved.push_back(position.y);
	upload.position_normal_interleaved.push_back(position.z);
	upload.position_normal_interleaved.push_back(normal.x);
	upload.position_normal_interleaved.push_back(normal.y);
	upload.position_normal_interleaved.push_back(normal.z);
	upload.indices.push_back(static_cast<std::uint32_t>(upload.indices.size()));
}

[[nodiscard]] quader::math::Vec3 normalized_or(quader::math::Vec3 value,
		quader::math::Vec3 fallback) noexcept {
	const quader::math::Vec3 kNormalized = quader::math::normalized(value);
	return quader::math::length_squared(kNormalized) <= quader::math::kDefaultEpsilon * quader::math::kDefaultEpsilon
			? fallback
			: kNormalized;
}

[[nodiscard]] crimson::RenderMeshRevision revision_for_mesh(
		const quader::mesh::Polyhedron &mesh) {
	std::uint64_t geometry_hash = 1469598103934665603ULL;
	for (const auto kVertex : mesh.vertex_ids()) {
		auto position = mesh.vertex_position(kVertex);
		if (position) {
			append_revision_point(geometry_hash, position.value());
		}
	}

	std::uint64_t topology_hash = 1099511628211ULL;
	for (const auto kFace : mesh.face_ids()) {
		topology_hash = hash_combine(topology_hash, kFace.index());
		auto vertices = quader::mesh::face_vertices(mesh, kFace);
		if (vertices) {
			topology_hash = hash_combine(topology_hash, vertices.value().size());
			for (const auto kVertex : vertices.value()) {
				topology_hash = hash_combine(topology_hash, kVertex.index());
			}
		}
	}

	return crimson::RenderMeshRevision{
		.geometry_version = geometry_hash,
		.topology_version = topology_hash,
		.attribute_version = geometry_hash ^ topology_hash,
	};
}

[[nodiscard]] std::optional<crimson::RenderMeshUpload> make_mesh_upload(
		const quader::document::MeshObject &object) {
	crimson::RenderMeshUpload upload;
	upload.handle = render_mesh_handle_for(object.id);
	upload.revision = revision_for_mesh(object.mesh);

	for (const auto kVertex : object.mesh.vertex_ids()) {
		auto position = object.mesh.vertex_position(kVertex);
		if (!position) {
			return std::nullopt;
		}
		quader::math::expand(upload.bounds, position.value());
	}
	if (quader::math::empty(upload.bounds)) {
		return std::nullopt;
	}

	for (const auto kFace : object.mesh.face_ids()) {
		auto face_vertices = quader::mesh::face_vertices(object.mesh, kFace);
		if (!face_vertices || face_vertices.value().size() < 3U) {
			continue;
		}

		std::vector<quader::math::Vec3> points;
		points.reserve(face_vertices.value().size());
		for (const auto kVertex : face_vertices.value()) {
			auto position = object.mesh.vertex_position(kVertex);
			if (!position) {
				points.clear();
				break;
			}
			points.push_back(position.value());
		}
		if (points.size() < 3U) {
			continue;
		}

		const quader::math::Vec3 kNormal = normalized_or(
				quader::geometry::polygon_normal(points),
				{ 0.0F, 1.0F, 0.0F });
		for (std::size_t index = 1U; index + 1U < points.size(); ++index) {
			append_upload_vertex(upload, points[0], kNormal);
			append_upload_vertex(upload, points[index], kNormal);
			append_upload_vertex(upload, points[index + 1U], kNormal);
		}
	}

	if (upload.indices.empty()) {
		return std::nullopt;
	}
	return upload;
}

[[nodiscard]] quader::math::Aabb world_bounds_for(
		const quader::document::MeshObject &object) {
	quader::math::Aabb bounds;
	for (const auto kVertex : object.mesh.vertex_ids()) {
		auto position = object.mesh.vertex_position(kVertex);
		if (position) {
			quader::math::expand(bounds, transform_point(position.value(), object.transform));
		}
	}
	return bounds;
}

[[nodiscard]] ViewportFrameStats viewport_frame_stats_from(const crimson::FrameStats &stats) noexcept {
	return ViewportFrameStats{
		.width = bounded_int(stats.width_px),
		.height = bounded_int(stats.height_px),
		.viewport_count = bounded_int(stats.view_count),
		.fps = stats.fps,
	};
}

[[nodiscard]] ViewportPickResult viewport_pick_result_from(const crimson::PickingResult &result) noexcept {
	return ViewportPickResult{
		.request_id = result.request_id,
		.hit = result.hit,
		.payload = ViewportPickPayload{
				.object_id = result.payload.object_id,
				.submesh_index = result.payload.submesh_index,
				.element_kind = viewport_kind_from(result.payload.element_kind),
				.element_index = result.payload.element_index,
		},
	};
}

} // namespace

ViewportDiagnosticsSnapshot viewport_diagnostics_from_crimson(
		const crimson::RendererDiagnosticsSnapshot &snapshot,
		QString renderer_name) {
	ViewportDiagnosticsSnapshot result;
	result.renderer_name = !renderer_name.isEmpty()
			? std::move(renderer_name)
			: QString::fromStdString(snapshot.status.backend_name);
	if (snapshot.latest_frame_stats) {
		result.frame = viewport_frame_stats_from(*snapshot.latest_frame_stats);
	}

	result.passes.reserve(static_cast<qsizetype>(snapshot.pass_stats.size()));
	for (const crimson::RenderPassStats &pass : snapshot.pass_stats) {
		result.passes.push_back(ViewportRenderPassStats{
				.pass_name = QString::fromStdString(pass.pass_name),
				.resource_read_count = bounded_int(pass.resource_read_count),
				.resource_write_count = bounded_int(pass.resource_write_count),
				.draw_call_count = bounded_int(pass.draw_call_count),
				.draw_packet_count = bounded_int(pass.draw_packet_count),
				.cpu_time_us = static_cast<quint64>(pass.cpu_time_us),
		});
	}

	result.counters.reserve(static_cast<qsizetype>(snapshot.counters.size()));
	for (const crimson::RendererCounter &counter : snapshot.counters) {
		result.counters.push_back(ViewportRendererCounter{
				.domain = qstring_from_ascii(crimson::renderer_counter_domain_name(counter.domain)),
				.name = QString::fromStdString(counter.name),
				.value = static_cast<quint64>(counter.value),
				.unit = qstring_from_ascii(crimson::renderer_metric_unit_name(counter.unit)),
		});
	}

	result.diagnostics.reserve(static_cast<qsizetype>(snapshot.recent_diagnostics.size()));
	for (const crimson::RendererDiagnostic &diagnostic : snapshot.recent_diagnostics) {
		result.diagnostics.push_back(ViewportRendererDiagnosticRow{
				.severity = qstring_from_ascii(crimson::renderer_diagnostic_severity_name(diagnostic.severity)),
				.code = qstring_from_ascii(crimson::renderer_diagnostic_code_name(diagnostic.code)),
				.subsystem = QString::fromStdString(diagnostic.subsystem),
				.resource_name = QString::fromStdString(diagnostic.resource_name),
				.message = QString::fromStdString(diagnostic.message),
				.detail = QString::fromStdString(diagnostic.detail),
				.frame_index = static_cast<quint64>(diagnostic.frame_index),
		});
	}

	result.frame_graph_dump = QString::fromStdString(snapshot.frame_graph_dump);
	return result;
}

class CrimsonViewportRenderHost final : public IViewportRenderHost {
public:
	CrimsonViewportRenderHost();
	CrimsonViewportRenderHost(
			const quader::document::Document &document,
			const quader::tools::ToolManager &tool_manager);
	~CrimsonViewportRenderHost() override;

	[[nodiscard]] ViewportRenderResult initialize_surface(
			const NativeViewportSurface &surface,
			ViewportPixelSize size,
			double device_pixel_ratio) override;
	[[nodiscard]] ViewportRenderResult resize_surface(
			ViewportPixelSize size,
			double device_pixel_ratio) override;
	[[nodiscard]] ViewportRenderResult render_frame(const ViewportRenderRequest &request) override;
	void shutdown_surface() override;

	[[nodiscard]] std::optional<ViewportFrameStats> latest_frame_stats() const override;
	[[nodiscard]] std::optional<ViewportDiagnosticsSnapshot> latest_diagnostics() const override;
	[[nodiscard]] QString renderer_name() const override;

private:
	[[nodiscard]] crimson::NativeSurfaceDescriptor make_surface_descriptor(
			const NativeViewportSurface &surface,
			ViewportPixelSize size,
			double device_pixel_ratio) const;
	[[nodiscard]] crimson::ViewportExtent make_extent(ViewportPixelSize size, double device_pixel_ratio) const;
	[[nodiscard]] ViewportRenderResult result_from_diagnostic(const crimson::RendererDiagnostic &diagnostic) const;
	[[nodiscard]] std::array<crimson::PrototypeCamera, 4> make_cameras(
			std::span<const ViewportCameraSnapshot> cameras) const;
	[[nodiscard]] std::array<crimson::PrototypeViewportView, 4> make_views(
			std::span<const ViewportPane> panes) const;
	[[nodiscard]] std::vector<crimson::PickingRequest> make_picking_requests(
			std::span<const ViewportPickRequest> requests) const;
	void append_document_render_data(
			std::vector<crimson::RenderMeshUpload> &mesh_uploads,
			std::vector<crimson::RenderObject> &objects) const;
	void append_tool_preview_overlays(
			std::size_t view_count,
			std::vector<crimson::OverlayCommand> &overlays,
			std::vector<crimson::LineOverlaySegment> &line_payloads) const;

	std::unique_ptr<crimson::Renderer> renderer_;
	QString renderer_name_;
	std::optional<ViewportFrameStats> latest_stats_;
	const quader::document::Document *document_ = nullptr;
	const quader::tools::ToolManager *tool_manager_ = nullptr;
};

CrimsonViewportRenderHost::CrimsonViewportRenderHost() = default;

CrimsonViewportRenderHost::CrimsonViewportRenderHost(
		const quader::document::Document &document,
		const quader::tools::ToolManager &tool_manager) : document_(&document),
														  tool_manager_(&tool_manager) {
}

CrimsonViewportRenderHost::~CrimsonViewportRenderHost() {
	shutdown_surface();
}

ViewportRenderResult CrimsonViewportRenderHost::initialize_surface(
		const NativeViewportSurface &surface,
		ViewportPixelSize size,
		double device_pixel_ratio) {
	shutdown_surface();

	crimson::RendererConfig config;
	config.backend_preference = crimson::GraphicsBackendPreference::Auto;
	config.shader_root = (QCoreApplication::applicationDirPath() + QStringLiteral("/shaders")).toStdString();
	config.vsync = true;
	config.enable_debug_text = true;

	auto created = crimson::Renderer::create(config, make_surface_descriptor(surface, size, device_pixel_ratio));
	if (!created) {
		return result_from_diagnostic(created.error());
	}

	renderer_ = std::move(created).value();
	const crimson::RendererStatus kStatus = renderer_->status();
	renderer_name_ = QString::fromStdString(kStatus.backend_name);
	return ViewportRenderResult::success(renderer_name_);
}

ViewportRenderResult CrimsonViewportRenderHost::resize_surface(
		ViewportPixelSize size,
		double device_pixel_ratio) {
	if (renderer_ == nullptr) {
		return ViewportRenderResult::failure(QStringLiteral("Renderer is not initialized."));
	}

	auto result = renderer_->resize(make_extent(size, device_pixel_ratio));
	if (!result) {
		return result_from_diagnostic(result.error());
	}

	return ViewportRenderResult::success(renderer_name_);
}

ViewportRenderResult CrimsonViewportRenderHost::render_frame(const ViewportRenderRequest &request) {
	if (renderer_ == nullptr) {
		return ViewportRenderResult::failure(QStringLiteral("Renderer is not initialized."));
	}

	const auto kCameras = make_cameras(request.cameras);
	const auto kViews = make_views(request.panes);
	const std::vector<crimson::PickingRequest> kPickingRequests = make_picking_requests(request.picking_requests);
	std::vector<crimson::RenderMeshUpload> mesh_uploads;
	std::vector<crimson::RenderObject> objects;
	append_document_render_data(mesh_uploads, objects);
	std::vector<crimson::OverlayCommand> overlays;
	std::vector<crimson::LineOverlaySegment> line_overlay_payloads;
	append_tool_preview_overlays(request.panes.size(), overlays, line_overlay_payloads);
	const auto kFrame = crimson::PrototypeViewportFrame{
		.target_extent = make_extent(request.surface_size, request.device_pixel_ratio),
		.views = std::span<const crimson::PrototypeViewportView>(kViews.data(), request.panes.size()),
		.cameras = std::span<const crimson::PrototypeCamera>(kCameras.data(), kCameras.size()),
		.mesh_uploads = std::span<const crimson::RenderMeshUpload>(mesh_uploads.data(), mesh_uploads.size()),
		.objects = std::span<const crimson::RenderObject>(objects.data(), objects.size()),
		.overlays = std::span<const crimson::OverlayCommand>(overlays.data(), overlays.size()),
		.line_overlay_payloads = std::span<const crimson::LineOverlaySegment>(line_overlay_payloads.data(), line_overlay_payloads.size()),
		.picking_requests = std::span<const crimson::PickingRequest>(kPickingRequests.data(), kPickingRequests.size()),
		.animation_enabled = request.prototype_animation_enabled,
		.elapsed_seconds = request.elapsed_seconds,
	};

	const crimson::FrameBuilder kFrameBuilder;
	auto snapshot = kFrameBuilder.build_prototype_snapshot(kFrame);
	if (!snapshot) {
		return result_from_diagnostic(snapshot.error());
	}

	auto rendered = renderer_->render(snapshot.value());
	if (!rendered) {
		return result_from_diagnostic(rendered.error());
	}

	crimson::FrameRenderResult frame_result = std::move(rendered).value();
	const crimson::FrameStats kStats = frame_result.stats;
	latest_stats_ = viewport_frame_stats_from(kStats);
	std::vector<ViewportPickResult> completed_pick_results;
	completed_pick_results.reserve(frame_result.completed_picking_results.size());
	for (const crimson::PickingResult &result : frame_result.completed_picking_results) {
		completed_pick_results.push_back(viewport_pick_result_from(result));
	}
	return ViewportRenderResult::success(renderer_name_, std::move(completed_pick_results));
}

void CrimsonViewportRenderHost::shutdown_surface() {
	renderer_.reset();
	latest_stats_ = std::nullopt;
}

std::optional<ViewportFrameStats> CrimsonViewportRenderHost::latest_frame_stats() const {
	return latest_stats_;
}

std::optional<ViewportDiagnosticsSnapshot> CrimsonViewportRenderHost::latest_diagnostics() const {
	if (renderer_ == nullptr) {
		return std::nullopt;
	}

	return viewport_diagnostics_from_crimson(renderer_->diagnostics_snapshot(), renderer_name_);
}

QString CrimsonViewportRenderHost::renderer_name() const {
	return renderer_name_;
}

crimson::NativeSurfaceDescriptor CrimsonViewportRenderHost::make_surface_descriptor(
		const NativeViewportSurface &surface,
		ViewportPixelSize size,
		double device_pixel_ratio) const {
	crimson::NativeSurfacePlatform platform = crimson::NativeSurfacePlatform::Unknown;
	if (surface.platform == NativeViewportSurface::Platform::WindowsHwnd) {
		platform = crimson::NativeSurfacePlatform::Windows;
	}

	return crimson::NativeSurfaceDescriptor{
		.platform = platform,
		.native_window_handle = surface.native_window,
		.native_display_handle = nullptr,
		.native_layer_handle = nullptr,
		.extent = make_extent(size, device_pixel_ratio),
	};
}

crimson::ViewportExtent CrimsonViewportRenderHost::make_extent(
		ViewportPixelSize size,
		double device_pixel_ratio) const {
	return crimson::ViewportExtent{
		.width_px = positive_u32(size.width),
		.height_px = positive_u32(size.height),
		.device_pixel_ratio = static_cast<float>(std::max(0.01, device_pixel_ratio)),
	};
}

ViewportRenderResult CrimsonViewportRenderHost::result_from_diagnostic(
		const crimson::RendererDiagnostic &diagnostic) const {
	QString summary = QString::fromStdString(diagnostic.message);
	if (summary.isEmpty()) {
		summary = QStringLiteral("Crimson renderer error");
	}

	return ViewportRenderResult::failure(summary, QString::fromStdString(diagnostic.detail));
}

std::array<crimson::PrototypeCamera, 4> CrimsonViewportRenderHost::make_cameras(
		std::span<const ViewportCameraSnapshot> cameras) const {
	std::array<crimson::PrototypeCamera, 4> result{};
	for (std::size_t index = 0; index < result.size(); ++index) {
		const ViewportCameraSnapshot &source = cameras[index < cameras.size() ? index : 0];
		result[index] = crimson::PrototypeCamera{
			.eye = source.eye,
			.target = source.target,
			.up = source.up,
			.forward = source.forward,
			.projection = projection_from(source.projection),
			.fov_degrees = source.fov_degrees,
			.orthographic_size = source.orthographic_size,
		};
	}
	return result;
}

std::array<crimson::PrototypeViewportView, 4> CrimsonViewportRenderHost::make_views(
		std::span<const ViewportPane> panes) const {
	std::array<crimson::PrototypeViewportView, 4> result{};
	for (std::size_t index = 0; index < result.size(); ++index) {
		const ViewportPane &source = panes[index < panes.size() ? index : 0];
		result[index] = crimson::PrototypeViewportView{
			.rect = crimson::PrototypeViewportRect{
					.x = coordinate_u16(source.rect.x),
					.y = coordinate_u16(source.rect.y),
					.width = positive_u16(source.rect.width),
					.height = positive_u16(source.rect.height),
			},
			.camera_index = static_cast<std::uint8_t>(std::clamp(source.camera_index, 0, 3)),
			.debug_name = view_name_for_pane(source),
		};
	}
	return result;
}

std::vector<crimson::PickingRequest> CrimsonViewportRenderHost::make_picking_requests(
		std::span<const ViewportPickRequest> requests) const {
	std::vector<crimson::PickingRequest> result;
	result.reserve(requests.size());
	for (const ViewportPickRequest &request : requests) {
		result.push_back(crimson::PickingRequest{
				.request_id = request.request_id,
				.view_index = request.view_index,
				.x_px = request.x_px,
				.y_px = request.y_px,
		});
	}
	return result;
}

void CrimsonViewportRenderHost::append_document_render_data(
		std::vector<crimson::RenderMeshUpload> &mesh_uploads,
		std::vector<crimson::RenderObject> &objects) const {
	if (document_ == nullptr) {
		return;
	}

	const std::vector<quader::document::ObjectId> kObjectIds = document_->object_ids();
	mesh_uploads.reserve(mesh_uploads.size() + kObjectIds.size());
	objects.reserve(objects.size() + kObjectIds.size());
	for (const quader::document::ObjectId kObjectId : kObjectIds) {
		const quader::document::MeshObject *object = document_->find_mesh_object(kObjectId);
		if (object == nullptr) {
			continue;
		}

		std::optional<crimson::RenderMeshUpload> upload = make_mesh_upload(*object);
		if (!upload) {
			continue;
		}

		const crimson::RenderMeshHandle kMeshHandle = upload->handle;
		const quader::math::Aabb kWorldBounds = world_bounds_for(*object);
		if (quader::math::empty(kWorldBounds)) {
			continue;
		}

		mesh_uploads.push_back(std::move(*upload));
		objects.push_back(crimson::RenderObject{
				.object_id = render_object_id_for(kObjectId),
				.mesh = kMeshHandle,
				.built_in_mesh = crimson::BuiltInRenderMesh::None,
				.material = {},
				.base_shader = crimson::BaseShaderId::OpaquePbr,
				.world_from_object = render_transform_from(object->transform),
				.world_bounds = kWorldBounds,
				.queue = crimson::RenderQueue::Opaque,
				.submesh_index = 0,
				.visible = true,
				.pickable = true,
		});
	}
}

void CrimsonViewportRenderHost::append_tool_preview_overlays(
		std::size_t view_count,
		std::vector<crimson::OverlayCommand> &overlays,
		std::vector<crimson::LineOverlaySegment> &line_payloads) const {
	if (tool_manager_ == nullptr) {
		return;
	}

	const quader::tools::ToolPreview kPreview = tool_manager_->preview();
	append_tool_preview_line_overlays(kPreview, view_count, overlays, line_payloads);
}

std::unique_ptr<IViewportRenderHost> create_crimson_viewport_render_host() {
	return std::make_unique<CrimsonViewportRenderHost>();
}

std::unique_ptr<IViewportRenderHost> create_crimson_viewport_render_host(
		const quader::document::Document &document,
		const quader::tools::ToolManager &tool_manager) {
	return std::make_unique<CrimsonViewportRenderHost>(document, tool_manager);
}

} // namespace quader::ui
