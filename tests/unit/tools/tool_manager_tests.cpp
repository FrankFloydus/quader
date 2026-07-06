/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
#include "../document/document_test_helpers.hpp"

#include <gtest/gtest.h>

#include "commands/document_commands.hpp"
#include "foundation/logging.hpp"
#include "geometry/normals.hpp"
#include "mesh/core/mesh_traversal.hpp"
#include "mesh/core/mesh_validation.hpp"
#include "tools/box_tool.hpp"
#include "tools/tool_manager.hpp"
#include "tools/transform_gizmo_tool.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace {

static_assert(std::is_same_v<decltype(std::declval<const quader::tools::ToolContext &>().document()),
		const quader::document::Document &>);

using quader::tests::document_fixtures::DocumentSemanticState;

void discard_pending_changes(quader::document::Document &document) {
	const auto kChanges = document.take_pending_changes();
	(void)kChanges;
}

DocumentSemanticState capture_tool_state(const quader::document::Document &document) {
	auto state = quader::tests::document_fixtures::capture_document_state(document);
	state.dirty = false;
	state.dirty_flags = quader::document::kDocumentDirtyNone;
	return state;
}

bool vec2_exactly_equal(quader::math::Vec2 left, quader::math::Vec2 right) {
	return left.x == right.x && left.y == right.y;
}

quader::math::Vec3 average_points(std::span<const quader::math::Vec3> points) {
	quader::math::Vec3 average;
	for (quader::math::Vec3 point : points) {
		average = average + point;
	}
	return points.empty() ? average : average / static_cast<float>(points.size());
}

std::vector<quader::math::Vec3> face_points(
		const quader::mesh::Polyhedron &mesh,
		quader::mesh::FaceId face) {
	auto vertices = quader::mesh::face_vertices(mesh, face);
	EXPECT_TRUE(vertices);
	std::vector<quader::math::Vec3> points;
	if (!vertices) {
		return points;
	}

	points.reserve(vertices.value().size());
	for (const quader::mesh::VertexId kVertex : vertices.value()) {
		auto position = mesh.vertex_position(kVertex);
		EXPECT_TRUE(position);
		if (position) {
			points.push_back(position.value());
		}
	}
	return points;
}

quader::math::Vec3 mesh_center(const quader::mesh::Polyhedron &mesh) {
	std::vector<quader::math::Vec3> points;
	points.reserve(mesh.vertex_count());
	for (const quader::mesh::VertexId kVertex : mesh.vertex_ids()) {
		auto position = mesh.vertex_position(kVertex);
		EXPECT_TRUE(position);
		if (position) {
			points.push_back(position.value());
		}
	}
	return average_points(points);
}

void expect_ground_box_faces_oriented_outward(const quader::mesh::Polyhedron &mesh) {
	const quader::math::Vec3 kCenter = mesh_center(mesh);
	int up_faces = 0;
	int down_faces = 0;
	for (const quader::mesh::FaceId kFace : mesh.face_ids()) {
		const std::vector<quader::math::Vec3> kPoints = face_points(mesh, kFace);
		ASSERT_GE(kPoints.size(), 3U);
		const quader::math::Vec3 kNormal = quader::geometry::polygon_normal(kPoints);
		const quader::math::Vec3 kFaceCenter = average_points(kPoints);
		EXPECT_GT(quader::math::dot(kNormal, kFaceCenter - kCenter), 0.0F);

		const quader::math::Vec3 kUnitNormal = quader::math::normalized(kNormal);
		if (quader::math::dot(kUnitNormal, { 0.0F, 1.0F, 0.0F }) > 0.999F) {
			++up_faces;
		}
		if (quader::math::dot(kUnitNormal, { 0.0F, -1.0F, 0.0F }) > 0.999F) {
			++down_faces;
		}
	}
	EXPECT_EQ(up_faces, 1);
	EXPECT_EQ(down_faces, 1);
}

quader::tools::PointerEvent grid_event(quader::math::Vec3 point,
		quader::math::Vec2 screen,
		quader::tools::PointerPhase phase,
		quader::tools::PointerButton button = quader::tools::PointerButton::None,
		bool pressed = false) {
	return quader::tools::PointerEvent{
		.position = screen,
		.button = button,
		.pressed = pressed,
		.phase = phase,
		.snap_to_grid = true,
		.grid_size = 1.0F,
		.ray = quader::tools::ViewportRay{
				.origin = point + quader::math::Vec3{ 0.0F, 10.0F, 0.0F },
				.direction = { 0.0F, -1.0F, 0.0F },
		},
	};
}

quader::tools::PointerEvent surface_event(quader::math::Vec3 point,
		quader::math::Vec3 normal,
		quader::math::Vec2 screen,
		quader::tools::PointerPhase phase,
		quader::tools::PointerButton button = quader::tools::PointerButton::None,
		bool pressed = false,
		std::optional<quader::tools::ViewportRay> ray = std::nullopt) {
	return quader::tools::PointerEvent{
		.position = screen,
		.button = button,
		.pressed = pressed,
		.phase = phase,
		.snap_to_grid = true,
		.grid_size = 1.0F,
		.ray = ray,
		.surface_hit = quader::tools::SurfaceHit{
				.position = point,
				.normal = normal,
				.object_id = 7,
				.kind = quader::tools::SurfaceHitKind::Face,
		},
	};
}

quader::tools::ViewportRay x_axis_ray_to(quader::math::Vec3 point_on_x_plane) {
	return quader::tools::ViewportRay{
		.origin = { 0.0F, point_on_x_plane.y, point_on_x_plane.z },
		.direction = { 1.0F, 0.0F, 0.0F },
	};
}

quader::tools::ViewportRay negative_x_axis_ray_to(quader::math::Vec3 point_on_x_plane) {
	return quader::tools::ViewportRay{
		.origin = { 10.0F, point_on_x_plane.y, point_on_x_plane.z },
		.direction = { -1.0F, 0.0F, 0.0F },
	};
}

quader::tools::ViewportRay downward_ray_to(quader::math::Vec3 point_on_y_plane) {
	return quader::tools::ViewportRay{
		.origin = point_on_y_plane + quader::math::Vec3{ 0.0F, 10.0F, 0.0F },
		.direction = { 0.0F, -1.0F, 0.0F },
	};
}

quader::tools::ViewportRay front_ray_to(quader::math::Vec3 point) {
	return quader::tools::ViewportRay{
		.origin = point + quader::math::Vec3{ 0.0F, 0.0F, 10.0F },
		.direction = { 0.0F, 0.0F, -1.0F },
	};
}

quader::tools::PointerEvent gizmo_event(quader::math::Vec3 point,
		quader::math::Vec2 screen,
		quader::tools::PointerPhase phase,
		quader::tools::PointerButton button = quader::tools::PointerButton::None,
		bool pressed = false) {
	return quader::tools::PointerEvent{
		.position = screen,
		.button = button,
		.pressed = pressed,
		.phase = phase,
		.snap_to_grid = true,
		.grid_size = 1.0F,
		.ray = front_ray_to(point),
	};
}

quader::tools::ViewportCameraInput reference_gizmo_camera() {
	return quader::tools::ViewportCameraInput{
		.eye = { 8.0F, 7.0F, 10.0F },
		.target = { 0.5F, 0.5F, 0.0F },
		.up = { 0.0F, 1.0F, 0.0F },
		.forward = quader::math::normalized(quader::math::Vec3{ 0.5F, 0.5F, 0.0F } - quader::math::Vec3{ 8.0F, 7.0F, 10.0F }),
		.projection = quader::tools::ViewportCameraProjection::Perspective,
		.fov_degrees = 60.0F,
		.orthographic_size = 48.0F,
		.viewport_size_pixels = { 640.0F, 480.0F },
	};
}

quader::tools::PointerEvent reference_gizmo_hover_event(quader::math::Vec2 screen) {
	const quader::tools::ViewportCameraInput kCamera = reference_gizmo_camera();
	return quader::tools::PointerEvent{
		.position = screen,
		.phase = quader::tools::PointerPhase::Hover,
		.snap_to_grid = true,
		.grid_size = 1.0F,
		.ray = quader::tools::ViewportRay{
				.origin = kCamera.eye,
				.direction = quader::math::normalized(kCamera.target - kCamera.eye),
		},
		.camera = kCamera,
	};
}

quader::tools::ViewportCameraInput front_gizmo_camera() {
	return quader::tools::ViewportCameraInput{
		.eye = { 0.5F, 0.5F, 10.0F },
		.target = { 0.5F, 0.5F, 0.0F },
		.up = { 0.0F, 1.0F, 0.0F },
		.forward = { 0.0F, 0.0F, -1.0F },
		.projection = quader::tools::ViewportCameraProjection::Orthographic,
		.fov_degrees = 60.0F,
		.orthographic_size = 48.0F,
		.viewport_size_pixels = { 640.0F, 480.0F },
	};
}

quader::tools::ViewportRay front_gizmo_ray_from_screen(quader::math::Vec2 screen) {
	const quader::tools::ViewportCameraInput kCamera = front_gizmo_camera();
	const float kWidth = kCamera.viewport_size_pixels.x;
	const float kHeight = kCamera.viewport_size_pixels.y;
	const float kAspect = kWidth / kHeight;
	const float kHalfHeight = kCamera.orthographic_size * 0.5F;
	const float kHalfWidth = kHalfHeight * kAspect;
	const float kNdcX = (screen.x / kWidth) * 2.0F - 1.0F;
	const float kNdcY = 1.0F - (screen.y / kHeight) * 2.0F;
	const quader::math::Vec3 kRight{ -1.0F, 0.0F, 0.0F };
	const quader::math::Vec3 kUp{ 0.0F, 1.0F, 0.0F };
	return quader::tools::ViewportRay{
		.origin = kCamera.eye + kRight * (kNdcX * kHalfWidth) + kUp * (kNdcY * kHalfHeight),
		.direction = { 0.0F, 0.0F, -1.0F },
	};
}

quader::tools::PointerEvent front_gizmo_event(quader::math::Vec2 screen,
		quader::tools::PointerPhase phase,
		quader::tools::PointerButton button = quader::tools::PointerButton::None,
		bool pressed = false,
		bool snap_to_grid = true) {
	return quader::tools::PointerEvent{
		.position = screen,
		.button = button,
		.pressed = pressed,
		.phase = phase,
		.snap_to_grid = snap_to_grid,
		.grid_size = 1.0F,
		.ray = front_gizmo_ray_from_screen(screen),
		.camera = front_gizmo_camera(),
	};
}

