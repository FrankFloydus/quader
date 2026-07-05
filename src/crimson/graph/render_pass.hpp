#pragma once

#include "crimson/graph/render_resource_desc.hpp"

#include <string>
#include <vector>

namespace crimson {

struct ResourceUse {
    std::string resource_name;
    RenderResourceAccess access = RenderResourceAccess::Read;
};

struct RenderPass {
    std::string name;
    std::vector<ResourceUse> resources;
};

[[nodiscard]] RenderPass make_cpu_pass(std::string name);
[[nodiscard]] RenderPass make_pass(std::string name, std::vector<ResourceUse> resources);

} // namespace crimson
