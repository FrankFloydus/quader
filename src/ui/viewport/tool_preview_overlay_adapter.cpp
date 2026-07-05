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
constexpr crimson::ColorSrgb kSourceWireColorSrgb{ 92.0F / 255.0F, 224.0F / 255.0F, 1.0F, 0.92F };
constexpr crimson::ColorSrgb kSelectedComponentLineColorSrgb{ 92.0F / 255.0F, 224.0F / 255.0F, 1.0F, 1.0F };
constexpr crimson::ColorSrgb kSelectedVertexColorSrgb{ 1.0F, 173.0F / 255.0F, 31.0F / 255.0F, 1.0F };
constexpr crimson::ColorSrgb kSelectedFaceFillColorSrgb{ 1.0F, 0.66F, 0.10F, 0.16F };
constexpr crimson::ColorSrgb kHoverWireColorSrgb{ 81.0F / 255.0F, 1.0F, 0.0F, 1.0F };
constexpr crimson::ColorSrgb kHoverFaceFillColorSrgb{ 102.0F / 255.0F, 1.0F, 31.0F / 255.0F, 0.12F };
constexpr crimson::ColorSrgb kSourceVertexColorSrgb{ 107.0F / 255.0F, 230.0F / 255.0F, 1.0F, 1.0F };
constexpr crimson::ColorSrgb kHoverVertexColorSrgb{ 0.0F, 1.0F, 56.0F / 255.0F, 1.0F };
constexpr float kSelectedObjectTopologyThicknessPx = 3.0F;
constexpr float kSourceWireThicknessPx = 1.5F;
constexpr float kSelectedComponentLineThicknessPx = 2.5F;
constexpr float kSelectedVertexSizePx = 7.5F;
constexpr float kSourceVertexSizePx = 7.0F;
constexpr float kHoverVertexSizePx = 7.5F;

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
	const crimson::RenderObjectId kRenderObject = render_object_id_for_document_object(object.id);
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
				.element = crimson::OverlayElementRef{
						.object_id = kRenderObject,
						.edge_index = kEdge.index(),
				},
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

	const crimson::RenderObjectId kRenderObject = render_object_id_for_document_object(object.id);
	for (const quader::mesh::EdgeId kEdge : edges.value()) {
		auto points = edge_world_points(object, kEdge);
		if (!points) {
			continue;
		}

		line_payloads.push_back(crimson::LineOverlaySegment{
				.start = points->start,
				.end = points->end,
				.semantic_role = role,
				.source_kind = source,
				.element = crimson::OverlayElementRef{
						.object_id = kRenderObject,
						.face_index = face.index(),
						.edge_index = kEdge.index(),
				},
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
		std::vector<crimson::LineOverlaySegment> &line_payloads) {
	const std::uint32_t kLineOffset = static_cast<std::uint32_t>(line_payloads.size());
	for (const quader::document::ObjectId kObject : document.selection().selected_objects()) {
		const auto *object = document.find_mesh_object(kObject);
		if (object == nullptr) {
			continue;
		}
		append_mesh_wire_payloads(*object,
				crimson::OverlaySemanticRole::SelectedObjectTopology,
				crimson::OverlaySourceKind::ObjectSelection,
				line_payloads);
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
}

void append_source_wire_overlays_for_object(
		const quader::document::MeshObject &object,
		const quader::document::Selection &selection,
		bool include_vertices,
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
			crimson::OverlaySourceKind::SourceWire,
			line_payloads);
	append_source_wire_depth_stamps(object, triangle_payloads);
	if (include_vertices) {
		append_mesh_vertex_payloads(object,
				selection,
				crimson::OverlaySemanticRole::SourceVertex,
				crimson::OverlaySourceKind::SourceWire,
				kSourceVertexSizePx,
				point_payloads);
	}

	const std::uint32_t kLineCount = static_cast<std::uint32_t>(line_payloads.size()) - kLineOffset;
	append_overlay_command_for_views(view_count,
			overlays,
			crimson::OverlayPrimitive::LineList,
			crimson::OverlayDepthMode::AlwaysOnTop,
			crimson::OverlaySemanticRole::SourceWire,
			crimson::OverlaySourceKind::SourceWire,
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
			crimson::OverlaySourceKind::SourceWire,
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
		std::vector<crimson::PointOverlayPrimitive> &point_payloads) {
	const std::uint32_t kFaceLineOffset = static_cast<std::uint32_t>(line_payloads.size());
	const std::uint32_t kFaceTriangleOffset = static_cast<std::uint32_t>(triangle_payloads.size());
	const std::uint32_t kEdgeLineOffset = kFaceLineOffset;
	const std::uint32_t kPointOffset = static_cast<std::uint32_t>(point_payloads.size());

	for (const quader::document::ComponentRef &component : document.selection().selected_components()) {
		const auto *object = document.find_mesh_object(component.object);
		if (object == nullptr) {
			continue;
		}

		if (const auto *face = std::get_if<quader::mesh::FaceId>(&component.component)) {
			append_face_triangle_payloads(*object,
					*face,
					crimson::OverlaySemanticRole::SelectedFaceFill,
					crimson::OverlaySourceKind::ComponentSelection,
					triangle_payloads);
			append_face_boundary_line_payloads(*object,
					*face,
					crimson::OverlaySemanticRole::SelectedFaceEdge,
					crimson::OverlaySourceKind::ComponentSelection,
					line_payloads);
			continue;
		}

		if (const auto *edge = std::get_if<quader::mesh::EdgeId>(&component.component)) {
			auto points = edge_world_points(*object, *edge);
			if (!points) {
				continue;
			}

			line_payloads.push_back(crimson::LineOverlaySegment{
					.start = points->start,
					.end = points->end,
					.semantic_role = crimson::OverlaySemanticRole::SelectedEdge,
					.source_kind = crimson::OverlaySourceKind::ComponentSelection,
					.element = crimson::OverlayElementRef{
							.object_id = render_object_id_for_document_object(object->id),
							.edge_index = edge->index(),
					},
			});
			continue;
		}

		if (const auto *vertex = std::get_if<quader::mesh::VertexId>(&component.component)) {
			auto position = vertex_world_position(*object, *vertex);
			if (!position) {
				continue;
			}

			point_payloads.push_back(crimson::PointOverlayPrimitive{
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
			crimson::OverlayDepthMode::XRay,
			crimson::OverlaySemanticRole::SelectedFaceFill,
			crimson::OverlaySourceKind::ComponentSelection,
			kSelectedFaceFillColorSrgb,
			1.0F,
			1.0F,
			kFaceTriangleOffset,
			kFaceTriangleCount);

	const std::uint32_t kLineCount = static_cast<std::uint32_t>(line_payloads.size()) - kEdgeLineOffset;
	append_overlay_command_for_views(view_count,
			overlays,
			crimson::OverlayPrimitive::LineList,
			crimson::OverlayDepthMode::AlwaysOnTop,
			crimson::OverlaySemanticRole::SelectedEdge,
			crimson::OverlaySourceKind::ComponentSelection,
			kSelectedComponentLineColorSrgb,
			1.0F,
			kSelectedComponentLineThicknessPx,
			kEdgeLineOffset,
			kLineCount);

	const std::uint32_t kPointCount = static_cast<std::uint32_t>(point_payloads.size()) - kPointOffset;
	append_overlay_command_for_views(view_count,
			overlays,
			crimson::OverlayPrimitive::PointList,
			crimson::OverlayDepthMode::AlwaysOnTop,
			crimson::OverlaySemanticRole::SelectedVertex,
			crimson::OverlaySourceKind::ComponentSelection,
			kSelectedVertexColorSrgb,
			1.0F,
			kSelectedVertexSizePx,
			kPointOffset,
			kPointCount);
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
	if (kCurrentSourceWireObject != kObjectId) {
		append_source_wire_overlays_for_object(*object,
				document.selection(),
				document.selection().mode() == quader::document::SelectionMode::Vertex,
				view_count,
				overlays,
				line_payloads,
				triangle_payloads,
				point_payloads);
	}

	const std::uint32_t kLineOffset = static_cast<std::uint32_t>(line_payloads.size());
	const std::uint32_t kTriangleOffset = static_cast<std::uint32_t>(triangle_payloads.size());
	const std::uint32_t kPointOffset = static_cast<std::uint32_t>(point_payloads.size());

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
					line_payloads);
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
			line_payloads.push_back(crimson::LineOverlaySegment{
					.start = points->start,
					.end = points->end,
					.semantic_role = crimson::OverlaySemanticRole::HoverEdge,
					.source_kind = crimson::OverlaySourceKind::ComponentHover,
					.element = crimson::OverlayElementRef{
							.object_id = render_object_id_for_document_object(object->id),
							.edge_index = kEdge->index(),
					},
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
			point_payloads.push_back(crimson::PointOverlayPrimitive{
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

	const std::uint32_t kLineCount = static_cast<std::uint32_t>(line_payloads.size()) - kLineOffset;
	append_overlay_command_for_views(view_count,
			overlays,
			crimson::OverlayPrimitive::LineList,
			crimson::OverlayDepthMode::AlwaysOnTop,
			crimson::OverlaySemanticRole::HoverEdge,
			crimson::OverlaySourceKind::ComponentHover,
			kHoverWireColorSrgb,
			1.0F,
			kSelectedComponentLineThicknessPx,
			kLineOffset,
			kLineCount);

	const std::uint32_t kPointCount = static_cast<std::uint32_t>(point_payloads.size()) - kPointOffset;
	append_overlay_command_for_views(view_count,
			overlays,
			crimson::OverlayPrimitive::PointList,
			crimson::OverlayDepthMode::AlwaysOnTop,
			crimson::OverlaySemanticRole::HoverVertex,
			crimson::OverlaySourceKind::ComponentHover,
			kHoverVertexColorSrgb,
			1.0F,
			kHoverVertexSizePx,
			kPointOffset,
			kPointCount);
}

} // namespace

crimson::RenderObjectId render_object_id_for_document_object(
		quader::document::ObjectId id) noexcept {
	return (static_cast<crimson::RenderObjectId>(id.generation()) << 32U) |
			static_cast<crimson::RenderObjectId>(id.index());
}

void append_tool_preview_line_overlays(
		const quader::tools::ToolPreview &preview,
		std::size_t view_count,
		std::vector<crimson::OverlayCommand> &overlays,
		std::vector<crimson::LineOverlaySegment> &line_payloads) {
	if (!preview.active || (preview.world_segments.empty() && preview.boxes.empty())) {
		return;
	}

	const std::uint32_t kPayloadOffset = static_cast<std::uint32_t>(line_payloads.size());
	for (const quader::tools::ToolPreviewWorldSegment &segment : preview.world_segments) {
		line_payloads.push_back(crimson::LineOverlaySegment{
				.start = segment.start,
				.end = segment.end,
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
			});
		}
	}

	const std::uint32_t kPayloadCount = static_cast<std::uint32_t>(line_payloads.size()) - kPayloadOffset;
	if (kPayloadCount == 0U) {
		return;
	}

	auto append_overlay = [&](std::uint32_t view_index) {
		overlays.push_back(crimson::OverlayCommand{
				.view_index = view_index,
				.primitive = crimson::OverlayPrimitive::LineList,
				.depth_mode = crimson::OverlayDepthMode::AlwaysOnTop,
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
		return;
	}

	overlays.reserve(overlays.size() + view_count);
	for (std::size_t index = 0; index < view_count; ++index) {
		append_overlay(static_cast<std::uint32_t>(index));
	}
}

void append_document_selection_overlays(
		const quader::document::Document &document,
		std::size_t view_count,
		std::vector<crimson::OverlayCommand> &overlays,
		std::vector<crimson::LineOverlaySegment> &line_payloads,
		std::vector<crimson::TriangleOverlayPrimitive> &triangle_payloads,
		std::vector<crimson::PointOverlayPrimitive> &point_payloads,
		const std::optional<quader::tools::SurfaceHit> &selection_hover) {
	if (view_count == 0U) {
		return;
	}

	if (document.selection().mode() == quader::document::SelectionMode::Object) {
		if (document.selection().empty()) {
			return;
		}
		append_object_selection_overlays(document, view_count, overlays, line_payloads);
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
			point_payloads);
	append_hover_component_overlays(document,
			view_count,
			overlays,
			line_payloads,
			triangle_payloads,
			point_payloads,
			selection_hover);
}

} // namespace quader::ui
