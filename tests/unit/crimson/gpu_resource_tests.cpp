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
#include <array>
#include <cstdint>
#include <cmath>
#include <iostream>
#include <optional>
#include <string_view>
#include <vector>

namespace {

void expect_true(bool condition, std::string_view message) {
	EXPECT_TRUE(condition) << message;
}

[[nodiscard]] bool near(float left, float right, float tolerance = 0.001F) noexcept {
	return std::fabs(left - right) <= tolerance;
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

[[nodiscard]] crimson::RenderView make_overlay_geometry_test_view_from(
		quader::math::Vec3 eye,
		quader::math::Vec3 target) {
	crimson::RenderView view = make_overlay_geometry_test_view(crimson::CameraProjection::Perspective);
	view.camera.eye = eye;
	view.camera.target = target;
	view.camera.forward = quader::math::normalized(target - eye);
	return view;
}

void append_source_wire_box_stamp(
		std::vector<crimson::SourceWireDepthStampMetadata> &stamps,
		crimson::RenderObjectId object_id,
		std::uint32_t face_index,
		quader::math::Vec3 a,
		quader::math::Vec3 b,
		quader::math::Vec3 c,
		bool inverted) {
	if (inverted) {
		std::swap(b, c);
	}
	const std::uint32_t kPayloadOffset = static_cast<std::uint32_t>(stamps.size());
	stamps.push_back(crimson::SourceWireDepthStampMetadata{
			.view_index = 0,
			.source_kind = crimson::OverlaySourceKind::SourceWire,
			.triangle = crimson::TriangleOverlayPrimitive{
					.a = a,
					.b = b,
					.c = c,
					.semantic_role = crimson::OverlaySemanticRole::SourceWireDepthStamp,
					.source_kind = crimson::OverlaySourceKind::SourceWire,
					.element = crimson::OverlayElementRef{ .object_id = object_id, .face_index = face_index },
			},
			.payload_offset = kPayloadOffset,
			.payload_count = 1,
			.element = crimson::OverlayElementRef{ .object_id = object_id, .face_index = face_index },
	});
}

[[nodiscard]] std::vector<crimson::SourceWireDepthStampMetadata> make_source_wire_box_stamps(
		crimson::RenderObjectId object_id,
		quader::math::Vec3 min_corner,
		quader::math::Vec3 max_corner,
		bool inverted) {
	const quader::math::Vec3 p000{ min_corner.x, min_corner.y, min_corner.z };
	const quader::math::Vec3 p100{ max_corner.x, min_corner.y, min_corner.z };
	const quader::math::Vec3 p010{ min_corner.x, max_corner.y, min_corner.z };
	const quader::math::Vec3 p110{ max_corner.x, max_corner.y, min_corner.z };
	const quader::math::Vec3 p001{ min_corner.x, min_corner.y, max_corner.z };
	const quader::math::Vec3 p101{ max_corner.x, min_corner.y, max_corner.z };
	const quader::math::Vec3 p011{ min_corner.x, max_corner.y, max_corner.z };
	const quader::math::Vec3 p111{ max_corner.x, max_corner.y, max_corner.z };

	std::vector<crimson::SourceWireDepthStampMetadata> stamps;
	stamps.reserve(12);
	std::uint32_t face_index = 0;
	append_source_wire_box_stamp(stamps, object_id, face_index++, p000, p010, p110, inverted);
	append_source_wire_box_stamp(stamps, object_id, face_index++, p000, p110, p100, inverted);
	append_source_wire_box_stamp(stamps, object_id, face_index++, p001, p101, p111, inverted);
	append_source_wire_box_stamp(stamps, object_id, face_index++, p001, p111, p011, inverted);
	append_source_wire_box_stamp(stamps, object_id, face_index++, p000, p001, p011, inverted);
	append_source_wire_box_stamp(stamps, object_id, face_index++, p000, p011, p010, inverted);
	append_source_wire_box_stamp(stamps, object_id, face_index++, p100, p110, p111, inverted);
	append_source_wire_box_stamp(stamps, object_id, face_index++, p100, p111, p101, inverted);
	append_source_wire_box_stamp(stamps, object_id, face_index++, p000, p100, p101, inverted);
	append_source_wire_box_stamp(stamps, object_id, face_index++, p000, p101, p001, inverted);
	append_source_wire_box_stamp(stamps, object_id, face_index++, p010, p011, p111, inverted);
	append_source_wire_box_stamp(stamps, object_id, face_index++, p010, p111, p110, inverted);
	return stamps;
}

[[nodiscard]] std::vector<crimson::SourceWireDepthStampMetadata> make_source_wire_open_room_stamps(
		crimson::RenderObjectId object_id,
		quader::math::Vec3 min_corner,
		quader::math::Vec3 max_corner,
		bool inverted) {
	const quader::math::Vec3 p000{ min_corner.x, min_corner.y, min_corner.z };
	const quader::math::Vec3 p100{ max_corner.x, min_corner.y, min_corner.z };
	const quader::math::Vec3 p010{ min_corner.x, max_corner.y, min_corner.z };
	const quader::math::Vec3 p110{ max_corner.x, max_corner.y, min_corner.z };
	const quader::math::Vec3 p001{ min_corner.x, min_corner.y, max_corner.z };
	const quader::math::Vec3 p101{ max_corner.x, min_corner.y, max_corner.z };
	const quader::math::Vec3 p011{ min_corner.x, max_corner.y, max_corner.z };
	const quader::math::Vec3 p111{ max_corner.x, max_corner.y, max_corner.z };

	std::vector<crimson::SourceWireDepthStampMetadata> stamps;
	stamps.reserve(10);
	std::uint32_t face_index = 0;
	append_source_wire_box_stamp(stamps, object_id, face_index++, p001, p101, p111, inverted);
	append_source_wire_box_stamp(stamps, object_id, face_index++, p001, p111, p011, inverted);
	append_source_wire_box_stamp(stamps, object_id, face_index++, p000, p001, p011, inverted);
	append_source_wire_box_stamp(stamps, object_id, face_index++, p000, p011, p010, inverted);
	append_source_wire_box_stamp(stamps, object_id, face_index++, p100, p110, p111, inverted);
	append_source_wire_box_stamp(stamps, object_id, face_index++, p100, p111, p101, inverted);
	append_source_wire_box_stamp(stamps, object_id, face_index++, p000, p100, p101, inverted);
	append_source_wire_box_stamp(stamps, object_id, face_index++, p000, p101, p001, inverted);
	append_source_wire_box_stamp(stamps, object_id, face_index++, p010, p011, p111, inverted);
	append_source_wire_box_stamp(stamps, object_id, face_index++, p010, p111, p110, inverted);
	return stamps;
}

[[nodiscard]] std::array<crimson::SourceWireDepthStampMetadata, 2> make_front_plane_stamps(
		crimson::RenderObjectId object_id,
		float z) {
	return {
		crimson::SourceWireDepthStampMetadata{
				.view_index = 0,
				.source_kind = crimson::OverlaySourceKind::SourceWire,
				.triangle = crimson::TriangleOverlayPrimitive{
						.a = { -1.0F, -1.0F, z },
						.b = { -1.0F, 1.0F, z },
						.c = { 1.0F, 1.0F, z },
						.semantic_role = crimson::OverlaySemanticRole::SourceWireDepthStamp,
						.source_kind = crimson::OverlaySourceKind::SourceWire,
						.element = crimson::OverlayElementRef{ .object_id = object_id, .face_index = 3 },
				},
				.payload_offset = 0,
				.payload_count = 1,
				.element = crimson::OverlayElementRef{ .object_id = object_id, .face_index = 3 },
		},
		crimson::SourceWireDepthStampMetadata{
				.view_index = 0,
				.source_kind = crimson::OverlaySourceKind::SourceWire,
				.triangle = crimson::TriangleOverlayPrimitive{
						.a = { -1.0F, -1.0F, z },
						.b = { 1.0F, 1.0F, z },
						.c = { 1.0F, -1.0F, z },
						.semantic_role = crimson::OverlaySemanticRole::SourceWireDepthStamp,
						.source_kind = crimson::OverlaySourceKind::SourceWire,
						.element = crimson::OverlayElementRef{ .object_id = object_id, .face_index = 3 },
				},
				.payload_offset = 1,
				.payload_count = 1,
				.element = crimson::OverlayElementRef{ .object_id = object_id, .face_index = 3 },
		},
	};
}

[[nodiscard]] crimson::PreparedPointOverlayCommand make_visibility_point_command(
		crimson::OverlaySemanticRole role,
		crimson::OverlaySourceKind source_kind,
		crimson::RenderObjectId object_id,
		std::uint32_t vertex_index,
		quader::math::Vec3 position) {
	crimson::PreparedPointOverlayCommand points;
	points.command = crimson::OverlayCommand{
		.view_index = 0,
		.primitive = crimson::OverlayPrimitive::PointList,
		.depth_mode = crimson::OverlayDepthMode::AlwaysOnTop,
		.semantic_role = role,
		.source_kind = source_kind,
		.payload_count = 1,
	};
	points.semantic_role = role;
	points.source_kind = source_kind;
	points.render_state.depth_mode = crimson::OverlayDepthMode::AlwaysOnTop;
	points.render_state.depth_test_enabled = role != crimson::OverlaySemanticRole::SourceVertex;
	points.size_px = 7.0F;
	points.points = {
		crimson::PointOverlayPrimitive{
				.position = position,
				.size_px = 7.0F,
				.semantic_role = role,
				.source_kind = source_kind,
				.element = crimson::OverlayElementRef{ .object_id = object_id, .vertex_index = vertex_index },
		},
	};
	return points;
}

[[nodiscard]] crimson::PreparedLineOverlayCommand make_visibility_line_command(
		crimson::OverlaySemanticRole role,
		crimson::OverlaySourceKind source_kind,
		crimson::RenderObjectId object_id,
		std::uint32_t edge_index,
		quader::math::Vec3 start,
		quader::math::Vec3 end,
		bool depth_test_enabled,
		std::uint32_t face_index = crimson::kInvalidOverlayElementIndex,
		std::uint32_t incident_face_index0 = crimson::kInvalidOverlayElementIndex,
		std::uint32_t incident_face_index1 = crimson::kInvalidOverlayElementIndex) {
	crimson::PreparedLineOverlayCommand lines;
	lines.command = crimson::OverlayCommand{
		.view_index = 0,
		.primitive = crimson::OverlayPrimitive::LineList,
		.depth_mode = crimson::OverlayDepthMode::AlwaysOnTop,
		.semantic_role = role,
		.source_kind = source_kind,
		.payload_count = 1,
	};
	lines.semantic_role = role;
	lines.source_kind = source_kind;
	lines.render_state.depth_mode = crimson::OverlayDepthMode::AlwaysOnTop;
	lines.render_state.depth_test_enabled = depth_test_enabled;
	lines.segments = {
		crimson::LineOverlaySegment{
				.start = start,
				.end = end,
				.semantic_role = role,
				.source_kind = source_kind,
				.element = crimson::OverlayElementRef{
						.object_id = object_id,
						.face_index = face_index,
						.edge_index = edge_index,
						.incident_face_index0 = incident_face_index0,
						.incident_face_index1 = incident_face_index1,
				},
		},
	};
	return lines;
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
	constexpr std::uint64_t kReferenceTransparentBlend =
			BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_INV_SRC_ALPHA);
	constexpr std::uint64_t kPremultipliedBlend =
			BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_ONE, BGFX_STATE_BLEND_INV_SRC_ALPHA);