bool preview_color_nearly_equal(quader::tools::ToolPreviewColorSrgb left,
		quader::tools::ToolPreviewColorSrgb right) {
	constexpr float kEpsilon = 0.0001F;
	return std::abs(left.red - right.red) <= kEpsilon &&
			std::abs(left.green - right.green) <= kEpsilon &&
			std::abs(left.blue - right.blue) <= kEpsilon &&
			std::abs(left.alpha - right.alpha) <= kEpsilon;
}

std::size_t count_preview_triangles_with_color(const quader::tools::ToolPreview &preview,
		quader::tools::ToolPreviewColorSrgb color) {
	return static_cast<std::size_t>(std::count_if(preview.colored_world_triangles.begin(),
			preview.colored_world_triangles.end(),
			[color](const quader::tools::ToolPreviewColoredWorldTriangle &triangle) {
				return preview_color_nearly_equal(triangle.color_srgb, color);
			}));
}

std::size_t count_preview_segments_with_color(const quader::tools::ToolPreview &preview,
		quader::tools::ToolPreviewColorSrgb color) {
	return static_cast<std::size_t>(std::count_if(preview.colored_world_segments.begin(),
			preview.colored_world_segments.end(),
			[color](const quader::tools::ToolPreviewColoredWorldSegment &segment) {
				return preview_color_nearly_equal(segment.color_srgb, color);
			}));
}

std::size_t count_preview_segments_with_thickness(const quader::tools::ToolPreview &preview,
		float thickness_px) {
	return static_cast<std::size_t>(std::count_if(preview.colored_world_segments.begin(),
			preview.colored_world_segments.end(),
			[thickness_px](const quader::tools::ToolPreviewColoredWorldSegment &segment) {
				return std::abs(segment.thickness_px - thickness_px) <= 0.0001F;
			}));
}

void select_object(quader::document::Document &document,
		quader::document::ObjectId object) {
	quader::document::Selection selection;
	ASSERT_TRUE(selection.set_objects({ object }));
	ASSERT_TRUE(document.set_selection(std::move(selection)));
	document.clear_dirty();
	discard_pending_changes(document);
}

class RecordingTool final : public quader::tools::ITool {
public:
	explicit RecordingTool(quader::tools::ToolId tool_id,
			std::optional<quader::document::ObjectId> command_object = std::nullopt) : tool_id_(tool_id),
																					   command_object_(command_object) {
	}

	[[nodiscard]] quader::tools::ToolId id() const noexcept override {
		return tool_id_;
	}

	void activate(quader::tools::ToolContext &context) override {
		(void)context;
		++activations;
	}

	void deactivate(quader::tools::ToolContext &context) override {
		(void)context;
		++deactivations;
		preview_data.clear();
	}

	void cancel(quader::tools::ToolContext &context) override {
		(void)context;
		++cancellations;
		preview_data.clear();
	}

	[[nodiscard]] bool on_pointer_event(const quader::tools::PointerEvent &event,
			quader::tools::ToolContext &context) override {
		(void)context;
		++pointer_events;
		last_pointer = event;
		preview_data.clear();
		preview_data.active = true;
		preview_data.status_text = "pointer";
		preview_data.points.push_back(event.position);
		return true;
	}

	[[nodiscard]] bool on_key_event(const quader::tools::KeyEvent &event,
			quader::tools::ToolContext &context) override {
		++key_events;
		last_key = event;
		preview_data.clear();
		preview_data.active = true;
		preview_data.status_text = "key";

		if (command_object_.has_value() && event.pressed && !event.auto_repeat) {
			auto result = context.execute_command(
					std::make_unique<quader::commands::RenameObjectCommand>(*command_object_,
							"Renamed By Tool"));
			last_command_status = result.status;
			return result.is_applied();
		}

		return true;
	}

	[[nodiscard]] quader::tools::ToolPreview preview() const override {
		return preview_data;
	}

	int activations = 0;
	int deactivations = 0;
	int cancellations = 0;
	int pointer_events = 0;
	int key_events = 0;
	std::optional<quader::tools::PointerEvent> last_pointer;
	std::optional<quader::tools::KeyEvent> last_key;
	std::optional<quader::commands::CommandStatus> last_command_status;
	quader::tools::ToolPreview preview_data;

private:
	quader::tools::ToolId tool_id_;
	std::optional<quader::document::ObjectId> command_object_;
};

class PassiveTool final : public quader::tools::ITool {
public:
	[[nodiscard]] quader::tools::ToolId id() const noexcept override {
		return quader::tools::ToolId::Select;
	}
};

[[nodiscard]] std::uint64_t encoded_object_id(quader::document::ObjectId object) noexcept {
	return (static_cast<std::uint64_t>(object.generation()) << 32U) |
			static_cast<std::uint64_t>(object.index());
}

TEST(ToolManager, ToolActivationRegistersAndReusesActiveTool) {
	quader::document::Document document;
	quader::commands::CommandHistory history;
	quader::tools::ToolManager manager{ quader::tools::ToolContext{ document, history } };

	auto select_tool = std::make_unique<RecordingTool>(quader::tools::ToolId::Select);
	auto *select = select_tool.get();

	EXPECT_TRUE(manager.register_tool(std::move(select_tool)));
	EXPECT_TRUE(manager.has_tool(quader::tools::ToolId::Select));
	EXPECT_FALSE(manager.has_tool(quader::tools::ToolId::Move));
	EXPECT_FALSE(manager.register_tool(nullptr));
	EXPECT_FALSE(manager.register_tool(
			std::make_unique<RecordingTool>(quader::tools::ToolId::Select)));

	EXPECT_TRUE(manager.set_active_tool(quader::tools::ToolId::Select));
	const auto kInitialActiveToolId = manager.active_tool_id();
	if (!kInitialActiveToolId.has_value()) {
		ADD_FAILURE() << "active tool id was not set";
		return;
	}
	EXPECT_EQ(*kInitialActiveToolId, quader::tools::ToolId::Select);
	EXPECT_EQ(manager.active_tool(), static_cast<quader::tools::ITool *>(select));
	EXPECT_EQ(select->activations, 1);
	EXPECT_EQ(select->deactivations, 0);

	EXPECT_TRUE(manager.set_active_tool(quader::tools::ToolId::Select));
	EXPECT_EQ(select->activations, 1);
	EXPECT_EQ(select->deactivations, 0);
	EXPECT_FALSE(manager.set_active_tool(quader::tools::ToolId::Move));
	const auto kRejectedActiveToolId = manager.active_tool_id();
	if (!kRejectedActiveToolId.has_value()) {
		ADD_FAILURE() << "active tool id was cleared after rejected switch";
		return;
	}
	EXPECT_EQ(*kRejectedActiveToolId, quader::tools::ToolId::Select);
}

TEST(ToolManager, NoToolIgnoresEventsAndPreservesDocumentState) {
	quader::document::Document document;
	quader::commands::CommandHistory history;
	quader::tools::ToolManager manager{ quader::tools::ToolContext{ document, history } };
	const auto kBefore = capture_tool_state(document);

	EXPECT_FALSE(manager.active_tool_id().has_value());
	EXPECT_TRUE(manager.active_tool() == nullptr);
	EXPECT_TRUE(manager.preview().empty());
	EXPECT_FALSE(manager.cancel_active_tool());
	EXPECT_FALSE(manager.dispatch_pointer_event(quader::tools::PointerEvent{
			quader::math::Vec2{ 1.0F, 2.0F },
			quader::tools::PointerButton::Left,
			true,
	}));
	EXPECT_FALSE(manager.dispatch_key_event(quader::tools::KeyEvent{ 65, true, false }));
	EXPECT_FALSE(manager.set_active_tool(quader::tools::ToolId::Select));
	EXPECT_EQ(capture_tool_state(document), kBefore);
	EXPECT_FALSE(history.can_undo());
	EXPECT_FALSE(history.can_redo());
}

TEST(ToolManager, SelectFallbackReplacesObjectSelectionThroughCommandHistory) {
	auto fixture = quader::tests::document_fixtures::make_document_with_triangle_object();
	fixture.document.clear_dirty();
	discard_pending_changes(fixture.document);

	quader::commands::CommandHistory history;
	quader::tools::ToolManager manager{ quader::tools::ToolContext{ fixture.document, history } };
	int applied_callback_count = 0;
	manager.context().set_after_command_applied([&applied_callback_count]() {
		++applied_callback_count;
	});
	EXPECT_TRUE(manager.register_tool(std::make_unique<PassiveTool>()));
	EXPECT_TRUE(manager.set_active_tool(quader::tools::ToolId::Select));

	auto event = surface_event({ 0.0F, 0.0F, 0.0F },
			{ 0.0F, 1.0F, 0.0F },
			{ 10.0F, 12.0F },
			quader::tools::PointerPhase::Press,
			quader::tools::PointerButton::Left,
			true);
	event.surface_hit->object_id = encoded_object_id(fixture.object);

	EXPECT_TRUE(manager.dispatch_pointer_event(event));
	ASSERT_EQ(fixture.document.selection().selected_objects().size(), 1U);
	EXPECT_EQ(fixture.document.selection().selected_objects().front(), fixture.object);
	EXPECT_TRUE(fixture.document.selection().selected_components().empty());
	EXPECT_TRUE(history.can_undo());
	EXPECT_EQ(history.undo_name(), std::string_view("Set Selection"));
	EXPECT_EQ(applied_callback_count, 1);

	auto result = history.undo(fixture.document);
	EXPECT_EQ((result).status, quader::commands::CommandStatus::Applied);
	EXPECT_TRUE(fixture.document.selection().empty());
}

TEST(ToolManager, SelectHoverStoresHitWithoutMutatingSelectionOrHistory) {
	auto fixture = quader::tests::document_fixtures::make_document_with_triangle_object();
	fixture.document.clear_dirty();
	discard_pending_changes(fixture.document);

	quader::commands::CommandHistory history;
	quader::tools::ToolManager manager{ quader::tools::ToolContext{ fixture.document, history } };
	EXPECT_TRUE(manager.register_tool(std::make_unique<PassiveTool>()));
	EXPECT_TRUE(manager.set_active_tool(quader::tools::ToolId::Select));
	EXPECT_TRUE(manager.set_selection_mode(quader::tools::SelectionMode::Face));
	const auto kBefore = capture_tool_state(fixture.document);

	auto event = surface_event({ 0.0F, 0.0F, 0.0F },
			{ 0.0F, 1.0F, 0.0F },
			{ 10.0F, 12.0F },
			quader::tools::PointerPhase::Hover);
	event.surface_hit->object_id = encoded_object_id(fixture.object);
	event.surface_hit->document_object_id = fixture.object;
	event.surface_hit->component_index = fixture.face.index();
	event.surface_hit->component = fixture.face;
	event.surface_hit->kind = quader::tools::SurfaceHitKind::Face;

	EXPECT_TRUE(manager.dispatch_pointer_event(event));
	ASSERT_TRUE(manager.selection_hover().has_value());
	EXPECT_EQ(manager.selection_hover()->document_object_id, fixture.object);
	EXPECT_TRUE(std::holds_alternative<quader::mesh::FaceId>(manager.selection_hover()->component));
	EXPECT_EQ(std::get<quader::mesh::FaceId>(manager.selection_hover()->component), fixture.face);
	EXPECT_TRUE(fixture.document.selection().empty());
	EXPECT_FALSE(history.can_undo());
	EXPECT_EQ(capture_tool_state(fixture.document), kBefore);

	EXPECT_FALSE(manager.dispatch_pointer_event(event));

	event.surface_hit.reset();
	EXPECT_TRUE(manager.dispatch_pointer_event(event));
	EXPECT_FALSE(manager.selection_hover().has_value());
	EXPECT_TRUE(fixture.document.selection().empty());
	EXPECT_FALSE(history.can_undo());
	EXPECT_EQ(capture_tool_state(fixture.document), kBefore);
}

