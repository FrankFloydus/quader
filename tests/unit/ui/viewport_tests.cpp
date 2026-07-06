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

#include "ui/viewport/crimson_viewport_render_host.hpp"
#include "ui/viewport/viewport_camera_controller.hpp"
#include "ui/viewport/tool_preview_overlay_adapter.hpp"
#include "ui/viewport/viewport_controller.hpp"
#include "ui/viewport/viewport_layout_state.hpp"
#include "ui/viewport/viewport_render_host.hpp"

#include <gtest/gtest.h>

#include "commands/command_history.hpp"
#include "document/document.hpp"
#include "geometry/normals.hpp"
#include "math/vec3.hpp"
#include "mesh/core/mesh_traversal.hpp"
#include "mesh/core/polyhedron.hpp"
#include "mesh/ops/box_builder.hpp"
#include "tools/tool.hpp"
#include "tools/box_tool.hpp"
#include "tools/tool_manager.hpp"
#include "tools/transform_gizmo_tool.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace {

struct FakeRenderHost final : quader::ui::IViewportRenderHost {
	int initialize_calls = 0;
	int resize_calls = 0;
	int render_calls = 0;
	quader::ui::ViewportPixelSize initialized_size;
	quader::ui::ViewportPixelSize resized_size;
	quader::ui::ViewportLayoutMode last_layout = quader::ui::ViewportLayoutMode::Single;
	std::size_t last_pane_count = 0;
	std::size_t last_camera_count = 0;
	bool last_animation_enabled = false;
	std::optional<quader::ui::ViewportFrameStats> stats;
	std::optional<quader::ui::ViewportDiagnosticsSnapshot> diagnostics;

	quader::ui::ViewportRenderResult initialize_surface(
			const quader::ui::NativeViewportSurface &,
			quader::ui::ViewportPixelSize size,
			double) override {
		++initialize_calls;
		initialized_size = size;
		stats = quader::ui::ViewportFrameStats{ size.width, size.height, 1, 60.0 };
		return quader::ui::ViewportRenderResult::success(QStringLiteral("FakeRenderer"));
	}

	quader::ui::ViewportRenderResult resize_surface(quader::ui::ViewportPixelSize size, double) override {
		++resize_calls;
		resized_size = size;
		stats = quader::ui::ViewportFrameStats{ size.width, size.height, 1, 60.0 };
		return quader::ui::ViewportRenderResult::success(QStringLiteral("FakeRenderer"));
	}

	quader::ui::ViewportRenderResult render_frame(const quader::ui::ViewportRenderRequest &request) override {
		++render_calls;
		last_layout = request.layout_mode;
		last_pane_count = request.panes.size();
		last_camera_count = request.cameras.size();
		last_animation_enabled = request.scene_animation_enabled;
		stats = quader::ui::ViewportFrameStats{
			request.surface_size.width,
			request.surface_size.height,
			static_cast<int>(request.panes.size()),
			60.0,
		};
		return quader::ui::ViewportRenderResult::success(QStringLiteral("FakeRenderer"));
	}

	void shutdown_surface() override {}

	std::optional<quader::ui::ViewportFrameStats> latest_frame_stats() const override {
		return stats;
	}

	std::optional<quader::ui::ViewportDiagnosticsSnapshot> latest_diagnostics() const override {
		return diagnostics;
	}

	QString renderer_name() const override {
		return QStringLiteral("FakeRenderer");
	}
};

class ViewportRecordingTool final : public quader::tools::ITool {
public:
	[[nodiscard]] quader::tools::ToolId id() const noexcept override {
		return quader::tools::ToolId::Select;
	}

	void cancel(quader::tools::ToolContext &context) override {
		(void)context;
		++cancellations;
	}

	[[nodiscard]] bool on_pointer_event(const quader::tools::PointerEvent &event,
			quader::tools::ToolContext &context) override {
		(void)context;
		++pointer_events;
		last_pointer = event;
		return true;
	}

	int pointer_events = 0;
	int cancellations = 0;
	std::optional<quader::tools::PointerEvent> last_pointer;
};

class ViewportRecordingPassiveSelectTool final : public quader::tools::ITool {
public:
	[[nodiscard]] quader::tools::ToolId id() const noexcept override {
		return quader::tools::ToolId::Select;
	}

	[[nodiscard]] bool on_pointer_event(const quader::tools::PointerEvent &event,
			quader::tools::ToolContext &context) override {
		(void)context;
		++pointer_events;
		last_pointer = event;
		return false;
	}

	int pointer_events = 0;
	std::optional<quader::tools::PointerEvent> last_pointer;
};

class ViewportPassiveSelectTool final : public quader::tools::ITool {
public:
	[[nodiscard]] quader::tools::ToolId id() const noexcept override {
		return quader::tools::ToolId::Select;
	}
};

struct ViewportTriangleFixture {
	quader::document::Document document;
	quader::document::ObjectId object;
	quader::mesh::VertexId near_vertex;
	quader::mesh::EdgeId near_edge;
};

ViewportTriangleFixture make_viewport_triangle_fixture() {
	quader::mesh::Polyhedron mesh;
	const quader::mesh::VertexId kNearVertex = mesh.create_vertex({ -0.2F, -0.1F, 0.0F });
	const quader::mesh::VertexId kRightVertex = mesh.create_vertex({ 1.0F, -0.1F, 0.0F });
	const quader::mesh::VertexId kTopVertex = mesh.create_vertex({ -0.2F, 1.0F, 0.0F });
	const std::array<quader::mesh::VertexId, 3> kVertices{ kNearVertex, kRightVertex, kTopVertex };
	auto face = mesh.create_face(kVertices);
	EXPECT_TRUE(face);
	std::vector<quader::mesh::EdgeId> edges;
	if (face) {
		auto face_edges = quader::mesh::face_edges(mesh, face.value());
		EXPECT_TRUE(face_edges);
		if (face_edges) {
			edges = std::move(face_edges).value();
		}
	}
	EXPECT_FALSE(edges.empty());

	quader::document::Document document;
	auto object = document.create_mesh_object("Viewport Triangle", std::move(mesh));
	EXPECT_TRUE(object);

	return ViewportTriangleFixture{
		std::move(document),
		object ? object.value() : quader::document::ObjectId::invalid(),
		kNearVertex,
		edges.empty() ? quader::mesh::EdgeId::invalid() : edges.front(),
	};
}

ViewportTriangleFixture make_viewport_parallel_edge_fixture() {
	quader::mesh::Polyhedron mesh;
	const quader::mesh::VertexId kBottom = mesh.create_vertex({ 0.0F, -20.0F, 0.0F });
	const quader::mesh::VertexId kTop = mesh.create_vertex({ 0.0F, 20.0F, 0.0F });
	const quader::mesh::VertexId kSide = mesh.create_vertex({ 1.0F, 20.0F, 1.0F });
	const std::array<quader::mesh::VertexId, 3> kVertices{ kBottom, kTop, kSide };
	auto face = mesh.create_face(kVertices);
	EXPECT_TRUE(face);
	std::vector<quader::mesh::EdgeId> edges;
	if (face) {
		auto face_edges = quader::mesh::face_edges(mesh, face.value());
		EXPECT_TRUE(face_edges);
		if (face_edges) {
			edges = std::move(face_edges).value();
		}
	}
	EXPECT_FALSE(edges.empty());

	quader::document::Document document;
	auto object = document.create_mesh_object("Viewport Parallel Edge", std::move(mesh));
	EXPECT_TRUE(object);

	return ViewportTriangleFixture{
		std::move(document),
		object ? object.value() : quader::document::ObjectId::invalid(),
		kBottom,
		edges.empty() ? quader::mesh::EdgeId::invalid() : edges.front(),
	};
}

quader::document::ObjectId add_box_object(quader::document::Document &document,
		std::string_view name,
		quader::math::Vec3 min,
		quader::math::Vec3 max) {
	auto mesh = quader::mesh::ops::make_box_from_bounds(quader::mesh::ops::BoxDimensions{
			.min = min,
			.max = max,
	});
	EXPECT_TRUE(mesh);
	if (!mesh) {
		return quader::document::ObjectId::invalid();
	}

	auto object = document.create_mesh_object(std::string(name), std::move(mesh).value());
	EXPECT_TRUE(object);
	return object ? object.value() : quader::document::ObjectId::invalid();
}

struct ComponentBounds {
	float min_value = std::numeric_limits<float>::infinity();
	float max_value = -std::numeric_limits<float>::infinity();
};

ComponentBounds object_component_bounds(
		const quader::document::MeshObject &object,
		float (*component)(quader::math::Vec3)) {
	ComponentBounds bounds;
	for (const quader::mesh::VertexId vertex : object.mesh.vertex_ids()) {
		auto position = object.mesh.vertex_position(vertex);
		EXPECT_TRUE(position);
		if (!position) {
			continue;
		}

		const float value = component(position.value());
		bounds.min_value = std::min(bounds.min_value, value);
		bounds.max_value = std::max(bounds.max_value, value);
	}
	return bounds;
}

