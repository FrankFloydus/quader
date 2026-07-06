/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
#include "crimson/gpu/gpu_frame_resources.hpp"
#include "crimson/gpu/gpu_overlay_renderer.hpp"
#include "crimson/gpu/gpu_resources.hpp"
#include "crimson/gpu/gpu_view_policy.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <cmath>
#include <optional>
#include <string_view>
#include <utility>

namespace {

void expect_true(bool condition, std::string_view message) {
	EXPECT_TRUE(condition) << message;
}

[[nodiscard]] bool near(float left, float right, float tolerance = 0.001F) noexcept {
	return std::fabs(left - right) <= tolerance;
}

[[nodiscard]] float perspective_device_z(float near_plane, float far_plane, float depth) noexcept {
	const float kDiff = far_plane - near_plane;
	return far_plane / kDiff - (far_plane * near_plane) / (kDiff * depth);
}

[[nodiscard]] crimson::RenderView make_overlay_geometry_test_view(
		crimson::CameraProjection projection) {
	crimson::RenderView view;
	view.rect = crimson::RenderViewportRect{ 0, 0, 100, 100 };
	view.camera = crimson::RenderCamera{
		.eye = { 0.0F, 0.0F, 0.0F },
		.target = { 0.0F, 0.0F, 1.0F },
		.up = { 0.0F, 1.0F, 0.0F },
		.forward = { 0.0F, 0.0F, 1.0F },
		.projection = projection,
		.near_plane_m = 0.1F,
		.far_plane_m = 100.0F,
		.vertical_fov_degrees = 90.0F,
		.orthographic_height_m = 100.0F,
	};
	return view;
}

[[nodiscard]] quader::math::Vec3 quad_center(
		const crimson::gpu::OverlayScreenSpaceQuad &quad) noexcept {
	quader::math::Vec3 center;
	for (const quader::math::Vec3 corner : quad.corners) {
		center = center + corner;
	}
	return center / 4.0F;
}

struct FakeHandle {
	int id = 0;

	friend bool operator==(const FakeHandle &, const FakeHandle &) = default;
};

struct FakeHandleTraits {
	static int destroy_count;
	static int last_destroyed;

	static FakeHandle invalid() noexcept {
		return FakeHandle{};
	}

	static bool is_valid(FakeHandle handle) noexcept {
		return handle.id != 0;
	}

	static void destroy(FakeHandle handle) noexcept {
		++destroy_count;
		last_destroyed = handle.id;
	}

	static void reset_counts() noexcept {
		destroy_count = 0;
		last_destroyed = 0;
	}
};

int FakeHandleTraits::destroy_count = 0;
int FakeHandleTraits::last_destroyed = 0;

using UniqueFakeHandle = crimson::gpu::UniqueGpuHandle<FakeHandle, FakeHandleTraits>;

struct MoveOnlyResource {
	UniqueFakeHandle handle;

	MoveOnlyResource() noexcept = default;

	explicit MoveOnlyResource(int id) noexcept
			: handle(FakeHandle{ id }) {
	}

	MoveOnlyResource(const MoveOnlyResource &) = delete;
	MoveOnlyResource &operator=(const MoveOnlyResource &) = delete;
	MoveOnlyResource(MoveOnlyResource &&) noexcept = default;
	MoveOnlyResource &operator=(MoveOnlyResource &&) noexcept = default;
};

TEST(GpuResource, UniqueGpuHandleDestroysOwnedHandleOnce) {
	FakeHandleTraits::reset_counts();
	{
		UniqueFakeHandle handle(FakeHandle{ 7 });
		expect_true(handle.valid(), "unique GPU handle reports valid owned handle");
	}

	expect_true(FakeHandleTraits::destroy_count == 1, "owned handle is destroyed on scope exit");
	expect_true(FakeHandleTraits::last_destroyed == 7, "destroy callback receives the owned handle");
}

TEST(GpuResource, GridOverlayUsesReferenceTransparentBlendState) {
	const std::uint64_t kAlwaysOnTopState = crimson::gpu::grid_overlay_submit_state(
			crimson::OverlayDepthMode::AlwaysOnTop);
	const std::uint64_t kDepthTestedState = crimson::gpu::grid_overlay_submit_state(
			crimson::OverlayDepthMode::DepthTested);
	constexpr std::uint64_t kReferencePremultipliedBlend =
			BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_ONE, BGFX_STATE_BLEND_INV_SRC_ALPHA);

