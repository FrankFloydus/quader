#include "crimson/frame/frame_targets.hpp"
#include "crimson/graph/render_graph.hpp"

#include <gtest/gtest.h>

#include <array>
#include <iostream>
#include <string_view>

namespace {

void expect_true(bool condition, std::string_view message) {
	EXPECT_TRUE(condition) << message;
}

const crimson::RenderPass *find_pass(const crimson::RenderGraph &graph, std::string_view name) {
	for (const crimson::RenderPass &pass : graph.passes()) {
		if (pass.name == name) {
			return &pass;
		}
	}
	return nullptr;
}

const crimson::ResourceUse *find_use(const crimson::RenderPass &pass, std::string_view resource_name) {
	for (const crimson::ResourceUse &use : pass.resources) {
		if (use.resource_name == resource_name) {
			return &use;
		}
	}
	return nullptr;
}

TEST(RenderGraphCorrectness, Task9GraphDeclaresStablePassOrder) {
	const crimson::RenderGraph kGraph = crimson::make_v1_correctness_render_graph(
			crimson::ViewportExtent{ 1280, 720, 1.0F });
	const std::array<std::string_view, 12> kExpected = {
		"FrameSetupPass",
		"ResourceUploadPass",
		"DepthPrepass",
		"PickingPass",
		"OpaquePbrPass",
		"AlphaCutoutPbrPass",
		"TransparentPbrPass",
		"ToneMapPass",
		"OverlayDepthTestedPass",
		"OverlayXRayPass",
		"OverlayAlwaysOnTopPass",
		"PresentPass",
	};

	expect_true(kGraph.validate().has_value(), "task #9 V1 correctness graph validates");
	expect_true(kGraph.passes().size() == kExpected.size(), "task #9 graph declares twelve passes");
	for (std::size_t index = 0; index < kExpected.size(); ++index) {
		expect_true(kGraph.passes()[index].name == kExpected[index], "task #9 graph pass order is stable");
	}
}

TEST(RenderGraphCorrectness, Task9GraphDeclaresExpectedFrameTargets) {
	const crimson::RenderGraph kGraph = crimson::make_v1_correctness_render_graph(
			crimson::ViewportExtent{ 1920, 1080, 1.0F });

	const crimson::RenderResourceRecord *hdr = kGraph.resources().find(crimson::kHdrSceneColorTargetName);
	expect_true(hdr != nullptr, "HDR scene color target exists");
	expect_true(
			hdr != nullptr && hdr->desc.format == crimson::RenderResourceFormat::Rgba16Float,
			"HDR scene color is RGBA16F");
	expect_true(hdr != nullptr && hdr->desc.sample_count == 1, "HDR scene color target is not MSAA");
	expect_true(crimson::is_hdr_frame_target(crimson::kHdrSceneColorTargetName), "HDR scene color is marked HDR");

	const crimson::RenderResourceRecord *tone = kGraph.resources().find(crimson::kToneMappedColorTargetName);
	expect_true(tone != nullptr, "tone-mapped color target exists");
	expect_true(
			tone != nullptr && tone->desc.format == crimson::RenderResourceFormat::Rgba16Float,
			"tone-mapped color uses RGBA16F linear SDR");

	const crimson::RenderResourceRecord *picking = kGraph.resources().find(crimson::kPickingIdTargetName);
	expect_true(picking != nullptr, "picking ID target exists");
	expect_true(
			picking != nullptr && picking->desc.format == crimson::RenderResourceFormat::R32Uint,
			"default picking ID target uses R32U");
	expect_true(crimson::is_data_frame_target(crimson::kPickingIdTargetName), "picking ID target is data");

	const crimson::RenderGraph kRgba8Graph = crimson::make_v1_correctness_render_graph(
			crimson::ViewportExtent{ 320, 240, 1.0F },
			crimson::PickingIdTargetFormat::Rgba8Data);
	const crimson::RenderResourceRecord *rgba8_picking = kRgba8Graph.resources().find(crimson::kPickingIdTargetName);
	expect_true(
			rgba8_picking != nullptr && rgba8_picking->desc.format == crimson::RenderResourceFormat::Rgba8Unorm,
			"picking ID fallback uses RGBA8 data target");
}

TEST(RenderGraphCorrectness, ToneMapOverlayAndPresentResourceIsolationIsExplicit) {
	const crimson::RenderGraph kGraph = crimson::make_v1_correctness_render_graph(
			crimson::ViewportExtent{ 800, 600, 1.0F });

	const crimson::RenderPass *depth_prepass = find_pass(kGraph, "DepthPrepass");
	expect_true(depth_prepass != nullptr, "depth prepass exists");
	expect_true(
			depth_prepass != nullptr && depth_prepass->resources.size() == 1 && find_use(*depth_prepass, crimson::kSceneDepthTargetName) != nullptr && find_use(*depth_prepass, crimson::kHdrSceneColorTargetName) == nullptr,
			"depth prepass writes SceneDepth without touching HDR scene color");

	const crimson::RenderPass *opaque_pbr = find_pass(kGraph, "OpaquePbrPass");
	expect_true(opaque_pbr != nullptr, "opaque PBR pass exists");
	expect_true(
			opaque_pbr != nullptr && find_use(*opaque_pbr, crimson::kSceneDepthTargetName) != nullptr,
			"opaque PBR pass reads the shared SceneDepth target");
	expect_true(
			opaque_pbr != nullptr && find_use(*opaque_pbr, crimson::kHdrSceneColorTargetName) != nullptr,
			"opaque PBR pass writes HDR scene color");

	const crimson::RenderPass *tone_map = find_pass(kGraph, "ToneMapPass");
	expect_true(tone_map != nullptr, "tone-map pass exists");
	expect_true(
			tone_map != nullptr && find_use(*tone_map, crimson::kHdrSceneColorTargetName) != nullptr,
			"tone-map pass reads HDR scene color");
	expect_true(
			tone_map != nullptr && find_use(*tone_map, crimson::kToneMappedColorTargetName) != nullptr,
			"tone-map pass writes tone-mapped color");
	expect_true(
			tone_map != nullptr && find_use(*tone_map, crimson::kSceneDepthTargetName) == nullptr,
			"tone-map pass does not bind scene depth");

	for (std::string_view pass_name : {
				 "OverlayDepthTestedPass",
				 "OverlayXRayPass",
				 "OverlayAlwaysOnTopPass",
		 }) {
		const crimson::RenderPass *overlay = find_pass(kGraph, pass_name);
		expect_true(overlay != nullptr, "overlay pass exists");
		expect_true(
				overlay != nullptr && find_use(*overlay, crimson::kHdrSceneColorTargetName) == nullptr,
				"overlay pass never touches HDR scene color");
		expect_true(
				overlay != nullptr && find_use(*overlay, crimson::kToneMappedColorTargetName) != nullptr,
				"overlay pass modifies tone-mapped color");
	}

	const crimson::RenderPass *picking = find_pass(kGraph, "PickingPass");
	expect_true(picking != nullptr, "picking pass exists");
	if (picking != nullptr) {
		for (const crimson::ResourceUse &use : picking->resources) {
			expect_true(
					!crimson::is_color_managed_frame_target(use.resource_name),
					"picking pass avoids color-managed targets");
		}
	}

	std::uint32_t backbuffer_users = 0;
	for (const crimson::RenderPass &pass : kGraph.passes()) {
		if (find_use(pass, crimson::kBackbufferColorTargetName) != nullptr) {
			++backbuffer_users;
			expect_true(pass.name == "PresentPass", "only PresentPass writes final display target");
		}
	}
	expect_true(backbuffer_users == 1, "display conversion reaches the backbuffer exactly once");
}

TEST(RenderGraphCorrectness, ResizeUpdatesV1Targets) {
	crimson::RenderGraph graph = crimson::make_v1_correctness_render_graph(crimson::ViewportExtent{ 640, 480, 1.0F });
	const crimson::RenderResourceRecord *before = graph.resources().find(crimson::kHdrSceneColorTargetName);
	const std::uint64_t kBeforeGeneration = before == nullptr ? 0 : before->generation;

	graph.resize(crimson::ViewportExtent{ 1024, 768, 1.0F });
	const crimson::RenderResourceRecord *after = graph.resources().find(crimson::kHdrSceneColorTargetName);
	expect_true(after != nullptr, "HDR scene color target exists after resize");
	expect_true(after != nullptr && after->generation != kBeforeGeneration, "resize updates target generation");
	expect_true(after != nullptr && after->desc.extent.width_px == 1024, "resize updates target width");
}

} // namespace
