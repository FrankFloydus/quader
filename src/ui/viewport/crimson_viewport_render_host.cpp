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
#include "math/vec2.hpp"
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

[[nodiscard]] crimson::ViewportCameraProjection projection_from(CameraProjection projection) noexcept {
	return projection == CameraProjection::Orthographic
			? crimson::ViewportCameraProjection::Orthographic
			: crimson::ViewportCameraProjection::Perspective;
}

[[nodiscard]] crimson::ViewportSettings crimson_viewport_settings_from(
		ViewportDisplaySettings display_settings) noexcept {
	crimson::ViewportSettings settings;
	settings.draw_grid_overlay = display_settings.show_grid;
	settings.draw_overlays = display_settings.show_overlays;
	settings.draw_mesh_grid = display_settings.show_mesh_grid;
	settings.surface_grid_minor_color = crimson::ColorSrgb{ 0.02F, 0.02F, 0.02F, 1.0F };
	settings.surface_grid_major_color = settings.surface_grid_minor_color;
	settings.surface_grid_major_size_m = std::max(settings.surface_grid_size_m * 4.0F, settings.surface_grid_size_m);
	settings.surface_grid_minor_line_thickness = 0.325F;
	settings.surface_grid_major_line_thickness = 0.250F;
	return settings;
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

[[nodiscard]] bool object_is_selected(
		const quader::document::Document &document,
		quader::document::ObjectId object) noexcept {
	const std::span<const quader::document::ObjectId> kSelected = document.selection().selected_objects();
	if (std::find(kSelected.begin(), kSelected.end(), object) != kSelected.end()) {
		return true;
	}
	const std::span<const quader::document::ComponentRef> kComponents = document.selection().selected_components();
	return std::any_of(kComponents.begin(), kComponents.end(), [object](const quader::document::ComponentRef &component) {
		return component.object == object;
	});
}

[[nodiscard]] std::optional<crimson::LineOverlaySegment> scene_wire_segment_for_edge(
		const quader::document::MeshObject &object,
		quader::mesh::EdgeId edge,
		bool selected) {
	const auto halfedges = object.mesh.edge_halfedges(edge);
	if (!halfedges) {
		return std::nullopt;
	}

	const quader::mesh::HalfedgeId kHalfedge = halfedges.value()[0];
	const auto origin = object.mesh.halfedge_origin(kHalfedge);
	const auto target = object.mesh.halfedge_target(kHalfedge);
	if (!origin || !target) {
		return std::nullopt;
	}

	const auto start = object.mesh.vertex_position(origin.value());
	const auto end = object.mesh.vertex_position(target.value());
	if (!start || !end) {
		return std::nullopt;
	}

	return crimson::LineOverlaySegment{
		.start = transform_point(start.value(), object.transform),
		.end = transform_point(end.value(), object.transform),
		.semantic_role = selected
				? crimson::OverlaySemanticRole::SelectedObjectTopology
				: crimson::OverlaySemanticRole::Generic,
		.source_kind = selected
				? crimson::OverlaySourceKind::ObjectSelection
				: crimson::OverlaySourceKind::Unknown,
		.element = crimson::OverlayElementRef{
				.object_id = render_object_id_for_document_object(object.id),
				.edge_index = edge.index(),
		},
	};
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

constexpr float kGeneratedUvTilesPerWorldUnit = 0.5F;

struct ViewportFaceUvBasis {
	quader::math::Vec3 u_axis;
	quader::math::Vec3 v_axis;
	bool valid = false;
};

[[nodiscard]] int dominant_axis(quader::math::Vec3 value) noexcept {
	const quader::math::Vec3 kAbs{
		std::abs(value.x),
		std::abs(value.y),
		std::abs(value.z),
	};
	if (kAbs.x >= kAbs.y && kAbs.x >= kAbs.z) {
		return 0;
	}
	if (kAbs.y >= kAbs.z) {
		return 1;
	}
	return 2;
}

[[nodiscard]] quader::math::Vec3 quader_to_source_vector(quader::math::Vec3 value) noexcept {
	return { value.x, -value.z, value.y };
}

[[nodiscard]] quader::math::Vec3 source_to_quader_vector(quader::math::Vec3 value) noexcept {
	return { value.x, value.z, -value.y };
}

[[nodiscard]] ViewportFaceUvBasis generated_uv_basis_for_normal(quader::math::Vec3 normal) noexcept {
	quader::math::Vec3 source_normal = quader::math::normalized(quader_to_source_vector(normal));
	if (quader::math::length_squared(source_normal) <= quader::math::kDefaultEpsilon * quader::math::kDefaultEpsilon) {
		source_normal = { 0.0F, 0.0F, 1.0F };
	}
	ViewportFaceUvBasis basis;
	switch (dominant_axis(source_normal)) {
		case 0: {
			const float kSideSign = source_normal.x >= 0.0F ? 1.0F : -1.0F;
			basis.u_axis = source_to_quader_vector({ 0.0F, kSideSign, 0.0F });
			basis.v_axis = source_to_quader_vector({ 0.0F, 0.0F, -1.0F });
			break;
		}
		case 1: {
			const float kSideSign = source_normal.y >= 0.0F ? 1.0F : -1.0F;
			basis.u_axis = source_to_quader_vector({ -kSideSign, 0.0F, 0.0F });
			basis.v_axis = source_to_quader_vector({ 0.0F, 0.0F, -1.0F });
			break;
		}
		default: {
			const float kVerticalSign = source_normal.z >= 0.0F ? 1.0F : -1.0F;
			basis.u_axis = source_to_quader_vector({ 1.0F, 0.0F, 0.0F });
			basis.v_axis = source_to_quader_vector({ 0.0F, -kVerticalSign, 0.0F });
			break;
		}
	}
	basis.valid = true;
	return basis;
}

[[nodiscard]] quader::math::Vec2 generated_uv(
		quader::math::Vec3 position,
		const ViewportFaceUvBasis &basis) noexcept {
	if (!basis.valid) {
		return {};
	}
	return {
		quader::math::dot(position, basis.u_axis) * kGeneratedUvTilesPerWorldUnit,
		quader::math::dot(position, basis.v_axis) * kGeneratedUvTilesPerWorldUnit,
	};
}

void append_upload_vertex(crimson::RenderMeshUpload &upload,
		quader::math::Vec3 position,
		quader::math::Vec3 normal,
		quader::math::Vec2 uv) {
	upload.position_normal_uv_interleaved.push_back(position.x);
	upload.position_normal_uv_interleaved.push_back(position.y);
	upload.position_normal_uv_interleaved.push_back(position.z);
	upload.position_normal_uv_interleaved.push_back(normal.x);
	upload.position_normal_uv_interleaved.push_back(normal.y);
	upload.position_normal_uv_interleaved.push_back(normal.z);
	upload.position_normal_uv_interleaved.push_back(uv.x);
	upload.position_normal_uv_interleaved.push_back(uv.y);
	upload.indices.push_back(static_cast<std::uint32_t>(upload.indices.size()));
}

[[nodiscard]] quader::math::Vec3 normalized_or(quader::math::Vec3 value,
		quader::math::Vec3 fallback) noexcept {
	const quader::math::Vec3 kNormalized = quader::math::normalized(value);
	return quader::math::length_squared(kNormalized) <= quader::math::kDefaultEpsilon * quader::math::kDefaultEpsilon
			? fallback
			: kNormalized;
}

void append_upload_triangle(crimson::RenderMeshUpload &upload,
		quader::math::Vec3 a,
		quader::math::Vec3 b,
		quader::math::Vec3 c,
		quader::math::Vec3 normal,
		const ViewportFaceUvBasis &uv_basis) {
	const quader::math::Vec3 kTriangleNormal = quader::math::cross(b - a, c - a);
	if (quader::math::dot(kTriangleNormal, normal) < 0.0F) {
		std::swap(b, c);
	}

	append_upload_vertex(upload, a, normal, generated_uv(a, uv_basis));
	append_upload_vertex(upload, b, normal, generated_uv(b, uv_basis));
	append_upload_vertex(upload, c, normal, generated_uv(c, uv_basis));
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
		const quader::document::MeshObject &object,
		crimson::RenderMeshRevision revision) {
	crimson::RenderMeshUpload upload;
	upload.handle = render_mesh_handle_for(object.id);
	upload.revision = revision;

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
		const ViewportFaceUvBasis kUvBasis = generated_uv_basis_for_normal(kNormal);
		for (std::size_t index = 1U; index + 1U < points.size(); ++index) {
			append_upload_triangle(upload, points[0], points[index], points[index + 1U], kNormal, kUvBasis);
		}
	}

	if (upload.indices.empty()) {
		return std::nullopt;
	}
	return upload;
}

[[nodiscard]] std::optional<crimson::RenderMeshUpload> make_mesh_upload(
		const quader::document::MeshObject &object) {
	return make_mesh_upload(object, revision_for_mesh(object.mesh));
}

[[nodiscard]] quader::math::Aabb transform_bounds(
		quader::math::Aabb bounds,
		quader::document::Transform transform) noexcept {
	if (quader::math::empty(bounds)) {
		return {};
	}

	const std::array<quader::math::Vec3, 8> kCorners{ {
			{ bounds.min.x, bounds.min.y, bounds.min.z },
			{ bounds.max.x, bounds.min.y, bounds.min.z },
			{ bounds.min.x, bounds.max.y, bounds.min.z },
			{ bounds.max.x, bounds.max.y, bounds.min.z },
			{ bounds.min.x, bounds.min.y, bounds.max.z },
			{ bounds.max.x, bounds.min.y, bounds.max.z },
			{ bounds.min.x, bounds.max.y, bounds.max.z },
			{ bounds.max.x, bounds.max.y, bounds.max.z },
	} };

	quader::math::Aabb transformed;
	for (const quader::math::Vec3 kCorner : kCorners) {
		quader::math::expand(transformed, transform_point(kCorner, transform));
	}
	return transformed;
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

crimson::BaseShaderId viewport_base_shader_for_shading_mode(
		ViewportShadingMode mode) noexcept {
	switch (mode) {
		case ViewportShadingMode::Wireframe:
		case ViewportShadingMode::Shaded:
			return crimson::BaseShaderId::UnlitSurface;
		case ViewportShadingMode::Rendered:
			return crimson::BaseShaderId::OpaquePbr;
	}
	return crimson::BaseShaderId::UnlitSurface;
}

std::optional<crimson::RenderMeshUpload> make_crimson_viewport_mesh_upload(
		const quader::document::MeshObject &object) {
	return make_mesh_upload(object);
}

std::optional<ViewportDocumentRenderMesh> ViewportDocumentRenderCache::mesh_for(
		const quader::document::MeshObject &object) {
	const crimson::RenderMeshRevision kRevision = revision_for_mesh(object.mesh);
	auto entry = std::find_if(entries_.begin(), entries_.end(), [object_id = object.id](const Entry &candidate) {
		return candidate.object == object_id;
	});
	if (entry != entries_.end() && entry->revision == kRevision) {
		return ViewportDocumentRenderMesh{
			.handle = entry->handle,
			.local_bounds = entry->local_bounds,
			.upload = std::nullopt,
		};
	}

	std::optional<crimson::RenderMeshUpload> upload = make_mesh_upload(object, kRevision);
	if (!upload) {
		if (entry != entries_.end()) {
			entries_.erase(entry);
		}
		return std::nullopt;
	}

	const crimson::RenderMeshHandle kHandle = upload->handle;
	const quader::math::Aabb kLocalBounds = upload->bounds;
	if (entry == entries_.end()) {
		entries_.push_back(Entry{
				.object = object.id,
				.revision = kRevision,
				.handle = kHandle,
				.local_bounds = kLocalBounds,
		});
	} else {
		*entry = Entry{
			.object = object.id,
			.revision = kRevision,
			.handle = kHandle,
			.local_bounds = kLocalBounds,
		};
	}

	return ViewportDocumentRenderMesh{
		.handle = kHandle,
		.local_bounds = kLocalBounds,
		.upload = std::move(upload),
	};
}

void ViewportDocumentRenderCache::prune(
		std::span<const quader::document::ObjectId> live_objects) {
	entries_.erase(std::remove_if(entries_.begin(), entries_.end(), [live_objects](const Entry &entry) {
		return std::find(live_objects.begin(), live_objects.end(), entry.object) == live_objects.end();
	}), entries_.end());
}

void ViewportDocumentRenderCache::clear() noexcept {
	entries_.clear();
}

std::size_t ViewportDocumentRenderCache::cached_mesh_count() const noexcept {
	return entries_.size();
}

void append_crimson_document_render_data(
		const quader::document::Document &document,
		ViewportDocumentRenderCache &document_render_cache,
		std::vector<crimson::RenderMeshUpload> &mesh_uploads,
		std::vector<crimson::RenderObject> &objects,
		ViewportShadingMode shading_mode) {
	const std::vector<quader::document::ObjectId> kObjectIds = document.object_ids();
	if (shading_mode == ViewportShadingMode::Wireframe) {
		document_render_cache.prune(kObjectIds);
		return;
	}

	const crimson::BaseShaderId kBaseShader = viewport_base_shader_for_shading_mode(shading_mode);
	mesh_uploads.reserve(mesh_uploads.size() + kObjectIds.size());
	objects.reserve(objects.size() + kObjectIds.size());
	for (const quader::document::ObjectId kObjectId : kObjectIds) {
		const quader::document::MeshObject *object = document.find_mesh_object(kObjectId);
		if (object == nullptr) {
			continue;
		}

		std::optional<ViewportDocumentRenderMesh> render_mesh =
				document_render_cache.mesh_for(*object);
		if (!render_mesh) {
			continue;
		}

		const quader::math::Aabb kWorldBounds =
				transform_bounds(render_mesh->local_bounds, object->transform);
		if (quader::math::empty(kWorldBounds)) {
			continue;
		}

		if (render_mesh->upload) {
			mesh_uploads.push_back(std::move(*render_mesh->upload));
		}
		objects.push_back(crimson::RenderObject{
				.object_id = render_object_id_for_document_object(kObjectId),
				.mesh = render_mesh->handle,
				.material = {},
				.base_shader = kBaseShader,
				.world_from_object = render_transform_from(object->transform),
				.world_bounds = kWorldBounds,
				.queue = crimson::RenderQueue::Opaque,
				.submesh_index = 0,
				.visible = true,
				.pickable = true,
		});
	}
	document_render_cache.prune(kObjectIds);
}

void append_crimson_scene_wireframe_overlays(
		const quader::document::Document &document,
		std::size_t view_count,
		std::vector<crimson::OverlayCommand> &overlays,
		std::vector<crimson::LineOverlaySegment> &line_payloads) {
	if (view_count == 0U) {
		return;
	}

	const std::uint32_t kUnselectedOffset = static_cast<std::uint32_t>(line_payloads.size());
	std::uint32_t unselected_count = 0;
	std::vector<crimson::LineOverlaySegment> selected_segments;
	for (const quader::document::ObjectId kObjectId : document.object_ids()) {
		const quader::document::MeshObject *object = document.find_mesh_object(kObjectId);
		if (object == nullptr) {
			continue;
		}

		const bool kSelected = object_is_selected(document, kObjectId);
		for (const quader::mesh::EdgeId kEdge : object->mesh.edge_ids()) {
			std::optional<crimson::LineOverlaySegment> segment =
					scene_wire_segment_for_edge(*object, kEdge, kSelected);
			if (!segment) {
				continue;
			}
			if (kSelected) {
				selected_segments.push_back(*segment);
			} else {
				line_payloads.push_back(*segment);
				++unselected_count;
			}
		}
	}

	const std::uint32_t kSelectedPayloadOffset = static_cast<std::uint32_t>(line_payloads.size());
	line_payloads.insert(line_payloads.end(), selected_segments.begin(), selected_segments.end());
	const std::uint32_t kSelectedCount = static_cast<std::uint32_t>(selected_segments.size());

	const auto append_commands = [&](std::uint32_t payload_offset,
									 std::uint32_t payload_count,
									 crimson::OverlaySemanticRole role,
									 crimson::OverlaySourceKind source_kind,
									 crimson::ColorSrgb color,
									 float thickness_px) {
		if (payload_count == 0U) {
			return;
		}
		for (std::size_t view_index = 0; view_index < view_count; ++view_index) {
			overlays.push_back(crimson::OverlayCommand{
					.view_index = static_cast<std::uint32_t>(view_index),
					.primitive = crimson::OverlayPrimitive::LineList,
					.depth_mode = crimson::OverlayDepthMode::AlwaysOnTop,
					.semantic_role = role,
					.source_kind = source_kind,
					.base_shader = crimson::BaseShaderId::OverlayUnlit,
					.color_srgb = color,
					.opacity = 1.0F,
					.thickness_px = thickness_px,
					.payload_offset = payload_offset,
					.payload_count = payload_count,
			});
		}
	};

	append_commands(
			kUnselectedOffset,
			unselected_count,
			crimson::OverlaySemanticRole::Generic,
			crimson::OverlaySourceKind::Unknown,
			crimson::ColorSrgb{ 0.02F, 0.02F, 0.02F, 0.62F },
			1.0F);
	append_commands(
			kSelectedPayloadOffset,
			kSelectedCount,
			crimson::OverlaySemanticRole::SelectedObjectTopology,
			crimson::OverlaySourceKind::ObjectSelection,
			crimson::ColorSrgb{ 1.0F, 0.84F, 0.22F, 1.0F },
			1.5F);
}

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
	[[nodiscard]] std::array<crimson::ViewportCamera, 4> make_cameras(
			std::span<const ViewportCameraSnapshot> cameras) const;
	[[nodiscard]] std::array<crimson::ViewportFrameView, 4> make_views(
			std::span<const ViewportPane> panes) const;
	[[nodiscard]] std::vector<crimson::PickingRequest> make_picking_requests(
			std::span<const ViewportPickRequest> requests) const;
	void append_document_render_data(
			std::vector<crimson::RenderMeshUpload> &mesh_uploads,
			std::vector<crimson::RenderObject> &objects,
			ViewportShadingMode shading_mode) const;
	void append_tool_preview_overlays(
			std::size_t view_count,
			std::vector<crimson::OverlayCommand> &overlays,
			std::vector<crimson::LineOverlaySegment> &line_payloads,
			std::vector<crimson::TriangleOverlayPrimitive> &triangle_payloads) const;
	void append_scene_wireframe_overlays(
			std::size_t view_count,
			std::vector<crimson::OverlayCommand> &overlays,
			std::vector<crimson::LineOverlaySegment> &line_payloads) const;
	void append_document_selection_overlays(
			std::size_t view_count,
			std::vector<crimson::OverlayCommand> &overlays,
			std::vector<crimson::LineOverlaySegment> &line_payloads,
			std::vector<crimson::TriangleOverlayPrimitive> &triangle_payloads,
			std::vector<crimson::PointOverlayPrimitive> &point_payloads) const;

	std::unique_ptr<crimson::Renderer> renderer_;
	QString renderer_name_;
	std::optional<ViewportFrameStats> latest_stats_;
	const quader::document::Document *document_ = nullptr;
	const quader::tools::ToolManager *tool_manager_ = nullptr;
	mutable ViewportDocumentRenderCache document_render_cache_;
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
	config.asset_root = (QCoreApplication::applicationDirPath() + QStringLiteral("/resources/modeling_assets")).toStdString();
	config.vsync = true;
	config.enable_debug_text = true;

	auto created = crimson::Renderer::create(config, make_surface_descriptor(surface, size, device_pixel_ratio));
	if (!created) {
		return result_from_diagnostic(created.error());
	}

	document_render_cache_.clear();
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
	append_document_render_data(mesh_uploads, objects, request.shading_mode);
	std::vector<crimson::OverlayCommand> overlays;
	std::vector<crimson::LineOverlaySegment> line_overlay_payloads;
	std::vector<crimson::TriangleOverlayPrimitive> triangle_overlay_payloads;
	std::vector<crimson::PointOverlayPrimitive> point_overlay_payloads;
	if (request.shading_mode == ViewportShadingMode::Wireframe) {
		append_scene_wireframe_overlays(request.panes.size(), overlays, line_overlay_payloads);
	}
	append_document_selection_overlays(request.panes.size(),
			overlays,
			line_overlay_payloads,
			triangle_overlay_payloads,
			point_overlay_payloads);
	append_tool_preview_overlays(request.panes.size(),
			overlays,
			line_overlay_payloads,
			triangle_overlay_payloads);
	const auto kFrame = crimson::ViewportFrameInput{
		.target_extent = make_extent(request.surface_size, request.device_pixel_ratio),
		.views = std::span<const crimson::ViewportFrameView>(kViews.data(), request.panes.size()),
		.cameras = std::span<const crimson::ViewportCamera>(kCameras.data(), kCameras.size()),
		.mesh_uploads = std::span<const crimson::RenderMeshUpload>(mesh_uploads.data(), mesh_uploads.size()),
		.objects = std::span<const crimson::RenderObject>(objects.data(), objects.size()),
		.overlays = std::span<const crimson::OverlayCommand>(overlays.data(), overlays.size()),
		.line_overlay_payloads = std::span<const crimson::LineOverlaySegment>(line_overlay_payloads.data(), line_overlay_payloads.size()),
		.triangle_overlay_payloads = std::span<const crimson::TriangleOverlayPrimitive>(triangle_overlay_payloads.data(), triangle_overlay_payloads.size()),
		.point_overlay_payloads = std::span<const crimson::PointOverlayPrimitive>(point_overlay_payloads.data(), point_overlay_payloads.size()),
		.picking_requests = std::span<const crimson::PickingRequest>(kPickingRequests.data(), kPickingRequests.size()),
		.viewport_settings = crimson_viewport_settings_from(request.display_settings),
		.animation_enabled = request.scene_animation_enabled,
		.elapsed_seconds = request.elapsed_seconds,
	};

	const crimson::FrameBuilder kFrameBuilder;
	auto snapshot = kFrameBuilder.build_snapshot(kFrame);
	if (!snapshot) {
		document_render_cache_.clear();
		return result_from_diagnostic(snapshot.error());
	}

	auto rendered = renderer_->render(snapshot.value());
	if (!rendered) {
		document_render_cache_.clear();
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
	document_render_cache_.clear();
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

std::array<crimson::ViewportCamera, 4> CrimsonViewportRenderHost::make_cameras(
		std::span<const ViewportCameraSnapshot> cameras) const {
	std::array<crimson::ViewportCamera, 4> result{};
	for (std::size_t index = 0; index < result.size(); ++index) {
		const ViewportCameraSnapshot &source = cameras[index < cameras.size() ? index : 0];
		result[index] = crimson::ViewportCamera{
			.eye = source.eye,
			.target = source.target,
			.up = source.up,
			.forward = source.forward,
			.projection = projection_from(source.projection),
			.near_plane_m = source.settings.clip.near_clip_m,
			.far_plane_m = source.settings.clip.far_clip_m,
			.fov_degrees = source.settings.fov_degrees,
			.orthographic_size = source.settings.orthographic_size,
		};
	}
	return result;
}

std::array<crimson::ViewportFrameView, 4> CrimsonViewportRenderHost::make_views(
		std::span<const ViewportPane> panes) const {
	std::array<crimson::ViewportFrameView, 4> result{};
	for (std::size_t index = 0; index < result.size(); ++index) {
		const ViewportPane &source = panes[index < panes.size() ? index : 0];
		result[index] = crimson::ViewportFrameView{
			.rect = crimson::ViewportFrameRect{
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
		std::vector<crimson::RenderObject> &objects,
		ViewportShadingMode shading_mode) const {
	if (document_ == nullptr) {
		return;
	}

	append_crimson_document_render_data(*document_, document_render_cache_, mesh_uploads, objects, shading_mode);
}

void CrimsonViewportRenderHost::append_tool_preview_overlays(
		std::size_t view_count,
		std::vector<crimson::OverlayCommand> &overlays,
		std::vector<crimson::LineOverlaySegment> &line_payloads,
		std::vector<crimson::TriangleOverlayPrimitive> &triangle_payloads) const {
	if (tool_manager_ == nullptr) {
		return;
	}

	const quader::tools::ToolPreview kPreview = tool_manager_->preview();
	quader::ui::append_tool_preview_overlays(kPreview, view_count, overlays, line_payloads, triangle_payloads);
}

void CrimsonViewportRenderHost::append_scene_wireframe_overlays(
		std::size_t view_count,
		std::vector<crimson::OverlayCommand> &overlays,
		std::vector<crimson::LineOverlaySegment> &line_payloads) const {
	if (document_ == nullptr) {
		return;
	}

	append_crimson_scene_wireframe_overlays(*document_, view_count, overlays, line_payloads);
}

void CrimsonViewportRenderHost::append_document_selection_overlays(
		std::size_t view_count,
		std::vector<crimson::OverlayCommand> &overlays,
		std::vector<crimson::LineOverlaySegment> &line_payloads,
		std::vector<crimson::TriangleOverlayPrimitive> &triangle_payloads,
		std::vector<crimson::PointOverlayPrimitive> &point_payloads) const {
	if (document_ == nullptr) {
		return;
	}

	quader::ui::append_document_selection_overlays(*document_,
			view_count,
			overlays,
			line_payloads,
			triangle_payloads,
			point_payloads,
			tool_manager_ != nullptr ? tool_manager_->selection_hover() : std::optional<quader::tools::SurfaceHit>{},
			tool_manager_ != nullptr && tool_manager_->selection_hover_suppresses_selected());
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