	expect_true(
			(kAlwaysOnTopState & BGFX_STATE_BLEND_MASK) == (kReferencePremultipliedBlend & BGFX_STATE_BLEND_MASK),
			"grid uses the Windows reference premultiplied overlay blend state");
	expect_true(
			(kAlwaysOnTopState & BGFX_STATE_DEPTH_TEST_LESS) == 0,
			"always-on-top grid does not depth-test");
	expect_true(
			(kDepthTestedState & BGFX_STATE_DEPTH_TEST_MASK) == BGFX_STATE_DEPTH_TEST_LEQUAL,
			"depth-tested grid uses bgfx normal-depth LEQUAL");
	const std::uint64_t kXrayState = crimson::gpu::grid_overlay_submit_state(
			crimson::OverlayDepthMode::XRay);
	expect_true(
			(kXrayState & BGFX_STATE_DEPTH_TEST_MASK) == 0,
			"XRay grid draws on top without depth testing");
}

TEST(GpuResource, LineQuadUsesDeviceSpaceWidthCapsAndFeatherDistance) {
	const crimson::RenderView kView = make_overlay_geometry_test_view(crimson::CameraProjection::Orthographic);
	const crimson::LineOverlaySegment kSegment{
		.start = { -10.0F, 0.0F, 10.0F },
		.end = { 10.0F, 0.0F, 10.0F },
	};

	const std::optional<crimson::gpu::OverlayLineDeviceQuad> kQuad =
			crimson::gpu::make_overlay_line_device_quad(
					kView,
					kSegment,
					2.0F,
					0.0F,
					false);
	ASSERT_TRUE(kQuad.has_value());

	float min_x = kQuad->vertices.front().position.x;
	float max_x = kQuad->vertices.front().position.x;
	float min_y = kQuad->vertices.front().position.y;
	float max_y = kQuad->vertices.front().position.y;
	for (const crimson::gpu::OverlayLineDeviceVertex &vertex : kQuad->vertices) {
		min_x = std::min(min_x, vertex.position.x);
		max_x = std::max(max_x, vertex.position.x);
		min_y = std::min(min_y, vertex.position.y);
		max_y = std::max(max_y, vertex.position.y);
		EXPECT_TRUE(near(vertex.position.z, 0.099099F));
	}

	EXPECT_TRUE(near(min_x, -0.23F));
	EXPECT_TRUE(near(max_x, 0.23F));
	EXPECT_TRUE(near(min_y, -0.03F));
	EXPECT_TRUE(near(max_y, 0.03F));
	EXPECT_TRUE(near(kQuad->vertices[0].line_distance_pixels, -1.5F));
	EXPECT_TRUE(near(kQuad->vertices[1].line_distance_pixels, -1.5F));
	EXPECT_TRUE(near(kQuad->vertices[2].line_distance_pixels, 1.5F));
	EXPECT_TRUE(near(kQuad->vertices[3].line_distance_pixels, 1.5F));
}

TEST(GpuResource, NearPlaneClippedLineQuadStaysInDeviceSpace) {
	const crimson::RenderView kView = make_overlay_geometry_test_view(crimson::CameraProjection::Perspective);
	const crimson::LineOverlaySegment kSegment{
		.start = { -0.25F, 0.0F, 0.01F },
		.end = { 0.25F, 0.0F, 10.0F },
	};

	const std::optional<crimson::gpu::OverlayLineDeviceQuad> kQuad =
			crimson::gpu::make_overlay_line_device_quad(
					kView,
					kSegment,
					3.0F,
					0.0F,
					false);
	ASSERT_TRUE(kQuad.has_value());

	for (const crimson::gpu::OverlayLineDeviceVertex &vertex : kQuad->vertices) {
		EXPECT_TRUE(std::isfinite(vertex.position.x));
		EXPECT_TRUE(std::isfinite(vertex.position.y));
		EXPECT_TRUE(std::isfinite(vertex.position.z));
		EXPECT_GE(vertex.position.z, 0.0F);
		EXPECT_LE(vertex.position.z, 1.0F);
	}
	EXPECT_LT(kQuad->vertices[0].line_distance_pixels, 0.0F);
	EXPECT_GT(kQuad->vertices[2].line_distance_pixels, 0.0F);
}

