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
#include "crimson/gpu/gpu_resources.hpp"
#include "crimson/gpu/gpu_view_policy.hpp"

#include <gtest/gtest.h>

#include <iostream>
#include <string_view>

namespace {

void expect_true(bool condition, std::string_view message) {
	EXPECT_TRUE(condition) << message;
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
