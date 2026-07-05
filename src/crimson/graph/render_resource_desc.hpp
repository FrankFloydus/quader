#pragma once

#include "crimson/renderer_types.hpp"

#include <cstdint>
#include <string>

namespace crimson {

enum class RenderResourceFormat {
    BackbufferColor,
    BackbufferDepth,
    Rgba8Unorm,
    Rgba16Float,
    D24S8,
    D32Float,
    R32Uint,
};

enum class RenderResourceAccess {
    Read,
    Write,
    ReadWrite,
};

struct RenderResourceDesc {
    std::string name;
    RenderResourceFormat format = RenderResourceFormat::Rgba8Unorm;
    ViewportExtent extent;
    bool external = false;
    bool resize_dependent = true;
    std::uint8_t sample_count = 1;
};

struct RenderResourceRecord {
    RenderResourceDesc desc;
    std::uint64_t generation = 1;
};

[[nodiscard]] const char* render_resource_format_name(RenderResourceFormat format) noexcept;
[[nodiscard]] const char* render_resource_access_name(RenderResourceAccess access) noexcept;

} // namespace crimson
