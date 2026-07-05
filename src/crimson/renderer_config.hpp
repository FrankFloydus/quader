#pragma once

#include <cstdint>
#include <filesystem>
#include <string_view>

namespace crimson {

enum class GraphicsBackendPreference {
    Auto,
    Vulkan,
    Metal,
    Direct3D12,
    Direct3D11,
};

enum class ToneMapper : std::uint8_t {
    AcesFitted,
    AgxApprox,
    Reinhard,
    Linear,
};

constexpr std::string_view graphics_backend_preference_name(GraphicsBackendPreference preference) noexcept
{
    switch (preference) {
    case GraphicsBackendPreference::Auto:
        return "Auto";
    case GraphicsBackendPreference::Vulkan:
        return "Vulkan";
    case GraphicsBackendPreference::Metal:
        return "Metal";
    case GraphicsBackendPreference::Direct3D12:
        return "Direct3D12";
    case GraphicsBackendPreference::Direct3D11:
        return "Direct3D11";
    }

    return "Unknown";
}

constexpr std::string_view tone_mapper_name(ToneMapper tone_mapper) noexcept
{
    switch (tone_mapper) {
    case ToneMapper::AcesFitted:
        return "AcesFitted";
    case ToneMapper::AgxApprox:
        return "AgxApprox";
    case ToneMapper::Reinhard:
        return "Reinhard";
    case ToneMapper::Linear:
        return "Linear";
    }

    return "Unknown";
}

struct DisplayConversionSettings {
    bool prefer_srgb_backbuffer = true;
    bool allow_manual_final_srgb = true;
};

struct RendererConfig {
    GraphicsBackendPreference backend_preference = GraphicsBackendPreference::Auto;
    std::filesystem::path shader_root;
    bool vsync = true;
    bool enable_debug_text = true;
    ToneMapper default_tone_mapper = ToneMapper::AcesFitted;
    DisplayConversionSettings display_conversion;
};

} // namespace crimson