TEST(ToolManager, SelectClickClearsAndSuppressesSameComponentHover) {
	auto fixture = quader::tests::document_fixtures::make_document_with_triangle_object();
	quader::document::Selection selection;
	ASSERT_TRUE(selection.set_component_selection(quader::document::SelectionMode::Face,
			{ fixture.object },
			{ quader::document::ComponentRef{ fixture.object, fixture.face } }));
	ASSERT_TRUE(fixture.document.set_selection(std::move(selection)));
	fixture.document.clear_dirty();
	discard_pending_changes(fixture.document);

	quader::commands::CommandHistory history;
	quader::tools::ToolManager manager{ quader::tools::ToolContext{ fixture.document, history } };
	EXPECT_TRUE(manager.register_tool(std::make_unique<PassiveTool>()));
	EXPECT_TRUE(manager.set_active_tool(quader::tools::ToolId::Select));
	EXPECT_TRUE(manager.set_selection_mode(quader::tools::SelectionMode::Face));

	auto event = surface_event({ 0.0F, 0.0F, 0.0F },
			{ 0.0F, 1.0F, 0.0F },
			{ 10.0F, 12.0F },
			quader::tools::PointerPhase::Hover);
	event.surface_hit->object_id = encoded_object_id(fixture.object);
	event.surface_hit->document_object_id = fixture.object;
	event.surface_hit->component_index = fixture.face.index();
	event.surface_hit->component = fixture.face;
	event.surface_hit->kind = quader::tools::SurfaceHitKind::Face;

	EXPECT_TRUE(manager.dispatch_pointer_event(event));
	ASSERT_TRUE(manager.selection_hover().has_value());
	EXPECT_FALSE(manager.selection_hover_suppresses_selected());

	event.phase = quader::tools::PointerPhase::Press;
	event.button = quader::tools::PointerButton::Left;
	event.pressed = true;
	EXPECT_TRUE(manager.dispatch_pointer_event(event));
	EXPECT_FALSE(manager.selection_hover().has_value());
	EXPECT_FALSE(manager.selection_hover_suppresses_selected());
	EXPECT_FALSE(history.can_undo());
	ASSERT_EQ(fixture.document.selection().selected_components().size(), 1U);
	EXPECT_EQ(fixture.document.selection().selected_components().front().object, fixture.object);
	ASSERT_TRUE(std::holds_alternative<quader::mesh::FaceId>(
			fixture.document.selection().selected_components().front().component));
	EXPECT_EQ(std::get<quader::mesh::FaceId>(
					  fixture.document.selection().selected_components().front().component),
			fixture.face);

	event.phase = quader::tools::PointerPhase::Hover;
	event.button = quader::tools::PointerButton::None;
	event.pressed = false;
	EXPECT_FALSE(manager.dispatch_pointer_event(event));
	EXPECT_FALSE(manager.selection_hover().has_value());

	event.modifiers.shift = true;
	EXPECT_TRUE(manager.dispatch_pointer_event(event));
	ASSERT_TRUE(manager.selection_hover().has_value());
	EXPECT_TRUE(manager.selection_hover_suppresses_selected());
}

TEST(ToolManager, SelectEmptyObjectModeClickClearsSelectionThroughHistory) {
	auto fixture = quader::tests::document_fixtures::make_document_with_triangle_object();
	quader::document::Selection selection;
	ASSERT_TRUE(selection.set_objects({ fixture.object }));
	ASSERT_TRUE(fixture.document.set_selection(selection));
	fixture.document.clear_dirty();
	discard_pending_changes(fixture.document);

	quader::commands::CommandHistory history;
	quader::tools::ToolManager manager{ quader::tools::ToolContext{ fixture.document, history } };
	EXPECT_TRUE(manager.register_tool(std::make_unique<PassiveTool>()));
	EXPECT_TRUE(manager.set_active_tool(quader::tools::ToolId::Select));

	EXPECT_TRUE(manager.dispatch_pointer_event(quader::tools::PointerEvent{
			.position = { 10.0F, 12.0F },
			.button = quader::tools::PointerButton::Left,
			.pressed = true,
			.phase = quader::tools::PointerPhase::Press,
	}));
	EXPECT_TRUE(fixture.document.selection().empty());
	EXPECT_TRUE(history.can_undo());
	EXPECT_EQ(history.undo_name(), std::string_view("Set Selection"));

	auto result = history.undo(fixture.document);
	EXPECT_EQ((result).status, quader::commands::CommandStatus::Applied);
	ASSERT_EQ(fixture.document.selection().selected_objects().size(), 1U);
	EXPECT_EQ(fixture.document.selection().selected_objects().front(), fixture.object);
}

TEST(ToolManager, SelectEmptyComponentModeClickClearsTargetsAndPreservesMode) {
	auto fixture = quader::tests::document_fixtures::make_document_with_triangle_object();
	quader::document::Selection selection;
	ASSERT_TRUE(selection.set_component_selection(quader::document::SelectionMode::Edge,
			{ fixture.object },
			{ quader::document::ComponentRef{ fixture.object, fixture.edge } }));
	ASSERT_TRUE(fixture.document.set_selection(selection));
	fixture.document.clear_dirty();
	discard_pending_changes(fixture.document);

	quader::commands::CommandHistory history;
	quader::tools::ToolManager manager{ quader::tools::ToolContext{ fixture.document, history } };
	EXPECT_TRUE(manager.register_tool(std::make_unique<PassiveTool>()));
	EXPECT_TRUE(manager.set_active_tool(quader::tools::ToolId::Select));
	EXPECT_TRUE(manager.set_selection_mode(quader::tools::SelectionMode::Edge));

	EXPECT_TRUE(manager.dispatch_pointer_event(quader::tools::PointerEvent{
			.position = { 10.0F, 12.0F },
			.button = quader::tools::PointerButton::Left,
			.pressed = true,
			.phase = quader::tools::PointerPhase::Press,
	}));
	EXPECT_EQ(fixture.document.selection().mode(), quader::document::SelectionMode::Edge);
	EXPECT_TRUE(fixture.document.selection().selected_objects().empty());
	EXPECT_TRUE(fixture.document.selection().selected_components().empty());
	EXPECT_TRUE(history.can_undo());

	auto result = history.undo(fixture.document);
	EXPECT_EQ((result).status, quader::commands::CommandStatus::Applied);
	EXPECT_EQ(fixture.document.selection(), selection);
}

TEST(ToolManager, SelectFallbackResolvesFaceHitToLiveComponentSelection) {
	auto fixture = quader::tests::document_fixtures::make_document_with_triangle_object();
	const auto kFaces = fixture.document.find_mesh_object(fixture.object)->mesh.face_ids();
	ASSERT_EQ(kFaces.size(), 1U);
	fixture.document.clear_dirty();
	discard_pending_changes(fixture.document);

	quader::commands::CommandHistory history;
	quader::tools::ToolManager manager{ quader::tools::ToolContext{ fixture.document, history } };
	EXPECT_TRUE(manager.register_tool(std::make_unique<PassiveTool>()));
	EXPECT_TRUE(manager.set_active_tool(quader::tools::ToolId::Select));
	EXPECT_TRUE(manager.set_selection_mode(quader::tools::SelectionMode::Face));

	auto event = surface_event({ 0.0F, 0.0F, 0.0F },
			{ 0.0F, 1.0F, 0.0F },
			{ 10.0F, 12.0F },
			quader::tools::PointerPhase::Press,
			quader::tools::PointerButton::Left,
			true);
	event.surface_hit->object_id = encoded_object_id(fixture.object);
	event.surface_hit->document_object_id = fixture.object;
	event.surface_hit->component_index = kFaces.front().index();
	event.surface_hit->component = kFaces.front();

	EXPECT_TRUE(manager.dispatch_pointer_event(event));
	const auto kComponents = fixture.document.selection().selected_components();
	ASSERT_EQ(kComponents.size(), 1U);
	EXPECT_EQ(kComponents.front().object, fixture.object);
	EXPECT_EQ(quader::document::component_kind(kComponents.front()), quader::document::ComponentKind::Face);
	EXPECT_TRUE(std::holds_alternative<quader::mesh::FaceId>(kComponents.front().component));
	EXPECT_EQ(std::get<quader::mesh::FaceId>(kComponents.front().component), kFaces.front());

	event.modifiers.shift = true;
	EXPECT_TRUE(manager.dispatch_pointer_event(event));
	EXPECT_EQ(fixture.document.selection().mode(), quader::document::SelectionMode::Face);
	ASSERT_EQ(fixture.document.selection().selected_objects().size(), 1U);
	EXPECT_EQ(fixture.document.selection().selected_objects().front(), fixture.object);
	EXPECT_TRUE(fixture.document.selection().selected_components().empty());
}

TEST(ToolManager, SelectFallbackResolvesEdgeHitToLiveComponentSelection) {
	auto fixture = quader::tests::document_fixtures::make_document_with_triangle_object();
	fixture.document.clear_dirty();
	discard_pending_changes(fixture.document);

	quader::commands::CommandHistory history;
	quader::tools::ToolManager manager{ quader::tools::ToolContext{ fixture.document, history } };
	EXPECT_TRUE(manager.register_tool(std::make_unique<PassiveTool>()));
	EXPECT_TRUE(manager.set_active_tool(quader::tools::ToolId::Select));
	EXPECT_TRUE(manager.set_selection_mode(quader::tools::SelectionMode::Edge));

	auto event = surface_event({ 0.0F, 0.0F, 0.0F },
			{ 0.0F, 1.0F, 0.0F },
			{ 10.0F, 12.0F },
			quader::tools::PointerPhase::Press,
			quader::tools::PointerButton::Left,
			true);
	event.surface_hit->object_id = encoded_object_id(fixture.object);
	event.surface_hit->document_object_id = fixture.object;
	event.surface_hit->component_index = fixture.edge.index();
	event.surface_hit->component = fixture.edge;
	event.surface_hit->kind = quader::tools::SurfaceHitKind::Edge;

	EXPECT_TRUE(manager.dispatch_pointer_event(event));
	EXPECT_EQ(fixture.document.selection().mode(), quader::document::SelectionMode::Edge);
	ASSERT_EQ(fixture.document.selection().selected_objects().size(), 1U);
	EXPECT_EQ(fixture.document.selection().selected_objects().front(), fixture.object);
	const auto kComponents = fixture.document.selection().selected_components();
	ASSERT_EQ(kComponents.size(), 1U);
	EXPECT_EQ(kComponents.front().object, fixture.object);
	EXPECT_EQ(quader::document::component_kind(kComponents.front()), quader::document::ComponentKind::Edge);
	ASSERT_TRUE(std::holds_alternative<quader::mesh::EdgeId>(kComponents.front().component));
	EXPECT_EQ(std::get<quader::mesh::EdgeId>(kComponents.front().component), fixture.edge);
}