TEST(GpuResource, PointQuadUsesDeviceSpaceCornersAndBoundedDepthBias) {
	const crimson::RenderView kView = make_overlay_geometry_test_view(crimson::CameraProjection::Perspective);
	const std::optional<crimson::gpu::OverlayScreenSpaceQuad> kUnbiased =
			crimson::gpu::make_overlay_point_screen_space_quad(
					kView,
					{ 0.0F, 0.0F, 10.0F },
					8.0F,
					0.0F,
					false);
	const std::optional<crimson::gpu::OverlayScreenSpaceQuad> kBiased =
			crimson::gpu::make_overlay_point_screen_space_quad(
					kView,
					{ 0.0F, 0.0F, 10.0F },
					8.0F,
					1.5F,
					false);
	ASSERT_TRUE(kUnbiased.has_value());
	ASSERT_TRUE(kBiased.has_value());

	const quader::math::Vec3 kUnbiasedCenter = quad_center(*kUnbiased);
	const quader::math::Vec3 kBiasedCenter = quad_center(*kBiased);
	EXPECT_TRUE(near(kUnbiasedCenter.z, perspective_device_z(0.1F, 100.0F, 10.0F)));
	EXPECT_TRUE(near(kUnbiased->corners[0].x, -0.08F));
	EXPECT_TRUE(near(kUnbiased->corners[1].x, 0.08F));
	EXPECT_TRUE(near(kUnbiased->corners[0].y, 0.08F));
	EXPECT_TRUE(near(kUnbiased->corners[2].y, -0.08F));
	EXPECT_LT(kBiasedCenter.z, kUnbiasedCenter.z);
	EXPECT_LT(kUnbiasedCenter.z - kBiasedCenter.z, 0.00001F);
	EXPECT_GT(kBiasedCenter.z, 0.0F);
}

TEST(GpuResource, PointDepthBiasDoesNotPullFarHiddenVertexThroughFrontDepth) {
	const crimson::RenderView kView = make_overlay_geometry_test_view(crimson::CameraProjection::Perspective);
	const std::optional<crimson::gpu::OverlayScreenSpaceQuad> kFrontDepth =
			crimson::gpu::make_overlay_point_screen_space_quad(
					kView,
					{ 0.0F, 0.0F, 49.9F },
					8.0F,
					0.0F,
					false);
	const std::optional<crimson::gpu::OverlayScreenSpaceQuad> kHiddenBiased =
			crimson::gpu::make_overlay_point_screen_space_quad(
					kView,
					{ 0.0F, 0.0F, 50.0F },
					8.0F,
					1.5F,
					false);
	ASSERT_TRUE(kFrontDepth.has_value());
	ASSERT_TRUE(kHiddenBiased.has_value());

	EXPECT_GT(quad_center(*kHiddenBiased).z, quad_center(*kFrontDepth).z);
}

TEST(GpuResource, ComponentLineDepthBiasMovesTowardCamera) {
	const crimson::RenderView kView = make_overlay_geometry_test_view(crimson::CameraProjection::Perspective);
	const crimson::LineOverlaySegment kSegment{
		.start = { -0.5F, 0.0F, 10.0F },
		.end = { 0.5F, 0.0F, 10.0F },
	};

	const std::optional<crimson::gpu::OverlayLineDeviceQuad> kUnbiased =
			crimson::gpu::make_overlay_line_device_quad(
					kView,
					kSegment,
					2.5F,
					0.0F,
					false);
	const std::optional<crimson::gpu::OverlayLineDeviceQuad> kBiased =
			crimson::gpu::make_overlay_line_device_quad(
					kView,
					kSegment,
					2.5F,
					1.0F,
					false);
	ASSERT_TRUE(kUnbiased.has_value());
	ASSERT_TRUE(kBiased.has_value());

	EXPECT_LT(kBiased->vertices[0].position.z, kUnbiased->vertices[0].position.z);
}

