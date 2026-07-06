/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
#include "ui/viewport/tool_preview_overlay_adapter.hpp"

#include "document/document.hpp"
#include "document/selection.hpp"
#include "mesh/core/mesh_traversal.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <optional>
#include <utility>
#include <variant>

namespace quader::ui {
namespace {

constexpr std::array<std::pair<std::size_t, std::size_t>, 12> kBoxEdges{
	std::pair<std::size_t, std::size_t>{ 0U, 1U },
	std::pair<std::size_t, std::size_t>{ 1U, 2U },
	std::pair<std::size_t, std::size_t>{ 2U, 3U },
	std::pair<std::size_t, std::size_t>{ 3U, 0U },
	std::pair<std::size_t, std::size_t>{ 4U, 5U },
	std::pair<std::size_t, std::size_t>{ 5U, 6U },
	std::pair<std::size_t, std::size_t>{ 6U, 7U },
	std::pair<std::size_t, std::size_t>{ 7U, 4U },
	std::pair<std::size_t, std::size_t>{ 0U, 4U },
	std::pair<std::size_t, std::size_t>{ 1U, 5U },
	std::pair<std::size_t, std::size_t>{ 2U, 6U },
	std::pair<std::size_t, std::size_t>{ 3U, 7U },
};

constexpr crimson::ColorSrgb kSelectedObjectTopologyColorSrgb{ 1.0F, 211.0F / 255.0F, 31.0F / 255.0F, 1.0F };
constexpr crimson::ColorSrgb kSourceWireColorSrgb{ 92.0F / 255.0F, 224.0F / 255.0F, 1.0F, 235.0F / 255.0F };
constexpr crimson::ColorSrgb kSelectedComponentLineColorSrgb{ 1.0F, 211.0F / 255.0F, 31.0F / 255.0F, 1.0F };
constexpr crimson::ColorSrgb kSelectedVertexColorSrgb{ 1.0F, 173.0F / 255.0F, 31.0F / 255.0F, 1.0F };
constexpr crimson::ColorSrgb kSelectedFaceFillColorSrgb{ 1.0F, 172.0F / 255.0F, 31.0F / 255.0F, 8.0F / 255.0F };
constexpr crimson::ColorSrgb kHoverWireColorSrgb{ 81.0F / 255.0F, 1.0F, 0.0F, 1.0F };
constexpr crimson::ColorSrgb kHoverFaceFillColorSrgb{ 102.0F / 255.0F, 1.0F, 31.0F / 255.0F, 22.0F / 255.0F };
constexpr crimson::ColorSrgb kSourceVertexColorSrgb{ 107.0F / 255.0F, 230.0F / 255.0F, 1.0F, 1.0F };
constexpr crimson::ColorSrgb kHoverVertexColorSrgb{ 0.0F, 1.0F, 56.0F / 255.0F, 1.0F };
constexpr crimson::ColorSrgb kVertexOutlineColorSrgb{ 0.0F, 0.0F, 0.0F, 230.0F / 255.0F };
constexpr float kSelectedObjectTopologyThicknessPx = 2.5F;
constexpr float kSourceWireThicknessPx = 1.5F;
constexpr float kSelectedComponentLineThicknessPx = 2.5F;
constexpr float kSelectedVertexSizePx = 7.5F;
constexpr float kSourceVertexSizePx = 7.0F;
constexpr float kHoverVertexSizePx = 7.5F;
constexpr float kVertexOutlineGrowthPx = 1.0F;

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

[[nodiscard]] std::optional<quader::math::Vec3> vertex_world_position(
		const quader::document::MeshObject &object,
		quader::mesh::VertexId vertex) {
	auto position = object.mesh.vertex_position(vertex);
	if (!position) {
		return std::nullopt;
	}
	return transform_point(position.value(), object.transform);
}

struct EdgeWorldPoints {
	quader::math::Vec3 start;
	quader::math::Vec3 end;
};

[[nodiscard]] std::optional<EdgeWorldPoints> edge_world_points(
		const quader::document::MeshObject &object,
		quader::mesh::EdgeId edge) {
	auto halfedges = object.mesh.edge_halfedges(edge);
	if (!halfedges) {
		return std::nullopt;
	}

	const quader::mesh::HalfedgeId kHalfedge = halfedges.value()[0];
	auto origin = object.mesh.halfedge_origin(kHalfedge);
	auto target = object.mesh.halfedge_target(kHalfedge);
	if (!origin || !target) {
		return std::nullopt;
	}

	auto start = vertex_world_position(object, origin.value());
	auto end = vertex_world_position(object, target.value());
	if (!start || !end) {
		return std::nullopt;
	}

	return EdgeWorldPoints{ *start, *end };
}

[[nodiscard]] std::array<std::uint32_t, 2> edge_incident_face_indices(
		const quader::mesh::Polyhedron &mesh,
		quader::mesh::EdgeId edge) {
	std::array<std::uint32_t, 2> result{
		crimson::kInvalidOverlayElementIndex,
		crimson::kInvalidOverlayElementIndex,
	};
	auto halfedges = mesh.edge_halfedges(edge);
	if (!halfedges) {
		return result;
	}

	std::size_t out = 0U;
	for (const quader::mesh::HalfedgeId halfedge : halfedges.value()) {
		auto boundary = mesh.is_boundary_halfedge(halfedge);
		if (boundary && boundary.value()) {
			continue;
		}
		auto face = mesh.halfedge_face(halfedge);
		if (!face || out >= result.size()) {
			continue;
		}
		result[out++] = face.value().index();
	}
	return result;
}

[[nodiscard]] crimson::OverlayElementRef edge_element_ref(
		const quader::document::MeshObject &object,
		quader::mesh::EdgeId edge) {
	const std::array<std::uint32_t, 2> kIncidentFaces =
			edge_incident_face_indices(object.mesh, edge);
	return crimson::OverlayElementRef{
		.object_id = render_object_id_for_document_object(object.id),
		.edge_index = edge.index(),
		.incident_face_index0 = kIncidentFaces[0],
		.incident_face_index1 = kIncidentFaces[1],
	};
}

[[nodiscard]] quader::document::ObjectId object_id_from_encoded_hit(std::uint64_t encoded) noexcept {
	return quader::document::ObjectId{
		static_cast<quader::foundation::IdIndex>(encoded & 0xffffffffULL),
		static_cast<quader::foundation::IdGeneration>((encoded >> 32U) & 0xffffffffULL),
	};
}

[[nodiscard]] quader::document::ObjectId object_id_from_hit(
		const quader::tools::SurfaceHit &hit) noexcept {
	if (hit.document_object_id.is_valid()) {
		return hit.document_object_id;
	}
	return object_id_from_encoded_hit(hit.object_id);
}

[[nodiscard]] std::optional<quader::document::ObjectId> hover_object_id(
		const quader::document::Document &document,
		const std::optional<quader::tools::SurfaceHit> &selection_hover) {
	if (!selection_hover.has_value()) {
		return std::nullopt;
	}

	const quader::document::ObjectId kObject = object_id_from_hit(*selection_hover);
	if (!document.validate_object_id(kObject)) {
		return std::nullopt;
	}
	return kObject;
}

[[nodiscard]] std::optional<quader::mesh::FaceId> face_id_from_hit(
		const quader::mesh::Polyhedron &mesh,
		const quader::tools::SurfaceHit &hit) {
	if (const auto *face = std::get_if<quader::mesh::FaceId>(&hit.component)) {
		return *face;
	}
	const auto kFaces = mesh.face_ids();
	const auto kIt = std::find_if(kFaces.begin(), kFaces.end(), [&hit](quader::mesh::FaceId face) {
		return face.index() == hit.component_index;
	});
	if (kIt == kFaces.end()) {
		return std::nullopt;
	}
	return *kIt;
}

[[nodiscard]] std::optional<quader::mesh::EdgeId> edge_id_from_hit(
		const quader::mesh::Polyhedron &mesh,
		const quader::tools::SurfaceHit &hit) {
	if (const auto *edge = std::get_if<quader::mesh::EdgeId>(&hit.component)) {
		return *edge;
	}
	const auto kEdges = mesh.edge_ids();
	const auto kIt = std::find_if(kEdges.begin(), kEdges.end(), [&hit](quader::mesh::EdgeId edge) {
		return edge.index() == hit.component_index;
	});
	if (kIt == kEdges.end()) {
		return std::nullopt;
	}
	return *kIt;
}

[[nodiscard]] std::optional<quader::mesh::VertexId> vertex_id_from_hit(
		const quader::mesh::Polyhedron &mesh,
		const quader::tools::SurfaceHit &hit) {
	if (const auto *vertex = std::get_if<quader::mesh::VertexId>(&hit.component)) {
		return *vertex;
	}
	const auto kVertices = mesh.vertex_ids();
	const auto kIt = std::find_if(kVertices.begin(), kVertices.end(), [&hit](quader::mesh::VertexId vertex) {
		return vertex.index() == hit.component_index;
	});
	if (kIt == kVertices.end()) {
		return std::nullopt;
	}
	return *kIt;
}

[[nodiscard]] bool mode_has_matching_component(
		const quader::document::ComponentRef &component,
		quader::document::SelectionMode mode) noexcept {
	switch (mode) {
		case quader::document::SelectionMode::Vertex:
			return quader::document::component_kind(component) == quader::document::ComponentKind::Vertex;
		case quader::document::SelectionMode::Edge:
			return quader::document::component_kind(component) == quader::document::ComponentKind::Edge;
		case quader::document::SelectionMode::Face:
			return quader::document::component_kind(component) == quader::document::ComponentKind::Face;
		case quader::document::SelectionMode::Object:
			return false;
	}
	return false;
}

[[nodiscard]] quader::document::ObjectId inferred_component_source_wire_object_id(
		const quader::document::Document &document,
		const std::optional<quader::tools::SurfaceHit> &selection_hover) {
	const quader::document::SelectionMode kMode = document.selection().mode();
	if (kMode == quader::document::SelectionMode::Object) {
		return quader::document::ObjectId::invalid();
	}

	for (const quader::document::ObjectId kSelectedObject : document.selection().selected_objects()) {
		for (const quader::document::ComponentRef &component : document.selection().selected_components()) {
			if (component.object == kSelectedObject && mode_has_matching_component(component, kMode)) {
				return component.object;
			}
		}
	}
	for (const quader::document::ComponentRef &component : document.selection().selected_components()) {
		if (mode_has_matching_component(component, kMode)) {
			return component.object;
		}
	}

	if (const std::optional<quader::document::ObjectId> kHoverObject = hover_object_id(document, selection_hover)) {
		return *kHoverObject;
	}

	const auto kSelectedObjects = document.selection().selected_objects();
	if (!kSelectedObjects.empty()) {
		return kSelectedObjects.front();
	}
	return quader::document::ObjectId::invalid();
}

[[nodiscard]] bool selection_has_current_mode_component(
		const quader::document::Selection &selection) noexcept {
	const quader::document::SelectionMode kMode = selection.mode();
	if (kMode == quader::document::SelectionMode::Object) {
		return false;
	}

	return std::any_of(selection.selected_components().begin(),
			selection.selected_components().end(),
			[kMode](const quader::document::ComponentRef &component) {
				return mode_has_matching_component(component, kMode);
			});
}

[[nodiscard]] bool hover_matches_face(
		const quader::document::MeshObject &object,
		quader::mesh::FaceId face,
		const std::optional<quader::tools::SurfaceHit> &selection_hover) {
	if (!selection_hover.has_value() ||
			selection_hover->kind != quader::tools::SurfaceHitKind::Face ||
			object_id_from_hit(*selection_hover) != object.id) {
		return false;
	}

	const std::optional<quader::mesh::FaceId> kHoverFace = face_id_from_hit(object.mesh, *selection_hover);
	return kHoverFace.has_value() && *kHoverFace == face;
}

[[nodiscard]] bool hover_matches_edge(
		const quader::document::MeshObject &object,
		quader::mesh::EdgeId edge,
		const std::optional<quader::tools::SurfaceHit> &selection_hover) {
	if (!selection_hover.has_value() ||
			selection_hover->kind != quader::tools::SurfaceHitKind::Edge ||
			object_id_from_hit(*selection_hover) != object.id) {
		return false;
	}

	const std::optional<quader::mesh::EdgeId> kHoverEdge = edge_id_from_hit(object.mesh, *selection_hover);
	return kHoverEdge.has_value() && *kHoverEdge == edge;
}

[[nodiscard]] bool hover_matches_vertex(
		const quader::document::MeshObject &object,
		quader::mesh::VertexId vertex,
		const std::optional<quader::tools::SurfaceHit> &selection_hover) {
	if (!selection_hover.has_value() ||
			selection_hover->kind != quader::tools::SurfaceHitKind::Vertex ||
			object_id_from_hit(*selection_hover) != object.id) {
		return false;
	}

	const std::optional<quader::mesh::VertexId> kHoverVertex = vertex_id_from_hit(object.mesh, *selection_hover);
	return kHoverVertex.has_value() && *kHoverVertex == vertex;
}

[[nodiscard]] std::vector<quader::math::Vec3> face_world_points(
		const quader::document::MeshObject &object,
		quader::mesh::FaceId face) {
	auto vertices = quader::mesh::face_vertices(object.mesh, face);
	if (!vertices || vertices.value().size() < 3U) {
		return {};
	}

	std::vector<quader::math::Vec3> points;
	points.reserve(vertices.value().size());
	for (const quader::mesh::VertexId kVertex : vertices.value()) {
		auto position = vertex_world_position(object, kVertex);
		if (!position) {
			return {};
		}
		points.push_back(*position);
	}
	return points;
}

void append_overlay_command_for_views(
		std::size_t view_count,
		std::vector<crimson::OverlayCommand> &overlays,
		crimson::OverlayPrimitive primitive,
		crimson::OverlayDepthMode depth_mode,
		crimson::OverlaySemanticRole role,
		crimson::OverlaySourceKind source,
		crimson::ColorSrgb color,
		float opacity,
		float thickness_px,
		std::uint32_t payload_offset,
		std::uint32_t payload_count) {
	if (view_count == 0U || payload_count == 0U) {
		return;
	}

	overlays.reserve(overlays.size() + view_count);
	for (std::size_t index = 0U; index < view_count; ++index) {
		overlays.push_back(crimson::OverlayCommand{
				.view_index = static_cast<std::uint32_t>(index),
				.primitive = primitive,
				.depth_mode = depth_mode,
				.semantic_role = role,
				.source_kind = source,
				.base_shader = crimson::BaseShaderId::OverlayUnlit,
				.color_srgb = color,
				.opacity = opacity,
				.thickness_px = thickness_px,
				.payload_offset = payload_offset,
				.payload_count = payload_count,
		});
	}
}

void append_mesh_wire_payloads(
		const quader::document::MeshObject &object,
		crimson::OverlaySemanticRole role,
		crimson::OverlaySourceKind source,
		std::vector<crimson::LineOverlaySegment> &line_payloads) {
	for (const quader::mesh::EdgeId kEdge : object.mesh.edge_ids()) {
		auto points = edge_world_points(object, kEdge);
		if (!points) {
			continue;
		}

		line_payloads.push_back(crimson::LineOverlaySegment{
				.start = points->start,
				.end = points->end,
				.semantic_role = role,
				.source_kind = source,
				.element = edge_element_ref(object, kEdge),
		});
	}
}

[[nodiscard]] bool selection_contains_vertex(
		const quader::document::Selection &selection,
		quader::document::ObjectId object_id,
		quader::mesh::VertexId vertex_id) noexcept {
	for (const quader::document::ComponentRef &component : selection.selected_components()) {
		if (component.object != object_id) {
			continue;
		}
		const auto *selected_vertex = std::get_if<quader::mesh::VertexId>(&component.component);
		if (selected_vertex != nullptr && *selected_vertex == vertex_id) {
			return true;
		}
	}
	return false;
}

void append_mesh_vertex_payloads(
		const quader::document::MeshObject &object,
		const quader::document::Selection &selection,
		crimson::OverlaySemanticRole role,
		crimson::OverlaySourceKind source,
		float size_px,
		std::vector<crimson::PointOverlayPrimitive> &point_payloads) {
	const crimson::RenderObjectId kRenderObject = render_object_id_for_document_object(object.id);
	for (const quader::mesh::VertexId kVertex : object.mesh.vertex_ids()) {
		if (selection_contains_vertex(selection, object.id, kVertex)) {
			continue;
		}

		auto position = vertex_world_position(object, kVertex);
		if (!position) {
			continue;
		}

		point_payloads.push_back(crimson::PointOverlayPrimitive{
				.position = *position,
				.size_px = size_px,
				.semantic_role = role,
				.source_kind = source,
				.element = crimson::OverlayElementRef{
						.object_id = kRenderObject,
						.vertex_index = kVertex.index(),
				},
		});
	}
}

void append_face_triangle_payloads(
		const quader::document::MeshObject &object,
		quader::mesh::FaceId face,
		crimson::OverlaySemanticRole role,
		crimson::OverlaySourceKind source,
		std::vector<crimson::TriangleOverlayPrimitive> &triangle_payloads) {
	const std::vector<quader::math::Vec3> kPoints = face_world_points(object, face);
	if (kPoints.size() < 3U) {
		return;
	}

	const crimson::OverlayElementRef kElement{
		.object_id = render_object_id_for_document_object(object.id),
		.face_index = face.index(),
	};
	for (std::size_t index = 1U; index + 1U < kPoints.size(); ++index) {
		triangle_payloads.push_back(crimson::TriangleOverlayPrimitive{
				.a = kPoints[0],
				.b = kPoints[index],
				.c = kPoints[index + 1U],
				.semantic_role = role,
				.source_kind = source,
				.element = kElement,
		});
	}
}

void append_face_boundary_line_payloads(
		const quader::document::MeshObject &object,
		quader::mesh::FaceId face,
		crimson::OverlaySemanticRole role,
		crimson::OverlaySourceKind source,
		std::vector<crimson::LineOverlaySegment> &line_payloads) {
	auto edges = quader::mesh::face_edges(object.mesh, face);
	if (!edges) {
		return;
	}

	for (const quader::mesh::EdgeId kEdge : edges.value()) {
		auto points = edge_world_points(object, kEdge);
		if (!points) {
			continue;
		}

		crimson::OverlayElementRef element = edge_element_ref(object, kEdge);
		element.face_index = face.index();
		line_payloads.push_back(crimson::LineOverlaySegment{
				.start = points->start,
				.end = points->end,
				.semantic_role = role,
				.source_kind = source,
				.element = element,
		});
	}
}

void append_source_wire_depth_stamps(
		const quader::document::MeshObject &object,
		std::vector<crimson::TriangleOverlayPrimitive> &triangle_payloads) {
	for (const quader::mesh::FaceId kFace : object.mesh.face_ids()) {
		append_face_triangle_payloads(object,
				kFace,
				crimson::OverlaySemanticRole::SourceWireDepthStamp,
				crimson::OverlaySourceKind::SourceWire,
				triangle_payloads);
	}
}

void append_object_selection_overlays(
		const quader::document::Document &document,
		std::size_t view_count,
		std::vector<crimson::OverlayCommand> &overlays,
		std::vector<crimson::LineOverlaySegment> &line_payloads,
		std::vector<crimson::TriangleOverlayPrimitive> &triangle_payloads) {
	const std::uint32_t kLineOffset = static_cast<std::uint32_t>(line_payloads.size());
	const std::uint32_t kTriangleOffset = static_cast<std::uint32_t>(triangle_payloads.size());
	for (const quader::document::ObjectId kObject : document.selection().selected_objects()) {
		const auto *object = document.find_mesh_object(kObject);
		if (object == nullptr) {
			continue;
		}
		append_mesh_wire_payloads(*object,
				crimson::OverlaySemanticRole::SelectedObjectTopology,
				crimson::OverlaySourceKind::ObjectSelection,
				line_payloads);
		for (const quader::mesh::FaceId kFace : object->mesh.face_ids()) {
			append_face_triangle_payloads(*object,
					kFace,
					crimson::OverlaySemanticRole::SelectedFaceFill,
					crimson::OverlaySourceKind::ObjectSelection,
					triangle_payloads);
		}
	}

	const std::uint32_t kLineCount = static_cast<std::uint32_t>(line_payloads.size()) - kLineOffset;
	append_overlay_command_for_views(view_count,
			overlays,
			crimson::OverlayPrimitive::LineList,
			crimson::OverlayDepthMode::AlwaysOnTop,
			crimson::OverlaySemanticRole::SelectedObjectTopology,
			crimson::OverlaySourceKind::ObjectSelection,
			kSelectedObjectTopologyColorSrgb,
			1.0F,
			kSelectedObjectTopologyThicknessPx,
			kLineOffset,
			kLineCount);

	const std::uint32_t kTriangleCount = static_cast<std::uint32_t>(triangle_payloads.size()) - kTriangleOffset;
	append_overlay_command_for_views(view_count,
			overlays,
			crimson::OverlayPrimitive::SolidTriangles,
			crimson::OverlayDepthMode::XRay,
			crimson::OverlaySemanticRole::SelectedFaceFill,
			crimson::OverlaySourceKind::ObjectSelection,
			kSelectedFaceFillColorSrgb,
			1.0F,
			1.0F,
			kTriangleOffset,
			kTriangleCount);
}

void append_source_wire_overlays_for_object(
		const quader::document::MeshObject &object,
		const quader::document::Selection &selection,
		bool include_vertices,
		crimson::OverlaySourceKind line_source_kind,
		std::size_t view_count,
		std::vector<crimson::OverlayCommand> &overlays,
		std::vector<crimson::LineOverlaySegment> &line_payloads,
		std::vector<crimson::TriangleOverlayPrimitive> &triangle_payloads,
		std::vector<crimson::PointOverlayPrimitive> &point_payloads) {
	const std::uint32_t kLineOffset = static_cast<std::uint32_t>(line_payloads.size());
	const std::uint32_t kTriangleOffset = static_cast<std::uint32_t>(triangle_payloads.size());
	const std::uint32_t kPointOffset = static_cast<std::uint32_t>(point_payloads.size());
	append_mesh_wire_payloads(object,
			crimson::OverlaySemanticRole::SourceWire,
			line_source_kind,
			line_payloads);
	append_source_wire_depth_stamps(object, triangle_payloads);
	if (include_vertices) {
		append_mesh_vertex_payloads(object,
				selection,
				crimson::OverlaySemanticRole::SourceVertex,
				line_source_kind,
				kSourceVertexSizePx,
				point_payloads);
	}

	const std::uint32_t kLineCount = static_cast<std::uint32_t>(line_payloads.size()) - kLineOffset;
	append_overlay_command_for_views(view_count,
			overlays,
			crimson::OverlayPrimitive::LineList,
			crimson::OverlayDepthMode::AlwaysOnTop,
			crimson::OverlaySemanticRole::SourceWire,
			line_source_kind,
			kSourceWireColorSrgb,
			1.0F,
			kSourceWireThicknessPx,
			kLineOffset,
			kLineCount);

	const std::uint32_t kTriangleCount = static_cast<std::uint32_t>(triangle_payloads.size()) - kTriangleOffset;
	append_overlay_command_for_views(view_count,
			overlays,
			crimson::OverlayPrimitive::SolidTriangles,
			crimson::OverlayDepthMode::AlwaysOnTop,
			crimson::OverlaySemanticRole::SourceWireDepthStamp,
			crimson::OverlaySourceKind::SourceWire,
			kSourceWireColorSrgb,
			0.0F,
			1.0F,
			kTriangleOffset,
			kTriangleCount);

	const std::uint32_t kPointCount = static_cast<std::uint32_t>(point_payloads.size()) - kPointOffset;
	append_overlay_command_for_views(view_count,
			overlays,
			crimson::OverlayPrimitive::PointList,
			crimson::OverlayDepthMode::AlwaysOnTop,
			crimson::OverlaySemanticRole::SourceVertex,
			line_source_kind,
			kSourceVertexColorSrgb,
			1.0F,
			kSourceVertexSizePx,
			kPointOffset,
			kPointCount);
}

void append_component_source_wire_overlays(
		const quader::document::Document &document,
		std::size_t view_count,
		std::vector<crimson::OverlayCommand> &overlays,
		std::vector<crimson::LineOverlaySegment> &line_payloads,
		std::vector<crimson::TriangleOverlayPrimitive> &triangle_payloads,
		std::vector<crimson::PointOverlayPrimitive> &point_payloads,
		const std::optional<quader::tools::SurfaceHit> &selection_hover) {
	const quader::document::ObjectId kSourceWireObject =
			inferred_component_source_wire_object_id(document, selection_hover);
	if (!kSourceWireObject.is_valid()) {
		return;
	}

	const auto *object = document.find_mesh_object(kSourceWireObject);
	if (object == nullptr) {
		return;
	}

	append_source_wire_overlays_for_object(*object,
			document.selection(),
			document.selection().mode() == quader::document::SelectionMode::Vertex,
			crimson::OverlaySourceKind::ComponentSelection,
			view_count,
			overlays,
			line_payloads,
			triangle_payloads,
			point_payloads);
}

void append_selected_component_overlays(
		const quader::document::Document &document,
		std::size_t view_count,
		std::vector<crimson::OverlayCommand> &overlays,
		std::vector<crimson::LineOverlaySegment> &line_payloads,
		std::vector<crimson::TriangleOverlayPrimitive> &triangle_payloads,
		std::vector<crimson::PointOverlayPrimitive> &point_payloads,
		const std::optional<quader::tools::SurfaceHit> &selection_hover,
		bool selection_hover_suppresses_selected) {
	const std::uint32_t kFaceTriangleOffset = static_cast<std::uint32_t>(triangle_payloads.size());
	std::vector<crimson::LineOverlaySegment> selected_face_lines;
	std::vector<crimson::LineOverlaySegment> selected_edge_lines;
	std::vector<crimson::PointOverlayPrimitive> selected_vertex_points;

	for (const quader::document::ComponentRef &component : document.selection().selected_components()) {
		const auto *object = document.find_mesh_object(component.object);
		if (object == nullptr) {
			continue;
		}

		if (const auto *face = std::get_if<quader::mesh::FaceId>(&component.component)) {
			if (selection_hover_suppresses_selected && hover_matches_face(*object, *face, selection_hover)) {
				continue;
			}
			append_face_triangle_payloads(*object,
					*face,
					crimson::OverlaySemanticRole::SelectedFaceFill,
					crimson::OverlaySourceKind::ComponentSelection,
					triangle_payloads);
			append_face_boundary_line_payloads(*object,
					*face,
					crimson::OverlaySemanticRole::SelectedFaceEdge,
					crimson::OverlaySourceKind::ComponentSelection,
					selected_face_lines);
			continue;
		}

		if (const auto *edge = std::get_if<quader::mesh::EdgeId>(&component.component)) {
			if (selection_hover_suppresses_selected && hover_matches_edge(*object, *edge, selection_hover)) {
				continue;
			}
			auto points = edge_world_points(*object, *edge);
			if (!points) {
				continue;
			}

			selected_edge_lines.push_back(crimson::LineOverlaySegment{
					.start = points->start,
					.end = points->end,
					.semantic_role = crimson::OverlaySemanticRole::SelectedEdge,
					.source_kind = crimson::OverlaySourceKind::ComponentSelection,
					.element = edge_element_ref(*object, *edge),
			});
			continue;
		}

		if (const auto *vertex = std::get_if<quader::mesh::VertexId>(&component.component)) {
			if (selection_hover_suppresses_selected && hover_matches_vertex(*object, *vertex, selection_hover)) {
				continue;
			}
			auto position = vertex_world_position(*object, *vertex);
			if (!position) {
				continue;
			}

			selected_vertex_points.push_back(crimson::PointOverlayPrimitive{
					.position = *position,
					.size_px = kSelectedVertexSizePx,
					.semantic_role = crimson::OverlaySemanticRole::SelectedVertex,
					.source_kind = crimson::OverlaySourceKind::ComponentSelection,
					.element = crimson::OverlayElementRef{
							.object_id = render_object_id_for_document_object(object->id),
							.vertex_index = vertex->index(),
					},
			});
		}
	}

	const std::uint32_t kFaceTriangleCount = static_cast<std::uint32_t>(triangle_payloads.size()) - kFaceTriangleOffset;
	append_overlay_command_for_views(view_count,
			overlays,
			crimson::OverlayPrimitive::SolidTriangles,
			crimson::OverlayDepthMode::DepthTested,
			crimson::OverlaySemanticRole::SelectedFaceFill,
			crimson::OverlaySourceKind::ComponentSelection,
			kSelectedFaceFillColorSrgb,
			1.0F,
			1.0F,
			kFaceTriangleOffset,
			kFaceTriangleCount);

	const std::uint32_t kFaceLineOffset = static_cast<std::uint32_t>(line_payloads.size());
	line_payloads.insert(line_payloads.end(), selected_face_lines.begin(), selected_face_lines.end());
	const std::uint32_t kFaceLineCount = static_cast<std::uint32_t>(selected_face_lines.size());
	append_overlay_command_for_views(view_count,
			overlays,
			crimson::OverlayPrimitive::LineList,
			crimson::OverlayDepthMode::DepthTested,
			crimson::OverlaySemanticRole::SelectedFaceEdge,
			crimson::OverlaySourceKind::ComponentSelection,
			kSelectedComponentLineColorSrgb,
			1.0F,
			kSelectedComponentLineThicknessPx,
			kFaceLineOffset,
			kFaceLineCount);

	const std::uint32_t kEdgeLineOffset = static_cast<std::uint32_t>(line_payloads.size());
	line_payloads.insert(line_payloads.end(), selected_edge_lines.begin(), selected_edge_lines.end());
	const std::uint32_t kEdgeLineCount = static_cast<std::uint32_t>(selected_edge_lines.size());
	append_overlay_command_for_views(view_count,
			overlays,
			crimson::OverlayPrimitive::LineList,
			crimson::OverlayDepthMode::DepthTested,
			crimson::OverlaySemanticRole::SelectedEdge,
			crimson::OverlaySourceKind::ComponentSelection,
			kSelectedComponentLineColorSrgb,
			1.0F,
			kSelectedComponentLineThicknessPx,
			kEdgeLineOffset,
			kEdgeLineCount);

	const std::uint32_t kPointFillOffset = static_cast<std::uint32_t>(point_payloads.size());
	point_payloads.insert(point_payloads.end(), selected_vertex_points.begin(), selected_vertex_points.end());
	const std::uint32_t kPointFillCount = static_cast<std::uint32_t>(selected_vertex_points.size());
	const std::uint32_t kPointOutlineOffset = static_cast<std::uint32_t>(point_payloads.size());
	for (crimson::PointOverlayPrimitive outline : selected_vertex_points) {
		outline.size_px += kVertexOutlineGrowthPx * 2.0F;
		point_payloads.push_back(outline);
	}
	const std::uint32_t kPointOutlineCount = static_cast<std::uint32_t>(selected_vertex_points.size());
	append_overlay_command_for_views(view_count,
			overlays,
			crimson::OverlayPrimitive::PointList,
			crimson::OverlayDepthMode::AlwaysOnTop,
			crimson::OverlaySemanticRole::SelectedVertex,
			crimson::OverlaySourceKind::ComponentSelection,
			kVertexOutlineColorSrgb,
			1.0F,
			kSelectedVertexSizePx + kVertexOutlineGrowthPx * 2.0F,
			kPointOutlineOffset,
			kPointOutlineCount);
	append_overlay_command_for_views(view_count,
			overlays,
			crimson::OverlayPrimitive::PointList,
			crimson::OverlayDepthMode::AlwaysOnTop,
			crimson::OverlaySemanticRole::SelectedVertex,
			crimson::OverlaySourceKind::ComponentSelection,
			kSelectedVertexColorSrgb,
			1.0F,
			kSelectedVertexSizePx,
			kPointFillOffset,
			kPointFillCount);
}

void append_hover_component_overlays(
		const quader::document::Document &document,
		std::size_t view_count,
		std::vector<crimson::OverlayCommand> &overlays,
		std::vector<crimson::LineOverlaySegment> &line_payloads,
		std::vector<crimson::TriangleOverlayPrimitive> &triangle_payloads,
		std::vector<crimson::PointOverlayPrimitive> &point_payloads,
		const std::optional<quader::tools::SurfaceHit> &selection_hover) {
	if (!selection_hover.has_value() || document.selection().mode() == quader::document::SelectionMode::Object) {
		return;
	}

	const quader::document::ObjectId kObjectId = object_id_from_hit(*selection_hover);
	const auto *object = document.find_mesh_object(kObjectId);
	if (object == nullptr) {
		return;
	}

	const quader::document::ObjectId kCurrentSourceWireObject =
			inferred_component_source_wire_object_id(document, selection_hover);
	if (kCurrentSourceWireObject != kObjectId && !selection_has_current_mode_component(document.selection())) {
		append_source_wire_overlays_for_object(*object,
				document.selection(),
				document.selection().mode() == quader::document::SelectionMode::Vertex,
				crimson::OverlaySourceKind::ComponentSelection,
				view_count,
				overlays,
				line_payloads,
				triangle_payloads,
				point_payloads);
	}

	const std::uint32_t kTriangleOffset = static_cast<std::uint32_t>(triangle_payloads.size());
	std::vector<crimson::LineOverlaySegment> hover_face_lines;
	std::vector<crimson::LineOverlaySegment> hover_edge_lines;
	std::vector<crimson::PointOverlayPrimitive> hover_vertex_points;

	switch (selection_hover->kind) {
		case quader::tools::SurfaceHitKind::Face: {
			const std::optional<quader::mesh::FaceId> kFace = face_id_from_hit(object->mesh, *selection_hover);
			if (!kFace.has_value()) {
				return;
			}
			append_face_triangle_payloads(*object,
					*kFace,
					crimson::OverlaySemanticRole::HoverFaceFill,
					crimson::OverlaySourceKind::ComponentHover,
					triangle_payloads);
			append_face_boundary_line_payloads(*object,
					*kFace,
					crimson::OverlaySemanticRole::HoverFaceEdge,
					crimson::OverlaySourceKind::ComponentHover,
					hover_face_lines);
			break;
		}
		case quader::tools::SurfaceHitKind::Edge: {
			const std::optional<quader::mesh::EdgeId> kEdge = edge_id_from_hit(object->mesh, *selection_hover);
			if (!kEdge.has_value()) {
				return;
			}
			auto points = edge_world_points(*object, *kEdge);
			if (!points) {
				return;
			}
			hover_edge_lines.push_back(crimson::LineOverlaySegment{
					.start = points->start,
					.end = points->end,
					.semantic_role = crimson::OverlaySemanticRole::HoverEdge,
					.source_kind = crimson::OverlaySourceKind::ComponentHover,
					.element = edge_element_ref(*object, *kEdge),
			});
			break;
		}
		case quader::tools::SurfaceHitKind::Vertex: {
			const std::optional<quader::mesh::VertexId> kVertex = vertex_id_from_hit(object->mesh, *selection_hover);
			if (!kVertex.has_value()) {
				return;
			}
			auto position = vertex_world_position(*object, *kVertex);
			if (!position) {
				return;
			}
			hover_vertex_points.push_back(crimson::PointOverlayPrimitive{
					.position = *position,
					.size_px = kHoverVertexSizePx,
					.semantic_role = crimson::OverlaySemanticRole::HoverVertex,
					.source_kind = crimson::OverlaySourceKind::ComponentHover,
					.element = crimson::OverlayElementRef{
							.object_id = render_object_id_for_document_object(object->id),
							.vertex_index = kVertex->index(),
					},
			});
			break;
		}
		case quader::tools::SurfaceHitKind::Object:
		case quader::tools::SurfaceHitKind::Grid:
			break;
	}

	const std::uint32_t kTriangleCount = static_cast<std::uint32_t>(triangle_payloads.size()) - kTriangleOffset;
	append_overlay_command_for_views(view_count,
			overlays,
			crimson::OverlayPrimitive::SolidTriangles,
			crimson::OverlayDepthMode::DepthTested,
			crimson::OverlaySemanticRole::HoverFaceFill,
			crimson::OverlaySourceKind::ComponentHover,
			kHoverFaceFillColorSrgb,
			1.0F,
			1.0F,
			kTriangleOffset,
			kTriangleCount);

	const std::uint32_t kFaceLineOffset = static_cast<std::uint32_t>(line_payloads.size());
	line_payloads.insert(line_payloads.end(), hover_face_lines.begin(), hover_face_lines.end());
	const std::uint32_t kFaceLineCount = static_cast<std::uint32_t>(hover_face_lines.size());
	append_overlay_command_for_views(view_count,
			overlays,
			crimson::OverlayPrimitive::LineList,
			crimson::OverlayDepthMode::DepthTested,
			crimson::OverlaySemanticRole::HoverFaceEdge,
			crimson::OverlaySourceKind::ComponentHover,
			kHoverWireColorSrgb,
			1.0F,
			kSelectedComponentLineThicknessPx,
			kFaceLineOffset,
			kFaceLineCount);

	const std::uint32_t kEdgeLineOffset = static_cast<std::uint32_t>(line_payloads.size());
	line_payloads.insert(line_payloads.end(), hover_edge_lines.begin(), hover_edge_lines.end());
	const std::uint32_t kEdgeLineCount = static_cast<std::uint32_t>(hover_edge_lines.size());
	append_overlay_command_for_views(view_count,
			overlays,
			crimson::OverlayPrimitive::LineList,
			crimson::OverlayDepthMode::DepthTested,
			crimson::OverlaySemanticRole::HoverEdge,
			crimson::OverlaySourceKind::ComponentHover,
			kHoverWireColorSrgb,
			1.0F,
			kSelectedComponentLineThicknessPx,
			kEdgeLineOffset,
			kEdgeLineCount);

	const std::uint32_t kPointFillOffset = static_cast<std::uint32_t>(point_payloads.size());
	point_payloads.insert(point_payloads.end(), hover_vertex_points.begin(), hover_vertex_points.end());
	const std::uint32_t kPointFillCount = static_cast<std::uint32_t>(hover_vertex_points.size());
	const std::uint32_t kPointOutlineOffset = static_cast<std::uint32_t>(point_payloads.size());
	for (crimson::PointOverlayPrimitive outline : hover_vertex_points) {
		outline.size_px += kVertexOutlineGrowthPx * 2.0F;
		point_payloads.push_back(outline);
	}
	const std::uint32_t kPointOutlineCount = static_cast<std::uint32_t>(hover_vertex_points.size());
	append_overlay_command_for_views(view_count,
			overlays,
			crimson::OverlayPrimitive::PointList,
			crimson::OverlayDepthMode::AlwaysOnTop,
			crimson::OverlaySemanticRole::HoverVertex,
			crimson::OverlaySourceKind::ComponentHover,
			kVertexOutlineColorSrgb,
			1.0F,
			kHoverVertexSizePx + kVertexOutlineGrowthPx * 2.0F,
			kPointOutlineOffset,
			kPointOutlineCount);
	append_overlay_command_for_views(view_count,
			overlays,
			crimson::OverlayPrimitive::PointList,
			crimson::OverlayDepthMode::AlwaysOnTop,
			crimson::OverlaySemanticRole::HoverVertex,
			crimson::OverlaySourceKind::ComponentHover,
			kHoverVertexColorSrgb,
			1.0F,
			kHoverVertexSizePx,
			kPointFillOffset,
			kPointFillCount);
}

} // namespace

crimson::RenderObjectId render_object_id_for_document_object(
		quader::document::ObjectId id) noexcept {
	return (static_cast<crimson::RenderObjectId>(id.generation()) << 32U) |
			static_cast<crimson::RenderObjectId>(id.index());
}

void append_tool_preview_overlays(
		const quader::tools::ToolPreview &preview,
		std::size_t view_count,
		std::vector<crimson::OverlayCommand> &overlays,
		std::vector<crimson::LineOverlaySegment> &line_payloads,
		std::vector<crimson::TriangleOverlayPrimitive> &triangle_payloads) {
	if (!preview.active || view_count == 0U) {
		return;
	}

	for (const quader::tools::ToolPreviewColoredWorldTriangle &triangle : preview.colored_world_triangles) {
		const std::uint32_t kPayloadOffset = static_cast<std::uint32_t>(triangle_payloads.size());
		triangle_payloads.push_back(crimson::TriangleOverlayPrimitive{
				.a = triangle.a,
				.b = triangle.b,
				.c = triangle.c,
				.semantic_role = crimson::OverlaySemanticRole::ToolPreview,
				.source_kind = crimson::OverlaySourceKind::ToolPreview,
		});

		const crimson::ColorSrgb kColor{
			triangle.color_srgb.red,
			triangle.color_srgb.green,
			triangle.color_srgb.blue,
			triangle.color_srgb.alpha,
		};
		auto append_overlay = [&](std::uint32_t view_index) {
			overlays.push_back(crimson::OverlayCommand{
					.view_index = view_index,
					.primitive = crimson::OverlayPrimitive::SolidTriangles,
					.depth_mode = crimson::OverlayDepthMode::AlwaysOnTop,
					.semantic_role = crimson::OverlaySemanticRole::ToolPreview,
					.source_kind = crimson::OverlaySourceKind::ToolPreview,
					.base_shader = crimson::BaseShaderId::OverlayUnlit,
					.color_srgb = kColor,
					.opacity = 1.0F,
					.thickness_px = 1.0F,
					.payload_offset = kPayloadOffset,
					.payload_count = 1,
			});
		};

		if (preview.view_index.has_value()) {
			if (*preview.view_index < view_count) {
				append_overlay(*preview.view_index);
			}
			continue;
		}

		overlays.reserve(overlays.size() + view_count);
		for (std::size_t index = 0; index < view_count; ++index) {
			append_overlay(static_cast<std::uint32_t>(index));
		}
	}

	if (!preview.world_segments.empty() || !preview.boxes.empty()) {
		const std::uint32_t kPayloadOffset = static_cast<std::uint32_t>(line_payloads.size());
		for (const quader::tools::ToolPreviewWorldSegment &segment : preview.world_segments) {
			line_payloads.push_back(crimson::LineOverlaySegment{
					.start = segment.start,
					.end = segment.end,
					.semantic_role = crimson::OverlaySemanticRole::ToolPreview,
					.source_kind = crimson::OverlaySourceKind::ToolPreview,
			});
		}
		for (const quader::tools::ToolPreviewBox &box : preview.boxes) {
			if (!box.active) {
				continue;
			}
			const std::size_t kFirstBoxEdge = preview.world_segments.empty() ? 0U : 4U;
			for (std::size_t edge_index = kFirstBoxEdge; edge_index < kBoxEdges.size(); ++edge_index) {
				const auto &[start_index, end_index] = kBoxEdges[edge_index];
				line_payloads.push_back(crimson::LineOverlaySegment{
						.start = box.corners[start_index],
						.end = box.corners[end_index],
						.semantic_role = crimson::OverlaySemanticRole::ToolPreview,
						.source_kind = crimson::OverlaySourceKind::ToolPreview,
				});
			}
		}

		const std::uint32_t kPayloadCount = static_cast<std::uint32_t>(line_payloads.size()) - kPayloadOffset;
		if (kPayloadCount != 0U) {
			auto append_overlay = [&](std::uint32_t view_index) {
				overlays.push_back(crimson::OverlayCommand{
						.view_index = view_index,
						.primitive = crimson::OverlayPrimitive::LineList,
						.depth_mode = crimson::OverlayDepthMode::AlwaysOnTop,
						.semantic_role = crimson::OverlaySemanticRole::ToolPreview,
						.source_kind = crimson::OverlaySourceKind::ToolPreview,
						.base_shader = crimson::BaseShaderId::OverlayUnlit,
						.color_srgb = kBoxToolPreviewLineColorSrgb,
						.opacity = kBoxToolPreviewLineOpacity,
						.thickness_px = kBoxToolPreviewLineThicknessPx,
						.payload_offset = kPayloadOffset,
						.payload_count = kPayloadCount,
				});
			};

			if (preview.view_index.has_value()) {
				if (*preview.view_index < view_count) {
					append_overlay(*preview.view_index);
				}
			} else {
				overlays.reserve(overlays.size() + view_count);
				for (std::size_t index = 0; index < view_count; ++index) {
					append_overlay(static_cast<std::uint32_t>(index));
				}
			}
		}
	}

	for (const quader::tools::ToolPreviewColoredWorldSegment &segment : preview.colored_world_segments) {
		const std::uint32_t kPayloadOffset = static_cast<std::uint32_t>(line_payloads.size());
		line_payloads.push_back(crimson::LineOverlaySegment{
				.start = segment.start,
				.end = segment.end,
				.semantic_role = crimson::OverlaySemanticRole::ToolPreview,
				.source_kind = crimson::OverlaySourceKind::ToolPreview,
		});

		const crimson::ColorSrgb kColor{
			segment.color_srgb.red,
			segment.color_srgb.green,
			segment.color_srgb.blue,
			segment.color_srgb.alpha,
		};
		auto append_overlay = [&](std::uint32_t view_index) {
			overlays.push_back(crimson::OverlayCommand{
					.view_index = view_index,
					.primitive = crimson::OverlayPrimitive::LineList,
					.depth_mode = crimson::OverlayDepthMode::AlwaysOnTop,
					.semantic_role = crimson::OverlaySemanticRole::ToolPreview,
					.source_kind = crimson::OverlaySourceKind::ToolPreview,
					.base_shader = crimson::BaseShaderId::OverlayUnlit,
					.color_srgb = kColor,
					.opacity = 1.0F,
					.thickness_px = segment.thickness_px,
					.payload_offset = kPayloadOffset,
					.payload_count = 1,
			});
		};

		if (preview.view_index.has_value()) {
			if (*preview.view_index < view_count) {
				append_overlay(*preview.view_index);
			}
			continue;
		}

		overlays.reserve(overlays.size() + view_count);
		for (std::size_t index = 0; index < view_count; ++index) {
			append_overlay(static_cast<std::uint32_t>(index));
		}
	}
}

void append_tool_preview_line_overlays(
		const quader::tools::ToolPreview &preview,
		std::size_t view_count,
		std::vector<crimson::OverlayCommand> &overlays,
		std::vector<crimson::LineOverlaySegment> &line_payloads) {
	std::vector<crimson::TriangleOverlayPrimitive> unused_triangle_payloads;
	append_tool_preview_overlays(preview, view_count, overlays, line_payloads, unused_triangle_payloads);
}

void append_document_selection_overlays(
		const quader::document::Document &document,
		std::size_t view_count,
		std::vector<crimson::OverlayCommand> &overlays,
		std::vector<crimson::LineOverlaySegment> &line_payloads,
		std::vector<crimson::TriangleOverlayPrimitive> &triangle_payloads,
		std::vector<crimson::PointOverlayPrimitive> &point_payloads,
		const std::optional<quader::tools::SurfaceHit> &selection_hover,
		bool selection_hover_suppresses_selected) {
	if (view_count == 0U) {
		return;
	}

	if (document.selection().mode() == quader::document::SelectionMode::Object) {
		if (document.selection().empty()) {
			return;
		}
		append_object_selection_overlays(document, view_count, overlays, line_payloads, triangle_payloads);
		return;
	}

	append_component_source_wire_overlays(document,
			view_count,
			overlays,
			line_payloads,
			triangle_payloads,
			point_payloads,
			selection_hover);
	append_selected_component_overlays(document,
			view_count,
			overlays,
			line_payloads,
			triangle_payloads,
			point_payloads,
			selection_hover,
			selection_hover_suppresses_selected);
	append_hover_component_overlays(document,
			view_count,
			overlays,
			line_payloads,
			triangle_payloads,
			point_payloads,
			selection_hover);
}

} // namespace quader::ui
