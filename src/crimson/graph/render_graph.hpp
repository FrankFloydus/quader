/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
#pragma once

#include "crimson/frame/frame_targets.hpp"
#include "crimson/graph/render_pass.hpp"
#include "crimson/graph/render_resource_registry.hpp"
#include "foundation/result.hpp"

#include <span>
#include <string>
#include <vector>

namespace crimson {

/// Minimal render graph for named frame resources and passes.
class RenderGraph final {
public:
	/**
	 * Add a resource declaration.
	 *
	 * @param desc Resource descriptor to store.
	 * @return Success, or a validation error string.
	 */
	[[nodiscard]] quader::foundation::Result<void, std::string> add_resource(RenderResourceDesc desc);
	/**
	 * Add a pass declaration.
	 *
	 * @param pass Pass descriptor to store.
	 */
	void add_pass(RenderPass pass);

	/**
	 * Validate resource and pass declarations.
	 *
	 * @return Success, or a validation error string.
	 */
	[[nodiscard]] quader::foundation::Result<void, std::string> validate() const;
	/**
	 * Build a human-readable graph dump.
	 *
	 * @return Debug dump of resources and passes.
	 */
	[[nodiscard]] std::string debug_dump() const;

	/**
	 * Resize graph resources.
	 *
	 * @param extent New viewport extent.
	 */
	void resize(ViewportExtent extent);
	/// Remove all resources and passes.
	void clear() noexcept;

	/**
	 * Access the resource registry.
	 *
	 * @return Resource registry owned by the graph.
	 */
	[[nodiscard]] const RenderResourceRegistry &resources() const noexcept;
	/**
	 * Return all passes in insertion order.
	 *
	 * @return Borrowed pass span valid until graph mutation.
	 */
	[[nodiscard]] std::span<const RenderPass> passes() const noexcept;
	/**
	 * Return the current graph resize generation.
	 *
	 * @return Resource registry resize generation.
	 */
	[[nodiscard]] std::uint64_t resize_generation() const noexcept;

private:
	RenderResourceRegistry resources_;
	std::vector<RenderPass> passes_;
};

/**
 * Create the minimal viewport render graph.
 *
 * @param extent Viewport extent used by resize-dependent resources.
 * @return Render graph configured for the viewport path.
 */
[[nodiscard]] RenderGraph make_minimal_viewport_render_graph(ViewportExtent extent);
/**
 * Create the V1 correctness render graph.
 *
 * @param extent Viewport extent used by resize-dependent resources.
 * @param picking_format Picking target format.
 * @return Render graph configured for the V1 pass sequence.
 */
[[nodiscard]] RenderGraph make_v1_correctness_render_graph(
		ViewportExtent extent,
		PickingIdTargetFormat picking_format = PickingIdTargetFormat::R32Uint);

} // namespace crimson