TEST(GpuResource, ComponentEditWireDepthBiasIsStrongerThanGenericNearCamera) {
	const crimson::RenderView kView = make_overlay_geometry_test_view(crimson::CameraProjection::Perspective);
	const crimson::LineOverlaySegment kSegment{
		.start = { -0.5F, 0.0F, 2.0F },
		.end = { 0.5F, 0.0F, 2.0F },
		.semantic_role = crimson::OverlaySemanticRole::SourceWire,
		.source_kind = crimson::OverlaySourceKind::ComponentSelection,
	};

	const std::optional<crimson::gpu::OverlayLineDeviceQuad> kUnbiased =
			crimson::gpu::make_overlay_line_device_quad(
					kView,
					kSegment,
					2.5F,
					0.0F,
					false);
	const std::optional<crimson::gpu::OverlayLineDeviceQuad> kGenericBiased =
			crimson::gpu::make_overlay_line_device_quad(
					kView,
					kSegment,
					2.5F,
					1.0F,
					false);
	const std::optional<crimson::gpu::OverlayLineDeviceQuad> kEditWireBiased =
			crimson::gpu::make_overlay_line_device_quad(
					kView,
					kSegment,
					2.5F,
					0.0F,
					false,
					true);
	ASSERT_TRUE(kUnbiased.has_value());
	ASSERT_TRUE(kGenericBiased.has_value());
	ASSERT_TRUE(kEditWireBiased.has_value());

	EXPECT_LT(kGenericBiased->vertices[0].position.z, kUnbiased->vertices[0].position.z);
	EXPECT_LT(kEditWireBiased->vertices[0].position.z, kGenericBiased->vertices[0].position.z);
	EXPECT_TRUE(near(kUnbiased->vertices[0].position.z - kEditWireBiased->vertices[0].position.z, 0.0008F));
	EXPECT_GE(kEditWireBiased->vertices[0].position.z, 0.0F);
}

TEST(GpuResource, ComponentEditWireDepthBiasOffsetsRoomDepthLines) {
	const crimson::RenderView kView = make_overlay_geometry_test_view(crimson::CameraProjection::Perspective);
	const crimson::LineOverlaySegment kSegment{
		.start = { -0.5F, 0.0F, 10.0F },
		.end = { 0.5F, 0.0F, 10.0F },
		.semantic_role = crimson::OverlaySemanticRole::SourceWire,
		.source_kind = crimson::OverlaySourceKind::ComponentSelection,
	};

	const std::optional<crimson::gpu::OverlayLineDeviceQuad> kUnbiased =
			crimson::gpu::make_overlay_line_device_quad(
					kView,
					kSegment,
					2.5F,
					0.0F,
					false);
	const std::optional<crimson::gpu::OverlayLineDeviceQuad> kGenericBiased =
			crimson::gpu::make_overlay_line_device_quad(
					kView,
					kSegment,
					2.5F,
					1.0F,
					false);
	const std::optional<crimson::gpu::OverlayLineDeviceQuad> kEditWireBiased =
			crimson::gpu::make_overlay_line_device_quad(
					kView,
					kSegment,
					2.5F,
					0.0F,
					false,
					true);
	ASSERT_TRUE(kUnbiased.has_value());
	ASSERT_TRUE(kGenericBiased.has_value());
	ASSERT_TRUE(kEditWireBiased.has_value());

	EXPECT_LT(kGenericBiased->vertices[0].position.z, kUnbiased->vertices[0].position.z);
	EXPECT_LT(kEditWireBiased->vertices[0].position.z, kUnbiased->vertices[0].position.z);
	EXPECT_GE(kEditWireBiased->vertices[0].position.z, 0.0F);
}