TEST(ToolManager, SelectFallbackResolvesVertexHitToLiveComponentSelection) {
	auto fixture = quader::tests::document_fixtures::make_document_with_triangle_object();
	fixture.document.clear_dirty();
	discard_pending_changes(fixture.document);

	quader::commands::CommandHistory history;
	quader::tools::ToolManager manager{ quader::tools::ToolContext{ fixture.document, history } };
	EXPECT_TRUE(manager.register_tool(std::make_unique<PassiveTool>()));
	EXPECT_TRUE(manager.set_active_tool(quader::tools::ToolId::Select));
	EXPECT_TRUE(manager.set_selection_mode(quader::tools::SelectionMode::Vertex));

	auto event = surface_event({ 0.0F, 0.0F, 0.0F },
			{ 0.0F, 1.0F, 0.0F },
			{ 10.0F, 12.0F },
			quader::tools::PointerPhase::Press,
			quader::tools::PointerButton::Left,
			true);
	event.surface_hit->object_id = encoded_object_id(fixture.object);
	event.surface_hit->document_object_id = fixture.object;
	event.surface_hit->component_index = fixture.vertices.front().index();
	event.surface_hit->component = fixture.vertices.front();
	event.surface_hit->kind = quader::tools::SurfaceHitKind::Vertex;

	EXPECT_TRUE(manager.dispatch_pointer_event(event));
	EXPECT_EQ(fixture.document.selection().mode(), quader::document::SelectionMode::Vertex);
	ASSERT_EQ(fixture.document.selection().selected_objects().size(), 1U);
	EXPECT_EQ(fixture.document.selection().selected_objects().front(), fixture.object);
	const auto kComponents = fixture.document.selection().selected_components();
	ASSERT_EQ(kComponents.size(), 1U);
	EXPECT_EQ(kComponents.front().object, fixture.object);
	EXPECT_EQ(quader::document::component_kind(kComponents.front()), quader::document::ComponentKind::Vertex);
	ASSERT_TRUE(std::holds_alternative<quader::mesh::VertexId>(kComponents.front().component));
	EXPECT_EQ(std::get<quader::mesh::VertexId>(kComponents.front().component), fixture.vertices.front());
}

TEST(ToolManager, SelectFallbackRetainsEditTargetOnComponentModeBodyHit) {
	for (const quader::tools::SelectionMode kMode : { quader::tools::SelectionMode::Edge, quader::tools::SelectionMode::Vertex }) {
		auto fixture = quader::tests::document_fixtures::make_document_with_triangle_object();
		fixture.document.clear_dirty();
		discard_pending_changes(fixture.document);

		quader::commands::CommandHistory history;
		quader::tools::ToolManager manager{ quader::tools::ToolContext{ fixture.document, history } };
		EXPECT_TRUE(manager.register_tool(std::make_unique<PassiveTool>()));
		EXPECT_TRUE(manager.set_active_tool(quader::tools::ToolId::Select));
		EXPECT_TRUE(manager.set_selection_mode(kMode));

		auto event = surface_event({ 0.0F, 0.0F, 0.0F },
				{ 0.0F, 1.0F, 0.0F },
				{ 10.0F, 12.0F },
				quader::tools::PointerPhase::Press,
				quader::tools::PointerButton::Left,
				true);
		event.surface_hit->object_id = encoded_object_id(fixture.object);
		event.surface_hit->document_object_id = fixture.object;
		event.surface_hit->kind = quader::tools::SurfaceHitKind::Object;

		EXPECT_TRUE(manager.dispatch_pointer_event(event));
		const quader::document::SelectionMode kExpectedDocumentMode = kMode == quader::tools::SelectionMode::Edge
				? quader::document::SelectionMode::Edge
				: quader::document::SelectionMode::Vertex;
		EXPECT_EQ(fixture.document.selection().mode(), kExpectedDocumentMode);
		ASSERT_EQ(fixture.document.selection().selected_objects().size(), 1U);
		EXPECT_EQ(fixture.document.selection().selected_objects().front(), fixture.object);
		EXPECT_TRUE(fixture.document.selection().selected_components().empty());
		EXPECT_TRUE(history.can_undo());
		EXPECT_EQ(history.undo_name(), std::string_view("Set Selection"));
	}
}

TEST(ToolManager, EventsForwardToActiveToolAndPreviewIsReported) {
	quader::document::Document document;
	quader::commands::CommandHistory history;
	quader::tools::ToolManager manager{ quader::tools::ToolContext{ document, history } };

	auto tool = std::make_unique<RecordingTool>(quader::tools::ToolId::Select);
	auto *recording_tool = tool.get();
	EXPECT_TRUE(manager.register_tool(std::move(tool)));
	EXPECT_TRUE(manager.set_active_tool(quader::tools::ToolId::Select));

	const quader::tools::PointerEvent kPointer{
		quader::math::Vec2{ 12.0F, 24.0F },
		quader::tools::PointerButton::Right,
		true,
	};
	EXPECT_TRUE(manager.dispatch_pointer_event(kPointer));
	EXPECT_EQ(recording_tool->pointer_events, 1);
	if (!recording_tool->last_pointer.has_value()) {
		ADD_FAILURE() << "active tool did not record pointer event";
		return;
	}
	const auto &last_pointer = *recording_tool->last_pointer;
	EXPECT_TRUE(vec2_exactly_equal(last_pointer.position, kPointer.position));
	EXPECT_EQ(last_pointer.button, quader::tools::PointerButton::Right);
	EXPECT_TRUE(last_pointer.pressed);

	auto preview = manager.preview();
	EXPECT_FALSE(preview.empty());
	EXPECT_TRUE(preview.active);
	EXPECT_EQ(preview.status_text, std::string("pointer"));
	EXPECT_EQ(preview.points.size(), 1U);
	EXPECT_TRUE(vec2_exactly_equal(preview.points.front(), kPointer.position));

	const quader::tools::KeyEvent kEy{ 90, true, false };
	EXPECT_TRUE(manager.dispatch_key_event(kEy));
	EXPECT_EQ(recording_tool->key_events, 1);
	if (!recording_tool->last_key.has_value()) {
		ADD_FAILURE() << "active tool did not record key event";
		return;
	}
	const auto &last_key = *recording_tool->last_key;
	EXPECT_EQ(last_key.key_code, 90);
	EXPECT_TRUE(last_key.pressed);
	EXPECT_FALSE(last_key.auto_repeat);
	EXPECT_EQ(manager.preview().status_text, std::string("key"));
}

TEST(ToolManager, ToolCommitsDocumentChangesOnlyThroughCommandHistory) {
	auto fixture = quader::tests::document_fixtures::make_document_with_triangle_object();
	fixture.document.clear_dirty();
	discard_pending_changes(fixture.document);
	const auto kBefore = capture_tool_state(fixture.document);

	quader::commands::CommandHistory history;
	quader::tools::ToolManager manager{ quader::tools::ToolContext{ fixture.document, history } };
	int applied_callback_count = 0;
	manager.context().set_after_command_applied([&applied_callback_count]() {
		++applied_callback_count;
	});

	auto tool = std::make_unique<RecordingTool>(quader::tools::ToolId::Select, fixture.object);
	auto *recording_tool = tool.get();
	EXPECT_TRUE(manager.register_tool(std::move(tool)));
	EXPECT_TRUE(manager.set_active_tool(quader::tools::ToolId::Select));

	EXPECT_TRUE(manager.dispatch_pointer_event(quader::tools::PointerEvent{
			quader::math::Vec2{ 3.0F, 4.0F },
			quader::tools::PointerButton::Left,
			true,
	}));
	EXPECT_EQ(capture_tool_state(fixture.document), kBefore);
	EXPECT_FALSE(history.can_undo());
	EXPECT_EQ(applied_callback_count, 0);

	EXPECT_TRUE(manager.dispatch_key_event(quader::tools::KeyEvent{ 13, true, false }));
	if (!recording_tool->last_command_status.has_value()) {
		ADD_FAILURE() << "tool command did not report a status";
		return;
	}
	EXPECT_EQ(*recording_tool->last_command_status, quader::commands::CommandStatus::Applied);
	EXPECT_TRUE(history.can_undo());
	EXPECT_EQ(history.undo_name(), std::string_view("Rename Object"));
	EXPECT_EQ(applied_callback_count, 1);

	const auto kAfter = capture_tool_state(fixture.document);
	EXPECT_EQ(kAfter.objects.size(), 1U);
	EXPECT_EQ(kAfter.objects.front().name, std::string("Renamed By Tool"));

	auto result = history.undo(fixture.document);
	EXPECT_EQ((result).status, quader::commands::CommandStatus::Applied);
	EXPECT_EQ(capture_tool_state(fixture.document), kBefore);

	result = history.redo(fixture.document);
	EXPECT_EQ((result).status, quader::commands::CommandStatus::Applied);
	EXPECT_EQ(capture_tool_state(fixture.document), kAfter);
}

