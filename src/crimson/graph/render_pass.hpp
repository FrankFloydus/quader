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

#include "crimson/graph/render_resource_desc.hpp"

#include <string>
#include <vector>

namespace crimson {

/// Resource access declared by one render pass.
struct ResourceUse {
	/// Name of the resource used by the pass.
	std::string resource_name;
	/// Access mode required by the pass.
	RenderResourceAccess access = RenderResourceAccess::Read;
};

/// CPU-side render graph pass declaration.
struct RenderPass {
	/// Unique pass name.
	std::string name;
	/// Resources read or written by the pass.
	std::vector<ResourceUse> resources;
};

/**
 * Create a CPU-only pass with no declared resources.
 *
 * @param name Pass name.
 * @return Render pass descriptor.
 */
[[nodiscard]] RenderPass make_cpu_pass(std::string name);
/**
 * Create a pass with declared resource uses.
 *
 * @param name Pass name.
 * @param resources Resource access declarations.
 * @return Render pass descriptor.
 */
[[nodiscard]] RenderPass make_pass(std::string name, std::vector<ResourceUse> resources);

} // namespace crimson