TEST(GpuResource, ComponentEditWireDepthBiasDoesNotPullHiddenEdgeThroughFrontDepth) {
	const crimson::RenderView kView = make_overlay_geometry_test_view(crimson::CameraProjection::Perspective);
	const crimson::LineOverlaySegment kFrontSegment{
		.start = { -0.5F, 0.0F, 9.9F },
		.end = { 0.5F, 0.0F, 9.9F },
		.semantic_role = crimson::OverlaySemanticRole::SourceWire,
		.source_kind = crimson::OverlaySourceKind::ComponentSelection,
	};
	const crimson::LineOverlaySegment kHiddenSegment{
		.start = { -0.5F, 0.0F, 10.0F },
		.end = { 0.5F, 0.0F, 10.0F },
		.semantic_role = crimson::OverlaySemanticRole::SourceWire,
		.source_kind = crimson::OverlaySourceKind::ComponentSelection,
	};

	const std::optional<crimson::gpu::OverlayLineDeviceQuad> kFront =
			crimson::gpu::make_overlay_line_device_quad(
					kView,
					kFrontSegment,
					2.5F,
					0.0F,
					false);
	const std::optional<crimson::gpu::OverlayLineDeviceQuad> kHiddenBiased =
			crimson::gpu::make_overlay_line_device_quad(
					kView,
					kHiddenSegment,
					2.5F,
					0.0F,
					false,
					true);
	ASSERT_TRUE(kFront.has_value());
	ASSERT_TRUE(kHiddenBiased.has_value());

	EXPECT_GT(kHiddenBiased->vertices[0].position.z, kFront->vertices[0].position.z);
}

TEST(GpuResource, MoveTransfersOwnershipWithoutDoubleDestroy) {
	FakeHandleTraits::reset_counts();
	{
		UniqueFakeHandle first(FakeHandle{ 11 });
		UniqueFakeHandle second(std::move(first));

		expect_true(second.valid() && second.get().id == 11, "moved-to handle owns the original handle");
	}

	expect_true(FakeHandleTraits::destroy_count == 1, "moved handle is destroyed exactly once");
	expect_true(FakeHandleTraits::last_destroyed == 11, "moved handle destroys the transferred id");
}

TEST(GpuResource, ResetAndReleaseHaveExplicitLifetimeBehavior) {
	FakeHandleTraits::reset_counts();
	{
		UniqueFakeHandle handle(FakeHandle{ 21 });
		handle.reset(FakeHandle{ 22 });
		expect_true(FakeHandleTraits::destroy_count == 1, "reset destroys the previous handle");
		expect_true(FakeHandleTraits::last_destroyed == 21, "reset destroys the previous id");

		const FakeHandle kReleased = handle.release();
		expect_true(kReleased.id == 22, "release returns the owned handle");
		expect_true(!handle.valid(), "release leaves the wrapper invalid");
	}

	expect_true(FakeHandleTraits::destroy_count == 1, "released handle is not destroyed by wrapper");
}

TEST(GpuResource, ResourceTableAcceptsMoveOnlyRaiiResources) {
	FakeHandleTraits::reset_counts();
	crimson::gpu::GpuResourceTable<MoveOnlyResource, crimson::RenderMeshHandle> table;

	const crimson::RenderMeshHandle kHandle = table.create(MoveOnlyResource{ 31 });
	expect_true(crimson::is_valid_handle(kHandle), "resource table creates a valid public handle");
	expect_true(table.live_count() == 1, "resource table counts move-only resource as live");
	expect_true(table.get(kHandle) != nullptr, "resource table resolves move-only resource");

	expect_true(table.destroy(kHandle), "destroying move-only table resource succeeds");
	expect_true(FakeHandleTraits::destroy_count == 1, "table destroy releases owned RAII resource");
	expect_true(FakeHandleTraits::last_destroyed == 31, "table destroy releases expected handle id");
	expect_true(table.live_count() == 0, "table destroy decrements live count");
	expect_true(table.get(kHandle) == nullptr, "destroyed public handle no longer resolves");
}

