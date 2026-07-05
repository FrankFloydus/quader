#include "crimson/frame/frame_targets.hpp"
#include "crimson/graph/render_graph.hpp"
#include "crimson/picking/picking.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <iostream>
#include <optional>
#include <string_view>

namespace {

void expect_true(bool condition, std::string_view message) {
	EXPECT_TRUE(condition) << message;
}

[[nodiscard]] const crimson::RenderPass *find_pass(
		const crimson::RenderGraph &graph,
		std::string_view name) {
	for (const crimson::RenderPass &pass : graph.passes()) {
		if (pass.name == name) {
			return &pass;
		}
	}
	return nullptr;
}

TEST(Picking, RawIdRgba8EncodingRoundTripsWithoutColorManagement) {
	const crimson::PackedPickingIdRgba8 kZero = crimson::encode_picking_id_rgba8(0);
	expect_true(kZero.bytes == std::array<std::uint8_t, 4>{ 0, 0, 0, 0 }, "raw ID 0 encodes to no-hit bytes");
	expect_true(crimson::decode_picking_id_rgba8(kZero) == 0, "raw ID 0 decodes as no-hit");

	const crimson::PackedPickingIdRgba8 kEncoded = crimson::encode_picking_id_rgba8(0x12345678U);
	expect_true(
			kEncoded.bytes == std::array<std::uint8_t, 4>{ 0x78, 0x56, 0x34, 0x12 },
			"RGBA8 fallback stores little-endian data bytes");
	expect_true(crimson::decode_picking_id_rgba8(kEncoded) == 0x12345678U, "RGBA8 fallback decodes raw ID");

	const std::array<float, 4> kUnorm = crimson::picking_id_rgba8_to_unorm(kEncoded);
	expect_true(std::fabs(kUnorm[0] - (120.0F / 255.0F)) < 0.0001F, "fallback byte becomes direct UNORM red");
	expect_true(std::fabs(kUnorm[3] - (18.0F / 255.0F)) < 0.0001F, "fallback byte becomes direct UNORM alpha");
}

TEST(Picking, AllocatorScopesIdsToOneRequest) {
	crimson::PickingIdAllocator allocator;
	allocator.begin_request(11);
	const std::uint32_t kFirstId = allocator.allocate(crimson::PickingPayload{
			.object_id = 100,
			.submesh_index = 3,
			.element_kind = crimson::PickingElementKind::Object,
	});

	const std::optional<crimson::PickingPayload> kFirstPayload = allocator.resolve(kFirstId);
	expect_true(kFirstId == 1, "request allocator starts raw IDs at 1");
	expect_true(kFirstPayload.has_value() && kFirstPayload->object_id == 100, "first request resolves its payload");

	allocator.begin_request(12);
	expect_true(allocator.request_id() == 12, "allocator records the active request");
	expect_true(!allocator.resolve(kFirstId).has_value(), "new request invalidates previous raw ID mapping");

	const std::uint32_t kSecondId = allocator.allocate(crimson::PickingPayload{
			.object_id = 200,
			.submesh_index = 0,
			.element_kind = crimson::PickingElementKind::Object,
	});
	const std::optional<crimson::PickingPayload> kSecondPayload = allocator.resolve(kSecondId);
	expect_true(kSecondId == 1, "request-scoped allocator reuses raw IDs per request");
	expect_true(kSecondPayload.has_value() && kSecondPayload->object_id == 200, "second request resolves new payload");
}

TEST(Picking, EmptyRequestSpanDoesNotScheduleReadback) {
	const std::array<crimson::PickingRequest, 0> kEmptyRequests{};
	const std::array<crimson::PickingRequest, 1> kOneRequest = {
		crimson::PickingRequest{ .request_id = 7, .view_index = 0, .x_px = 12, .y_px = 24 },
	};

	expect_true(!crimson::picking_readback_requested(kEmptyRequests), "empty picking request span is dormant");
	expect_true(crimson::picking_readback_requested(kOneRequest), "non-empty picking request span schedules readback");
}

TEST(Picking, PickingPassStaysOutOfColorManagedTargets) {
	const crimson::RenderGraph kGraph = crimson::make_v1_correctness_render_graph(
			crimson::ViewportExtent{ 640, 480, 1.0F },
			crimson::PickingIdTargetFormat::Rgba8Data);
	const crimson::RenderPass *picking = find_pass(kGraph, "PickingPass");
	expect_true(picking != nullptr, "picking pass exists in V1 graph");
	if (picking == nullptr) {
		return;
	}

	for (const crimson::ResourceUse &use : picking->resources) {
		expect_true(!crimson::is_color_managed_frame_target(use.resource_name), "picking pass avoids color targets");
	}

	const crimson::RenderResourceRecord *target = kGraph.resources().find(crimson::kPickingIdTargetName);
	expect_true(target != nullptr, "picking ID target exists");
	expect_true(
			target != nullptr && target->desc.format == crimson::RenderResourceFormat::Rgba8Unorm,
			"RGBA8 fallback remains a data target");
	expect_true(crimson::is_data_frame_target(crimson::kPickingIdTargetName), "picking ID target is data");
}

} // namespace