TEST(ToolManager, SwitchingAndCancelSemanticsAreExplicit) {
	quader::document::Document document;
	quader::commands::CommandHistory history;
	quader::tools::ToolManager manager{ quader::tools::ToolContext{ document, history } };

	auto select_tool = std::make_unique<RecordingTool>(quader::tools::ToolId::Select);
	auto move_tool = std::make_unique<RecordingTool>(quader::tools::ToolId::Move);
	auto *select = select_tool.get();
	auto *move = move_tool.get();

	EXPECT_TRUE(manager.register_tool(std::move(select_tool)));
	EXPECT_TRUE(manager.register_tool(std::move(move_tool)));
	EXPECT_TRUE(manager.set_active_tool(quader::tools::ToolId::Select));

	EXPECT_TRUE(manager.dispatch_pointer_event(quader::tools::PointerEvent{
			quader::math::Vec2{ 5.0F, 6.0F },
			quader::tools::PointerButton::Left,
			true,
	}));
	EXPECT_FALSE(manager.preview().empty());

	EXPECT_TRUE(manager.cancel_active_tool());
	EXPECT_EQ(select->cancellations, 1);
	const auto kCanceledActiveToolId = manager.active_tool_id();
	if (!kCanceledActiveToolId.has_value()) {
		ADD_FAILURE() << "cancel cleared active tool id";
		return;
	}
	EXPECT_EQ(*kCanceledActiveToolId, quader::tools::ToolId::Select);
	EXPECT_TRUE(manager.preview().empty());

	EXPECT_TRUE(manager.set_active_tool(quader::tools::ToolId::Move));
	EXPECT_EQ(select->deactivations, 1);
	EXPECT_EQ(move->activations, 1);
	const auto kSwitchedActiveToolId = manager.active_tool_id();
	if (!kSwitchedActiveToolId.has_value()) {
		ADD_FAILURE() << "switch did not set active tool id";
		return;
	}
	EXPECT_EQ(*kSwitchedActiveToolId, quader::tools::ToolId::Move);

	manager.clear_active_tool();
	EXPECT_EQ(move->deactivations, 1);
	EXPECT_FALSE(manager.active_tool_id().has_value());
	EXPECT_TRUE(manager.active_tool() == nullptr);

	manager.clear_active_tool();
	EXPECT_EQ(move->deactivations, 1);
	EXPECT_FALSE(manager.dispatch_key_event(quader::tools::KeyEvent{ 27, true, false }));
}

TEST(TransformGizmoTool, MoveHoverShowsGodotStyleAxisPreviewForSelection) {
	auto fixture = quader::tests::document_fixtures::make_document_with_triangle_object();
	select_object(fixture.document, fixture.object);

	quader::commands::CommandHistory history;
	quader::tools::ToolManager manager{ quader::tools::ToolContext{ fixture.document, history } };
	EXPECT_TRUE(manager.register_tool(std::make_unique<quader::tools::TransformGizmoTool>(
			quader::tools::ToolId::Move)));
	EXPECT_TRUE(manager.set_active_tool(quader::tools::ToolId::Move));

	const quader::math::Vec3 kPivot{ 0.5F, 0.5F, 0.0F };
	(void)kPivot;
	EXPECT_TRUE(manager.dispatch_pointer_event(front_gizmo_event({ 180.0F, 240.0F },
			quader::tools::PointerPhase::Hover)));

	const auto *tool = static_cast<const quader::tools::TransformGizmoTool *>(manager.active_tool());
	ASSERT_TRUE(tool != nullptr);
	EXPECT_EQ(tool->state().target_count, 1U);
	EXPECT_EQ(tool->state().hover_handle.kind, quader::tools::TransformGizmoHandleKind::Axis);
	EXPECT_EQ(tool->state().hover_handle.axis, quader::tools::TransformGizmoAxis::X);
	const quader::tools::ToolPreview kPreview = manager.preview();
	EXPECT_TRUE(kPreview.active);
	EXPECT_TRUE(kPreview.overlay_only);
	EXPECT_FALSE(kPreview.colored_world_segments.empty());
	EXPECT_EQ(fixture.document.find_mesh_object(fixture.object)->transform.translation.x, 0.0F);
	EXPECT_FALSE(history.can_undo());
}

TEST(TransformGizmoTool, MovePreviewUsesWindowsReferenceGizmoTopologyAndColors) {
	auto fixture = quader::tests::document_fixtures::make_document_with_triangle_object();
	select_object(fixture.document, fixture.object);

	quader::commands::CommandHistory history;
	quader::tools::ToolManager manager{ quader::tools::ToolContext{ fixture.document, history } };
	EXPECT_TRUE(manager.register_tool(std::make_unique<quader::tools::TransformGizmoTool>(
			quader::tools::ToolId::Move)));
	EXPECT_TRUE(manager.set_active_tool(quader::tools::ToolId::Move));

	EXPECT_TRUE(manager.dispatch_pointer_event(reference_gizmo_hover_event({ -1000.0F, -1000.0F })));
	const quader::tools::ToolPreview kPreview = manager.preview();
	EXPECT_TRUE(kPreview.active);
	EXPECT_EQ(kPreview.colored_world_triangles.size(), 102U);
	EXPECT_EQ(kPreview.colored_world_segments.size(), 15U);
	EXPECT_EQ(count_preview_segments_with_thickness(kPreview, 1.6F), 3U);
	EXPECT_EQ(count_preview_segments_with_thickness(kPreview, 1.2F), 12U);

	const quader::tools::ToolPreviewColorSrgb kX{ 245.0F / 255.0F, 51.0F / 255.0F, 82.0F / 255.0F, 230.0F / 255.0F };
	const quader::tools::ToolPreviewColorSrgb kY{ 135.0F / 255.0F, 214.0F / 255.0F, 3.0F / 255.0F, 230.0F / 255.0F };
	const quader::tools::ToolPreviewColorSrgb kZ{ 41.0F / 255.0F, 140.0F / 255.0F, 245.0F / 255.0F, 230.0F / 255.0F };
	EXPECT_EQ(count_preview_triangles_with_color(kPreview, kX), 34U);
	EXPECT_EQ(count_preview_triangles_with_color(kPreview, kY), 34U);
	EXPECT_EQ(count_preview_triangles_with_color(kPreview, kZ), 34U);
	EXPECT_EQ(count_preview_segments_with_color(kPreview, kX), 5U);
	EXPECT_EQ(count_preview_segments_with_color(kPreview, kY), 5U);
	EXPECT_EQ(count_preview_segments_with_color(kPreview, kZ), 5U);
	EXPECT_FALSE(history.can_undo());
}

TEST(TransformGizmoTool, ScalePreviewUsesReferencePrismsAndHoverOnlyCenterCube) {
	auto fixture = quader::tests::document_fixtures::make_document_with_triangle_object();
	select_object(fixture.document, fixture.object);

	quader::commands::CommandHistory history;
	quader::tools::ToolManager manager{ quader::tools::ToolContext{ fixture.document, history } };
	EXPECT_TRUE(manager.register_tool(std::make_unique<quader::tools::TransformGizmoTool>(
			quader::tools::ToolId::Scale)));
	EXPECT_TRUE(manager.set_active_tool(quader::tools::ToolId::Scale));

	EXPECT_TRUE(manager.dispatch_pointer_event(reference_gizmo_hover_event({ -1000.0F, -1000.0F })));
	quader::tools::ToolPreview preview = manager.preview();
	EXPECT_EQ(preview.colored_world_triangles.size(), 42U);
	EXPECT_EQ(preview.colored_world_segments.size(), 15U);
	EXPECT_EQ(count_preview_segments_with_thickness(preview, 1.6F), 3U);
	EXPECT_EQ(count_preview_segments_with_thickness(preview, 1.2F), 12U);

	const quader::tools::ToolPreviewColorSrgb kX{ 245.0F / 255.0F, 51.0F / 255.0F, 82.0F / 255.0F, 230.0F / 255.0F };
	const quader::tools::ToolPreviewColorSrgb kY{ 135.0F / 255.0F, 214.0F / 255.0F, 3.0F / 255.0F, 230.0F / 255.0F };
	const quader::tools::ToolPreviewColorSrgb kZ{ 41.0F / 255.0F, 140.0F / 255.0F, 245.0F / 255.0F, 230.0F / 255.0F };
	EXPECT_EQ(count_preview_triangles_with_color(preview, kX), 14U);
	EXPECT_EQ(count_preview_triangles_with_color(preview, kY), 14U);
	EXPECT_EQ(count_preview_triangles_with_color(preview, kZ), 14U);

	EXPECT_TRUE(manager.dispatch_pointer_event(reference_gizmo_hover_event({ 320.0F, 240.0F })));
	const auto *tool = static_cast<const quader::tools::TransformGizmoTool *>(manager.active_tool());
	ASSERT_TRUE(tool != nullptr);
	EXPECT_EQ(tool->state().hover_handle.kind, quader::tools::TransformGizmoHandleKind::Center);
	preview = manager.preview();
	EXPECT_GT(preview.colored_world_triangles.size(), 42U);
	const quader::tools::ToolPreviewColorSrgb kCubeOutline{ 255.0F / 255.0F, 246.0F / 255.0F, 128.0F / 255.0F, 180.0F / 255.0F };
	EXPECT_GT(count_preview_segments_with_color(preview, kCubeOutline), 0U);
}

TEST(TransformGizmoTool, RotatePreviewUsesReferenceTrackballAndFrontFacingRings) {
	auto fixture = quader::tests::document_fixtures::make_document_with_triangle_object();
	select_object(fixture.document, fixture.object);

	quader::commands::CommandHistory history;
	quader::tools::ToolManager manager{ quader::tools::ToolContext{ fixture.document, history } };
	EXPECT_TRUE(manager.register_tool(std::make_unique<quader::tools::TransformGizmoTool>(
			quader::tools::ToolId::Rotate)));
	EXPECT_TRUE(manager.set_active_tool(quader::tools::ToolId::Rotate));

	EXPECT_TRUE(manager.dispatch_pointer_event(reference_gizmo_hover_event({ -1000.0F, -1000.0F })));
	const quader::tools::ToolPreview kPreview = manager.preview();
	EXPECT_TRUE(kPreview.colored_world_triangles.empty());
	const quader::tools::ToolPreviewColorSrgb kTrackball{ 210.0F / 255.0F, 214.0F / 255.0F, 219.0F / 255.0F, 138.0F / 255.0F };
	EXPECT_EQ(count_preview_segments_with_color(kPreview, kTrackball), 64U);
	EXPECT_GT(count_preview_segments_with_thickness(kPreview, 3.0F), 0U);
	EXPECT_LT(count_preview_segments_with_thickness(kPreview, 3.0F), 192U);
	EXPECT_EQ(count_preview_segments_with_thickness(kPreview, 2.25F), 64U);
}

