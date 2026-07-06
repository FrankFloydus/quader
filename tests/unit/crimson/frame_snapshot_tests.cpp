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
#include "crimson/scene/render_world.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <iostream>
#include <string_view>

namespace {

void expect_true(bool condition, std::string_view message) {
	EXPECT_TRUE(condition) << message;
}

[[nodiscard]] bool near(float left, float right, float tolerance = 0.001F) noexcept {
	return std::fabs(left - right) <= tolerance;
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

TEST(FrameSnapshot, BuilderRejectsInvalidViews) {
	const std::array<crimson::ViewportCamera, 1> kCameras = { make_camera() };
	const std::array<crimson::ViewportFrameView, 1> kViews = {
		crimson::ViewportFrameView{
				.rect = crimson::ViewportFrameRect{ 0, 0, 0, 100 },
				.camera_index = 0,
				.debug_name = "Invalid",
		},
	};
	const crimson::ViewportFrameInput kFrame{
		.target_extent = crimson::ViewportExtent{ 100, 100, 1.0F },
		.views = kViews,
		.cameras = kCameras,
	};

	const crimson::FrameBuilder kBuilder;
	const auto kSnapshot = kBuilder.build_snapshot(kFrame);
	expect_true(!kSnapshot, "builder rejects a zero-width view");
	expect_true(
			!kSnapshot && kSnapshot.error().code == crimson::RendererDiagnosticCode::InvalidFrameSnapshot,
			"invalid view failure reports InvalidFrameSnapshot");
}

TEST(FrameSnapshot, BuilderFreezesViewportFrameIntoImmutableSnapshot) {
	std::array<crimson::ViewportCamera, 1> cameras = { make_camera() };
	cameras[0].near_plane_m = 0.25F;
	cameras[0].far_plane_m = 250.0F;
	std::array<crimson::ViewportFrameView, 1> views = {
		crimson::ViewportFrameView{
				.rect = crimson::ViewportFrameRect{ 4, 8, 320, 200 },
				.camera_index = 0,
				.debug_name = "Perspective",
		},
	};
	std::array<crimson::PickingRequest, 1> picking_requests = {
		crimson::PickingRequest{ .request_id = 77, .view_index = 0, .x_px = 12, .y_px = 24 },
	};
	crimson::ViewportFrameInput frame{
		.target_extent = crimson::ViewportExtent{ 640, 480, 1.0F },
		.views = views,
		.cameras = cameras,
		.picking_requests = picking_requests,
		.animation_enabled = true,
		.elapsed_seconds = 2.0,
	};

	const crimson::FrameBuilder kBuilder;
	auto built = kBuilder.build_snapshot(frame);
	expect_true(built.has_value(), "valid viewport frame builds a snapshot");
	if (!built) {
		return;
	}

	crimson::FrameSnapshot snapshot = std::move(built).value();
	views[0].rect.width = 1;
	cameras[0].eye.x = 99.0F;
	picking_requests[0].x_px = 99;
	frame.elapsed_seconds = 99.0;

	expect_true(snapshot.target_extent().width_px == 640, "snapshot keeps copied target extent");
	expect_true(snapshot.views().size() == 1, "snapshot keeps copied view count");
	expect_true(snapshot.views()[0].rect.width == 320, "snapshot view rect is immutable from source changes");
	expect_true(snapshot.views()[0].camera.eye.x == 0.0F, "snapshot camera is immutable from source changes");
	expect_true(
			near(snapshot.views()[0].camera.near_plane_m, 0.25F) &&
					near(snapshot.views()[0].camera.far_plane_m, 250.0F),
			"snapshot preserves viewport camera clip settings");
	expect_true(snapshot.elapsed_seconds() == 2.0, "snapshot keeps copied elapsed seconds");
	expect_true(snapshot.objects().empty(), "empty viewport frame has no default render object");
	expect_true(snapshot.mesh_uploads().empty(), "empty viewport frame has no mesh uploads");
	expect_true(snapshot.overlays().size() == 1, "viewport snapshot contains one overlay command");
	expect_true(snapshot.grid_overlay_payloads().size() == 1, "viewport snapshot contains one grid overlay payload");
	expect_true(snapshot.line_overlay_payloads().empty(), "empty viewport frame has no line overlay payload");
	expect_true(snapshot.triangle_overlay_payloads().empty(), "empty viewport frame has no triangle overlay payload");
	expect_true(snapshot.point_overlay_payloads().empty(), "empty viewport frame has no point overlay payload");
	expect_true(snapshot.picking_requests().size() == 1, "viewport snapshot copies picking requests");
	expect_true(snapshot.picking_requests()[0].x_px == 12, "picking request is immutable from source changes");
	expect_true(
			snapshot.viewport_settings().exposure_ev100 == 0.0F,
			"viewport snapshot preserves non-physical exposure until fixture lighting is physical");
	expect_true(
			snapshot.viewport_settings().tone_mapper == crimson::ToneMapper::Linear,
			"viewport defaults to linear tone mapping like the reference app's post-processing-disabled view");
	expect_true(
			snapshot.viewport_settings().clear_color_rgba8 == 0x020202ff,
			"viewport packed background color matches the reference app default 2/255 clear color");
	constexpr float kReferenceClearChannel = 2.0F / 255.0F;
	expect_true(
			near(snapshot.environment().clear_color_linear.x, kReferenceClearChannel) &&
					near(snapshot.environment().clear_color_linear.y, kReferenceClearChannel) &&
					near(snapshot.environment().clear_color_linear.z, kReferenceClearChannel),
			"viewport environment background color matches the reference app default 2/255 clear color");
	expect_true(
			snapshot.overlays()[0].primitive == crimson::OverlayPrimitive::Grid && snapshot.overlays()[0].base_shader == crimson::BaseShaderId::OverlayUnlit,
			"viewport grid is emitted as an OverlayUnlit grid command");
	expect_true(
			snapshot.overlays()[0].payload_offset == 0 && snapshot.overlays()[0].payload_count == 1,
			"grid overlay command points at immutable grid payload data");
}

TEST(FrameSnapshot, BuilderCopiesCallerRenderObjectsAndOverlayPayloads) {
	std::array<crimson::ViewportCamera, 1> cameras = { make_camera() };
	std::array<crimson::ViewportFrameView, 1> views = {
		crimson::ViewportFrameView{
				.rect = crimson::ViewportFrameRect{ 0, 0, 320, 200 },
				.camera_index = 0,
				.debug_name = "Perspective",
		},
	};
	crimson::RenderMeshUpload upload;
	upload.handle = crimson::RenderMeshHandle{ 7, 2 };
	upload.revision = crimson::RenderMeshRevision{ 1, 2, 3 };
	upload.position_normal_uv_interleaved = {
		0.0F,
		0.0F,
		0.0F,
		0.0F,
		1.0F,
		0.0F,
		0.0F,
		0.0F,
		1.0F,
		0.0F,
		0.0F,
		0.0F,
		1.0F,
		0.0F,
		1.0F,
		0.0F,
		0.0F,
		1.0F,
		0.0F,
		0.0F,
		1.0F,
		0.0F,
		0.0F,
		1.0F,
	};
	upload.indices = { 0, 1, 2 };
	upload.bounds = quader::math::Aabb{ .min = { 0.0F, 0.0F, 0.0F }, .max = { 1.0F, 1.0F, 0.0F } };
	std::array<crimson::RenderMeshUpload, 1> uploads = { upload };
	std::array<crimson::RenderObject, 1> objects = {
		crimson::RenderObject{
				.object_id = 99,
				.mesh = crimson::RenderMeshHandle{ 7, 2 },
				.base_shader = crimson::BaseShaderId::OpaquePbr,
				.world_bounds = quader::math::Aabb{ .min = { 0.0F, 0.0F, 0.0F }, .max = { 1.0F, 1.0F, 1.0F } },
				.queue = crimson::RenderQueue::Opaque,
		},
	};
	std::array<crimson::LineOverlaySegment, 1> lines = {
		crimson::LineOverlaySegment{ .start = { 0.0F, 0.0F, 0.0F }, .end = { 1.0F, 0.0F, 0.0F } },
	};
	std::array<crimson::TriangleOverlayPrimitive, 1> triangles = {
		crimson::TriangleOverlayPrimitive{
				.a = { 0.0F, 0.0F, 0.0F },
				.b = { 1.0F, 0.0F, 0.0F },
				.c = { 0.0F, 1.0F, 0.0F },
				.semantic_role = crimson::OverlaySemanticRole::SelectedFaceFill,
		},
	};
	std::array<crimson::PointOverlayPrimitive, 1> points = {
		crimson::PointOverlayPrimitive{
				.position = { 0.0F, 0.0F, 0.0F },
				.size_px = 6.0F,
				.semantic_role = crimson::OverlaySemanticRole::SelectedVertex,
		},
	};
	std::array<crimson::OverlayCommand, 1> overlays = {
		crimson::OverlayCommand{
				.view_index = 0,
				.primitive = crimson::OverlayPrimitive::LineList,
				.depth_mode = crimson::OverlayDepthMode::AlwaysOnTop,
				.payload_offset = 0,
				.payload_count = 1,
		},
	};
	crimson::ViewportFrameInput frame{
		.target_extent = crimson::ViewportExtent{ 640, 480, 1.0F },
		.views = views,
		.cameras = cameras,
		.mesh_uploads = uploads,
		.objects = objects,
		.overlays = overlays,
		.line_overlay_payloads = lines,
		.triangle_overlay_payloads = triangles,
		.point_overlay_payloads = points,
	};

	const crimson::FrameBuilder kBuilder;
	auto built = kBuilder.build_snapshot(frame);
	expect_true(built.has_value(), "valid frame with caller render data builds");
	if (!built) {
		return;
	}

	crimson::FrameSnapshot snapshot = std::move(built).value();
	objects[0].object_id = 1;
	uploads[0].indices[0] = 2;
	lines[0].end.x = 5.0F;
	triangles[0].b.x = 5.0F;
	points[0].position.x = 5.0F;

	expect_true(snapshot.mesh_uploads().size() == 1, "snapshot copies mesh upload payloads");
	expect_true(snapshot.mesh_uploads()[0].indices[0] == 0, "mesh upload indices are immutable from source changes");
	expect_true(snapshot.objects().size() == 1 && snapshot.objects()[0].object_id == 99, "snapshot copies caller render objects");
	expect_true(snapshot.line_overlay_payloads().size() == 1, "snapshot copies line overlay payloads");
	expect_true(snapshot.line_overlay_payloads()[0].end.x == 1.0F, "line overlay payload is immutable from source changes");
	expect_true(snapshot.triangle_overlay_payloads().size() == 1, "snapshot copies triangle overlay payloads");
	expect_true(snapshot.triangle_overlay_payloads()[0].b.x == 1.0F, "triangle overlay payload is immutable from source changes");
	expect_true(snapshot.point_overlay_payloads().size() == 1, "snapshot copies point overlay payloads");
	expect_true(snapshot.point_overlay_payloads()[0].position.x == 0.0F, "point overlay payload is immutable from source changes");
}

TEST(FrameSnapshot, BuilderCopiesViewportSettingsAndSuppressesGridOnly) {
	std::array<crimson::ViewportCamera, 1> cameras = { make_camera() };
	std::array<crimson::ViewportFrameView, 1> views = {
		crimson::ViewportFrameView{
				.rect = crimson::ViewportFrameRect{ 0, 0, 320, 200 },
				.camera_index = 0,
				.debug_name = "Perspective",
		},
	};
	std::array<crimson::LineOverlaySegment, 1> lines = {
		crimson::LineOverlaySegment{ .start = { 0.0F, 0.0F, 0.0F }, .end = { 1.0F, 0.0F, 0.0F } },
	};
	std::array<crimson::OverlayCommand, 1> overlays = {
		crimson::OverlayCommand{
				.view_index = 0,
				.primitive = crimson::OverlayPrimitive::LineList,
				.depth_mode = crimson::OverlayDepthMode::AlwaysOnTop,
				.payload_offset = 0,
				.payload_count = 1,
		},
	};
	crimson::ViewportSettings settings;
	settings.draw_grid_overlay = false;
	settings.draw_overlays = false;
	settings.draw_mesh_grid = true;
	crimson::ViewportFrameInput frame{
		.target_extent = crimson::ViewportExtent{ 640, 480, 1.0F },
		.views = views,
		.cameras = cameras,
		.overlays = overlays,
		.line_overlay_payloads = lines,
		.viewport_settings = settings,
	};

	const crimson::FrameBuilder kBuilder;
	auto built = kBuilder.build_snapshot(frame);
	expect_true(built.has_value(), "valid frame with viewport settings builds");
	if (!built) {
		return;
	}

	crimson::FrameSnapshot snapshot = std::move(built).value();
	expect_true(snapshot.grid_overlay_payloads().empty(), "ShowGrid=false suppresses only generated grid payloads");
	expect_true(snapshot.overlays().size() == 1, "caller overlay commands are preserved for renderer overlay toggle policy");
	expect_true(!snapshot.viewport_settings().draw_grid_overlay, "snapshot keeps ShowGrid=false");
	expect_true(!snapshot.viewport_settings().draw_overlays, "snapshot keeps ShowOverlays=false");
	expect_true(snapshot.viewport_settings().draw_mesh_grid, "snapshot keeps ShowMeshGrid=true");
}

TEST(FrameSnapshot, RenderWorldStoresPreparedObjectsById) {
	crimson::RenderWorld world;
	crimson::RenderObject object;
	object.object_id = 7;
	object.visible = true;

	const crimson::RenderObjectId kId = world.add_object(object);
	expect_true(kId == 7, "render world preserves explicit object ids");
	expect_true(world.objects().size() == 1, "render world stores added objects");

	object.visible = false;
	expect_true(world.update_object(kId, object), "render world updates existing objects");
	expect_true(!world.objects()[0].visible, "render world exposes updated prepared object data");

	expect_true(world.remove_object(kId), "render world removes existing objects");
	expect_true(world.objects().empty(), "render world is empty after removal");
}

} // namespace