	expect_true(
			(kAlwaysOnTopState & BGFX_STATE_BLEND_MASK) == (kReferenceTransparentBlend & BGFX_STATE_BLEND_MASK),
			"grid uses the reference transparent material blend state");
	expect_true(
			(kAlwaysOnTopState & BGFX_STATE_BLEND_MASK) != (kPremultipliedBlend & BGFX_STATE_BLEND_MASK),
			"grid does not use premultiplied blending because the reference shader output is alpha-weighted before transparent blending");
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

TEST(GpuResource, LineQuadUsesScreenSpaceWidthCapsAndFeather) {
	const crimson::RenderView kView = make_overlay_geometry_test_view(crimson::CameraProjection::Orthographic);
	const crimson::LineOverlaySegment kSegment{
		.start = { -10.0F, 0.0F, 10.0F },
		.end = { 10.0F, 0.0F, 10.0F },
	};

	const std::optional<crimson::gpu::OverlayScreenSpaceQuad> kQuad =
			crimson::gpu::make_overlay_line_screen_space_quad(
					kView,
					kSegment,
					2.0F,
					0.0F,
					false);
	ASSERT_TRUE(kQuad.has_value());

	float min_x = kQuad->corners.front().x;
	float max_x = kQuad->corners.front().x;
	float min_y = kQuad->corners.front().y;
	float max_y = kQuad->corners.front().y;
	for (const quader::math::Vec3 corner : kQuad->corners) {
		min_x = std::min(min_x, corner.x);
		max_x = std::max(max_x, corner.x);
		min_y = std::min(min_y, corner.y);
		max_y = std::max(max_y, corner.y);
		EXPECT_TRUE(near(corner.z, 10.0F));
	}

	EXPECT_TRUE(near(min_x, -11.5F));
	EXPECT_TRUE(near(max_x, 11.5F));
	EXPECT_TRUE(near(min_y, -1.5F));
	EXPECT_TRUE(near(max_y, 1.5F));
}

TEST(GpuResource, PointQuadAppliesPostProjectionNormalDepthBias) {
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
	EXPECT_TRUE(near(kUnbiasedCenter.z, 10.0F));
	EXPECT_LT(kBiasedCenter.z, kUnbiasedCenter.z);
	EXPECT_GT(kBiasedCenter.z, kView.camera.near_plane_m);
}

TEST(GpuResource, SourceWireDepthStampsCullHiddenLinesBeforeAlwaysOnTopSubmission) {
	const crimson::RenderView kView = make_overlay_geometry_test_view(crimson::CameraProjection::Perspective);
	crimson::PreparedLineOverlayCommand lines;
	lines.command = crimson::OverlayCommand{
		.view_index = 0,
		.primitive = crimson::OverlayPrimitive::LineList,
		.depth_mode = crimson::OverlayDepthMode::AlwaysOnTop,
		.semantic_role = crimson::OverlaySemanticRole::SourceWire,
		.source_kind = crimson::OverlaySourceKind::SourceWire,
		.payload_count = 2,
	};
	lines.semantic_role = crimson::OverlaySemanticRole::SourceWire;
	lines.source_kind = crimson::OverlaySourceKind::SourceWire;
	lines.render_state.depth_mode = crimson::OverlayDepthMode::AlwaysOnTop;
	lines.render_state.depth_test_enabled = false;
	lines.segments = {
		crimson::LineOverlaySegment{
				.start = { -0.5F, 0.0F, 5.0F },
				.end = { 0.5F, 0.0F, 5.0F },
				.semantic_role = crimson::OverlaySemanticRole::SourceWire,
				.source_kind = crimson::OverlaySourceKind::SourceWire,
				.element = crimson::OverlayElementRef{ .object_id = 7, .edge_index = 1 },
		},
		crimson::LineOverlaySegment{
				.start = { -0.5F, 0.0F, 10.0F },
				.end = { 0.5F, 0.0F, 10.0F },
				.semantic_role = crimson::OverlaySemanticRole::SourceWire,
				.source_kind = crimson::OverlaySourceKind::SourceWire,
				.element = crimson::OverlayElementRef{ .object_id = 7, .edge_index = 2 },
		},
	};
	const std::array<crimson::SourceWireDepthStampMetadata, 2> kStamps = {
		crimson::SourceWireDepthStampMetadata{
				.view_index = 0,
				.source_kind = crimson::OverlaySourceKind::SourceWire,
				.triangle = crimson::TriangleOverlayPrimitive{
						.a = { -1.0F, -1.0F, 5.0F },
						.b = { -1.0F, 1.0F, 5.0F },
						.c = { 1.0F, 1.0F, 5.0F },
						.semantic_role = crimson::OverlaySemanticRole::SourceWireDepthStamp,
						.source_kind = crimson::OverlaySourceKind::SourceWire,
						.element = crimson::OverlayElementRef{ .object_id = 7, .face_index = 3 },
				},
				.payload_offset = 0,
				.payload_count = 1,
				.element = crimson::OverlayElementRef{ .object_id = 7, .face_index = 3 },
		},
		crimson::SourceWireDepthStampMetadata{
				.view_index = 0,
				.source_kind = crimson::OverlaySourceKind::SourceWire,
				.triangle = crimson::TriangleOverlayPrimitive{
						.a = { -1.0F, -1.0F, 5.0F },
						.b = { 1.0F, 1.0F, 5.0F },
						.c = { 1.0F, -1.0F, 5.0F },
						.semantic_role = crimson::OverlaySemanticRole::SourceWireDepthStamp,
						.source_kind = crimson::OverlaySourceKind::SourceWire,
						.element = crimson::OverlayElementRef{ .object_id = 7, .face_index = 3 },
				},
				.payload_offset = 1,
				.payload_count = 1,
				.element = crimson::OverlayElementRef{ .object_id = 7, .face_index = 3 },
		},
	};

	const crimson::PreparedLineOverlayCommand kFiltered =
			crimson::gpu::make_source_wire_visibility_filtered_line_command(kView, lines, kStamps);

	ASSERT_EQ(kFiltered.segments.size(), 1U);
	EXPECT_EQ(kFiltered.segments.front().element.edge_index, 1U);
	EXPECT_EQ(kFiltered.command.payload_count, 1U);
	EXPECT_EQ(kFiltered.render_state.depth_mode, crimson::OverlayDepthMode::AlwaysOnTop);
	EXPECT_FALSE(kFiltered.render_state.depth_test_enabled);
}

TEST(GpuResource, SourceWireDepthFilterKeepsInsideOutStampSetsVisible) {
	const crimson::RenderView kView = make_overlay_geometry_test_view(crimson::CameraProjection::Perspective);
	crimson::PreparedLineOverlayCommand lines;
	lines.command = crimson::OverlayCommand{
		.view_index = 0,
		.primitive = crimson::OverlayPrimitive::LineList,
		.depth_mode = crimson::OverlayDepthMode::AlwaysOnTop,
		.semantic_role = crimson::OverlaySemanticRole::SourceWire,
		.source_kind = crimson::OverlaySourceKind::SourceWire,
		.payload_count = 1,
	};
	lines.semantic_role = crimson::OverlaySemanticRole::SourceWire;
	lines.source_kind = crimson::OverlaySourceKind::SourceWire;
	lines.segments = {
		crimson::LineOverlaySegment{
				.start = { -0.5F, 0.0F, 10.0F },
				.end = { 0.5F, 0.0F, 10.0F },
				.semantic_role = crimson::OverlaySemanticRole::SourceWire,
				.source_kind = crimson::OverlaySourceKind::SourceWire,
				.element = crimson::OverlayElementRef{ .object_id = 8, .edge_index = 4 },
		},
	};
	const std::vector<crimson::SourceWireDepthStampMetadata> kInvertedStamps =
			make_source_wire_box_stamps(8, { -1.0F, -1.0F, 4.0F }, { 1.0F, 1.0F, 6.0F }, true);

	const crimson::PreparedLineOverlayCommand kFiltered =
			crimson::gpu::make_source_wire_visibility_filtered_line_command(kView, lines, kInvertedStamps);

	ASSERT_EQ(kFiltered.segments.size(), 1U);
	EXPECT_EQ(kFiltered.segments.front().element.edge_index, 4U);
}

TEST(GpuResource, MixedOpenStampSetsDoNotBypassSourceWireFiltering) {
	const crimson::RenderView kView = make_overlay_geometry_test_view(crimson::CameraProjection::Perspective);
	const crimson::PreparedLineOverlayCommand lines = make_visibility_line_command(
			crimson::OverlaySemanticRole::SourceWire,
			crimson::OverlaySourceKind::SourceWire,
			16,
			5,
			{ -0.5F, 0.0F, 10.0F },
			{ 0.5F, 0.0F, 10.0F },
			false);
	const std::vector<crimson::SourceWireDepthStampMetadata> kMixedOpenStamps{
		crimson::SourceWireDepthStampMetadata{
				.view_index = 0,
				.source_kind = crimson::OverlaySourceKind::SourceWire,
				.triangle = crimson::TriangleOverlayPrimitive{
						.a = { -1.0F, -1.0F, 5.0F },
						.b = { -1.0F, 1.0F, 5.0F },
						.c = { 1.0F, 1.0F, 5.0F },
						.semantic_role = crimson::OverlaySemanticRole::SourceWireDepthStamp,
						.source_kind = crimson::OverlaySourceKind::SourceWire,
						.element = crimson::OverlayElementRef{ .object_id = 16, .face_index = 1 },
				},
				.payload_offset = 0,
				.payload_count = 1,
				.element = crimson::OverlayElementRef{ .object_id = 16, .face_index = 1 },
		},
		crimson::SourceWireDepthStampMetadata{
				.view_index = 0,
				.source_kind = crimson::OverlaySourceKind::SourceWire,
				.triangle = crimson::TriangleOverlayPrimitive{
						.a = { -1.0F, -1.0F, 7.0F },
						.b = { -1.0F, 1.0F, 7.0F },
						.c = { 1.0F, 1.0F, 7.0F },
						.semantic_role = crimson::OverlaySemanticRole::SourceWireDepthStamp,
						.source_kind = crimson::OverlaySourceKind::SourceWire,
						.element = crimson::OverlayElementRef{ .object_id = 16, .face_index = 2 },
				},
				.payload_offset = 1,
				.payload_count = 1,
				.element = crimson::OverlayElementRef{ .object_id = 16, .face_index = 2 },
		},
	};

	const crimson::PreparedLineOverlayCommand kFiltered =
			crimson::gpu::make_source_wire_visibility_filtered_line_command(kView, lines, kMixedOpenStamps);

	EXPECT_TRUE(kFiltered.segments.empty());
	EXPECT_EQ(kFiltered.command.payload_count, 0U);
}

TEST(GpuResource, SourceWireDepthFilterSkipsTrianglesOwningIncidentFace) {
	const crimson::RenderView kView = make_overlay_geometry_test_view_from(
			{ 0.0F, 0.1F, 0.0F },
			{ 0.0F, -0.0005F, 4.0F });
	const crimson::PreparedLineOverlayCommand lines = make_visibility_line_command(
			crimson::OverlaySemanticRole::SourceWire,
			crimson::OverlaySourceKind::SourceWire,
			13,
			6,
			{ -1.0F, -0.0005F, 4.0F },
			{ 1.0F, -0.0005F, 4.0F },
			false,
			crimson::kInvalidOverlayElementIndex,
			9);
	const std::array<crimson::SourceWireDepthStampMetadata, 1> kStamps = {
		crimson::SourceWireDepthStampMetadata{
				.view_index = 0,
				.source_kind = crimson::OverlaySourceKind::SourceWire,
				.triangle = crimson::TriangleOverlayPrimitive{
						.a = { -1.0F, 0.0F, 4.0F },
						.b = { 1.0F, 0.0F, 4.0F },
						.c = { 1.0F, 0.0F, 0.0F },
						.semantic_role = crimson::OverlaySemanticRole::SourceWireDepthStamp,
						.source_kind = crimson::OverlaySourceKind::SourceWire,
						.element = crimson::OverlayElementRef{ .object_id = 13, .face_index = 9 },
				},
				.payload_offset = 0,
				.payload_count = 1,
				.element = crimson::OverlayElementRef{ .object_id = 13, .face_index = 9 },
		},
	};

	const crimson::PreparedLineOverlayCommand kFiltered =
			crimson::gpu::make_source_wire_visibility_filtered_line_command(kView, lines, kStamps);

	ASSERT_EQ(kFiltered.segments.size(), 1U);
	EXPECT_EQ(kFiltered.segments.front().element.edge_index, 6U);
}

TEST(GpuResource, SourceWireDepthFilterDoesNotSkipNonIncidentTrianglesWithMatchingEndpoints) {
	const crimson::RenderView kView = make_overlay_geometry_test_view_from(
			{ 0.0F, 0.1F, 0.0F },
			{ 0.0F, -0.0005F, 4.0F });
	const crimson::PreparedLineOverlayCommand lines = make_visibility_line_command(
			crimson::OverlaySemanticRole::SourceWire,
			crimson::OverlaySourceKind::SourceWire,
			13,
			6,
			{ -1.0F, -0.0005F, 4.0F },
			{ 1.0F, -0.0005F, 4.0F },
			false);
	const std::array<crimson::SourceWireDepthStampMetadata, 1> kStamps = {
		crimson::SourceWireDepthStampMetadata{
				.view_index = 0,
				.source_kind = crimson::OverlaySourceKind::SourceWire,
				.triangle = crimson::TriangleOverlayPrimitive{
						.a = { -1.0F, 0.0F, 4.0F },
						.b = { 1.0F, 0.0F, 4.0F },
						.c = { 1.0F, 0.0F, 0.0F },
						.semantic_role = crimson::OverlaySemanticRole::SourceWireDepthStamp,
						.source_kind = crimson::OverlaySourceKind::SourceWire,
						.element = crimson::OverlayElementRef{ .object_id = 13, .face_index = 9 },
				},
				.payload_offset = 0,
				.payload_count = 1,
				.element = crimson::OverlayElementRef{ .object_id = 13, .face_index = 9 },
		},
	};

	const crimson::PreparedLineOverlayCommand kFiltered =
			crimson::gpu::make_source_wire_visibility_filtered_line_command(kView, lines, kStamps);

	EXPECT_TRUE(kFiltered.segments.empty());
	EXPECT_EQ(kFiltered.command.payload_count, 0U);
}

TEST(GpuResource, SourceWireInsideOutDetectionUsesObjectWindingNotCameraFacingNormals) {
	const crimson::RenderView kView = make_overlay_geometry_test_view(crimson::CameraProjection::Perspective);
	crimson::PreparedLineOverlayCommand lines;
	lines.command = crimson::OverlayCommand{
		.view_index = 0,
		.primitive = crimson::OverlayPrimitive::LineList,
		.depth_mode = crimson::OverlayDepthMode::AlwaysOnTop,
		.semantic_role = crimson::OverlaySemanticRole::SourceWire,
		.source_kind = crimson::OverlaySourceKind::SourceWire,
		.payload_count = 1,
	};
	lines.semantic_role = crimson::OverlaySemanticRole::SourceWire;
	lines.source_kind = crimson::OverlaySourceKind::SourceWire;
	lines.segments = {
		crimson::LineOverlaySegment{
				.start = { -0.5F, 0.0F, 2.0F },
				.end = { 0.5F, 0.0F, 2.0F },
				.semantic_role = crimson::OverlaySemanticRole::SourceWire,
				.source_kind = crimson::OverlaySourceKind::SourceWire,
				.element = crimson::OverlayElementRef{ .object_id = 9, .edge_index = 4 },
		},
	};
	const std::vector<crimson::SourceWireDepthStampMetadata> kOutwardStamps =
			make_source_wire_box_stamps(9, { -1.0F, -1.0F, -1.0F }, { 1.0F, 1.0F, 1.0F }, false);

	const crimson::PreparedLineOverlayCommand kFiltered =
			crimson::gpu::make_source_wire_visibility_filtered_line_command(kView, lines, kOutwardStamps);

	EXPECT_TRUE(kFiltered.segments.empty());
	EXPECT_EQ(kFiltered.command.payload_count, 0U);
}

TEST(GpuResource, OpenInvertedRoomKeepsInteriorCornerEdgesFromMultipleCameras) {
	const std::vector<crimson::SourceWireDepthStampMetadata> kInvertedStamps =
			make_source_wire_open_room_stamps(14, { -1.0F, -1.0F, 4.0F }, { 1.0F, 1.0F, 6.0F }, true);
	const crimson::PreparedLineOverlayCommand lines = make_visibility_line_command(
			crimson::OverlaySemanticRole::SourceWire,
			crimson::OverlaySourceKind::SourceWire,
			14,
			7,
			{ -1.0F, 1.0F, 4.0F },
			{ 1.0F, 1.0F, 4.0F },
			false);
	const crimson::RenderView kFrontView = make_overlay_geometry_test_view_from(
			{ 0.0F, 0.0F, 0.0F },
			{ 0.0F, 0.2F, 5.0F });
	const crimson::RenderView kAngledView = make_overlay_geometry_test_view_from(
			{ 0.85F, 0.45F, 0.0F },
			{ 0.0F, 0.2F, 5.0F });

	const crimson::PreparedLineOverlayCommand kFrontFiltered =
			crimson::gpu::make_source_wire_visibility_filtered_line_command(kFrontView, lines, kInvertedStamps);
	const crimson::PreparedLineOverlayCommand kAngledFiltered =
			crimson::gpu::make_source_wire_visibility_filtered_line_command(kAngledView, lines, kInvertedStamps);

	ASSERT_EQ(kFrontFiltered.segments.size(), 1U);
	ASSERT_EQ(kAngledFiltered.segments.size(), 1U);
	EXPECT_EQ(kFrontFiltered.segments.front().element.edge_index, 7U);
	EXPECT_EQ(kAngledFiltered.segments.front().element.edge_index, 7U);
}

TEST(GpuResource, ComponentEdgesBypassSourceWireCpuFiltering) {
	const crimson::RenderView kView = make_overlay_geometry_test_view(crimson::CameraProjection::Perspective);
	const std::array<crimson::SourceWireDepthStampMetadata, 2> kStamps =
			make_front_plane_stamps(15, 5.0F);
	const crimson::PreparedLineOverlayCommand selected = make_visibility_line_command(
			crimson::OverlaySemanticRole::SelectedEdge,
			crimson::OverlaySourceKind::ComponentSelection,
			15,
			8,
			{ -0.5F, 0.0F, 10.0F },
			{ 0.5F, 0.0F, 10.0F },
			true);
	const crimson::PreparedLineOverlayCommand hover = make_visibility_line_command(
			crimson::OverlaySemanticRole::HoverEdge,
			crimson::OverlaySourceKind::ComponentHover,
			15,
			9,
			{ -0.5F, 0.0F, 10.0F },
			{ 0.5F, 0.0F, 10.0F },
			true);

	const crimson::PreparedLineOverlayCommand kFilteredSelected =
			crimson::gpu::make_source_wire_visibility_filtered_line_command(kView, selected, kStamps);
	const crimson::PreparedLineOverlayCommand kFilteredHover =
			crimson::gpu::make_source_wire_visibility_filtered_line_command(kView, hover, kStamps);

	ASSERT_EQ(kFilteredSelected.segments.size(), 1U);
	ASSERT_EQ(kFilteredHover.segments.size(), 1U);
	EXPECT_EQ(kFilteredSelected.segments.front().element.edge_index, 8U);
	EXPECT_EQ(kFilteredHover.segments.front().element.edge_index, 9U);
	EXPECT_TRUE(kFilteredSelected.render_state.depth_test_enabled);
	EXPECT_TRUE(kFilteredHover.render_state.depth_test_enabled);
}

TEST(GpuResource, ComponentLineDepthBiasMovesTowardCamera) {
	const crimson::RenderView kView = make_overlay_geometry_test_view(crimson::CameraProjection::Perspective);
	const crimson::LineOverlaySegment kSegment{
		.start = { -0.5F, 0.0F, 10.0F },
		.end = { 0.5F, 0.0F, 10.0F },
	};

	const std::optional<crimson::gpu::OverlayScreenSpaceQuad> kUnbiased =
			crimson::gpu::make_overlay_line_screen_space_quad(
					kView,
					kSegment,
					2.5F,
					0.0F,
					false);
	const std::optional<crimson::gpu::OverlayScreenSpaceQuad> kBiased =
			crimson::gpu::make_overlay_line_screen_space_quad(
					kView,
					kSegment,
					2.5F,
					1.0F,
					false);
	ASSERT_TRUE(kUnbiased.has_value());
	ASSERT_TRUE(kBiased.has_value());

	EXPECT_LT(quad_center(*kBiased).z, quad_center(*kUnbiased).z);
}

TEST(GpuResource, SourceVertexDepthStampsCullHiddenPointHandlesBeforeAlwaysOnTopSubmission) {
	const crimson::RenderView kView = make_overlay_geometry_test_view(crimson::CameraProjection::Perspective);
	const crimson::PreparedPointOverlayCommand points = make_visibility_point_command(
			crimson::OverlaySemanticRole::SourceVertex,
			crimson::OverlaySourceKind::SourceWire,
			10,
			3,
			{ 0.0F, 0.0F, 10.0F });
	const std::array<crimson::SourceWireDepthStampMetadata, 2> kStamps =
			make_front_plane_stamps(10, 5.0F);

	const crimson::PreparedPointOverlayCommand kFiltered =
			crimson::gpu::make_source_wire_visibility_filtered_point_command(kView, points, kStamps);

	EXPECT_TRUE(kFiltered.points.empty());
	EXPECT_EQ(kFiltered.command.payload_count, 0U);
	EXPECT_EQ(kFiltered.render_state.depth_mode, crimson::OverlayDepthMode::AlwaysOnTop);
}

TEST(GpuResource, ComponentVerticesBypassSourceWireCpuFiltering) {
	const crimson::RenderView kView = make_overlay_geometry_test_view(crimson::CameraProjection::Perspective);
	const std::array<crimson::SourceWireDepthStampMetadata, 2> kStamps =
			make_front_plane_stamps(11, 5.0F);
	const crimson::PreparedPointOverlayCommand selected = make_visibility_point_command(
			crimson::OverlaySemanticRole::SelectedVertex,
			crimson::OverlaySourceKind::ComponentSelection,
			11,
			4,
			{ 0.0F, 0.0F, 10.0F });
	const crimson::PreparedPointOverlayCommand hover = make_visibility_point_command(
			crimson::OverlaySemanticRole::HoverVertex,
			crimson::OverlaySourceKind::ComponentHover,
			11,
			5,
			{ 0.0F, 0.0F, 10.0F });

	const crimson::PreparedPointOverlayCommand kFilteredSelected =
			crimson::gpu::make_source_wire_visibility_filtered_point_command(kView, selected, kStamps);
	const crimson::PreparedPointOverlayCommand kFilteredHover =
			crimson::gpu::make_source_wire_visibility_filtered_point_command(kView, hover, kStamps);

	ASSERT_EQ(kFilteredSelected.points.size(), 1U);
	ASSERT_EQ(kFilteredHover.points.size(), 1U);
	EXPECT_EQ(kFilteredSelected.points.front().element.vertex_index, 4U);
	EXPECT_EQ(kFilteredHover.points.front().element.vertex_index, 5U);
	EXPECT_TRUE(kFilteredSelected.render_state.depth_test_enabled);
	EXPECT_TRUE(kFilteredHover.render_state.depth_test_enabled);
}

TEST(GpuResource, InsideOutDepthStampsKeepSourcePointHandles) {
	const crimson::RenderView kView = make_overlay_geometry_test_view(crimson::CameraProjection::Perspective);
	const std::vector<crimson::SourceWireDepthStampMetadata> kInvertedStamps =
			make_source_wire_box_stamps(12, { -1.0F, -1.0F, 4.0F }, { 1.0F, 1.0F, 6.0F }, true);
	const crimson::PreparedPointOverlayCommand source = make_visibility_point_command(
			crimson::OverlaySemanticRole::SourceVertex,
			crimson::OverlaySourceKind::SourceWire,
			12,
			6,
			{ 0.0F, 0.0F, 10.0F });

	const crimson::PreparedPointOverlayCommand kFilteredSource =
			crimson::gpu::make_source_wire_visibility_filtered_point_command(kView, source, kInvertedStamps);

	ASSERT_EQ(kFilteredSource.points.size(), 1U);
	EXPECT_EQ(kFilteredSource.points.front().element.vertex_index, 6U);
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