TEST(GpuResource, ResourceTableUpsertsCallerOwnedHandles) {
	FakeHandleTraits::reset_counts();
	crimson::gpu::GpuResourceTable<MoveOnlyResource, crimson::RenderMeshHandle> table;
	const crimson::RenderMeshHandle kHandle{ 12, 3 };

	expect_true(table.upsert(kHandle, MoveOnlyResource{ 41 }), "external handle upsert succeeds");
	expect_true(table.live_count() == 1, "external handle upsert creates one live slot");
	expect_true(table.get(kHandle) != nullptr, "external handle resolves after upsert");

	expect_true(table.upsert(kHandle, MoveOnlyResource{ 42 }), "same external handle can replace resource");
	expect_true(table.live_count() == 1, "external handle replacement keeps live count stable");
	expect_true(FakeHandleTraits::destroy_count == 1, "replacement destroys previous resource");
	expect_true(FakeHandleTraits::last_destroyed == 41, "replacement destroys expected resource");
}

TEST(GpuResource, FrameResourcesFollowRenderGraphTargetGenerations) {
	crimson::RenderGraph graph = crimson::make_v1_correctness_render_graph(
			crimson::ViewportExtent{ 640, 480, 1.0F });
	crimson::gpu::GpuFrameResources resources;

	resources.synchronize_descriptors(graph);
	const crimson::gpu::GpuFrameTargetRecord *hdr = resources.find(crimson::kHdrSceneColorTargetName);
	expect_true(hdr != nullptr, "GPU frame resources track HDR scene target");
	expect_true(hdr != nullptr && hdr->recreate_count == 1, "initial target sync creates the HDR target");
	expect_true(hdr != nullptr && !hdr->external(), "HDR scene target is GPU-owned");
	expect_true(
			hdr != nullptr && !hdr->runtime_ready(),
			"descriptor-only frame resource sync does not claim runtime handles");
	const crimson::gpu::GpuFrameTargetRecord *backbuffer = resources.find(crimson::kBackbufferColorTargetName);
	expect_true(backbuffer != nullptr && backbuffer->external(), "backbuffer color remains external");
	expect_true(backbuffer != nullptr && backbuffer->runtime_ready(), "external backbuffer target is runtime-ready by contract");
	expect_true(resources.graph_resize_generation() == graph.resize_generation(), "frame resources remember graph generation");

	resources.synchronize_descriptors(graph);
	hdr = resources.find(crimson::kHdrSceneColorTargetName);
	expect_true(hdr != nullptr && hdr->recreate_count == 1, "unchanged graph does not recreate targets");

	graph.resize(crimson::ViewportExtent{ 1280, 720, 1.0F });
	resources.synchronize_descriptors(graph);
	hdr = resources.find(crimson::kHdrSceneColorTargetName);
	expect_true(hdr != nullptr && hdr->recreate_count == 2, "resize generation recreates dependent targets");
	expect_true(
			hdr != nullptr && hdr->desc.extent.width_px == 1280,
			"GPU frame target descriptor follows resized graph extent");
}

TEST(GpuResource, FullscreenUvPolicyFollowsRuntimeOriginConvention) {
	const crimson::gpu::FullscreenTriangleUvRange kTopLeft =
			crimson::gpu::fullscreen_triangle_uv_range(crimson::gpu::screen_origin_from_bgfx(false));
	expect_true(
			kTopLeft.min_u == -1.0F && kTopLeft.max_u == 1.0F,
			"top-left fullscreen triangle uses bgfx example U range");
	expect_true(
			kTopLeft.min_v == 0.0F && kTopLeft.max_v == 2.0F,
			"top-left fullscreen triangle keeps the default V range");

	const crimson::gpu::FullscreenTriangleUvRange kBottomLeft =
			crimson::gpu::fullscreen_triangle_uv_range(crimson::gpu::screen_origin_from_bgfx(true));
	expect_true(
			kBottomLeft.min_u == -1.0F && kBottomLeft.max_u == 1.0F,
			"bottom-left fullscreen triangle keeps the same U range");
	expect_true(
			kBottomLeft.min_v == 1.0F && kBottomLeft.max_v == -1.0F,
			"bottom-left fullscreen triangle derives V inversion from runtime origin");
}

} // namespace