quader::math::Vec3 average_points(std::span<const quader::math::Vec3> points) {
	quader::math::Vec3 average;
	for (const quader::math::Vec3 point : points) {
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
	for (const quader::mesh::VertexId vertex : vertices.value()) {
		auto position = mesh.vertex_position(vertex);
		EXPECT_TRUE(position);
		if (position) {
			points.push_back(position.value());
		}
	}
	return points;
}

quader::math::Vec3 object_center(const quader::document::MeshObject &object) {
	std::vector<quader::math::Vec3> points;
	points.reserve(object.mesh.vertex_count());
	for (const quader::mesh::VertexId vertex : object.mesh.vertex_ids()) {
		auto position = object.mesh.vertex_position(vertex);
		EXPECT_TRUE(position);
		if (position) {
			points.push_back(position.value());
		}
	}
	return average_points(points);
}

void expect_mesh_faces_oriented_outward(const quader::document::MeshObject &object) {
	const quader::math::Vec3 center = object_center(object);
	for (const quader::mesh::FaceId face : object.mesh.face_ids()) {
		const std::vector<quader::math::Vec3> points = face_points(object.mesh, face);
		ASSERT_GE(points.size(), 3U);
		const quader::math::Vec3 normal = quader::geometry::polygon_normal(points);
		const quader::math::Vec3 face_center = average_points(points);
		EXPECT_GT(quader::math::dot(normal, face_center - center), 0.0F);
	}
}

struct UploadVertex {
	quader::math::Vec3 position;
	quader::math::Vec3 normal;
	float u = 0.0F;
	float v = 0.0F;
};

UploadVertex upload_vertex(const crimson::RenderMeshUpload &upload, std::uint32_t index) {
	const std::size_t offset = static_cast<std::size_t>(index) * 8U;
	EXPECT_LE(offset + 7U, upload.position_normal_uv_interleaved.size());
	return UploadVertex{
		.position = {
				upload.position_normal_uv_interleaved[offset + 0U],
				upload.position_normal_uv_interleaved[offset + 1U],
				upload.position_normal_uv_interleaved[offset + 2U],
		},
		.normal = {
				upload.position_normal_uv_interleaved[offset + 3U],
				upload.position_normal_uv_interleaved[offset + 4U],
				upload.position_normal_uv_interleaved[offset + 5U],
		},
		.u = upload.position_normal_uv_interleaved[offset + 6U],
		.v = upload.position_normal_uv_interleaved[offset + 7U],
	};
}

void expect_viewport_upload_triangles_front_facing(const quader::document::MeshObject &object) {
	const std::optional<crimson::RenderMeshUpload> upload = quader::ui::make_crimson_viewport_mesh_upload(object);
	ASSERT_TRUE(upload.has_value());
	ASSERT_EQ(upload->indices.size() % 3U, 0U);
	for (std::size_t index = 0; index + 2U < upload->indices.size(); index += 3U) {
		const UploadVertex a = upload_vertex(*upload, upload->indices[index + 0U]);
		const UploadVertex b = upload_vertex(*upload, upload->indices[index + 1U]);
		const UploadVertex c = upload_vertex(*upload, upload->indices[index + 2U]);
		const quader::math::Vec3 triangle_normal = quader::math::cross(b.position - a.position, c.position - a.position);
		EXPECT_GT(quader::math::dot(triangle_normal, a.normal), 0.0F);
		EXPECT_TRUE(quader::math::nearly_equal(a.normal, b.normal, 0.0001F));
		EXPECT_TRUE(quader::math::nearly_equal(a.normal, c.normal, 0.0001F));
	}
}

float component_x(quader::math::Vec3 value) {
	return value.x;
}

float component_y(quader::math::Vec3 value) {
	return value.y;
}

quader::ui::ViewportPoint pane_center(const quader::ui::ViewportPane &pane) {
	return quader::ui::ViewportPoint{
		static_cast<double>(pane.rect.x) + static_cast<double>(pane.rect.width) * 0.5,
		static_cast<double>(pane.rect.y) + static_cast<double>(pane.rect.height) * 0.5,
	};
}

TEST(Viewport, CrimsonMeshUploadUsesReferenceUvScaleAndFrontFacingTriangles) {
	quader::document::Document document;
	const quader::document::ObjectId object_id = add_box_object(
			document,
			"Reference UV Box",
			{ -2.0F, 0.0F, -2.0F },
			{ 2.0F, 1.0F, 2.0F });
	const auto *object = document.find_mesh_object(object_id);
	ASSERT_TRUE(object != nullptr);

	const std::optional<crimson::RenderMeshUpload> upload = quader::ui::make_crimson_viewport_mesh_upload(*object);
	ASSERT_TRUE(upload.has_value());
	expect_viewport_upload_triangles_front_facing(*object);

	float min_u = std::numeric_limits<float>::infinity();
	float max_u = -std::numeric_limits<float>::infinity();
	float min_v = std::numeric_limits<float>::infinity();
	float max_v = -std::numeric_limits<float>::infinity();
	std::size_t top_vertex_count = 0;
	for (std::uint32_t index = 0; index < upload->position_normal_uv_interleaved.size() / 8U; ++index) {
		const UploadVertex vertex = upload_vertex(*upload, index);
		if (vertex.normal.y < 0.99F) {
			continue;
		}
		++top_vertex_count;
		min_u = std::min(min_u, vertex.u);
		max_u = std::max(max_u, vertex.u);
		min_v = std::min(min_v, vertex.v);
		max_v = std::max(max_v, vertex.v);
	}

	ASSERT_EQ(top_vertex_count, 6U);
	EXPECT_TRUE(quader::math::nearly_equal(max_u - min_u, 2.0F, 0.0001F));
	EXPECT_TRUE(quader::math::nearly_equal(max_v - min_v, 2.0F, 0.0001F));
}

TEST(Viewport, CrimsonMeshUploadPreservesFlipFacesWinding) {
	quader::document::Document document;
	const quader::document::ObjectId object_id = add_box_object(
			document,
			"Flipped Box",
			{ -1.0F, 0.0F, -1.0F },
			{ 1.0F, 1.0F, 1.0F });
	auto *object = document.find_mesh_object(object_id);
	ASSERT_TRUE(object != nullptr);

	std::vector<quader::mesh::FaceId> faces = object->mesh.face_ids();
	ASSERT_FALSE(faces.empty());
	auto reversed = object->mesh.reverse_face_windings(faces);
	ASSERT_TRUE(reversed);

	const std::optional<crimson::RenderMeshUpload> upload = quader::ui::make_crimson_viewport_mesh_upload(*object);
	ASSERT_TRUE(upload.has_value());
	ASSERT_EQ(upload->indices.size() % 3U, 0U);

	const quader::math::Vec3 center = object_center(*object);
	for (std::size_t index = 0; index + 2U < upload->indices.size(); index += 3U) {
		const UploadVertex a = upload_vertex(*upload, upload->indices[index + 0U]);
		const UploadVertex b = upload_vertex(*upload, upload->indices[index + 1U]);
		const UploadVertex c = upload_vertex(*upload, upload->indices[index + 2U]);
		const quader::math::Vec3 triangle_normal = quader::math::cross(b.position - a.position, c.position - a.position);
		const quader::math::Vec3 triangle_center = (a.position + b.position + c.position) / 3.0F;
		EXPECT_GT(quader::math::dot(triangle_normal, a.normal), 0.0F);
		EXPECT_LT(quader::math::dot(a.normal, triangle_center - center), 0.0F);
	}
}

TEST(Viewport, LayoutStateClampsQuadPanesAndHitTests) {
	quader::ui::ViewportLayoutState layout;
	EXPECT_EQ(layout.pane_count(), 1);

	const quader::ui::ViewportPaneArray kSingle = layout.panes_for({ 800, 600 });
	EXPECT_EQ(kSingle[0].rect.width, 800);
	EXPECT_EQ(kSingle[0].rect.height, 600);
	EXPECT_EQ(layout.pane_at({ 800, 600 }, { 20.0, 20.0 }), 0);

	layout.set_quad_enabled(true);
	layout.set_vertical_split(-1.0F);
	layout.set_horizontal_split(2.0F);
	const quader::ui::ViewportPaneArray kQuad = layout.panes_for({ 320, 240 });
	EXPECT_EQ(layout.pane_count(), 4);
	for (int index = 0; index < 4; ++index) {
		EXPECT_TRUE(kQuad[static_cast<std::size_t>(index)].rect.width > 0);
		EXPECT_TRUE(kQuad[static_cast<std::size_t>(index)].rect.height > 0);
	}

	EXPECT_TRUE(layout.pane_at({ 320, 240 }, { 10.0, 10.0 }) >= 0);
	EXPECT_NE(layout.splitter_at({ 320, 240 }, { 160.0, 10.0 }), quader::ui::ViewportSplitHandle::Horizontal);
}

TEST(Viewport, CameraControllerUpdatesFromSyntheticInputWithoutQtEvents) {
	quader::ui::ViewportCameraController cameras;
	const quader::ui::ViewportCameraSnapshot kBefore = cameras.camera_snapshots()[0];

	EXPECT_TRUE(cameras.begin_navigation(0, quader::ui::NavigationMode::Orbit, { 100.0, 100.0 }));
	cameras.update_navigation({ 140.0, 115.0 }, { 800, 600 });
	cameras.end_navigation();

	const quader::ui::ViewportCameraSnapshot kAfter = cameras.camera_snapshots()[0];
	EXPECT_TRUE(!quader::math::nearly_equal(kBefore.eye, kAfter.eye, 0.0001F));
	EXPECT_EQ(cameras.navigation_mode(), quader::ui::NavigationMode::None);

	const quader::ui::ViewportCameraSnapshot kTop = cameras.camera_snapshots()[1];
	EXPECT_EQ(kTop.projection, quader::ui::CameraProjection::Orthographic);
}

TEST(Viewport, ControllerForwardsSurfaceResizeAndQuadRenderRequests) {
	FakeRenderHost host;
	quader::ui::ViewportController controller(host);

	controller.initialize_surface(
			quader::ui::NativeViewportSurface{
					quader::ui::NativeViewportSurface::Platform::WindowsHwnd,
					reinterpret_cast<void *>(0x1),
			},
			{ 640, 480 },
			1.0);

	EXPECT_EQ(host.initialize_calls, 1);
	EXPECT_EQ(host.initialized_size.width, 640);
	EXPECT_EQ(host.initialized_size.height, 480);

	controller.resize_surface({ 800, 600 }, 1.0);
	EXPECT_EQ(host.resize_calls, 1);
	EXPECT_EQ(host.resized_size.width, 800);
	EXPECT_EQ(host.resized_size.height, 600);

	controller.set_quad_viewports_enabled(true);
	controller.render_frame(1.0, 0.016F);
	EXPECT_EQ(host.render_calls, 1);
	EXPECT_EQ(host.last_layout, quader::ui::ViewportLayoutMode::Quad);
	EXPECT_EQ(host.last_pane_count, 4U);
	EXPECT_EQ(host.last_camera_count, 4U);
	EXPECT_TRUE(host.last_animation_enabled);

	controller.set_scene_animation_enabled(false);
	controller.render_frame(2.0, 0.016F);
	EXPECT_FALSE(host.last_animation_enabled);
}

TEST(Viewport, ShadedViewportUsesUnlitSurfaceShader) {
	EXPECT_EQ(
			quader::ui::viewport_base_shader_for_shading_mode(quader::ui::ViewportShadingMode::Shaded),
			crimson::BaseShaderId::UnlitSurface);
	EXPECT_EQ(
			quader::ui::viewport_base_shader_for_shading_mode(quader::ui::ViewportShadingMode::Wireframe),
			crimson::BaseShaderId::UnlitSurface);
	EXPECT_EQ(
			quader::ui::viewport_base_shader_for_shading_mode(quader::ui::ViewportShadingMode::Rendered),
			crimson::BaseShaderId::OpaquePbr);
}

TEST(Viewport, ControllerRoutesUnconsumedLeftPointerEventsToActiveTool) {
	FakeRenderHost host;
	quader::document::Document document;
	quader::commands::CommandHistory history;
	quader::tools::ToolManager tools{ quader::tools::ToolContext{ document, history } };
	auto tool = std::make_unique<ViewportRecordingTool>();
	auto *recording_tool = tool.get();
	ASSERT_TRUE(tools.register_tool(std::move(tool)));
	ASSERT_TRUE(tools.set_active_tool(quader::tools::ToolId::Select));

	quader::ui::ViewportController controller(host, tools);
	EXPECT_TRUE(controller.handle_mouse_press(quader::ui::ViewportMouseButton::Left,
			{ 320.0, 240.0 },
			true,
			{ 640, 480 },
			true,
			true));

	ASSERT_EQ(recording_tool->pointer_events, 1);
	ASSERT_TRUE(recording_tool->last_pointer.has_value());
	const quader::tools::PointerEvent &kLastPointer = recording_tool->last_pointer.value();
	EXPECT_EQ(kLastPointer.button, quader::tools::PointerButton::Left);
	EXPECT_EQ(kLastPointer.phase, quader::tools::PointerPhase::Press);
	EXPECT_EQ(kLastPointer.view_index, 0U);
	EXPECT_FLOAT_EQ(kLastPointer.position.x, 320.0F);
	EXPECT_FLOAT_EQ(kLastPointer.position.y, 240.0F);
	EXPECT_TRUE(kLastPointer.ray.has_value());
	EXPECT_FALSE(kLastPointer.navigation_active);
	EXPECT_TRUE(kLastPointer.modifiers.shift);
	EXPECT_TRUE(kLastPointer.modifiers.control);
	EXPECT_TRUE(kLastPointer.modifiers.alt);
}

TEST(Viewport, ControllerRoutesNoButtonMoveAsHoverAndLeaveClearsSelectionHover) {
	auto fixture = make_viewport_triangle_fixture();
	quader::commands::CommandHistory history;
	quader::tools::ToolManager tools{ quader::tools::ToolContext{ fixture.document, history } };
	auto tool = std::make_unique<ViewportRecordingPassiveSelectTool>();
	auto *recording_tool = tool.get();
	ASSERT_TRUE(tools.register_tool(std::move(tool)));
	ASSERT_TRUE(tools.set_active_tool(quader::tools::ToolId::Select));

	FakeRenderHost host;
	quader::ui::ViewportController controller(host, tools);
	EXPECT_TRUE(controller.handle_mouse_move({ 320.0, 240.0 },
			{ 640, 480 },
			false,
			true,
			true,
			true));

	ASSERT_EQ(recording_tool->pointer_events, 1);
	ASSERT_TRUE(recording_tool->last_pointer.has_value());
	const quader::tools::PointerEvent &kLastPointer = recording_tool->last_pointer.value();
	EXPECT_EQ(kLastPointer.phase, quader::tools::PointerPhase::Hover);
	EXPECT_EQ(kLastPointer.button, quader::tools::PointerButton::None);
	EXPECT_FALSE(kLastPointer.pressed);
	EXPECT_TRUE(kLastPointer.modifiers.shift);
	EXPECT_TRUE(kLastPointer.modifiers.control);
	EXPECT_TRUE(kLastPointer.modifiers.alt);
	EXPECT_TRUE(tools.selection_hover().has_value());

	controller.handle_mouse_leave();
	EXPECT_FALSE(tools.selection_hover().has_value());
}

TEST(Viewport, BoxToolCreatesMeshOnExistingTopSurfaceThroughController) {
	quader::document::Document document;
	const quader::document::ObjectId base = add_box_object(
			document,
			"Base",
			{ -2.0F, 0.0F, -2.0F },
			{ 2.0F, 1.0F, 2.0F });
	ASSERT_TRUE(base.is_valid());

	quader::commands::CommandHistory history;
	quader::tools::ToolManager tools{ quader::tools::ToolContext{ document, history } };
	ASSERT_TRUE(tools.register_tool(std::make_unique<ViewportPassiveSelectTool>()));
	ASSERT_TRUE(tools.register_tool(std::make_unique<quader::tools::BoxTool>()));
	ASSERT_TRUE(tools.set_active_tool(quader::tools::ToolId::Box));

	FakeRenderHost host;
	quader::ui::ViewportController controller(host, tools);
	const quader::ui::ViewportPixelSize kSize{ 640, 480 };
	EXPECT_TRUE(controller.handle_mouse_press(quader::ui::ViewportMouseButton::Left,
			{ 320.0, 240.0 },
			false,
			kSize));
	EXPECT_TRUE(controller.handle_mouse_move({ 400.0, 240.0 }, kSize, true));
	EXPECT_TRUE(controller.handle_mouse_release(quader::ui::ViewportMouseButton::Left,
			{ 400.0, 240.0 },
			kSize));

	ASSERT_EQ(document.object_count(), 2U);
	ASSERT_EQ(document.selection().selected_objects().size(), 1U);
	const quader::document::ObjectId created = document.selection().selected_objects().front();
	EXPECT_NE(created, base);
	const auto *object = document.find_mesh_object(created);
	ASSERT_TRUE(object != nullptr);
	const ComponentBounds y_bounds = object_component_bounds(*object, component_y);
	EXPECT_TRUE(quader::math::nearly_equal(y_bounds.min_value, 1.0F));
	EXPECT_TRUE(quader::math::nearly_equal(y_bounds.max_value, 2.0F));
	expect_mesh_faces_oriented_outward(*object);
	expect_viewport_upload_triangles_front_facing(*object);
	ASSERT_TRUE(tools.active_tool_id().has_value());
	EXPECT_EQ(*tools.active_tool_id(), quader::tools::ToolId::Select);
}

TEST(Viewport, BoxToolCreatesMeshFromVerticalSurfaceThroughController) {
	quader::document::Document document;
	const quader::document::ObjectId base = add_box_object(
			document,
			"Vertical Base",
			{ 0.0F, -2.0F, -2.0F },
			{ 1.0F, 2.0F, 2.0F });
	ASSERT_TRUE(base.is_valid());

	quader::commands::CommandHistory history;
	quader::tools::ToolManager tools{ quader::tools::ToolContext{ document, history } };
	ASSERT_TRUE(tools.register_tool(std::make_unique<ViewportPassiveSelectTool>()));
	ASSERT_TRUE(tools.register_tool(std::make_unique<quader::tools::BoxTool>()));
	ASSERT_TRUE(tools.set_active_tool(quader::tools::ToolId::Box));

	FakeRenderHost host;
	quader::ui::ViewportController controller(host, tools);
	controller.set_quad_viewports_enabled(true);
	const quader::ui::ViewportPixelSize kSize{ 640, 480 };
	const quader::ui::ViewportPaneArray kPanes = controller.layout_state().panes_for(kSize);
	const auto kRightPaneIt = std::find_if(kPanes.begin(), kPanes.end(), [](const quader::ui::ViewportPane &pane) {
		return pane.camera_index == 3;
	});
	ASSERT_TRUE(kRightPaneIt != kPanes.end());

	const quader::ui::ViewportPoint kCenter = pane_center(*kRightPaneIt);
	const quader::ui::ViewportPoint kDrag{
		kCenter.x + 80.0,
		kCenter.y - 80.0,
	};
	EXPECT_TRUE(controller.handle_mouse_press(quader::ui::ViewportMouseButton::Left,
			kCenter,
			false,
			kSize));
	EXPECT_TRUE(controller.handle_mouse_move(kDrag, kSize, true));
	EXPECT_TRUE(controller.handle_mouse_release(quader::ui::ViewportMouseButton::Left,
			kDrag,
			kSize));

	ASSERT_EQ(document.object_count(), 2U);
	ASSERT_EQ(document.selection().selected_objects().size(), 1U);
	const quader::document::ObjectId created = document.selection().selected_objects().front();
	EXPECT_NE(created, base);
	const auto *object = document.find_mesh_object(created);
	ASSERT_TRUE(object != nullptr);
	const ComponentBounds x_bounds = object_component_bounds(*object, component_x);
	EXPECT_TRUE(quader::math::nearly_equal(x_bounds.min_value, 1.0F));
	EXPECT_TRUE(quader::math::nearly_equal(x_bounds.max_value, 2.0F));
	expect_mesh_faces_oriented_outward(*object);
	expect_viewport_upload_triangles_front_facing(*object);
	ASSERT_TRUE(tools.active_tool_id().has_value());
	EXPECT_EQ(*tools.active_tool_id(), quader::tools::ToolId::Select);
}

TEST(Viewport, ControllerClearsSelectionHoverWhenNavigationStarts) {
	auto fixture = make_viewport_triangle_fixture();
	quader::commands::CommandHistory history;
	quader::tools::ToolManager tools{ quader::tools::ToolContext{ fixture.document, history } };
	ASSERT_TRUE(tools.register_tool(std::make_unique<ViewportRecordingPassiveSelectTool>()));
	ASSERT_TRUE(tools.set_active_tool(quader::tools::ToolId::Select));
	ASSERT_TRUE(tools.set_selection_mode(quader::tools::SelectionMode::Face));

	FakeRenderHost host;
	quader::ui::ViewportController controller(host, tools);
	EXPECT_TRUE(controller.handle_mouse_move({ 320.0, 240.0 }, { 640, 480 }));
	ASSERT_TRUE(tools.selection_hover().has_value());

	EXPECT_TRUE(controller.handle_mouse_press(quader::ui::ViewportMouseButton::Right,
			{ 320.0, 240.0 },
			false,
			{ 640, 480 }));
	EXPECT_EQ(controller.navigation_mode(), quader::ui::NavigationMode::Fly);
	EXPECT_FALSE(tools.selection_hover().has_value());
}

TEST(Viewport, ControllerDispatchesPaneLocalPointerAndViewIndexInQuadLayout) {
	FakeRenderHost host;
	quader::document::Document document;
	quader::commands::CommandHistory history;
	quader::tools::ToolManager tools{ quader::tools::ToolContext{ document, history } };
	auto tool = std::make_unique<ViewportRecordingTool>();
	auto *recording_tool = tool.get();
	ASSERT_TRUE(tools.register_tool(std::move(tool)));
	ASSERT_TRUE(tools.set_active_tool(quader::tools::ToolId::Select));

	quader::ui::ViewportController controller(host, tools);
	controller.set_quad_viewports_enabled(true);

	EXPECT_TRUE(controller.handle_mouse_press(quader::ui::ViewportMouseButton::Left,
			{ 500.0, 100.0 },
			false,
			{ 640, 480 }));

	ASSERT_EQ(recording_tool->pointer_events, 1);
	ASSERT_TRUE(recording_tool->last_pointer.has_value());
	const quader::tools::PointerEvent &kLastPointer = recording_tool->last_pointer.value();
	EXPECT_EQ(kLastPointer.view_index, 1U);
	EXPECT_FLOAT_EQ(kLastPointer.position.x, 178.0F);
	EXPECT_FLOAT_EQ(kLastPointer.position.y, 100.0F);
	EXPECT_TRUE(kLastPointer.ray.has_value());
	ASSERT_TRUE(kLastPointer.camera.has_value());
	EXPECT_FLOAT_EQ(kLastPointer.camera->viewport_size_pixels.x, 318.0F);
	EXPECT_FLOAT_EQ(kLastPointer.camera->viewport_size_pixels.y, 238.0F);
	EXPECT_EQ(kLastPointer.camera->projection, quader::tools::ViewportCameraProjection::Orthographic);
}

TEST(Viewport, TransformGizmoRefreshesOnToolSwitchAndCameraZoomWithoutMouseMove) {
	FakeRenderHost host;
	auto fixture = quader::tests::document_fixtures::make_document_with_triangle_object();
	quader::document::Selection selection;
	ASSERT_TRUE(selection.set_objects({ fixture.object }));
	ASSERT_TRUE(fixture.document.set_selection(std::move(selection)));

	quader::commands::CommandHistory history;
	quader::tools::ToolManager tools{ quader::tools::ToolContext{ fixture.document, history } };
	ASSERT_TRUE(tools.register_tool(std::make_unique<quader::tools::TransformGizmoTool>(
			quader::tools::ToolId::Move)));

	quader::ui::ViewportController controller(host, tools);
	controller.initialize_surface(quader::ui::NativeViewportSurface{}, { 640, 480 }, 1.0);
	EXPECT_FALSE(controller.handle_mouse_move({ 320.0, 240.0 }, { 640, 480 }));
	ASSERT_TRUE(tools.set_active_tool(quader::tools::ToolId::Move));
	EXPECT_TRUE(tools.preview().empty());

	controller.render_frame(0.0, 0.0F);
	EXPECT_FALSE(tools.preview().empty());
	const auto *tool = static_cast<const quader::tools::TransformGizmoTool *>(tools.active_tool());
	ASSERT_TRUE(tool != nullptr);
	const float kInitialScale = tool->state().gizmo_scale;

	controller.handle_wheel(1.0F, { 320.0, 240.0 }, { 640, 480 });
	controller.render_frame(0.016, 0.016F);
	EXPECT_FALSE(tools.preview().empty());
	EXPECT_NE(tool->state().gizmo_scale, kInitialScale);
}

TEST(Viewport, ToolPreviewOverlayAdapterUsesReferenceStyleAndActiveViewOnly) {
	quader::tools::ToolPreview preview;
	preview.active = true;
	preview.view_index = 2U;
	preview.world_segments.push_back(quader::tools::ToolPreviewWorldSegment{
			.start = { 0.0F, 0.0F, 0.0F },
			.end = { 1.0F, 0.0F, 0.0F },
	});

	std::vector<crimson::OverlayCommand> overlays;
	std::vector<crimson::LineOverlaySegment> line_payloads;
	quader::ui::append_tool_preview_line_overlays(preview, 4U, overlays, line_payloads);

	ASSERT_EQ(overlays.size(), 1U);
	ASSERT_EQ(line_payloads.size(), 1U);
	EXPECT_EQ(overlays.front().view_index, 2U);
	EXPECT_EQ(overlays.front().primitive, crimson::OverlayPrimitive::LineList);
	EXPECT_EQ(overlays.front().depth_mode, crimson::OverlayDepthMode::AlwaysOnTop);
	EXPECT_FLOAT_EQ(overlays.front().color_srgb.r, 1.0F);
	EXPECT_FLOAT_EQ(overlays.front().color_srgb.g, 235.0F / 255.0F);
	EXPECT_FLOAT_EQ(overlays.front().color_srgb.b, 41.0F / 255.0F);
	EXPECT_FLOAT_EQ(overlays.front().color_srgb.a, 209.0F / 255.0F);
	EXPECT_FLOAT_EQ(overlays.front().opacity, 1.0F);
	EXPECT_FLOAT_EQ(overlays.front().thickness_px, 2.0F);
	EXPECT_EQ(overlays.front().payload_offset, 0U);
	EXPECT_EQ(overlays.front().payload_count, 1U);
}

TEST(Viewport, ToolPreviewOverlayAdapterPreservesColoredGizmoSegments) {
	quader::tools::ToolPreview preview;
	preview.active = true;
	preview.view_index = 1U;
	preview.colored_world_segments.push_back(quader::tools::ToolPreviewColoredWorldSegment{
			.start = { 0.0F, 0.0F, 0.0F },
			.end = { 1.0F, 0.0F, 0.0F },
			.color_srgb = { 0.1F, 0.2F, 0.3F, 0.4F },
			.thickness_px = 5.0F,
	});

	std::vector<crimson::OverlayCommand> overlays;
	std::vector<crimson::LineOverlaySegment> line_payloads;
	quader::ui::append_tool_preview_line_overlays(preview, 4U, overlays, line_payloads);

	ASSERT_EQ(overlays.size(), 1U);
	ASSERT_EQ(line_payloads.size(), 1U);
	EXPECT_EQ(overlays.front().view_index, 1U);
	EXPECT_EQ(overlays.front().semantic_role, crimson::OverlaySemanticRole::ToolPreview);
	EXPECT_EQ(overlays.front().source_kind, crimson::OverlaySourceKind::ToolPreview);
	EXPECT_FLOAT_EQ(overlays.front().color_srgb.r, 0.1F);
	EXPECT_FLOAT_EQ(overlays.front().color_srgb.g, 0.2F);
	EXPECT_FLOAT_EQ(overlays.front().color_srgb.b, 0.3F);
	EXPECT_FLOAT_EQ(overlays.front().color_srgb.a, 0.4F);
	EXPECT_FLOAT_EQ(overlays.front().thickness_px, 5.0F);
	EXPECT_EQ(line_payloads.front().semantic_role, crimson::OverlaySemanticRole::ToolPreview);
	EXPECT_EQ(line_payloads.front().source_kind, crimson::OverlaySourceKind::ToolPreview);
}

TEST(Viewport, ToolPreviewOverlayAdapterPreservesColoredGizmoTriangles) {
	quader::tools::ToolPreview preview;
	preview.active = true;
	preview.view_index = 1U;
	preview.colored_world_triangles.push_back(quader::tools::ToolPreviewColoredWorldTriangle{
			.a = { 0.0F, 0.0F, 0.0F },
			.b = { 1.0F, 0.0F, 0.0F },
			.c = { 0.0F, 1.0F, 0.0F },
			.color_srgb = { 0.1F, 0.2F, 0.3F, 0.4F },
	});

	std::vector<crimson::OverlayCommand> overlays;
	std::vector<crimson::LineOverlaySegment> line_payloads;
	std::vector<crimson::TriangleOverlayPrimitive> triangle_payloads;
	quader::ui::append_tool_preview_overlays(preview, 4U, overlays, line_payloads, triangle_payloads);

	ASSERT_EQ(overlays.size(), 1U);
	ASSERT_TRUE(line_payloads.empty());
	ASSERT_EQ(triangle_payloads.size(), 1U);
	EXPECT_EQ(overlays.front().view_index, 1U);
	EXPECT_EQ(overlays.front().primitive, crimson::OverlayPrimitive::SolidTriangles);
	EXPECT_EQ(overlays.front().semantic_role, crimson::OverlaySemanticRole::ToolPreview);
	EXPECT_EQ(overlays.front().source_kind, crimson::OverlaySourceKind::ToolPreview);
	EXPECT_FLOAT_EQ(overlays.front().color_srgb.r, 0.1F);
	EXPECT_FLOAT_EQ(overlays.front().color_srgb.g, 0.2F);
	EXPECT_FLOAT_EQ(overlays.front().color_srgb.b, 0.3F);
	EXPECT_FLOAT_EQ(overlays.front().color_srgb.a, 0.4F);
	EXPECT_EQ(triangle_payloads.front().semantic_role, crimson::OverlaySemanticRole::ToolPreview);
	EXPECT_EQ(triangle_payloads.front().source_kind, crimson::OverlaySourceKind::ToolPreview);
}

TEST(Viewport, ToolPreviewOverlayAdapterConvertsBoxPreviewToLineVolume) {
	quader::tools::ToolPreview preview;
	preview.active = true;
	preview.view_index = 1U;
	preview.boxes.push_back(quader::tools::ToolPreviewBox{
			.corners = {
					quader::math::Vec3{ 0.0F, 0.0F, 0.0F },
					quader::math::Vec3{ 1.0F, 0.0F, 0.0F },
					quader::math::Vec3{ 1.0F, 0.0F, 1.0F },
					quader::math::Vec3{ 0.0F, 0.0F, 1.0F },
					quader::math::Vec3{ 0.0F, 1.0F, 0.0F },
					quader::math::Vec3{ 1.0F, 1.0F, 0.0F },
					quader::math::Vec3{ 1.0F, 1.0F, 1.0F },
					quader::math::Vec3{ 0.0F, 1.0F, 1.0F },
			},
			.active = true,
	});

	std::vector<crimson::OverlayCommand> overlays;
	std::vector<crimson::LineOverlaySegment> line_payloads;
	quader::ui::append_tool_preview_line_overlays(preview, 2U, overlays, line_payloads);

	ASSERT_EQ(overlays.size(), 1U);
	EXPECT_EQ(overlays.front().view_index, 1U);
	EXPECT_EQ(overlays.front().payload_count, 12U);
	EXPECT_EQ(line_payloads.size(), 12U);
}

TEST(Viewport, DocumentSelectionOverlayAdapterEmitsSelectedObjectTopology) {
	auto fixture = quader::tests::document_fixtures::make_document_with_triangle_object();
	quader::document::Selection selection;
	ASSERT_TRUE(selection.set_objects({ fixture.object }));
	ASSERT_TRUE(fixture.document.set_selection(std::move(selection)));

	std::vector<crimson::OverlayCommand> overlays;
	std::vector<crimson::LineOverlaySegment> line_payloads;
	std::vector<crimson::TriangleOverlayPrimitive> triangle_payloads;
	std::vector<crimson::PointOverlayPrimitive> point_payloads;
	quader::ui::append_document_selection_overlays(fixture.document,
			2U,
			overlays,
			line_payloads,
			triangle_payloads,
			point_payloads);

	ASSERT_EQ(overlays.size(), 4U);
	ASSERT_EQ(line_payloads.size(), 3U);
	ASSERT_EQ(triangle_payloads.size(), 1U);
	EXPECT_TRUE(point_payloads.empty());
	EXPECT_EQ(overlays.front().primitive, crimson::OverlayPrimitive::LineList);
	EXPECT_EQ(overlays.front().semantic_role, crimson::OverlaySemanticRole::SelectedObjectTopology);
	EXPECT_EQ(overlays.front().source_kind, crimson::OverlaySourceKind::ObjectSelection);
	EXPECT_FLOAT_EQ(overlays.front().color_srgb.r, 1.0F);
	EXPECT_FLOAT_EQ(overlays.front().color_srgb.g, 211.0F / 255.0F);
	EXPECT_FLOAT_EQ(overlays.front().color_srgb.b, 31.0F / 255.0F);
	EXPECT_FLOAT_EQ(overlays.front().thickness_px, 2.5F);
	EXPECT_EQ(overlays.front().payload_count, 3U);
	const auto kFaceFillCommand = std::find_if(overlays.begin(), overlays.end(), [](const crimson::OverlayCommand &command) {
		return command.primitive == crimson::OverlayPrimitive::SolidTriangles &&
				command.semantic_role == crimson::OverlaySemanticRole::SelectedFaceFill &&
				command.source_kind == crimson::OverlaySourceKind::ObjectSelection;
	});
	ASSERT_TRUE(kFaceFillCommand != overlays.end());
	EXPECT_FLOAT_EQ(kFaceFillCommand->color_srgb.r, 1.0F);
	EXPECT_FLOAT_EQ(kFaceFillCommand->color_srgb.g, 172.0F / 255.0F);
	EXPECT_FLOAT_EQ(kFaceFillCommand->color_srgb.b, 31.0F / 255.0F);
	EXPECT_FLOAT_EQ(kFaceFillCommand->color_srgb.a, 8.0F / 255.0F);
	for (const crimson::LineOverlaySegment &line : line_payloads) {
		EXPECT_EQ(line.semantic_role, crimson::OverlaySemanticRole::SelectedObjectTopology);
		EXPECT_EQ(line.source_kind, crimson::OverlaySourceKind::ObjectSelection);
		EXPECT_EQ(line.element.object_id, quader::ui::render_object_id_for_document_object(fixture.object));
		EXPECT_NE(line.element.edge_index, crimson::kInvalidOverlayElementIndex);
	}
	EXPECT_EQ(triangle_payloads.front().semantic_role, crimson::OverlaySemanticRole::SelectedFaceFill);
	EXPECT_EQ(triangle_payloads.front().source_kind, crimson::OverlaySourceKind::ObjectSelection);
	EXPECT_EQ(triangle_payloads.front().element.object_id, quader::ui::render_object_id_for_document_object(fixture.object));
	EXPECT_NE(triangle_payloads.front().element.face_index, crimson::kInvalidOverlayElementIndex);
}

TEST(Viewport, DocumentSelectionOverlayAdapterUsesPreviewObjectTransform) {
	auto fixture = quader::tests::document_fixtures::make_document_with_triangle_object();
	quader::document::Selection selection;
	ASSERT_TRUE(selection.set_objects({ fixture.object }));
	ASSERT_TRUE(fixture.document.set_selection(std::move(selection)));
	fixture.document.clear_dirty();
	(void)fixture.document.take_pending_changes();

	auto *object = fixture.document.find_mesh_object(fixture.object);
	ASSERT_TRUE(object != nullptr);
	quader::document::Transform preview_transform = object->transform;
	preview_transform.translation = { 3.0F, 0.0F, 0.0F };
	ASSERT_TRUE(fixture.document.set_preview_transform(fixture.object, preview_transform));
	EXPECT_FALSE(fixture.document.is_dirty());

	std::vector<crimson::OverlayCommand> overlays;
	std::vector<crimson::LineOverlaySegment> line_payloads;
	std::vector<crimson::TriangleOverlayPrimitive> triangle_payloads;
	std::vector<crimson::PointOverlayPrimitive> point_payloads;
	quader::ui::append_document_selection_overlays(fixture.document,
			1U,
			overlays,
			line_payloads,
			triangle_payloads,
			point_payloads);

	ASSERT_EQ(line_payloads.size(), 3U);
	for (const crimson::LineOverlaySegment &line : line_payloads) {
		EXPECT_GE(line.start.x, 3.0F);
		EXPECT_LE(line.start.x, 4.0F);
		EXPECT_GE(line.end.x, 3.0F);
		EXPECT_LE(line.end.x, 4.0F);
	}
}

TEST(Viewport, DocumentSelectionOverlayAdapterEmitsComponentSourceWireAndSelectedFace) {
	auto fixture = quader::tests::document_fixtures::make_document_with_triangle_object();
	quader::document::Selection selection;
	ASSERT_TRUE(selection.set_component_selection(quader::document::SelectionMode::Face,
			{ fixture.object },
			{ quader::document::ComponentRef{ fixture.object, fixture.face } }));
	ASSERT_TRUE(fixture.document.set_selection(std::move(selection)));

	std::vector<crimson::OverlayCommand> overlays;
	std::vector<crimson::LineOverlaySegment> line_payloads;
	std::vector<crimson::TriangleOverlayPrimitive> triangle_payloads;
	std::vector<crimson::PointOverlayPrimitive> point_payloads;
	quader::ui::append_document_selection_overlays(fixture.document,
			1U,
			overlays,
			line_payloads,
			triangle_payloads,
			point_payloads);

	EXPECT_FALSE(overlays.empty());
	EXPECT_EQ(line_payloads.size(), 6U);
	EXPECT_EQ(triangle_payloads.size(), 2U);
	EXPECT_TRUE(point_payloads.empty());
	EXPECT_TRUE(std::any_of(line_payloads.begin(), line_payloads.end(), [](const crimson::LineOverlaySegment &line) {
		return line.semantic_role == crimson::OverlaySemanticRole::SourceWire &&
				line.source_kind == crimson::OverlaySourceKind::SourceWire;
	}));
	EXPECT_TRUE(std::any_of(line_payloads.begin(), line_payloads.end(), [](const crimson::LineOverlaySegment &line) {
		return line.semantic_role == crimson::OverlaySemanticRole::SelectedFaceEdge &&
				line.source_kind == crimson::OverlaySourceKind::ComponentSelection &&
				line.element.face_index != crimson::kInvalidOverlayElementIndex;
	}));
	EXPECT_TRUE(std::any_of(triangle_payloads.begin(), triangle_payloads.end(), [](const crimson::TriangleOverlayPrimitive &triangle) {
		return triangle.semantic_role == crimson::OverlaySemanticRole::SourceWireDepthStamp &&
				triangle.source_kind == crimson::OverlaySourceKind::SourceWire;
	}));
	EXPECT_TRUE(std::any_of(triangle_payloads.begin(), triangle_payloads.end(), [](const crimson::TriangleOverlayPrimitive &triangle) {
		return triangle.semantic_role == crimson::OverlaySemanticRole::SelectedFaceFill &&
				triangle.source_kind == crimson::OverlaySourceKind::ComponentSelection &&
				triangle.element.face_index != crimson::kInvalidOverlayElementIndex;
	}));
	const auto kSelectedLineCommand = std::find_if(overlays.begin(), overlays.end(), [](const crimson::OverlayCommand &command) {
		return command.primitive == crimson::OverlayPrimitive::LineList &&
				command.semantic_role == crimson::OverlaySemanticRole::SelectedFaceEdge &&
				command.source_kind == crimson::OverlaySourceKind::ComponentSelection;
	});
	ASSERT_TRUE(kSelectedLineCommand != overlays.end());
	EXPECT_FLOAT_EQ(kSelectedLineCommand->color_srgb.r, 1.0F);
	EXPECT_FLOAT_EQ(kSelectedLineCommand->color_srgb.g, 211.0F / 255.0F);
	EXPECT_FLOAT_EQ(kSelectedLineCommand->color_srgb.b, 31.0F / 255.0F);
	const auto kSourceLineCommand = std::find_if(overlays.begin(), overlays.end(), [](const crimson::OverlayCommand &command) {
		return command.primitive == crimson::OverlayPrimitive::LineList &&
				command.semantic_role == crimson::OverlaySemanticRole::SourceWire &&
				command.source_kind == crimson::OverlaySourceKind::SourceWire;
	});
	ASSERT_TRUE(kSourceLineCommand != overlays.end());
	EXPECT_LT(std::distance(overlays.begin(), kSourceLineCommand), std::distance(overlays.begin(), kSelectedLineCommand));
	const auto kSelectedFillCommand = std::find_if(overlays.begin(), overlays.end(), [](const crimson::OverlayCommand &command) {
		return command.primitive == crimson::OverlayPrimitive::SolidTriangles &&
				command.semantic_role == crimson::OverlaySemanticRole::SelectedFaceFill &&
				command.source_kind == crimson::OverlaySourceKind::ComponentSelection;
	});
	ASSERT_TRUE(kSelectedFillCommand != overlays.end());
	EXPECT_FLOAT_EQ(kSelectedFillCommand->color_srgb.a, 8.0F / 255.0F);
	EXPECT_EQ(kSelectedFillCommand->depth_mode, crimson::OverlayDepthMode::DepthTested);
	EXPECT_FALSE(std::any_of(overlays.begin(), overlays.end(), [](const crimson::OverlayCommand &command) {
		return command.semantic_role == crimson::OverlaySemanticRole::SelectedFaceFill &&
				command.source_kind == crimson::OverlaySourceKind::ObjectSelection;
	}));
}

TEST(Viewport, DocumentSelectionOverlayAdapterEmitsHoverComponentOverlays) {
	auto fixture = quader::tests::document_fixtures::make_document_with_triangle_object();
	quader::document::Selection selection;
	ASSERT_TRUE(selection.set_component_selection(quader::document::SelectionMode::Face,
			{ fixture.object },
			{}));
	ASSERT_TRUE(fixture.document.set_selection(std::move(selection)));

	const quader::tools::SurfaceHit kHover{
		.position = { 0.0F, 0.0F, 0.0F },
		.normal = { 0.0F, 1.0F, 0.0F },
		.object_id = quader::ui::render_object_id_for_document_object(fixture.object),
		.component_index = fixture.face.index(),
		.document_object_id = fixture.object,
		.component = fixture.face,
		.kind = quader::tools::SurfaceHitKind::Face,
	};

	std::vector<crimson::OverlayCommand> overlays;
	std::vector<crimson::LineOverlaySegment> line_payloads;
	std::vector<crimson::TriangleOverlayPrimitive> triangle_payloads;
	std::vector<crimson::PointOverlayPrimitive> point_payloads;
	quader::ui::append_document_selection_overlays(fixture.document,
			1U,
			overlays,
			line_payloads,
			triangle_payloads,
			point_payloads,
			kHover);

	EXPECT_FALSE(overlays.empty());
	EXPECT_TRUE(point_payloads.empty());
	EXPECT_TRUE(std::any_of(line_payloads.begin(), line_payloads.end(), [](const crimson::LineOverlaySegment &line) {
		return line.semantic_role == crimson::OverlaySemanticRole::SourceWire &&
				line.source_kind == crimson::OverlaySourceKind::SourceWire;
	}));
	EXPECT_TRUE(std::any_of(line_payloads.begin(), line_payloads.end(), [](const crimson::LineOverlaySegment &line) {
		return line.semantic_role == crimson::OverlaySemanticRole::HoverFaceEdge &&
				line.source_kind == crimson::OverlaySourceKind::ComponentHover &&
				line.element.face_index != crimson::kInvalidOverlayElementIndex;
	}));
	EXPECT_TRUE(std::any_of(triangle_payloads.begin(), triangle_payloads.end(), [](const crimson::TriangleOverlayPrimitive &triangle) {
		return triangle.semantic_role == crimson::OverlaySemanticRole::HoverFaceFill &&
				triangle.source_kind == crimson::OverlaySourceKind::ComponentHover &&
				triangle.element.face_index != crimson::kInvalidOverlayElementIndex;
	}));
	const auto kHoverFillCommand = std::find_if(overlays.begin(), overlays.end(), [](const crimson::OverlayCommand &command) {
		return command.primitive == crimson::OverlayPrimitive::SolidTriangles &&
				command.semantic_role == crimson::OverlaySemanticRole::HoverFaceFill &&
				command.source_kind == crimson::OverlaySourceKind::ComponentHover;
	});
	ASSERT_TRUE(kHoverFillCommand != overlays.end());
	EXPECT_FLOAT_EQ(kHoverFillCommand->color_srgb.a, 22.0F / 255.0F);
	const auto kHoverLineCommand = std::find_if(overlays.begin(), overlays.end(), [](const crimson::OverlayCommand &command) {
		return command.primitive == crimson::OverlayPrimitive::LineList &&
				command.semantic_role == crimson::OverlaySemanticRole::HoverFaceEdge &&
				command.source_kind == crimson::OverlaySourceKind::ComponentHover;
	});
	ASSERT_TRUE(kHoverLineCommand != overlays.end());
	EXPECT_FLOAT_EQ(kHoverLineCommand->color_srgb.r, 81.0F / 255.0F);
	EXPECT_FLOAT_EQ(kHoverLineCommand->color_srgb.g, 1.0F);
	EXPECT_FLOAT_EQ(kHoverLineCommand->color_srgb.b, 0.0F);
}

TEST(Viewport, DocumentSelectionOverlayAdapterKeepsSelectedFaceUnderNormalHover) {
	auto fixture = quader::tests::document_fixtures::make_document_with_triangle_object();
	quader::document::Selection selection;
	ASSERT_TRUE(selection.set_component_selection(quader::document::SelectionMode::Face,
			{ fixture.object },
			{ quader::document::ComponentRef{ fixture.object, fixture.face } }));
	ASSERT_TRUE(fixture.document.set_selection(std::move(selection)));

	const quader::tools::SurfaceHit kHover{
		.position = { 0.0F, 0.0F, 0.0F },
		.normal = { 0.0F, 1.0F, 0.0F },
		.object_id = quader::ui::render_object_id_for_document_object(fixture.object),
		.component_index = fixture.face.index(),
		.document_object_id = fixture.object,
		.component = fixture.face,
		.kind = quader::tools::SurfaceHitKind::Face,
	};

	std::vector<crimson::OverlayCommand> overlays;
	std::vector<crimson::LineOverlaySegment> line_payloads;
	std::vector<crimson::TriangleOverlayPrimitive> triangle_payloads;
	std::vector<crimson::PointOverlayPrimitive> point_payloads;
	quader::ui::append_document_selection_overlays(fixture.document,
			1U,
			overlays,
			line_payloads,
			triangle_payloads,
			point_payloads,
			kHover);

	EXPECT_TRUE(std::any_of(line_payloads.begin(), line_payloads.end(), [](const crimson::LineOverlaySegment &line) {
		return line.semantic_role == crimson::OverlaySemanticRole::SelectedFaceEdge &&
				line.source_kind == crimson::OverlaySourceKind::ComponentSelection;
	}));
	EXPECT_TRUE(std::any_of(triangle_payloads.begin(), triangle_payloads.end(), [](const crimson::TriangleOverlayPrimitive &triangle) {
		return triangle.semantic_role == crimson::OverlaySemanticRole::SelectedFaceFill &&
				triangle.source_kind == crimson::OverlaySourceKind::ComponentSelection;
	}));
	EXPECT_TRUE(std::any_of(line_payloads.begin(), line_payloads.end(), [](const crimson::LineOverlaySegment &line) {
		return line.semantic_role == crimson::OverlaySemanticRole::HoverFaceEdge &&
				line.source_kind == crimson::OverlaySourceKind::ComponentHover;
	}));
	EXPECT_TRUE(std::any_of(triangle_payloads.begin(), triangle_payloads.end(), [](const crimson::TriangleOverlayPrimitive &triangle) {
		return triangle.semantic_role == crimson::OverlaySemanticRole::HoverFaceFill &&
				triangle.source_kind == crimson::OverlaySourceKind::ComponentHover;
	}));
}

TEST(Viewport, DocumentSelectionOverlayAdapterSuppressesSelectedFaceOnlyForModifierHover) {
	auto fixture = quader::tests::document_fixtures::make_document_with_triangle_object();
	quader::document::Selection selection;
	ASSERT_TRUE(selection.set_component_selection(quader::document::SelectionMode::Face,
			{ fixture.object },
			{ quader::document::ComponentRef{ fixture.object, fixture.face } }));
	ASSERT_TRUE(fixture.document.set_selection(std::move(selection)));

	const quader::tools::SurfaceHit kHover{
		.position = { 0.0F, 0.0F, 0.0F },
		.normal = { 0.0F, 1.0F, 0.0F },
		.object_id = quader::ui::render_object_id_for_document_object(fixture.object),
		.component_index = fixture.face.index(),
		.document_object_id = fixture.object,
		.component = fixture.face,
		.kind = quader::tools::SurfaceHitKind::Face,
	};

	std::vector<crimson::OverlayCommand> overlays;
	std::vector<crimson::LineOverlaySegment> line_payloads;
	std::vector<crimson::TriangleOverlayPrimitive> triangle_payloads;
	std::vector<crimson::PointOverlayPrimitive> point_payloads;
	quader::ui::append_document_selection_overlays(fixture.document,
			1U,
			overlays,
			line_payloads,
			triangle_payloads,
			point_payloads,
			kHover,
			true);

	EXPECT_FALSE(std::any_of(line_payloads.begin(), line_payloads.end(), [](const crimson::LineOverlaySegment &line) {
		return line.semantic_role == crimson::OverlaySemanticRole::SelectedFaceEdge &&
				line.source_kind == crimson::OverlaySourceKind::ComponentSelection;
	}));
	EXPECT_FALSE(std::any_of(triangle_payloads.begin(), triangle_payloads.end(), [](const crimson::TriangleOverlayPrimitive &triangle) {
		return triangle.semantic_role == crimson::OverlaySemanticRole::SelectedFaceFill &&
				triangle.source_kind == crimson::OverlaySourceKind::ComponentSelection;
	}));
	EXPECT_TRUE(std::any_of(line_payloads.begin(), line_payloads.end(), [](const crimson::LineOverlaySegment &line) {
		return line.semantic_role == crimson::OverlaySemanticRole::HoverFaceEdge &&
				line.source_kind == crimson::OverlaySourceKind::ComponentHover;
	}));
	EXPECT_TRUE(std::any_of(triangle_payloads.begin(), triangle_payloads.end(), [](const crimson::TriangleOverlayPrimitive &triangle) {
		return triangle.semantic_role == crimson::OverlaySemanticRole::HoverFaceFill &&
				triangle.source_kind == crimson::OverlaySourceKind::ComponentHover;
	}));
}

TEST(Viewport, DocumentSelectionOverlayAdapterEmitsEdgeAndVertexComponentHandles) {
	auto edge_fixture = quader::tests::document_fixtures::make_document_with_triangle_object();
	quader::document::Selection edge_selection;
	ASSERT_TRUE(edge_selection.set_component_selection(quader::document::SelectionMode::Edge,
			{ edge_fixture.object },
			{ quader::document::ComponentRef{ edge_fixture.object, edge_fixture.edge } }));
	ASSERT_TRUE(edge_fixture.document.set_selection(std::move(edge_selection)));

	std::vector<crimson::OverlayCommand> overlays;
	std::vector<crimson::LineOverlaySegment> line_payloads;
	std::vector<crimson::TriangleOverlayPrimitive> triangle_payloads;
	std::vector<crimson::PointOverlayPrimitive> point_payloads;
	quader::ui::append_document_selection_overlays(edge_fixture.document,
			1U,
			overlays,
			line_payloads,
			triangle_payloads,
			point_payloads);

	EXPECT_TRUE(std::any_of(line_payloads.begin(), line_payloads.end(), [](const crimson::LineOverlaySegment &line) {
		return line.semantic_role == crimson::OverlaySemanticRole::SelectedEdge &&
				line.source_kind == crimson::OverlaySourceKind::ComponentSelection &&
				line.element.edge_index != crimson::kInvalidOverlayElementIndex;
	}));
	EXPECT_TRUE(point_payloads.empty());

	auto vertex_fixture = quader::tests::document_fixtures::make_document_with_triangle_object();
	quader::document::Selection vertex_selection;
	ASSERT_TRUE(vertex_selection.set_component_selection(quader::document::SelectionMode::Vertex,
			{ vertex_fixture.object },
			{ quader::document::ComponentRef{ vertex_fixture.object, vertex_fixture.vertices.front() } }));
	ASSERT_TRUE(vertex_fixture.document.set_selection(std::move(vertex_selection)));

	overlays.clear();
	line_payloads.clear();
	triangle_payloads.clear();
	point_payloads.clear();
	quader::ui::append_document_selection_overlays(vertex_fixture.document,
			1U,
			overlays,
			line_payloads,
			triangle_payloads,
			point_payloads);

	EXPECT_EQ(std::count_if(point_payloads.begin(), point_payloads.end(), [](const crimson::PointOverlayPrimitive &point) {
		return point.semantic_role == crimson::OverlaySemanticRole::SourceVertex &&
				point.source_kind == crimson::OverlaySourceKind::SourceWire;
	}), 2);
	const auto kSelectedPointIt = std::find_if(point_payloads.begin(),
			point_payloads.end(),
			[](const crimson::PointOverlayPrimitive &point) {
				return point.semantic_role == crimson::OverlaySemanticRole::SelectedVertex &&
						point.source_kind == crimson::OverlaySourceKind::ComponentSelection;
	});
	ASSERT_TRUE(kSelectedPointIt != point_payloads.end());
	EXPECT_EQ(kSelectedPointIt->element.vertex_index, vertex_fixture.vertices.front().index());
	EXPECT_FLOAT_EQ(kSelectedPointIt->size_px, 7.5F);
	const auto kSourceCommandIt = std::find_if(overlays.begin(), overlays.end(), [](const crimson::OverlayCommand &command) {
		return command.semantic_role == crimson::OverlaySemanticRole::SourceVertex &&
				command.source_kind == crimson::OverlaySourceKind::SourceWire;
	});
	const auto kSelectedCommandIt = std::find_if(overlays.begin(), overlays.end(), [](const crimson::OverlayCommand &command) {
		return command.semantic_role == crimson::OverlaySemanticRole::SelectedVertex &&
				command.source_kind == crimson::OverlaySourceKind::ComponentSelection;
	});
	ASSERT_TRUE(kSourceCommandIt != overlays.end());
	ASSERT_TRUE(kSelectedCommandIt != overlays.end());
	EXPECT_LT(std::distance(overlays.begin(), kSourceCommandIt), std::distance(overlays.begin(), kSelectedCommandIt));
}

TEST(Viewport, DocumentSelectionOverlayAdapterKeepsSelectedSourceWireForDifferentHoverMesh) {
	auto fixture = quader::tests::document_fixtures::make_document_with_triangle_object();
	auto second_mesh = quader::tests::document_fixtures::make_triangle_mesh();
	auto second_object = fixture.document.create_mesh_object("Second", std::move(second_mesh.mesh));
	ASSERT_TRUE(second_object);

	quader::document::Selection selection;
	ASSERT_TRUE(selection.set_component_selection(quader::document::SelectionMode::Face,
			{ fixture.object },
			{ quader::document::ComponentRef{ fixture.object, fixture.face } }));
	ASSERT_TRUE(fixture.document.set_selection(std::move(selection)));

	const quader::tools::SurfaceHit kHover{
		.position = { 0.0F, 0.0F, 0.0F },
		.normal = { 0.0F, 1.0F, 0.0F },
		.object_id = quader::ui::render_object_id_for_document_object(second_object.value()),
		.component_index = fixture.face.index(),
		.document_object_id = second_object.value(),
		.component = fixture.face,
		.kind = quader::tools::SurfaceHitKind::Face,
	};

	std::vector<crimson::OverlayCommand> overlays;
	std::vector<crimson::LineOverlaySegment> line_payloads;
	std::vector<crimson::TriangleOverlayPrimitive> triangle_payloads;
	std::vector<crimson::PointOverlayPrimitive> point_payloads;
	quader::ui::append_document_selection_overlays(fixture.document,
			1U,
			overlays,
			line_payloads,
			triangle_payloads,
			point_payloads,
			kHover);

	const auto kSelectedObjectWireCount = std::count_if(line_payloads.begin(), line_payloads.end(), [&fixture](const crimson::LineOverlaySegment &line) {
		return line.semantic_role == crimson::OverlaySemanticRole::SourceWire &&
				line.source_kind == crimson::OverlaySourceKind::SourceWire &&
				line.element.object_id == quader::ui::render_object_id_for_document_object(fixture.object);
	});
	const auto kHoverObjectWireCount = std::count_if(line_payloads.begin(), line_payloads.end(), [second_object](const crimson::LineOverlaySegment &line) {
		return line.semantic_role == crimson::OverlaySemanticRole::SourceWire &&
				line.source_kind == crimson::OverlaySourceKind::SourceWire &&
				line.element.object_id == quader::ui::render_object_id_for_document_object(second_object.value());
	});
	EXPECT_EQ(kSelectedObjectWireCount, 3);
	EXPECT_EQ(kHoverObjectWireCount, 0);
	EXPECT_TRUE(std::any_of(line_payloads.begin(), line_payloads.end(), [&fixture](const crimson::LineOverlaySegment &line) {
		return line.semantic_role == crimson::OverlaySemanticRole::SelectedFaceEdge &&
				line.source_kind == crimson::OverlaySourceKind::ComponentSelection &&
				line.element.object_id == quader::ui::render_object_id_for_document_object(fixture.object);
	}));
	EXPECT_TRUE(std::any_of(line_payloads.begin(), line_payloads.end(), [second_object](const crimson::LineOverlaySegment &line) {
		return line.semantic_role == crimson::OverlaySemanticRole::HoverFaceEdge &&
				line.source_kind == crimson::OverlaySourceKind::ComponentHover &&
				line.element.object_id == quader::ui::render_object_id_for_document_object(second_object.value());
	}));
}

TEST(Viewport, DocumentSelectionOverlayAdapterKeepsStickySourceWireForSameMeshHover) {
	quader::document::Document document;
	const quader::document::ObjectId object_id = add_box_object(
			document,
			"Box",
			{ 0.0F, 0.0F, 0.0F },
			{ 1.0F, 1.0F, 1.0F });
	ASSERT_TRUE(object_id.is_valid());
	const auto *object = document.find_mesh_object(object_id);
	ASSERT_TRUE(object != nullptr);
	const std::vector<quader::mesh::FaceId> kFaces = object->mesh.face_ids();
	ASSERT_GE(kFaces.size(), 2U);

	quader::document::Selection selection;
	ASSERT_TRUE(selection.set_component_selection(quader::document::SelectionMode::Face,
			{ object_id },
			{ quader::document::ComponentRef{ object_id, kFaces[0] } }));
	ASSERT_TRUE(document.set_selection(std::move(selection)));

	const quader::tools::SurfaceHit kHover{
		.position = { 0.5F, 0.5F, 1.0F },
		.normal = { 0.0F, 0.0F, 1.0F },
		.object_id = quader::ui::render_object_id_for_document_object(object_id),
		.component_index = kFaces[1].index(),
		.document_object_id = object_id,
		.component = kFaces[1],
		.kind = quader::tools::SurfaceHitKind::Face,
	};

	std::vector<crimson::OverlayCommand> overlays;
	std::vector<crimson::LineOverlaySegment> line_payloads;
	std::vector<crimson::TriangleOverlayPrimitive> triangle_payloads;
	std::vector<crimson::PointOverlayPrimitive> point_payloads;
	quader::ui::append_document_selection_overlays(document,
			1U,
			overlays,
			line_payloads,
			triangle_payloads,
			point_payloads,
			kHover);

	const auto kSourceWireCount = std::count_if(line_payloads.begin(), line_payloads.end(), [object_id](const crimson::LineOverlaySegment &line) {
		return line.semantic_role == crimson::OverlaySemanticRole::SourceWire &&
				line.source_kind == crimson::OverlaySourceKind::SourceWire &&
				line.element.object_id == quader::ui::render_object_id_for_document_object(object_id);
	});
	EXPECT_EQ(kSourceWireCount, static_cast<std::ptrdiff_t>(object->mesh.edge_count()));
	EXPECT_TRUE(std::any_of(line_payloads.begin(), line_payloads.end(), [object_id](const crimson::LineOverlaySegment &line) {
		return line.semantic_role == crimson::OverlaySemanticRole::SelectedFaceEdge &&
				line.source_kind == crimson::OverlaySourceKind::ComponentSelection &&
				line.element.object_id == quader::ui::render_object_id_for_document_object(object_id);
	}));
	EXPECT_TRUE(std::any_of(line_payloads.begin(), line_payloads.end(), [object_id](const crimson::LineOverlaySegment &line) {
		return line.semantic_role == crimson::OverlaySemanticRole::HoverFaceEdge &&
				line.source_kind == crimson::OverlaySourceKind::ComponentHover &&
				line.element.object_id == quader::ui::render_object_id_for_document_object(object_id);
	}));
}

TEST(Viewport, DocumentSelectionOverlayAdapterUsesHoverSourceWireWhenNoCurrentModeComponentSelected) {
	auto fixture = quader::tests::document_fixtures::make_document_with_triangle_object();
	auto second_mesh = quader::tests::document_fixtures::make_triangle_mesh();
	const quader::mesh::FaceId kSecondFace = second_mesh.face;
	auto second_object = fixture.document.create_mesh_object("Second", std::move(second_mesh.mesh));
	ASSERT_TRUE(second_object);

	quader::document::Selection selection;
	ASSERT_TRUE(selection.set_component_selection(quader::document::SelectionMode::Face,
			{ fixture.object },
			{}));
	ASSERT_TRUE(fixture.document.set_selection(std::move(selection)));

	const quader::tools::SurfaceHit kHover{
		.position = { 0.0F, 0.0F, 0.0F },
		.normal = { 0.0F, 1.0F, 0.0F },
		.object_id = quader::ui::render_object_id_for_document_object(second_object.value()),
		.component_index = kSecondFace.index(),
		.document_object_id = second_object.value(),
		.component = kSecondFace,
		.kind = quader::tools::SurfaceHitKind::Face,
	};

	std::vector<crimson::OverlayCommand> overlays;
	std::vector<crimson::LineOverlaySegment> line_payloads;
	std::vector<crimson::TriangleOverlayPrimitive> triangle_payloads;
	std::vector<crimson::PointOverlayPrimitive> point_payloads;
	quader::ui::append_document_selection_overlays(fixture.document,
			1U,
			overlays,
			line_payloads,
			triangle_payloads,
			point_payloads,
			kHover);

	const auto kFallbackObjectWireCount = std::count_if(line_payloads.begin(), line_payloads.end(), [&fixture](const crimson::LineOverlaySegment &line) {
		return line.semantic_role == crimson::OverlaySemanticRole::SourceWire &&
				line.source_kind == crimson::OverlaySourceKind::SourceWire &&
				line.element.object_id == quader::ui::render_object_id_for_document_object(fixture.object);
	});
	const auto kHoverObjectWireCount = std::count_if(line_payloads.begin(), line_payloads.end(), [second_object](const crimson::LineOverlaySegment &line) {
		return line.semantic_role == crimson::OverlaySemanticRole::SourceWire &&
				line.source_kind == crimson::OverlaySourceKind::SourceWire &&
				line.element.object_id == quader::ui::render_object_id_for_document_object(second_object.value());
	});
	EXPECT_EQ(kFallbackObjectWireCount, 0);
	EXPECT_EQ(kHoverObjectWireCount, 3);
	EXPECT_TRUE(std::any_of(line_payloads.begin(), line_payloads.end(), [second_object](const crimson::LineOverlaySegment &line) {
		return line.semantic_role == crimson::OverlaySemanticRole::HoverFaceEdge &&
				line.source_kind == crimson::OverlaySourceKind::ComponentHover &&
				line.element.object_id == quader::ui::render_object_id_for_document_object(second_object.value());
	}));
}

TEST(Viewport, ControllerRoutesCenterRayToEdgeAndVertexSelectionModes) {
	{
		auto fixture = make_viewport_triangle_fixture();
		quader::commands::CommandHistory history;
		quader::tools::ToolManager tools{ quader::tools::ToolContext{ fixture.document, history } };
		ASSERT_TRUE(tools.register_tool(std::make_unique<ViewportPassiveSelectTool>()));
		ASSERT_TRUE(tools.set_active_tool(quader::tools::ToolId::Select));
		ASSERT_TRUE(tools.set_selection_mode(quader::tools::SelectionMode::Edge));

		FakeRenderHost host;
		quader::ui::ViewportController controller(host, tools);
		EXPECT_TRUE(controller.handle_mouse_move({ 320.0, 240.0 }, { 640, 480 }));
		ASSERT_TRUE(tools.selection_hover().has_value());
		EXPECT_EQ(tools.selection_hover()->kind, quader::tools::SurfaceHitKind::Edge);
		ASSERT_TRUE(std::holds_alternative<quader::mesh::EdgeId>(tools.selection_hover()->component));
		const quader::mesh::EdgeId kHoverEdge = std::get<quader::mesh::EdgeId>(tools.selection_hover()->component);

		EXPECT_TRUE(controller.handle_mouse_press(quader::ui::ViewportMouseButton::Left,
				{ 320.0, 240.0 },
				false,
				{ 640, 480 }));

		const auto kComponents = fixture.document.selection().selected_components();
		ASSERT_EQ(kComponents.size(), 1U);
		ASSERT_TRUE(std::holds_alternative<quader::mesh::EdgeId>(kComponents.front().component));
		EXPECT_EQ(std::get<quader::mesh::EdgeId>(kComponents.front().component), kHoverEdge);
		const auto *object = fixture.document.find_mesh_object(fixture.object);
		ASSERT_TRUE(object != nullptr);
		EXPECT_TRUE(object->mesh.is_valid(std::get<quader::mesh::EdgeId>(kComponents.front().component)));
	}

	{
		auto fixture = make_viewport_triangle_fixture();
		quader::commands::CommandHistory history;
		quader::tools::ToolManager tools{ quader::tools::ToolContext{ fixture.document, history } };
		ASSERT_TRUE(tools.register_tool(std::make_unique<ViewportPassiveSelectTool>()));
		ASSERT_TRUE(tools.set_active_tool(quader::tools::ToolId::Select));
		ASSERT_TRUE(tools.set_selection_mode(quader::tools::SelectionMode::Vertex));

		FakeRenderHost host;
		quader::ui::ViewportController controller(host, tools);
		EXPECT_TRUE(controller.handle_mouse_move({ 320.0, 240.0 }, { 640, 480 }));
		ASSERT_TRUE(tools.selection_hover().has_value());
		EXPECT_EQ(tools.selection_hover()->kind, quader::tools::SurfaceHitKind::Vertex);
		ASSERT_TRUE(std::holds_alternative<quader::mesh::VertexId>(tools.selection_hover()->component));
		EXPECT_EQ(std::get<quader::mesh::VertexId>(tools.selection_hover()->component), fixture.near_vertex);

		EXPECT_TRUE(controller.handle_mouse_press(quader::ui::ViewportMouseButton::Left,
				{ 320.0, 240.0 },
				false,
				{ 640, 480 }));

		const auto kComponents = fixture.document.selection().selected_components();
		ASSERT_EQ(kComponents.size(), 1U);
		ASSERT_TRUE(std::holds_alternative<quader::mesh::VertexId>(kComponents.front().component));
		EXPECT_EQ(std::get<quader::mesh::VertexId>(kComponents.front().component), fixture.near_vertex);
	}
}

TEST(Viewport, ControllerPicksParallelEdgeNearMidpointInTopPane) {
	auto fixture = make_viewport_parallel_edge_fixture();
	quader::commands::CommandHistory history;
	quader::tools::ToolManager tools{ quader::tools::ToolContext{ fixture.document, history } };
	ASSERT_TRUE(tools.register_tool(std::make_unique<ViewportPassiveSelectTool>()));
	ASSERT_TRUE(tools.set_active_tool(quader::tools::ToolId::Select));
	ASSERT_TRUE(tools.set_selection_mode(quader::tools::SelectionMode::Edge));

	FakeRenderHost host;
	quader::ui::ViewportController controller(host, tools);
	controller.set_quad_viewports_enabled(true);
	const quader::ui::ViewportPixelSize kSize{ 640, 480 };
	const quader::ui::ViewportPaneArray kPanes = controller.layout_state().panes_for(kSize);
	const auto kTopPaneIt = std::find_if(kPanes.begin(), kPanes.end(), [](const quader::ui::ViewportPane &pane) {
		return pane.camera_index == 1;
	});
	ASSERT_TRUE(kTopPaneIt != kPanes.end());

	const quader::ui::ViewportPoint kPaneCenter{
		static_cast<double>(kTopPaneIt->rect.x) + static_cast<double>(kTopPaneIt->rect.width) * 0.5,
		static_cast<double>(kTopPaneIt->rect.y) + static_cast<double>(kTopPaneIt->rect.height) * 0.5,
	};
	EXPECT_TRUE(controller.handle_mouse_press(quader::ui::ViewportMouseButton::Left,
			kPaneCenter,
			false,
			kSize));

	const auto kComponents = fixture.document.selection().selected_components();
	ASSERT_EQ(kComponents.size(), 1U);
	ASSERT_TRUE(std::holds_alternative<quader::mesh::EdgeId>(kComponents.front().component));
	const auto *object = fixture.document.find_mesh_object(fixture.object);
	ASSERT_TRUE(object != nullptr);
	EXPECT_TRUE(object->mesh.is_valid(std::get<quader::mesh::EdgeId>(kComponents.front().component)));
}

TEST(Viewport, NavigationCaptureTakesPrecedenceOverToolRoutingAndShortcuts) {
	FakeRenderHost host;
	quader::document::Document document;
	quader::commands::CommandHistory history;
	quader::tools::ToolManager tools{ quader::tools::ToolContext{ document, history } };
	auto tool = std::make_unique<ViewportRecordingTool>();
	auto *recording_tool = tool.get();
	ASSERT_TRUE(tools.register_tool(std::move(tool)));
	ASSERT_TRUE(tools.set_active_tool(quader::tools::ToolId::Select));

	quader::ui::ViewportController controller(host, tools);
	EXPECT_TRUE(controller.handle_mouse_press(quader::ui::ViewportMouseButton::Right,
			{ 320.0, 240.0 },
			false,
			{ 640, 480 }));
	EXPECT_EQ(controller.navigation_mode(), quader::ui::NavigationMode::Fly);
	EXPECT_TRUE(controller.handle_mouse_move({ 360.0, 240.0 }, { 640, 480 }));
	EXPECT_TRUE(controller.handle_key(quader::ui::ViewportKey::Other, true, false));
	EXPECT_EQ(recording_tool->pointer_events, 0);

	EXPECT_TRUE(controller.handle_mouse_release(quader::ui::ViewportMouseButton::Right,
			{ 360.0, 240.0 },
			{ 640, 480 }));
	EXPECT_EQ(controller.navigation_mode(), quader::ui::NavigationMode::None);
}

TEST(Viewport, FlyNavigationOverridesWasdActionShortcutsOnlyWhileActive) {
	FakeRenderHost host;
	quader::ui::ViewportController controller(host);
	const quader::ui::ViewportPixelSize kSize{ 640, 480 };

	EXPECT_FALSE(controller.overrides_action_shortcut(quader::ui::ViewportKey::W));
	EXPECT_FALSE(controller.overrides_action_shortcut(quader::ui::ViewportKey::A));
	EXPECT_FALSE(controller.overrides_action_shortcut(quader::ui::ViewportKey::S));
	EXPECT_FALSE(controller.overrides_action_shortcut(quader::ui::ViewportKey::D));

	EXPECT_TRUE(controller.handle_mouse_press(quader::ui::ViewportMouseButton::Right,
			{ 320.0, 240.0 },
			false,
			kSize));
	EXPECT_EQ(controller.navigation_mode(), quader::ui::NavigationMode::Fly);
	EXPECT_TRUE(controller.overrides_action_shortcut(quader::ui::ViewportKey::W));
	EXPECT_TRUE(controller.overrides_action_shortcut(quader::ui::ViewportKey::A));
	EXPECT_TRUE(controller.overrides_action_shortcut(quader::ui::ViewportKey::S));
	EXPECT_TRUE(controller.overrides_action_shortcut(quader::ui::ViewportKey::D));
	EXPECT_FALSE(controller.overrides_action_shortcut(quader::ui::ViewportKey::Escape));
	EXPECT_FALSE(controller.overrides_action_shortcut(quader::ui::ViewportKey::Other));

	EXPECT_TRUE(controller.handle_mouse_release(quader::ui::ViewportMouseButton::Right,
			{ 320.0, 240.0 },
			kSize));
	EXPECT_EQ(controller.navigation_mode(), quader::ui::NavigationMode::None);
	EXPECT_FALSE(controller.overrides_action_shortcut(quader::ui::ViewportKey::W));
	EXPECT_FALSE(controller.overrides_action_shortcut(quader::ui::ViewportKey::A));
	EXPECT_FALSE(controller.overrides_action_shortcut(quader::ui::ViewportKey::S));
	EXPECT_FALSE(controller.overrides_action_shortcut(quader::ui::ViewportKey::D));
}

TEST(Viewport, EscapeCancelsActiveToolWhenNavigationIsInactive) {
	FakeRenderHost host;
	quader::document::Document document;
	quader::commands::CommandHistory history;
	quader::tools::ToolManager tools{ quader::tools::ToolContext{ document, history } };
	auto tool = std::make_unique<ViewportRecordingTool>();
	auto *recording_tool = tool.get();
	ASSERT_TRUE(tools.register_tool(std::move(tool)));
	ASSERT_TRUE(tools.set_active_tool(quader::tools::ToolId::Select));

	quader::ui::ViewportController controller(host, tools);
	EXPECT_TRUE(controller.handle_key(quader::ui::ViewportKey::Escape, true, false));
	EXPECT_EQ(recording_tool->cancellations, 1);
}

TEST(Viewport, RenderResultCarriesTypedPickResultsWithoutSelectionMutation) {
	std::vector<quader::ui::ViewportPickResult> pick_results = {
		quader::ui::ViewportPickResult{
				.request_id = 9,
				.hit = true,
				.payload = quader::ui::ViewportPickPayload{
						.object_id = 42,
						.submesh_index = 2,
						.element_kind = quader::ui::ViewportPickElementKind::Object,
				},
		},
	};

	const quader::ui::ViewportRenderResult kResult = quader::ui::ViewportRenderResult::success(
			QStringLiteral("FakeRenderer"),
			std::move(pick_results));
	EXPECT_TRUE(kResult.ok);
	EXPECT_EQ(kResult.completed_pick_results.size(), 1U);
	EXPECT_EQ(kResult.completed_pick_results[0].request_id, 9U);
	EXPECT_TRUE(kResult.completed_pick_results[0].hit);
	EXPECT_EQ(kResult.completed_pick_results[0].payload.object_id, 42U);
}

TEST(Viewport, RenderHostCanExposeDiagnosticsSnapshotWithoutGpuHandles) {
	FakeRenderHost host;
	host.diagnostics = quader::ui::ViewportDiagnosticsSnapshot{
		.renderer_name = QStringLiteral("FakeRenderer"),
		.frame = quader::ui::ViewportFrameStats{
				.width = 320,
				.height = 200,
				.viewport_count = 1,
				.fps = 30.0,
		},
		.passes = {
				quader::ui::ViewportRenderPassStats{
						.pass_name = QStringLiteral("PresentPass"),
						.draw_call_count = 1,
						.draw_packet_count = 1,
				},
		},
		.counters = {
				quader::ui::ViewportRendererCounter{
						.domain = QStringLiteral("Frame"),
						.name = QStringLiteral("frame_index"),
						.value = 7,
						.unit = QStringLiteral("Frames"),
				},
		},
		.diagnostics = {
				quader::ui::ViewportRendererDiagnosticRow{
						.severity = QStringLiteral("Warning"),
						.code = QStringLiteral("GoldenCaptureUnsupported"),
						.subsystem = QStringLiteral("golden"),
						.resource_name = QStringLiteral("GoldenCapturePass"),
						.message = QStringLiteral("Capture unsupported"),
						.detail = QStringLiteral("No active readback path"),
						.frame_index = 7,
				},
		},
		.frame_graph_dump = QStringLiteral("FrameSetupPass -> PresentPass"),
	};

	const std::optional<quader::ui::ViewportDiagnosticsSnapshot> kDiagnostics = host.latest_diagnostics();
	if (!kDiagnostics.has_value()) {
		ADD_FAILURE() << "render host did not expose diagnostics";
		return;
	}
	const auto &diagnostics_snapshot = *kDiagnostics;
	EXPECT_EQ(diagnostics_snapshot.renderer_name, QStringLiteral("FakeRenderer"));
	EXPECT_EQ(diagnostics_snapshot.frame.width, 320);
	EXPECT_EQ(diagnostics_snapshot.passes.size(), 1);
	EXPECT_EQ(diagnostics_snapshot.counters.size(), 1);
	EXPECT_EQ(diagnostics_snapshot.diagnostics.size(), 1);
	EXPECT_EQ(diagnostics_snapshot.diagnostics[0].resource_name, QStringLiteral("GoldenCapturePass"));
}

TEST(Viewport, CrimsonDiagnosticsMappingPreservesStructuredFields) {
	crimson::RendererDiagnosticsSnapshot source;
	source.status.backend_name = "SyntheticBackend";
	source.latest_frame_stats = crimson::FrameStats{
		.frame_index = 11,
		.width_px = 1920,
		.height_px = 1080,
		.view_count = 4,
		.fps = 59.5,
	};
	source.pass_stats = {
		crimson::RenderPassStats{
				.pass_name = "OpaquePbrPass",
				.resource_read_count = 2,
				.resource_write_count = 1,
				.draw_call_count = 3,
				.draw_packet_count = 5,
				.cpu_time_us = 17,
		},
	};
	source.counters = {
		crimson::RendererCounter{
				.domain = crimson::RendererCounterDomain::Upload,
				.name = "uploaded_vertex_bytes",
				.value = 128,
				.unit = crimson::RendererMetricUnit::Bytes,
		},
	};
	source.recent_diagnostics = {
		crimson::RendererDiagnostic{
				.severity = crimson::RendererDiagnosticSeverity::Warning,
				.code = crimson::RendererDiagnosticCode::GoldenCaptureUnsupported,
				.message = "Capture unsupported",
				.detail = "Readback is not stable on this backend.",
				.subsystem = "golden",
				.resource_name = "GoldenCapturePass",
				.frame_index = 11,
		},
	};
	source.frame_graph_dump = "FrameSetupPass -> OpaquePbrPass -> PresentPass";

	const quader::ui::ViewportDiagnosticsSnapshot kMapped =
			quader::ui::viewport_diagnostics_from_crimson(source);

	EXPECT_EQ(kMapped.renderer_name, QStringLiteral("SyntheticBackend"));
	EXPECT_EQ(kMapped.frame.width, 1920);
	EXPECT_EQ(kMapped.frame.height, 1080);
	EXPECT_EQ(kMapped.frame.viewport_count, 4);
	EXPECT_EQ(kMapped.passes.size(), 1);
	EXPECT_EQ(kMapped.passes[0].pass_name, QStringLiteral("OpaquePbrPass"));
	EXPECT_EQ(kMapped.passes[0].resource_read_count, 2);
	EXPECT_EQ(kMapped.passes[0].resource_write_count, 1);
	EXPECT_EQ(kMapped.passes[0].draw_call_count, 3);
	EXPECT_EQ(kMapped.passes[0].draw_packet_count, 5);
	EXPECT_EQ(kMapped.passes[0].cpu_time_us, 17ULL);
	EXPECT_EQ(kMapped.counters.size(), 1);
	EXPECT_EQ(kMapped.counters[0].domain, QStringLiteral("Upload"));
	EXPECT_EQ(kMapped.counters[0].name, QStringLiteral("uploaded_vertex_bytes"));
	EXPECT_EQ(kMapped.counters[0].value, 128ULL);
	EXPECT_EQ(kMapped.counters[0].unit, QStringLiteral("Bytes"));
	EXPECT_EQ(kMapped.diagnostics.size(), 1);
	EXPECT_EQ(kMapped.diagnostics[0].severity, QStringLiteral("Warning"));
	EXPECT_EQ(kMapped.diagnostics[0].code, QStringLiteral("GoldenCaptureUnsupported"));
	EXPECT_EQ(kMapped.diagnostics[0].subsystem, QStringLiteral("golden"));
	EXPECT_EQ(kMapped.diagnostics[0].resource_name, QStringLiteral("GoldenCapturePass"));
	EXPECT_EQ(kMapped.diagnostics[0].frame_index, 11ULL);
	EXPECT_EQ(kMapped.frame_graph_dump, QStringLiteral("FrameSetupPass -> OpaquePbrPass -> PresentPass"));
}

} // namespace
