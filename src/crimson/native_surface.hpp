#pragma once

#include <string_view>

#include "crimson/renderer_types.hpp"

namespace crimson {

enum class NativeSurfacePlatform {
    Unknown,
    Windows,
    LinuxX11,
    LinuxWayland,
    MacOS,
};

struct NativeSurfaceDescriptor {
    NativeSurfacePlatform platform = NativeSurfacePlatform::Unknown;
    void* native_window_handle = nullptr;
    void* native_display_handle = nullptr;
    void* native_layer_handle = nullptr;
    ViewportExtent extent;
};

constexpr std::string_view native_surface_platform_name(NativeSurfacePlatform platform) noexcept
{
    switch (platform) {
    case NativeSurfacePlatform::Unknown:
        return "Unknown";
    case NativeSurfacePlatform::Windows:
        return "Windows";
    case NativeSurfacePlatform::LinuxX11:
        return "LinuxX11";
    case NativeSurfacePlatform::LinuxWayland:
        return "LinuxWayland";
    case NativeSurfacePlatform::MacOS:
        return "MacOS";
    }

    return "Unknown";
}

constexpr bool is_valid_native_surface_descriptor(const NativeSurfaceDescriptor& surface) noexcept
{
    return surface.platform != NativeSurfacePlatform::Unknown
        && surface.native_window_handle != nullptr
        && is_valid_extent(surface.extent);
}

} // namespace crimson
