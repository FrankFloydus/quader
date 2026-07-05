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

#include <algorithm>
#include <sstream>
#include <string>
#include <unordered_set>
#include <utility>

namespace crimson {
namespace {

[[nodiscard]] bool access_reads(RenderResourceAccess access) noexcept {
	return access == RenderResourceAccess::Read || access == RenderResourceAccess::ReadWrite;
}

[[nodiscard]] bool access_writes(RenderResourceAccess access) noexcept {
	return access == RenderResourceAccess::Write || access == RenderResourceAccess::ReadWrite;
}

[[nodiscard]] bool contains_name(const std::unordered_set<std::string> &names, std::string_view name) {
	return names.find(std::string(name)) != names.end();
}

void insert_name(std::unordered_set<std::string> &names, std::string_view name) {
	names.insert(std::string(name));
}

} // namespace

const char *render_resource_format_name(RenderResourceFormat format) noexcept {
	switch (format) {
		case RenderResourceFormat::BackbufferColor:
			return "BackbufferColor";
		case RenderResourceFormat::BackbufferDepth:
			return "BackbufferDepth";
		case RenderResourceFormat::Rgba8Unorm:
			return "Rgba8Unorm";
		case RenderResourceFormat::Rgba16Float:
			return "Rgba16Float";
		case RenderResourceFormat::D24S8:
			return "D24S8";
		case RenderResourceFormat::D32Float:
			return "D32Float";
		case RenderResourceFormat::R32Uint:
			return "R32Uint";
	}

	return "Unknown";
}

const char *render_resource_access_name(RenderResourceAccess access) noexcept {
	switch (access) {
		case RenderResourceAccess::Read:
			return "Read";
		case RenderResourceAccess::Write:
			return "Write";
		case RenderResourceAccess::ReadWrite:
			return "ReadWrite";
	}

	return "Unknown";
}

RenderPass make_cpu_pass(std::string name) {
	return RenderPass{ .name = std::move(name) };
}

RenderPass make_pass(std::string name, std::vector<ResourceUse> resources) {
	return RenderPass{
		.name = std::move(name),
		.resources = std::move(resources),
	};
}

quader::foundation::Result<void, std::string> RenderResourceRegistry::add(RenderResourceDesc desc) {
	if (desc.name.empty()) {
		return quader::foundation::Result<void, std::string>::failure("Render resource name must not be empty.");
	}
	if (!is_valid_extent(desc.extent)) {
		return quader::foundation::Result<void, std::string>::failure(
				"Render resource '" + desc.name + "' has an invalid extent.");
	}
	if (desc.sample_count == 0) {
		return quader::foundation::Result<void, std::string>::failure(
				"Render resource '" + desc.name + "' has an invalid sample count.");
	}
	if (find(desc.name) != nullptr) {
		return quader::foundation::Result<void, std::string>::failure(
				"Render resource '" + desc.name + "' is already registered.");
	}

	resources_.push_back(RenderResourceRecord{ .desc = std::move(desc), .generation = resize_generation_ });
	return quader::foundation::Result<void, std::string>::success();
}

const RenderResourceRecord *RenderResourceRegistry::find(std::string_view name) const noexcept {
	const auto kIt = std::find_if(resources_.begin(), resources_.end(), [name](const RenderResourceRecord &record) {
		return record.desc.name == name;
	});
	return kIt == resources_.end() ? nullptr : &*kIt;
}

RenderResourceRecord *RenderResourceRegistry::find(std::string_view name) noexcept {
	const auto kIt = std::find_if(resources_.begin(), resources_.end(), [name](const RenderResourceRecord &record) {
		return record.desc.name == name;
	});
	return kIt == resources_.end() ? nullptr : &*kIt;
}

std::span<const RenderResourceRecord> RenderResourceRegistry::resources() const noexcept {
	return resources_;
}

std::uint64_t RenderResourceRegistry::resize_generation() const noexcept {
	return resize_generation_;
}

void RenderResourceRegistry::resize(ViewportExtent extent) {
	if (!is_valid_extent(extent)) {
		return;
	}

	++resize_generation_;
	if (resize_generation_ == 0) {
		resize_generation_ = 1;
	}

	for (RenderResourceRecord &record : resources_) {
		if (record.desc.resize_dependent) {
			record.desc.extent = extent;
			record.generation = resize_generation_;
		}
	}
}

void RenderResourceRegistry::clear() noexcept {
	resources_.clear();
	resize_generation_ = 1;
}

quader::foundation::Result<void, std::string> RenderGraph::add_resource(RenderResourceDesc desc) {
	return resources_.add(std::move(desc));
}

void RenderGraph::add_pass(RenderPass pass) {
	passes_.push_back(std::move(pass));
}

quader::foundation::Result<void, std::string> RenderGraph::validate() const {
	std::unordered_set<std::string> written_resources;
	for (const RenderResourceRecord &resource : resources_.resources()) {
		if (resource.desc.external) {
			insert_name(written_resources, resource.desc.name);
		}
	}

	std::unordered_set<std::string> pass_names;
	for (const RenderPass &pass : passes_) {
		if (pass.name.empty()) {
			return quader::foundation::Result<void, std::string>::failure("Render pass name must not be empty.");
		}
		if (contains_name(pass_names, pass.name)) {
			return quader::foundation::Result<void, std::string>::failure(
					"Render pass '" + pass.name + "' is registered more than once.");
		}
		insert_name(pass_names, pass.name);

		std::unordered_set<std::string> resources_used_by_pass;
		for (const ResourceUse &use : pass.resources) {
			const RenderResourceRecord *resource = resources_.find(use.resource_name);
			if (resource == nullptr) {
				return quader::foundation::Result<void, std::string>::failure(
						"Render pass '" + pass.name + "' references missing resource '" + use.resource_name + "'.");
			}
			if (contains_name(resources_used_by_pass, use.resource_name)) {
				return quader::foundation::Result<void, std::string>::failure(
						"Render pass '" + pass.name + "' references resource '" + use.resource_name + "' more than once.");
			}
			insert_name(resources_used_by_pass, use.resource_name);

			const bool kWasWritten = contains_name(written_resources, use.resource_name);
			if (use.access == RenderResourceAccess::Read && !kWasWritten) {
				return quader::foundation::Result<void, std::string>::failure(
						"Render pass '" + pass.name + "' reads resource '" + use.resource_name + "' before it is written.");
			}
			if (use.access == RenderResourceAccess::Write && kWasWritten) {
				return quader::foundation::Result<void, std::string>::failure(
						"Render pass '" + pass.name + "' writes resource '" + use.resource_name + "' after it was already written; use ReadWrite for ordered modification.");
			}
			if (access_writes(use.access)) {
				insert_name(written_resources, use.resource_name);
			}
		}
	}

	return quader::foundation::Result<void, std::string>::success();
}

std::string RenderGraph::debug_dump() const {
	std::ostringstream output;
	output << "RenderGraph generation=" << resources_.resize_generation() << '\n';
	output << "Resources:\n";
	for (const RenderResourceRecord &resource : resources_.resources()) {
		output << "  - " << resource.desc.name
			   << " format=" << render_resource_format_name(resource.desc.format)
			   << " extent=" << resource.desc.extent.width_px << 'x' << resource.desc.extent.height_px
			   << " external=" << (resource.desc.external ? "true" : "false")
			   << " generation=" << resource.generation << '\n';
	}

	output << "Passes:\n";
	for (const RenderPass &pass : passes_) {
		output << "  - " << pass.name;
		if (pass.resources.empty()) {
			output << " resources=[]\n";
			continue;
		}

		output << " resources=[";
		for (std::size_t index = 0; index < pass.resources.size(); ++index) {
			const ResourceUse &use = pass.resources[index];
			if (index != 0) {
				output << ", ";
			}
			output << use.resource_name << ':' << render_resource_access_name(use.access);
		}
		output << "]\n";
	}

	return output.str();
}

void RenderGraph::resize(ViewportExtent extent) {
	resources_.resize(extent);
}

void RenderGraph::clear() noexcept {
	resources_.clear();
	passes_.clear();
}

const RenderResourceRegistry &RenderGraph::resources() const noexcept {
	return resources_;
}

std::span<const RenderPass> RenderGraph::passes() const noexcept {
	return passes_;
}

std::uint64_t RenderGraph::resize_generation() const noexcept {
	return resources_.resize_generation();
}

RenderGraph make_minimal_prototype_render_graph(ViewportExtent extent) {
	RenderGraph graph;
	(void)graph.add_resource(RenderResourceDesc{
			.name = "BackbufferColor",
			.format = RenderResourceFormat::BackbufferColor,
			.extent = extent,
			.external = true,
			.resize_dependent = true,
	});
	(void)graph.add_resource(RenderResourceDesc{
			.name = "BackbufferDepth",
			.format = RenderResourceFormat::BackbufferDepth,
			.extent = extent,
			.external = true,
			.resize_dependent = true,
	});

	graph.add_pass(make_cpu_pass("FrameSetupPass"));
	graph.add_pass(make_pass("BackbufferClearPass", {
															ResourceUse{ .resource_name = "BackbufferColor", .access = RenderResourceAccess::ReadWrite },
															ResourceUse{ .resource_name = "BackbufferDepth", .access = RenderResourceAccess::ReadWrite },
													}));
	graph.add_pass(make_pass("PrototypeOpaquePass", {
															ResourceUse{ .resource_name = "BackbufferColor", .access = RenderResourceAccess::ReadWrite },
															ResourceUse{ .resource_name = "BackbufferDepth", .access = RenderResourceAccess::ReadWrite },
													}));
	graph.add_pass(make_pass("OverlayDepthTestedPass", {
															   ResourceUse{ .resource_name = "BackbufferDepth", .access = RenderResourceAccess::Read },
															   ResourceUse{ .resource_name = "BackbufferColor", .access = RenderResourceAccess::ReadWrite },
													   }));
	graph.add_pass(make_pass("PresentPass", {
													ResourceUse{ .resource_name = "BackbufferColor", .access = RenderResourceAccess::Read },
											}));

	return graph;
}

RenderGraph make_v1_correctness_render_graph(ViewportExtent extent, PickingIdTargetFormat picking_format) {
	RenderGraph graph;
	for (RenderResourceDesc &resource : make_v1_frame_target_resource_descs(extent, picking_format)) {
		(void)graph.add_resource(std::move(resource));
	}

	graph.add_pass(make_cpu_pass("FrameSetupPass"));
	graph.add_pass(make_cpu_pass("ResourceUploadPass"));
	graph.add_pass(make_pass("DepthPrepass", {
													 ResourceUse{ .resource_name = std::string(kSceneDepthTargetName), .access = RenderResourceAccess::Write },
											 }));
	graph.add_pass(make_pass("PickingPass", {
													ResourceUse{ .resource_name = std::string(kSceneDepthTargetName), .access = RenderResourceAccess::Read },
													ResourceUse{ .resource_name = std::string(kPickingIdTargetName), .access = RenderResourceAccess::Write },
											}));
	graph.add_pass(make_pass("GridSceneUnderlayPass", {
														   ResourceUse{ .resource_name = std::string(kHdrSceneColorTargetName), .access = RenderResourceAccess::Write },
												   }));
	graph.add_pass(make_pass("OpaquePbrPass", {
													  ResourceUse{ .resource_name = std::string(kSceneDepthTargetName), .access = RenderResourceAccess::Read },
													  ResourceUse{ .resource_name = std::string(kHdrSceneColorTargetName), .access = RenderResourceAccess::ReadWrite },
											  }));
	graph.add_pass(make_pass("AlphaCutoutPbrPass", {
														   ResourceUse{ .resource_name = std::string(kSceneDepthTargetName), .access = RenderResourceAccess::Read },
														   ResourceUse{ .resource_name = std::string(kHdrSceneColorTargetName), .access = RenderResourceAccess::ReadWrite },
												   }));
	graph.add_pass(make_pass("TransparentPbrPass", {
														   ResourceUse{ .resource_name = std::string(kSceneDepthTargetName), .access = RenderResourceAccess::Read },
														   ResourceUse{ .resource_name = std::string(kHdrSceneColorTargetName), .access = RenderResourceAccess::ReadWrite },
												   }));
	graph.add_pass(make_pass("ToneMapPass", {
													ResourceUse{ .resource_name = std::string(kHdrSceneColorTargetName), .access = RenderResourceAccess::Read },
													ResourceUse{ .resource_name = std::string(kToneMappedColorTargetName), .access = RenderResourceAccess::Write },
											}));
	graph.add_pass(make_pass("OverlayDepthTestedPass", {
															   ResourceUse{ .resource_name = std::string(kSceneDepthTargetName), .access = RenderResourceAccess::Read },
															   ResourceUse{ .resource_name = std::string(kToneMappedColorTargetName), .access = RenderResourceAccess::ReadWrite },
													   }));
	graph.add_pass(make_pass("OverlayXRayPass", {
														ResourceUse{ .resource_name = std::string(kSceneDepthTargetName), .access = RenderResourceAccess::Read },
														ResourceUse{ .resource_name = std::string(kToneMappedColorTargetName), .access = RenderResourceAccess::ReadWrite },
												}));
	graph.add_pass(make_pass("OverlayAlwaysOnTopPass", {
															   ResourceUse{ .resource_name = std::string(kToneMappedColorTargetName), .access = RenderResourceAccess::ReadWrite },
													   }));
	graph.add_pass(make_pass("PresentPass", {
													ResourceUse{ .resource_name = std::string(kToneMappedColorTargetName), .access = RenderResourceAccess::Read },
													ResourceUse{ .resource_name = std::string(kBackbufferColorTargetName), .access = RenderResourceAccess::ReadWrite },
											}));

	return graph;
}

} // namespace crimson
