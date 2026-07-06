/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
#include "crimson/graph/render_graph.hpp"

#include <gtest/gtest.h>

#include <iostream>
#include <string>
#include <string_view>

namespace {

void expect_true(bool condition, std::string_view message) {
	EXPECT_TRUE(condition) << message;
}

TEST(RenderGraph, MinimalViewportGraphValidatesWithStablePassOrder) {
	crimson::RenderGraph graph = crimson::make_minimal_viewport_render_graph(crimson::ViewportExtent{ 1280, 720, 1.0F });

	expect_true(graph.validate().has_value(), "minimal viewport render graph validates");
	expect_true(graph.passes().size() == 5, "minimal viewport graph has five passes");
	expect_true(graph.passes()[0].name == "FrameSetupPass", "frame setup pass is first");
	expect_true(graph.passes()[1].name == "BackbufferClearPass", "backbuffer clear pass is second");
	expect_true(graph.passes()[2].name == "ViewportOpaquePass", "viewport opaque pass is third");
	expect_true(graph.passes()[3].name == "OverlayDepthTestedPass", "depth-tested overlay pass is fourth");
	expect_true(graph.passes()[4].name == "PresentPass", "present pass is last");
}

TEST(RenderGraph, ValidationRejectsMissingUnwrittenAndAmbiguousResources) {
	crimson::RenderGraph missing;
	missing.add_pass(crimson::make_pass("ReadsMissing", {
																crimson::ResourceUse{ .resource_name = "Missing", .access = crimson::RenderResourceAccess::Read },
														}));
	expect_true(!missing.validate(), "reading an undeclared resource fails validation");

	crimson::RenderGraph unwritten;
	(void)unwritten.add_resource(crimson::RenderResourceDesc{
			.name = "TransientColor",
			.format = crimson::RenderResourceFormat::Rgba8Unorm,
			.extent = crimson::ViewportExtent{ 32, 32, 1.0F },
			.external = false,
	});
	unwritten.add_pass(crimson::make_pass("ReadsBeforeWrite", {
																	  crimson::ResourceUse{ .resource_name = "TransientColor", .access = crimson::RenderResourceAccess::Read },
															  }));
	expect_true(!unwritten.validate(), "reading an unwritten non-external resource fails validation");

	crimson::RenderGraph double_write;
	(void)double_write.add_resource(crimson::RenderResourceDesc{
			.name = "TransientColor",
			.format = crimson::RenderResourceFormat::Rgba8Unorm,
			.extent = crimson::ViewportExtent{ 32, 32, 1.0F },
			.external = false,
	});
	double_write.add_pass(crimson::make_pass("FirstWrite", {
																   crimson::ResourceUse{ .resource_name = "TransientColor", .access = crimson::RenderResourceAccess::Write },
														   }));
	double_write.add_pass(crimson::make_pass("SecondWrite", {
																	crimson::ResourceUse{ .resource_name = "TransientColor", .access = crimson::RenderResourceAccess::Write },
															}));
	expect_true(!double_write.validate(), "a second write without ReadWrite intent fails validation");

	crimson::RenderGraph read_write;
	(void)read_write.add_resource(crimson::RenderResourceDesc{
			.name = "Color",
			.format = crimson::RenderResourceFormat::BackbufferColor,
			.extent = crimson::ViewportExtent{ 32, 32, 1.0F },
			.external = true,
	});
	read_write.add_pass(crimson::make_pass("ModifyColor", {
																  crimson::ResourceUse{ .resource_name = "Color", .access = crimson::RenderResourceAccess::ReadWrite },
														  }));
	expect_true(read_write.validate().has_value(), "ReadWrite explicitly allows ordered modification");
}

TEST(RenderGraph, ResourceRegistryResizeUpdatesGenerations) {
	crimson::RenderGraph graph = crimson::make_minimal_viewport_render_graph(crimson::ViewportExtent{ 640, 480, 1.0F });
	const crimson::RenderResourceRecord *before = graph.resources().find("BackbufferColor");
	expect_true(before != nullptr, "backbuffer color resource exists");
	const std::uint64_t kGenerationBefore = before == nullptr ? 0 : before->generation;

	graph.resize(crimson::ViewportExtent{ 1920, 1080, 1.0F });
	const crimson::RenderResourceRecord *after = graph.resources().find("BackbufferColor");
	expect_true(after != nullptr, "backbuffer color resource still exists after resize");
	expect_true(after != nullptr && after->generation != kGenerationBefore, "resize advances resource generation");
	expect_true(after != nullptr && after->desc.extent.width_px == 1920, "resize updates resource width");
	expect_true(graph.resize_generation() == 2, "graph resize generation advances once");
}

TEST(RenderGraph, DebugDumpIsStableForMinimalGraph) {
	const crimson::RenderGraph kGraph = crimson::make_minimal_viewport_render_graph(
			crimson::ViewportExtent{ 320, 200, 1.0F });

	const std::string kExpected =
			"RenderGraph generation=1\n"
			"Resources:\n"
			"  - BackbufferColor format=BackbufferColor extent=320x200 external=true generation=1\n"
			"  - BackbufferDepth format=BackbufferDepth extent=320x200 external=true generation=1\n"
			"Passes:\n"
			"  - FrameSetupPass resources=[]\n"
			"  - BackbufferClearPass resources=[BackbufferColor:ReadWrite, BackbufferDepth:ReadWrite]\n"
			"  - ViewportOpaquePass resources=[BackbufferColor:ReadWrite, BackbufferDepth:ReadWrite]\n"
			"  - OverlayDepthTestedPass resources=[BackbufferDepth:Read, BackbufferColor:ReadWrite]\n"
			"  - PresentPass resources=[BackbufferColor:Read]\n";

	expect_true(kGraph.debug_dump() == kExpected, "minimal graph debug dump is stable");
}

} // namespace