TEST(TransformGizmoTool, MoveAxisDragCommitsBatchTransformAndUndo) {
	auto fixture = quader::tests::document_fixtures::make_document_with_triangle_object();
	select_object(fixture.document, fixture.object);

	quader::commands::CommandHistory history;
	quader::tools::ToolManager manager{ quader::tools::ToolContext{ fixture.document, history } };
	int preview_callback_count = 0;
	manager.context().set_after_preview_mutation_applied([&preview_callback_count]() {
		++preview_callback_count;
	});
	EXPECT_TRUE(manager.register_tool(std::make_unique<quader::tools::TransformGizmoTool>(
			quader::tools::ToolId::Move)));
	EXPECT_TRUE(manager.set_active_tool(quader::tools::ToolId::Move));

	EXPECT_TRUE(manager.dispatch_pointer_event(front_gizmo_event({ 180.0F, 240.0F },
			quader::tools::PointerPhase::Press,
			quader::tools::PointerButton::Left,
			true,
			false)));
	EXPECT_TRUE(manager.dispatch_pointer_event(front_gizmo_event({ 170.0F, 240.0F },
			quader::tools::PointerPhase::Move,
			quader::tools::PointerButton::None,
			false,
			false)));
	const quader::tools::ToolPreview kDragPreview = manager.preview();
	EXPECT_FALSE(kDragPreview.empty());
	const auto *object_before_release = fixture.document.find_mesh_object(fixture.object);
	ASSERT_TRUE(object_before_release != nullptr);
	EXPECT_TRUE(quader::math::nearly_equal(object_before_release->transform.translation,
			quader::math::Vec3{ 1.0F, 0.0F, 0.0F }));
	EXPECT_FALSE(history.can_undo());
	EXPECT_FALSE(fixture.document.is_dirty());
	EXPECT_EQ(preview_callback_count, 1);
	EXPECT_TRUE(manager.dispatch_pointer_event(front_gizmo_event({ 170.0F, 240.0F },
			quader::tools::PointerPhase::Release,
			quader::tools::PointerButton::Left,
			false,
			false)));

	const auto *object = fixture.document.find_mesh_object(fixture.object);
	ASSERT_TRUE(object != nullptr);
	EXPECT_TRUE(quader::math::nearly_equal(object->transform.translation,
			quader::math::Vec3{ 1.0F, 0.0F, 0.0F }));
	EXPECT_TRUE(history.can_undo());
	EXPECT_EQ(history.undo_name(), std::string_view("Transform Objects"));
	EXPECT_TRUE(fixture.document.is_dirty());
	EXPECT_GE(preview_callback_count, 2);

	auto result = history.undo(fixture.document);
	EXPECT_EQ((result).status, quader::commands::CommandStatus::Applied);
	object = fixture.document.find_mesh_object(fixture.object);
	ASSERT_TRUE(object != nullptr);
	EXPECT_TRUE(quader::math::nearly_equal(object->transform.translation,
			quader::math::Vec3{}));
}

TEST(TransformGizmoTool, EmptyClickFallsBackToSelectionClearWhileMoveToolIsActive) {
	auto fixture = quader::tests::document_fixtures::make_document_with_triangle_object();
	select_object(fixture.document, fixture.object);

	quader::commands::CommandHistory history;
	quader::tools::ToolManager manager{ quader::tools::ToolContext{ fixture.document, history } };
	EXPECT_TRUE(manager.register_tool(std::make_unique<quader::tools::TransformGizmoTool>(
			quader::tools::ToolId::Move)));
	EXPECT_TRUE(manager.set_active_tool(quader::tools::ToolId::Move));

	EXPECT_TRUE(manager.dispatch_pointer_event(quader::tools::PointerEvent{
			.position = { 10.0F, 12.0F },
			.button = quader::tools::PointerButton::Left,
			.pressed = true,
			.phase = quader::tools::PointerPhase::Press,
	}));
	EXPECT_TRUE(fixture.document.selection().empty());
	EXPECT_TRUE(history.can_undo());
	EXPECT_EQ(history.undo_name(), std::string_view("Set Selection"));

	auto result = history.undo(fixture.document);
	EXPECT_EQ((result).status, quader::commands::CommandStatus::Applied);
	ASSERT_EQ(fixture.document.selection().selected_objects().size(), 1U);
	EXPECT_EQ(fixture.document.selection().selected_objects().front(), fixture.object);
}

TEST(TransformGizmoTool, ScaleCenterCubeUniformlyScalesSelection) {
	auto fixture = quader::tests::document_fixtures::make_document_with_triangle_object();
	select_object(fixture.document, fixture.object);

	quader::commands::CommandHistory history;
	quader::tools::ToolManager manager{ quader::tools::ToolContext{ fixture.document, history } };
	EXPECT_TRUE(manager.register_tool(std::make_unique<quader::tools::TransformGizmoTool>(
			quader::tools::ToolId::Scale)));
	EXPECT_TRUE(manager.set_active_tool(quader::tools::ToolId::Scale));

	EXPECT_TRUE(manager.dispatch_pointer_event(front_gizmo_event({ 320.0F, 240.0F },
			quader::tools::PointerPhase::Press,
			quader::tools::PointerButton::Left,
			true)));
	EXPECT_TRUE(manager.dispatch_pointer_event(front_gizmo_event({ 416.0F, 240.0F },
			quader::tools::PointerPhase::Move)));
	EXPECT_TRUE(manager.dispatch_pointer_event(front_gizmo_event({ 416.0F, 240.0F },
			quader::tools::PointerPhase::Release,
			quader::tools::PointerButton::Left,
			false)));

	const auto *object = fixture.document.find_mesh_object(fixture.object);
	ASSERT_TRUE(object != nullptr);
	EXPECT_TRUE(quader::math::nearly_equal(object->transform.scale,
			quader::math::Vec3{ 2.0F, 2.0F, 2.0F },
			0.0001F));
	EXPECT_TRUE(history.can_undo());
}

TEST(TransformGizmoTool, RotateZRingCommitsSnappedAngle) {
	auto fixture = quader::tests::document_fixtures::make_document_with_triangle_object();
	select_object(fixture.document, fixture.object);

	quader::commands::CommandHistory history;
	quader::tools::ToolManager manager{ quader::tools::ToolContext{ fixture.document, history } };
	EXPECT_TRUE(manager.register_tool(std::make_unique<quader::tools::TransformGizmoTool>(
			quader::tools::ToolId::Rotate)));
	EXPECT_TRUE(manager.set_active_tool(quader::tools::ToolId::Rotate));

	EXPECT_TRUE(manager.dispatch_pointer_event(front_gizmo_event({ 382.0F, 178.0F },
			quader::tools::PointerPhase::Press,
			quader::tools::PointerButton::Left,
			true)));
	EXPECT_TRUE(manager.dispatch_pointer_event(front_gizmo_event({ 258.0F, 178.0F },
			quader::tools::PointerPhase::Move)));
	const quader::tools::ToolPreview kRotatePreview = manager.preview();
	const quader::tools::ToolPreviewColorSrgb kOldPreviewBoundsCyan{ 0.0F, 1.0F, 1.0F, 0.95F };
	EXPECT_EQ(count_preview_segments_with_color(kRotatePreview, kOldPreviewBoundsCyan), 0U);
	const auto *object_before_release = fixture.document.find_mesh_object(fixture.object);
	ASSERT_TRUE(object_before_release != nullptr);
	EXPECT_TRUE(quader::math::nearly_equal(object_before_release->transform.rotation_euler.z, -90.0F, 0.0001F));
	EXPECT_FALSE(history.can_undo());
	EXPECT_FALSE(fixture.document.is_dirty());
	EXPECT_TRUE(manager.dispatch_pointer_event(front_gizmo_event({ 258.0F, 178.0F },
			quader::tools::PointerPhase::Release,
			quader::tools::PointerButton::Left,
			false)));

	const auto *object = fixture.document.find_mesh_object(fixture.object);
	ASSERT_TRUE(object != nullptr);
	EXPECT_TRUE(quader::math::nearly_equal(object->transform.rotation_euler.z, -90.0F, 0.0001F));
	EXPECT_TRUE(history.can_undo());
}

TEST(TransformGizmoTool, EscapeCancelsDragWithoutMutatingDocument) {
	auto fixture = quader::tests::document_fixtures::make_document_with_triangle_object();
	select_object(fixture.document, fixture.object);

	quader::commands::CommandHistory history;
	quader::tools::ToolManager manager{ quader::tools::ToolContext{ fixture.document, history } };
	EXPECT_TRUE(manager.register_tool(std::make_unique<quader::tools::TransformGizmoTool>(
			quader::tools::ToolId::Move)));
	EXPECT_TRUE(manager.set_active_tool(quader::tools::ToolId::Move));

	EXPECT_TRUE(manager.dispatch_pointer_event(front_gizmo_event({ 180.0F, 240.0F },
			quader::tools::PointerPhase::Press,
			quader::tools::PointerButton::Left,
			true,
			false)));
	EXPECT_TRUE(manager.dispatch_pointer_event(front_gizmo_event({ 170.0F, 240.0F },
			quader::tools::PointerPhase::Move,
			quader::tools::PointerButton::None,
			false,
			false)));
	EXPECT_FALSE(manager.preview().empty());
	const auto *preview_object = fixture.document.find_mesh_object(fixture.object);
	ASSERT_TRUE(preview_object != nullptr);
	EXPECT_TRUE(quader::math::nearly_equal(preview_object->transform.translation,
			quader::math::Vec3{ 1.0F, 0.0F, 0.0F }));
	EXPECT_FALSE(fixture.document.is_dirty());
	EXPECT_TRUE(manager.dispatch_key_event(quader::tools::KeyEvent{ 27, true, false }));

	const auto *object = fixture.document.find_mesh_object(fixture.object);
	ASSERT_TRUE(object != nullptr);
	EXPECT_TRUE(quader::math::nearly_equal(object->transform.translation,
			quader::math::Vec3{}));
	EXPECT_TRUE(manager.preview().empty());
	EXPECT_FALSE(history.can_undo());
	EXPECT_FALSE(fixture.document.is_dirty());
}

TEST(BoxTool, HoverShowsSnappedFootprintInSourceView) {
	quader::document::Document document;
	quader::commands::CommandHistory history;
	quader::tools::ToolManager manager{ quader::tools::ToolContext{ document, history } };
	EXPECT_TRUE(manager.register_tool(std::make_unique<PassiveTool>()));
	EXPECT_TRUE(manager.register_tool(std::make_unique<quader::tools::BoxTool>()));
	EXPECT_TRUE(manager.set_active_tool(quader::tools::ToolId::Box));

	auto event = grid_event({ 0.2F, 0.0F, 0.3F },
			{ 12.0F, 16.0F },
			quader::tools::PointerPhase::Hover);
	event.view_index = 2;
	EXPECT_TRUE(manager.dispatch_pointer_event(event));

	const auto kPreview = manager.preview();
	EXPECT_TRUE(kPreview.active);
	EXPECT_TRUE(kPreview.overlay_only);
	ASSERT_TRUE(kPreview.view_index.has_value());
	EXPECT_EQ(*kPreview.view_index, 2U);
	EXPECT_EQ(kPreview.world_segments.size(), 4U);
	EXPECT_TRUE(kPreview.boxes.empty());
	EXPECT_EQ(document.object_count(), 0U);
	EXPECT_FALSE(history.can_undo());
}

