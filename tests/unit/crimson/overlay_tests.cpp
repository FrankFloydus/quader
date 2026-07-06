/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
#include "crimson/frame/frame_builder.hpp"
#include "crimson/frame/frame_targets.hpp"
#include "crimson/graph/render_graph.hpp"
#include "crimson/overlays/grid_overlay.hpp"
#include "crimson/overlays/overlay_system.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <span>
#include <string_view>
#include <utility>

namespace {

void expect_true(bool condition, std::string_view message) {
	EXPECT_TRUE(condition) << message;
}

[[nodiscard]] bool near(float left, float right, float tolerance = 0.001F) noexcept {
	return std::fabs(left - right) <= tolerance;
}

void expect_color_near(const crimson::ColorSrgb &actual, const crimson::ColorSrgb &expected, std::string_view message) {
	expect_true(
			near(actual.r, expected.r) &&
					near(actual.g, expected.g) &&
					near(actual.b, expected.b) &&
					near(actual.a, expected.a),
			message);
}

[[nodiscard]] const crimson::RenderPass *find_pass(const crimson::RenderGraph &graph, std::string_view name) {
	for (const crimson::RenderPass &pass : graph.passes()) {
		if (pass.name == name) {
			return &pass;
		}
	}
	return nullptr;
}

[[nodiscard]] bool pass_uses_resource(const crimson::RenderPass &pass, std::string_view resource_name) {
	for (const crimson::ResourceUse &use : pass.resources) {
		if (use.resource_name == resource_name) {
			return true;
		}
	}
	return false;
}

[[nodiscard]] crimson::ViewportCamera make_camera() {
	return crimson::ViewportCamera{
		.eye = { 0.0F, 4.0F, 8.0F },
		.target = { 0.0F, 0.0F, 0.0F },
		.up = { 0.0F, 1.0F, 0.0F },
		.forward = { 0.0F, -0.4F, -1.0F },
		.projection = crimson::ViewportCameraProjection::Perspective,
	};
}

TEST(Overlay, ViewportGridIsOverlayCommandNotLitObject) {
	const std::array<crimson::ViewportCamera, 1> kCameras = { make_camera() };
	const std::array<crimson::ViewportFrameView, 1> kViews = {
		crimson::ViewportFrameView{
				.rect = crimson::ViewportFrameRect{ 0, 0, 320, 240 },
				.camera_index = 0,
				.debug_name = "Perspective",
		},
	};
	const crimson::ViewportFrameInput kFrame{
		.target_extent = crimson::ViewportExtent{ 640, 480, 1.0F },
		.views = kViews,
		.cameras = kCameras,
	};

	const crimson::FrameBuilder kBuilder;
	auto built = kBuilder.build_snapshot(kFrame);
	expect_true(built.has_value(), "valid viewport frame builds for overlay tests");
	if (!built) {
		return;
	}

	const crimson::FrameSnapshot kSnapshot = std::move(built).value();
	expect_true(kSnapshot.objects().empty(), "empty document viewport has no default lit render object");
	expect_true(kSnapshot.overlays().size() == 1, "viewport grid is emitted through overlay commands");
	expect_true(kSnapshot.grid_overlay_payloads().size() == 1, "viewport grid payload is snapshot-owned");
	expect_true(kSnapshot.line_overlay_payloads().empty(), "empty document viewport has no preview line payload");
	expect_true(kSnapshot.triangle_overlay_payloads().empty(), "empty document viewport has no triangle overlay payload");
	expect_true(kSnapshot.point_overlay_payloads().empty(), "empty document viewport has no point overlay payload");
	expect_true(
			kSnapshot.overlays()[0].primitive == crimson::OverlayPrimitive::Grid,
			"viewport overlay command is a grid primitive");
	expect_true(
			kSnapshot.overlays()[0].base_shader == crimson::BaseShaderId::OverlayUnlit,
			"viewport grid uses OverlayUnlit schema");
	for (const crimson::RenderObject &object : kSnapshot.objects()) {
		expect_true(
				object.queue != crimson::RenderQueue::OverlayDepthTested && object.queue != crimson::RenderQueue::OverlayXRay && object.queue != crimson::RenderQueue::OverlayAlwaysOnTop,
				"overlay queues are not represented as lit render objects");
	}
}

TEST(Overlay, OverlaySystemConvertsSrgbToLinearSdr) {
	const std::array<float, 4> kConverted = crimson::to_linear_sdr_array(
			crimson::ColorSrgb{ 0.5F, 0.0F, 0.0F, 0.8F },
			0.5F);
	expect_true(near(kConverted[0], 0.214F), "overlay sRGB color converts to linear SDR");
	expect_true(near(kConverted[1], 0.0F), "overlay green channel remains black");
	expect_true(near(kConverted[3], 0.4F), "overlay opacity multiplies alpha");
}

TEST(Overlay, GridDepthModeFollowsProjectionAndUsesReferenceOffset) {
	crimson::RenderView perspective_view;
	perspective_view.view_index = 3;
	perspective_view.rect = crimson::RenderViewportRect{ 0, 0, 640, 480 };
	perspective_view.camera = crimson::RenderCamera{
		.eye = { 4.0F, 4.0F, 4.0F },
		.target = { 0.0F, 0.0F, 0.0F },
		.up = { 0.0F, 1.0F, 0.0F },
		.forward = { -1.0F, -1.0F, -1.0F },
		.projection = crimson::CameraProjection::Perspective,
	};

	const crimson::GridOverlayCommand kPerspectiveGrid = crimson::make_grid_overlay_for_view(perspective_view);
	const crimson::OverlayCommand kPerspectiveCommand = crimson::make_grid_overlay_command(kPerspectiveGrid, 0);
	expect_true(near(kPerspectiveGrid.origin.y, -0.002F), "perspective grid keeps the reference app plane offset");
	expect_true(
			kPerspectiveCommand.depth_mode == crimson::OverlayDepthMode::AlwaysOnTop,
			"perspective grid disables scene-depth testing to avoid z-fighting");

	crimson::RenderView orthographic_view = perspective_view;
	orthographic_view.view_index = 4;
	orthographic_view.camera.projection = crimson::CameraProjection::Orthographic;
	const crimson::GridOverlayCommand kOrthographicGrid = crimson::make_grid_overlay_for_view(orthographic_view);
	const crimson::OverlayCommand kOrthographicCommand = crimson::make_grid_overlay_command(kOrthographicGrid, 0);
	expect_true(
			kOrthographicCommand.depth_mode == crimson::OverlayDepthMode::DepthTested,
			"orthographic grid remains depth-tested");
}

TEST(Overlay, GridDefaultsMatchReferenceViewportSettings) {
	crimson::RenderView view;
	view.rect = crimson::RenderViewportRect{ 0, 0, 640, 480 };
	view.camera = crimson::RenderCamera{
		.eye = { 4.0F, 4.0F, 4.0F },
		.target = { 0.0F, 0.0F, 0.0F },
		.up = { 0.0F, 1.0F, 0.0F },
		.forward = { -1.0F, -1.0F, -1.0F },
		.projection = crimson::CameraProjection::Perspective,
		.orthographic_height_m = 12.0F,
	};

	const crimson::GridOverlayCommand kDefaultGrid = crimson::make_grid_overlay_for_view(view);
	expect_color_near(
			kDefaultGrid.minor_color,
			crimson::ColorSrgb{ 150.0F / 255.0F, 150.0F / 255.0F, 150.0F / 255.0F, 1.0F },
			"minor grid color matches the reference viewport default");
	expect_color_near(
			kDefaultGrid.major_color,
			crimson::ColorSrgb{ 210.0F / 255.0F, 210.0F / 255.0F, 210.0F / 255.0F, 1.0F },
			"major grid color matches the reference viewport default");
	expect_color_near(
			kDefaultGrid.axis_u_color,
			crimson::ColorSrgb{ 1.0F, 0.239F, 0.0F, 0.72F },
			"X axis grid color matches the reference viewport default");
	expect_color_near(
			kDefaultGrid.axis_v_color,
			crimson::ColorSrgb{ 0.059F, 0.612F, 1.0F, 0.72F },
			"Z axis grid color matches the reference viewport default");
	expect_true(near(kDefaultGrid.minor_line_scale, 0.325F), "minor grid thickness matches the reference viewport default");
	expect_true(near(kDefaultGrid.major_line_scale, 0.250F), "major grid thickness matches the reference viewport default");
	expect_true(near(kDefaultGrid.axis_line_scale, 1.0F), "axis grid thickness matches the reference viewport default");

	view.camera.projection = crimson::CameraProjection::Orthographic;
	view.camera.eye = { 16.0F, 0.0F, 0.0F };
	view.camera.target = { 0.0F, 0.0F, 0.0F };
	view.camera.forward = { -1.0F, 0.0F, 0.0F };
	const crimson::GridOverlayCommand kYzGrid = crimson::make_grid_overlay_for_view(view);
	expect_color_near(
			kYzGrid.axis_u_color,
			crimson::ColorSrgb{ 0.059F, 0.612F, 1.0F, 0.72F },
			"YZ grid U axis uses the reference Z axis color");
	expect_color_near(
			kYzGrid.axis_v_color,
			crimson::ColorSrgb{ 0.29F, 0.58F, 0.0F, 0.0F },
			"YZ grid V axis uses the reference hidden Y axis color");

	view.camera.eye = { 0.0F, 0.0F, 16.0F };
	view.camera.target = { 0.0F, 0.0F, 0.0F };
	view.camera.forward = { 0.0F, 0.0F, -1.0F };
	const crimson::GridOverlayCommand kXyGrid = crimson::make_grid_overlay_for_view(view);
	expect_color_near(
			kXyGrid.axis_u_color,
			crimson::ColorSrgb{ 1.0F, 0.239F, 0.0F, 0.72F },
			"XY grid U axis uses the reference X axis color");
	expect_color_near(
			kXyGrid.axis_v_color,
			crimson::ColorSrgb{ 0.29F, 0.58F, 0.0F, 0.0F },
			"XY grid V axis uses the reference hidden Y axis color");
}

TEST(Overlay, OverlaySystemBucketsGridCommandsByDepthMode) {
	crimson::GridOverlayCommand grid;
	grid.view_index = 7;
	grid.minor_color = crimson::ColorSrgb{ 0.5F, 0.0F, 0.0F, 1.0F };

	std::array<crimson::GridOverlayCommand, 3> grids = { grid, grid, grid };
	std::array<crimson::OverlayCommand, 3> commands = {
		crimson::OverlayCommand{
				.view_index = 7,
				.primitive = crimson::OverlayPrimitive::Grid,
				.depth_mode = crimson::OverlayDepthMode::DepthTested,
				.payload_offset = 0,
				.payload_count = 1,
		},
		crimson::OverlayCommand{
				.view_index = 7,
				.primitive = crimson::OverlayPrimitive::Grid,
				.depth_mode = crimson::OverlayDepthMode::XRay,
				.opacity = 0.5F,
				.payload_offset = 1,
				.payload_count = 1,
		},
		crimson::OverlayCommand{
				.view_index = 7,
				.primitive = crimson::OverlayPrimitive::Grid,
				.depth_mode = crimson::OverlayDepthMode::AlwaysOnTop,
				.payload_offset = 2,
				.payload_count = 1,
		},
	};

	const crimson::OverlayDrawLists kLists = crimson::OverlaySystem{}.prepare(commands, grids);
	expect_true(kLists.depth_tested.grid_commands.size() == 1, "depth-tested grid command is bucketed");
	expect_true(kLists.xray.grid_commands.size() == 1, "XRay grid command is bucketed");
	expect_true(kLists.always_on_top.grid_commands.size() == 1, "always-on-top grid command is bucketed");
	expect_true(kLists.command_count() == 6, "overlay draw lists retain command and grid payload records");
	expect_true(
			near(kLists.xray.grid_commands[0].minor_color_linear_sdr[3], 0.5F),
			"prepared grid payload alpha honors overlay command opacity");
}

TEST(Overlay, OverlaySystemBucketsLineCommandsByDepthMode) {
	std::array<crimson::LineOverlaySegment, 2> lines = {
		crimson::LineOverlaySegment{ .start = { 0.0F, 0.0F, 0.0F }, .end = { 1.0F, 0.0F, 0.0F } },
		crimson::LineOverlaySegment{ .start = { 1.0F, 0.0F, 0.0F }, .end = { 1.0F, 1.0F, 0.0F } },
	};
	std::array<crimson::OverlayCommand, 1> commands = {
		crimson::OverlayCommand{
				.view_index = 0,
				.primitive = crimson::OverlayPrimitive::LineList,
				.depth_mode = crimson::OverlayDepthMode::AlwaysOnTop,
				.color_srgb = crimson::ColorSrgb{ 0.0F, 1.0F, 0.0F, 0.5F },
				.opacity = 0.5F,
				.payload_offset = 0,
				.payload_count = 2,
		},
	};

	const crimson::OverlayDrawLists kLists = crimson::OverlaySystem{}.prepare(commands, {}, lines);
	expect_true(kLists.always_on_top.line_commands.size() == 1, "line command is bucketed by overlay depth mode");
	expect_true(kLists.always_on_top.line_commands[0].segments.size() == 2, "line payload range is copied");
	expect_true(
			near(kLists.always_on_top.line_commands[0].color_linear_sdr[3], 0.25F),
			"line overlay command alpha honors command opacity");
	expect_true(kLists.command_count() == 2, "line draw lists retain command and line payload records");
}

TEST(Overlay, XrayLineBatchesDrawOnTopWithoutDepthTest) {
	std::array<crimson::LineOverlaySegment, 1> lines = {
		crimson::LineOverlaySegment{ .start = { 0.0F, 0.0F, 0.0F }, .end = { 1.0F, 0.0F, 0.0F } },
	};
	std::array<crimson::OverlayCommand, 1> commands = {
		crimson::OverlayCommand{
				.view_index = 0,
				.primitive = crimson::OverlayPrimitive::LineList,
				.depth_mode = crimson::OverlayDepthMode::XRay,
				.payload_offset = 0,
				.payload_count = 1,
		},
	};

	const crimson::OverlayDrawLists kLists = crimson::OverlaySystem{}.prepare(commands, {}, lines);
	ASSERT_EQ(kLists.xray.line_commands.size(), 1U);
	EXPECT_FALSE(kLists.xray.line_commands[0].render_state.depth_test_enabled);
	EXPECT_FALSE(kLists.xray.line_commands[0].render_state.depth_write_enabled);
}

TEST(Overlay, SourceWireDepthStampIsMetadataOnly) {
	std::array<crimson::TriangleOverlayPrimitive, 1> triangles = {
		crimson::TriangleOverlayPrimitive{
				.a = { 0.0F, 0.0F, 0.0F },
				.b = { 1.0F, 0.0F, 0.0F },
				.c = { 0.0F, 1.0F, 0.0F },
				.semantic_role = crimson::OverlaySemanticRole::SourceWireDepthStamp,
				.source_kind = crimson::OverlaySourceKind::SourceWire,
				.element = crimson::OverlayElementRef{ .object_id = 42, .face_index = 7 },
		},
	};
	std::array<crimson::OverlayCommand, 1> commands = {
		crimson::OverlayCommand{
				.view_index = 3,
				.primitive = crimson::OverlayPrimitive::SolidTriangles,
				.depth_mode = crimson::OverlayDepthMode::AlwaysOnTop,
				.payload_offset = 0,
				.payload_count = 1,
		},
	};

	const crimson::OverlayDrawLists kLists = crimson::OverlaySystem{}.prepare(
			commands,
			std::span<const crimson::GridOverlayCommand>{},
			std::span<const crimson::LineOverlaySegment>{},
			triangles);
	expect_true(kLists.command_count() == 0, "source-wire depth stamps do not become render batches");
	expect_true(kLists.source_wire_depth_stamp_count() == 1, "source-wire depth stamp metadata is retained");
	expect_true(kLists.source_wire_depth_stamps[0].view_index == 3, "depth-stamp metadata keeps the view index");
	expect_true(kLists.source_wire_depth_stamps[0].element.object_id == 42, "depth-stamp metadata keeps topology reference");
	expect_true(kLists.source_wire_depth_stamps[0].triangle.element.face_index == 7, "depth-stamp metadata keeps triangle topology");
	expect_true(kLists.source_wire_depth_stamps[0].triangle.b.x == 1.0F, "depth-stamp metadata keeps triangle geometry");
}

TEST(Overlay, SourceWireStaysAlwaysOnTopDespiteCommandDepth) {
	std::array<crimson::LineOverlaySegment, 1> lines = {
		crimson::LineOverlaySegment{
				.start = { 0.0F, 0.0F, 0.0F },
				.end = { 1.0F, 0.0F, 0.0F },
				.semantic_role = crimson::OverlaySemanticRole::SourceWire,
				.source_kind = crimson::OverlaySourceKind::SourceWire,
		},
	};
	std::array<crimson::OverlayCommand, 1> commands = {
		crimson::OverlayCommand{
				.view_index = 0,
				.primitive = crimson::OverlayPrimitive::LineList,
				.depth_mode = crimson::OverlayDepthMode::DepthTested,
				.payload_offset = 0,
				.payload_count = 1,
		},
	};

	const crimson::OverlayDrawLists kLists = crimson::OverlaySystem{}.prepare(commands, {}, lines);
	expect_true(kLists.depth_tested.line_commands.empty(), "source-wire payload is removed from depth-tested line batches");
	expect_true(kLists.always_on_top.line_commands.size() == 1, "source wire is always-on-top");
	expect_true(
			kLists.always_on_top.line_commands[0].render_state.depth_mode == crimson::OverlayDepthMode::AlwaysOnTop,
			"source wire prepared state is always-on-top");
	expect_true(
			!kLists.always_on_top.line_commands[0].render_state.depth_test_enabled,
			"source wire disables depth testing");
}

TEST(Overlay, ComponentLineHandlesPreserveLayerOrderAndDepthState) {
	std::array<crimson::LineOverlaySegment, 2> lines = {
		crimson::LineOverlaySegment{
				.start = { 0.0F, 0.0F, 0.0F },
				.end = { 1.0F, 0.0F, 0.0F },
				.semantic_role = crimson::OverlaySemanticRole::SourceWire,
				.source_kind = crimson::OverlaySourceKind::SourceWire,
		},
		crimson::LineOverlaySegment{
				.start = { 0.0F, 1.0F, 0.0F },
				.end = { 1.0F, 1.0F, 0.0F },
				.semantic_role = crimson::OverlaySemanticRole::SelectedEdge,
				.source_kind = crimson::OverlaySourceKind::ComponentSelection,
				.element = crimson::OverlayElementRef{ .object_id = 7, .edge_index = 12 },
		},
	};
	std::array<crimson::OverlayCommand, 1> commands = {
		crimson::OverlayCommand{
				.view_index = 0,
				.primitive = crimson::OverlayPrimitive::LineList,
				.depth_mode = crimson::OverlayDepthMode::AlwaysOnTop,
				.payload_offset = 0,
				.payload_count = 2,
		},
	};

	const crimson::OverlayDrawLists kLists = crimson::OverlaySystem{}.prepare(commands, {}, lines);
	ASSERT_EQ(kLists.always_on_top.line_commands.size(), 2U);
	expect_true(kLists.depth_tested.line_commands.empty(), "component selected edge stays in layer-order bucket");
	expect_true(
			kLists.always_on_top.line_commands[0].semantic_role == crimson::OverlaySemanticRole::SourceWire &&
					!kLists.always_on_top.line_commands[0].render_state.depth_test_enabled,
			"source wire draws first without depth testing");
	expect_true(
			kLists.always_on_top.line_commands[1].source_kind == crimson::OverlaySourceKind::ComponentSelection,
			"component edge batch keeps component selection source");
	expect_true(
			kLists.always_on_top.line_commands[1].render_state.depth_test_enabled,
			"component edge keeps depth-tested render state");
	expect_true(
			kLists.always_on_top.line_commands[1].segments[0].element.edge_index == 12,
			"component edge batch keeps semantic edge reference");
}

TEST(Overlay, FaceFillCreatesTwoSidedDepthStampAndEqualDepthColorPasses) {
	std::array<crimson::TriangleOverlayPrimitive, 1> triangles = {
		crimson::TriangleOverlayPrimitive{
				.a = { 0.0F, 0.0F, 0.0F },
				.b = { 1.0F, 0.0F, 0.0F },
				.c = { 0.0F, 1.0F, 0.0F },
				.semantic_role = crimson::OverlaySemanticRole::SelectedFaceFill,
				.source_kind = crimson::OverlaySourceKind::ComponentSelection,
				.element = crimson::OverlayElementRef{ .object_id = 9, .face_index = 4 },
		},
	};
	std::array<crimson::OverlayCommand, 1> commands = {
		crimson::OverlayCommand{
				.view_index = 0,
				.primitive = crimson::OverlayPrimitive::SolidTriangles,
				.depth_mode = crimson::OverlayDepthMode::XRay,
				.payload_offset = 0,
				.payload_count = 1,
		},
	};

	const crimson::OverlayDrawLists kLists = crimson::OverlaySystem{}.prepare(
			commands,
			std::span<const crimson::GridOverlayCommand>{},
			std::span<const crimson::LineOverlaySegment>{},
			triangles);
	ASSERT_EQ(kLists.depth_tested.triangle_commands.size(), 2U);
	const crimson::PreparedTriangleOverlayCommand &depth_stamp = kLists.depth_tested.triangle_commands[0];
	const crimson::PreparedTriangleOverlayCommand &color = kLists.depth_tested.triangle_commands[1];
	expect_true(
			depth_stamp.render_state.pass_kind == crimson::PreparedOverlayPassKind::DepthStamp,
			"face fill emits a depth-stamp pass first");
	expect_true(
			!depth_stamp.render_state.color_write_enabled && depth_stamp.render_state.depth_write_enabled,
			"depth-stamp pass writes only depth");
	expect_true(
			color.render_state.pass_kind == crimson::PreparedOverlayPassKind::EqualDepthColor,
			"face fill emits an equal-depth color pass second");
	expect_true(
			color.render_state.equal_depth_test_enabled && !color.render_state.depth_write_enabled,
			"equal-depth color pass reads stamped depth without rewriting it");
	expect_true(depth_stamp.render_state.two_sided && color.render_state.two_sided, "face fill is two-sided");
	expect_true(kLists.source_wire_depth_stamp_count() == 0, "face fill depth stamp is render state, not source-wire metadata");
}

TEST(Overlay, VertexPointHandlesDepthTestWhenSourcedFromComponents) {
	std::array<crimson::PointOverlayPrimitive, 1> points = {
		crimson::PointOverlayPrimitive{
				.position = { 0.0F, 0.0F, 0.0F },
				.size_px = 7.0F,
				.semantic_role = crimson::OverlaySemanticRole::SelectedVertex,
				.source_kind = crimson::OverlaySourceKind::ComponentSelection,
				.element = crimson::OverlayElementRef{ .object_id = 9, .vertex_index = 5 },
		},
	};
	std::array<crimson::OverlayCommand, 1> commands = {
		crimson::OverlayCommand{
				.view_index = 0,
				.primitive = crimson::OverlayPrimitive::PointList,
				.depth_mode = crimson::OverlayDepthMode::AlwaysOnTop,
				.payload_offset = 0,
				.payload_count = 1,
		},
	};

	const crimson::OverlayDrawLists kLists = crimson::OverlaySystem{}.prepare(
			commands,
			std::span<const crimson::GridOverlayCommand>{},
			std::span<const crimson::LineOverlaySegment>{},
			std::span<const crimson::TriangleOverlayPrimitive>{},
			points);
	expect_true(kLists.depth_tested.point_commands.empty(), "component vertex stays in layer-order point bucket");
	ASSERT_EQ(kLists.always_on_top.point_commands.size(), 1U);
	expect_true(kLists.always_on_top.point_commands[0].render_state.depth_test_enabled, "component vertex keeps depth-tested render state");
	expect_true(kLists.always_on_top.point_commands[0].size_px == 7.0F, "point handle size is preserved");
	expect_true(
			kLists.always_on_top.point_commands[0].points[0].element.vertex_index == 5,
			"point handle keeps semantic vertex reference");
}

TEST(Overlay, OverlayPassesStayAfterToneMapAndOutOfHdrSceneColor) {
	const crimson::RenderGraph kGraph = crimson::make_v1_correctness_render_graph(
			crimson::ViewportExtent{ 800, 600, 1.0F });
	const crimson::RenderPass *tone_map = find_pass(kGraph, "ToneMapPass");
	const crimson::RenderPass *depth_tested = find_pass(kGraph, "OverlayDepthTestedPass");
	const crimson::RenderPass *xray = find_pass(kGraph, "OverlayXRayPass");
	const crimson::RenderPass *always_on_top = find_pass(kGraph, "OverlayAlwaysOnTopPass");
	expect_true(tone_map != nullptr, "tone-map pass exists before overlays");
	expect_true(depth_tested != nullptr && xray != nullptr && always_on_top != nullptr, "all overlay passes exist");

	bool seen_tone_map = false;
	for (const crimson::RenderPass &pass : kGraph.passes()) {
		if (pass.name == "ToneMapPass") {
			seen_tone_map = true;
		}
		if (pass.name == "OverlayDepthTestedPass" || pass.name == "OverlayXRayPass" || pass.name == "OverlayAlwaysOnTopPass") {
			expect_true(seen_tone_map, "overlay pass is ordered after ToneMapPass");
			expect_true(
					!pass_uses_resource(pass, crimson::kHdrSceneColorTargetName),
					"overlay pass never touches HdrSceneColor");
			expect_true(
					pass_uses_resource(pass, crimson::kToneMappedColorTargetName),
					"overlay pass composites into ToneMappedColor");
		}
	}
}

} // namespace