TEST(BoxTool, GroundPlaneBasisMatchesReferenceSignedParity) {
	const quader::tools::BoxConstructionPlane kPlane = quader::tools::make_box_construction_plane(
			{ 0.0F, 0.0F, 0.0F },
			{ 0.0F, 1.0F, 0.0F });

	EXPECT_TRUE(quader::math::nearly_equal(kPlane.axis_u, quader::math::Vec3{ 1.0F, 0.0F, 0.0F }));
	EXPECT_TRUE(quader::math::nearly_equal(kPlane.axis_v, quader::math::Vec3{ 0.0F, 0.0F, -1.0F }));
}

TEST(BoxTool, GridFootprintReleaseCommitsDefaultHeightThroughCommandHistory) {
	quader::document::Document document;
	quader::commands::CommandHistory history;
	quader::tools::ToolManager manager{ quader::tools::ToolContext{ document, history } };
	EXPECT_TRUE(manager.register_tool(std::make_unique<PassiveTool>()));
	EXPECT_TRUE(manager.register_tool(std::make_unique<quader::tools::BoxTool>()));
	EXPECT_TRUE(manager.set_active_tool(quader::tools::ToolId::Box));

	EXPECT_TRUE(manager.dispatch_pointer_event(grid_event({ 0.2F, 0.0F, 0.3F },
			{ 10.0F, 100.0F },
			quader::tools::PointerPhase::Press,
			quader::tools::PointerButton::Left,
			true)));
	ASSERT_TRUE(manager.active_tool_id().has_value());
	EXPECT_EQ(*manager.active_tool_id(), quader::tools::ToolId::Box);
	EXPECT_TRUE(manager.dispatch_pointer_event(grid_event({ 2.2F, 0.0F, 3.2F },
			{ 40.0F, 100.0F },
			quader::tools::PointerPhase::Move)));
	const auto kPreview = manager.preview();
	EXPECT_TRUE(kPreview.active);
	EXPECT_TRUE(kPreview.overlay_only);
	EXPECT_EQ(kPreview.world_segments.size(), 4U);
	ASSERT_EQ(kPreview.boxes.size(), 1U);
	EXPECT_TRUE(kPreview.boxes.front().active);
	EXPECT_TRUE(manager.dispatch_pointer_event(grid_event({ 2.2F, 0.0F, 3.2F },
			{ 40.0F, 100.0F },
			quader::tools::PointerPhase::Release,
			quader::tools::PointerButton::Left,
			false)));

	ASSERT_EQ(document.object_count(), 1U);
	EXPECT_TRUE(history.can_undo());
	EXPECT_TRUE(manager.preview().empty());
	ASSERT_TRUE(manager.active_tool_id().has_value());
	EXPECT_EQ(*manager.active_tool_id(), quader::tools::ToolId::Select);
	EXPECT_EQ(manager.selection_mode(), quader::tools::SelectionMode::Object);
	const auto kIds = document.object_ids();
	ASSERT_EQ(kIds.size(), 1U);
	ASSERT_EQ(document.selection().selected_objects().size(), 1U);
	EXPECT_EQ(document.selection().selected_objects().front(), kIds.front());
	const auto *object = document.find_mesh_object(kIds.front());
	ASSERT_TRUE(object != nullptr);
	EXPECT_EQ(object->name, std::string("Box"));
	EXPECT_EQ(object->material, quader::document::default_box_material());
	EXPECT_EQ(object->mesh.vertex_count(), 8U);
	EXPECT_EQ(object->mesh.edge_count(), 12U);
	EXPECT_EQ(object->mesh.face_count(), 6U);
	float max_y = -std::numeric_limits<float>::infinity();
	for (const auto kVertex : object->mesh.vertex_ids()) {
		max_y = std::max(max_y, object->mesh.vertex_position(kVertex).value().y);
	}
	EXPECT_TRUE(quader::math::nearly_equal(max_y, 1.0F));
	EXPECT_TRUE(quader::mesh::validate_mesh(object->mesh).ok());

	auto result = history.undo(document);
	EXPECT_EQ((result).status, quader::commands::CommandStatus::Applied);
	EXPECT_EQ(document.object_count(), 0U);
}

TEST(BoxTool, CommitSelectsOnlyNewBoxAndUndoRedoRestoresSelection) {
	auto fixture = quader::tests::document_fixtures::make_document_with_triangle_object();
	quader::document::Selection selection;
	ASSERT_TRUE(selection.set_objects({ fixture.object }));
	ASSERT_TRUE(fixture.document.set_selection(selection));
	fixture.document.clear_dirty();
	discard_pending_changes(fixture.document);

	quader::commands::CommandHistory history;
	quader::tools::ToolManager manager{ quader::tools::ToolContext{ fixture.document, history } };
	EXPECT_TRUE(manager.register_tool(std::make_unique<PassiveTool>()));
	EXPECT_TRUE(manager.register_tool(std::make_unique<quader::tools::BoxTool>()));
	EXPECT_TRUE(manager.set_selection_mode(quader::tools::SelectionMode::Face));
	EXPECT_TRUE(manager.set_active_tool(quader::tools::ToolId::Box));

	EXPECT_TRUE(manager.dispatch_pointer_event(grid_event({ 0.2F, 0.0F, 0.3F },
			{ 10.0F, 100.0F },
			quader::tools::PointerPhase::Press,
			quader::tools::PointerButton::Left,
			true)));
	EXPECT_TRUE(manager.dispatch_pointer_event(grid_event({ 2.2F, 0.0F, 3.2F },
			{ 40.0F, 100.0F },
			quader::tools::PointerPhase::Release,
			quader::tools::PointerButton::Left,
			false)));

	const auto kIds = fixture.document.object_ids();
	ASSERT_EQ(kIds.size(), 2U);
	ASSERT_EQ(fixture.document.selection().selected_objects().size(), 1U);
	const quader::document::ObjectId kCreated = fixture.document.selection().selected_objects().front();
	EXPECT_NE(kCreated, fixture.object);
	ASSERT_TRUE(manager.active_tool_id().has_value());
	EXPECT_EQ(*manager.active_tool_id(), quader::tools::ToolId::Select);
	EXPECT_EQ(manager.selection_mode(), quader::tools::SelectionMode::Object);

	auto result = history.undo(fixture.document);
	EXPECT_EQ((result).status, quader::commands::CommandStatus::Applied);
	EXPECT_TRUE(fixture.document.find_mesh_object(kCreated) == nullptr);
	ASSERT_EQ(fixture.document.selection().selected_objects().size(), 1U);
	EXPECT_EQ(fixture.document.selection().selected_objects().front(), fixture.object);

	result = history.redo(fixture.document);
	EXPECT_EQ((result).status, quader::commands::CommandStatus::Applied);
	EXPECT_TRUE(fixture.document.find_mesh_object(kCreated) != nullptr);
	ASSERT_EQ(fixture.document.selection().selected_objects().size(), 1U);
	EXPECT_EQ(fixture.document.selection().selected_objects().front(), kCreated);
}

TEST(BoxTool, GridFootprintDragDirectionsCommitOutwardNormals) {
	const quader::math::Vec3 kStart{ 0.2F, 0.0F, 0.2F };
	const std::array<quader::math::Vec3, 4> kEnds{
		quader::math::Vec3{ 2.2F, 0.0F, 2.2F },
		quader::math::Vec3{ -2.2F, 0.0F, 2.2F },
		quader::math::Vec3{ 2.2F, 0.0F, -2.2F },
		quader::math::Vec3{ -2.2F, 0.0F, -2.2F },
	};

	for (const quader::math::Vec3 kEnd : kEnds) {
		SCOPED_TRACE(::testing::Message() << "end " << kEnd.x << ", " << kEnd.y << ", " << kEnd.z);
		quader::document::Document document;
		quader::commands::CommandHistory history;
		quader::tools::ToolManager manager{ quader::tools::ToolContext{ document, history } };
		EXPECT_TRUE(manager.register_tool(std::make_unique<quader::tools::BoxTool>()));
		EXPECT_TRUE(manager.set_active_tool(quader::tools::ToolId::Box));

		EXPECT_TRUE(manager.dispatch_pointer_event(grid_event(kStart,
				{ 10.0F, 100.0F },
				quader::tools::PointerPhase::Press,
				quader::tools::PointerButton::Left,
				true)));
		EXPECT_TRUE(manager.dispatch_pointer_event(grid_event(kEnd,
				{ 40.0F, 100.0F },
				quader::tools::PointerPhase::Move)));
		EXPECT_TRUE(manager.dispatch_pointer_event(grid_event(kEnd,
				{ 40.0F, 100.0F },
				quader::tools::PointerPhase::Release,
				quader::tools::PointerButton::Left,
				false)));

		ASSERT_EQ(document.object_count(), 1U);
		const auto kIds = document.object_ids();
		ASSERT_EQ(kIds.size(), 1U);
		const auto *object = document.find_mesh_object(kIds.front());
		ASSERT_TRUE(object != nullptr);
		EXPECT_TRUE(quader::mesh::validate_mesh(object->mesh).ok());
		expect_ground_box_faces_oriented_outward(object->mesh);
	}
}

TEST(BoxTool, SecondClickWhileDraggingCommitsFootprint) {
	quader::document::Document document;
	quader::commands::CommandHistory history;
	quader::tools::ToolManager manager{ quader::tools::ToolContext{ document, history } };
	EXPECT_TRUE(manager.register_tool(std::make_unique<PassiveTool>()));
	EXPECT_TRUE(manager.register_tool(std::make_unique<quader::tools::BoxTool>()));
	EXPECT_TRUE(manager.set_active_tool(quader::tools::ToolId::Box));

	EXPECT_TRUE(manager.dispatch_pointer_event(grid_event({ 0.0F, 0.0F, 0.0F },
			{ 10.0F, 100.0F },
			quader::tools::PointerPhase::Press,
			quader::tools::PointerButton::Left,
			true)));
	EXPECT_TRUE(manager.dispatch_pointer_event(grid_event({ 1.2F, 0.0F, 1.2F },
			{ 24.0F, 100.0F },
			quader::tools::PointerPhase::Move)));
	EXPECT_TRUE(manager.dispatch_pointer_event(grid_event({ 1.2F, 0.0F, 1.2F },
			{ 24.0F, 100.0F },
			quader::tools::PointerPhase::Press,
			quader::tools::PointerButton::Left,
			true)));

	EXPECT_EQ(document.object_count(), 1U);
	EXPECT_TRUE(history.can_undo());
	EXPECT_TRUE(manager.preview().empty());
	ASSERT_TRUE(manager.active_tool_id().has_value());
	EXPECT_EQ(*manager.active_tool_id(), quader::tools::ToolId::Select);
}

TEST(BoxTool, SurfaceHitSeedsVerticalConstructionPlane) {
	quader::document::Document document;
	quader::commands::CommandHistory history;
	quader::tools::ToolManager manager{ quader::tools::ToolContext{ document, history } };
	EXPECT_TRUE(manager.register_tool(std::make_unique<quader::tools::BoxTool>()));
	EXPECT_TRUE(manager.set_active_tool(quader::tools::ToolId::Box));

	const quader::math::Vec3 kNormal{ -1.0F, 0.0F, 0.0F };
	EXPECT_TRUE(manager.dispatch_pointer_event(surface_event({ 5.0F, 0.2F, 0.2F },
			kNormal,
			{ 10.0F, 90.0F },
			quader::tools::PointerPhase::Press,
			quader::tools::PointerButton::Left,
			true,
			x_axis_ray_to({ 5.0F, 0.2F, 0.2F }))));
	EXPECT_TRUE(manager.dispatch_pointer_event(surface_event({ 5.0F, 2.2F, 3.2F },
			kNormal,
			{ 40.0F, 90.0F },
			quader::tools::PointerPhase::Move,
			quader::tools::PointerButton::None,
			false,
			x_axis_ray_to({ 5.0F, 2.2F, 3.2F }))));
	EXPECT_TRUE(manager.dispatch_pointer_event(surface_event({ 5.0F, 2.2F, 3.2F },
			kNormal,
			{ 40.0F, 90.0F },
			quader::tools::PointerPhase::Release,
			quader::tools::PointerButton::Left,
			false,
			x_axis_ray_to({ 5.0F, 2.2F, 3.2F }))));
	const auto *box_tool = static_cast<const quader::tools::BoxTool *>(manager.active_tool());
	ASSERT_TRUE(box_tool != nullptr);
	EXPECT_EQ(box_tool->state().stage, quader::tools::BoxToolStage::Idle);
	EXPECT_TRUE(manager.preview().empty());
	ASSERT_EQ(document.object_count(), 1U);
	const auto kIds = document.object_ids();
	const auto *object = document.find_mesh_object(kIds.front());
	ASSERT_TRUE(object != nullptr);
	float min_x = std::numeric_limits<float>::infinity();
	float max_x = -std::numeric_limits<float>::infinity();
	for (const auto kVertex : object->mesh.vertex_ids()) {
		const auto kPosition = object->mesh.vertex_position(kVertex).value();
		min_x = std::min(min_x, kPosition.x);
		max_x = std::max(max_x, kPosition.x);
	}
	EXPECT_TRUE(quader::math::nearly_equal(min_x, 4.0F));
	EXPECT_TRUE(quader::math::nearly_equal(max_x, 5.0F));
}

TEST(BoxTool, TopSurfaceHitUsesAuthoredNormalForFlipFaces) {
	quader::document::Document document;
	quader::commands::CommandHistory history;
	quader::tools::ToolManager manager{ quader::tools::ToolContext{ document, history } };
	EXPECT_TRUE(manager.register_tool(std::make_unique<quader::tools::BoxTool>()));
	EXPECT_TRUE(manager.set_active_tool(quader::tools::ToolId::Box));

	const quader::math::Vec3 kNormal{ 0.0F, -1.0F, 0.0F };
	EXPECT_TRUE(manager.dispatch_pointer_event(surface_event({ 0.2F, 1.0F, 0.2F },
			kNormal,
			{ 10.0F, 90.0F },
			quader::tools::PointerPhase::Press,
			quader::tools::PointerButton::Left,
			true,
			downward_ray_to({ 0.2F, 1.0F, 0.2F }))));
	EXPECT_TRUE(manager.dispatch_pointer_event(surface_event({ 2.2F, 1.0F, 3.2F },
			kNormal,
			{ 40.0F, 90.0F },
			quader::tools::PointerPhase::Move,
			quader::tools::PointerButton::None,
			false,
			downward_ray_to({ 2.2F, 1.0F, 3.2F }))));
	EXPECT_TRUE(manager.dispatch_pointer_event(surface_event({ 2.2F, 1.0F, 3.2F },
			kNormal,
			{ 40.0F, 90.0F },
			quader::tools::PointerPhase::Release,
			quader::tools::PointerButton::Left,
			false,
			downward_ray_to({ 2.2F, 1.0F, 3.2F }))));

	ASSERT_EQ(document.object_count(), 1U);
	const auto kIds = document.object_ids();
	const auto *object = document.find_mesh_object(kIds.front());
	ASSERT_TRUE(object != nullptr);
	float min_y = std::numeric_limits<float>::infinity();
	float max_y = -std::numeric_limits<float>::infinity();
	for (const auto kVertex : object->mesh.vertex_ids()) {
		const auto kPosition = object->mesh.vertex_position(kVertex).value();
		min_y = std::min(min_y, kPosition.y);
		max_y = std::max(max_y, kPosition.y);
	}
	EXPECT_TRUE(quader::math::nearly_equal(min_y, 0.0F));
	EXPECT_TRUE(quader::math::nearly_equal(max_y, 1.0F));
}

TEST(BoxTool, VerticalSurfaceHitUsesAuthoredNormalForFlipFaces) {
	quader::document::Document document;
	quader::commands::CommandHistory history;
	quader::tools::ToolManager manager{ quader::tools::ToolContext{ document, history } };
	EXPECT_TRUE(manager.register_tool(std::make_unique<quader::tools::BoxTool>()));
	EXPECT_TRUE(manager.set_active_tool(quader::tools::ToolId::Box));

	const quader::math::Vec3 kNormal{ -1.0F, 0.0F, 0.0F };
	EXPECT_TRUE(manager.dispatch_pointer_event(surface_event({ 1.0F, 0.2F, 0.2F },
			kNormal,
			{ 10.0F, 90.0F },
			quader::tools::PointerPhase::Press,
			quader::tools::PointerButton::Left,
			true,
			negative_x_axis_ray_to({ 1.0F, 0.2F, 0.2F }))));
	EXPECT_TRUE(manager.dispatch_pointer_event(surface_event({ 1.0F, 2.2F, 3.2F },
			kNormal,
			{ 40.0F, 90.0F },
			quader::tools::PointerPhase::Move,
			quader::tools::PointerButton::None,
			false,
			negative_x_axis_ray_to({ 1.0F, 2.2F, 3.2F }))));
	EXPECT_TRUE(manager.dispatch_pointer_event(surface_event({ 1.0F, 2.2F, 3.2F },
			kNormal,
			{ 40.0F, 90.0F },
			quader::tools::PointerPhase::Release,
			quader::tools::PointerButton::Left,
			false,
			negative_x_axis_ray_to({ 1.0F, 2.2F, 3.2F }))));

	ASSERT_EQ(document.object_count(), 1U);
	const auto kIds = document.object_ids();
	const auto *object = document.find_mesh_object(kIds.front());
	ASSERT_TRUE(object != nullptr);
	float min_x = std::numeric_limits<float>::infinity();
	float max_x = -std::numeric_limits<float>::infinity();
	for (const auto kVertex : object->mesh.vertex_ids()) {
		const auto kPosition = object->mesh.vertex_position(kVertex).value();
		min_x = std::min(min_x, kPosition.x);
		max_x = std::max(max_x, kPosition.x);
	}
	EXPECT_TRUE(quader::math::nearly_equal(min_x, 0.0F));
	EXPECT_TRUE(quader::math::nearly_equal(max_x, 1.0F));
}

TEST(BoxTool, ActiveVerticalSurfaceDragIgnoresLaterSurfaceHits) {
	quader::document::Document document;
	quader::commands::CommandHistory history;
	quader::tools::ToolManager manager{ quader::tools::ToolContext{ document, history } };
	EXPECT_TRUE(manager.register_tool(std::make_unique<quader::tools::BoxTool>()));
	EXPECT_TRUE(manager.set_active_tool(quader::tools::ToolId::Box));

	const quader::math::Vec3 kNormal{ -1.0F, 0.0F, 0.0F };
	EXPECT_TRUE(manager.dispatch_pointer_event(surface_event({ 5.0F, 0.2F, 0.2F },
			kNormal,
			{ 10.0F, 90.0F },
			quader::tools::PointerPhase::Press,
			quader::tools::PointerButton::Left,
			true,
			x_axis_ray_to({ 5.0F, 0.2F, 0.2F }))));

	const quader::tools::PointerEvent kMoveWithDistractingHit = surface_event({ 5.0F, 0.2F, 3.2F },
			kNormal,
			{ 40.0F, 90.0F },
			quader::tools::PointerPhase::Move,
			quader::tools::PointerButton::None,
			false,
			x_axis_ray_to({ 5.0F, 2.2F, 3.2F }));
	EXPECT_TRUE(manager.dispatch_pointer_event(kMoveWithDistractingHit));
	EXPECT_TRUE(manager.dispatch_pointer_event(surface_event({ 5.0F, 0.2F, 3.2F },
			kNormal,
			{ 40.0F, 90.0F },
			quader::tools::PointerPhase::Release,
			quader::tools::PointerButton::Left,
			false,
			x_axis_ray_to({ 5.0F, 2.2F, 3.2F }))));

	ASSERT_EQ(document.object_count(), 1U);
	const auto kIds = document.object_ids();
	const auto *object = document.find_mesh_object(kIds.front());
	ASSERT_TRUE(object != nullptr);
	float max_y = -std::numeric_limits<float>::infinity();
	for (const auto kVertex : object->mesh.vertex_ids()) {
		max_y = std::max(max_y, object->mesh.vertex_position(kVertex).value().y);
	}
	EXPECT_TRUE(quader::math::nearly_equal(max_y, 3.0F));
}

TEST(BoxTool, EscapeCancelClearsPreviewWithoutMutatingDocument) {
	quader::document::Document document;
	quader::commands::CommandHistory history;
	quader::tools::ToolManager manager{ quader::tools::ToolContext{ document, history } };
	EXPECT_TRUE(manager.register_tool(std::make_unique<quader::tools::BoxTool>()));
	EXPECT_TRUE(manager.set_active_tool(quader::tools::ToolId::Box));

	EXPECT_TRUE(manager.dispatch_pointer_event(grid_event({ 0.0F, 0.0F, 0.0F },
			{ 10.0F, 100.0F },
			quader::tools::PointerPhase::Press,
			quader::tools::PointerButton::Left,
			true)));
	EXPECT_FALSE(manager.preview().empty());
	EXPECT_TRUE(manager.dispatch_key_event(quader::tools::KeyEvent{ 27, true, false }));
	EXPECT_TRUE(manager.preview().empty());
	ASSERT_TRUE(manager.active_tool_id().has_value());
	EXPECT_EQ(*manager.active_tool_id(), quader::tools::ToolId::Box);
	EXPECT_EQ(document.object_count(), 0U);
	EXPECT_FALSE(history.can_undo());
}

} // namespace
