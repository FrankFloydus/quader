#pragma once

#include <cstdint>

namespace crimson {

struct ColorSrgb {
    float r = 0.0F;
    float g = 0.0F;
    float b = 0.0F;
    float a = 1.0F;
};

struct ColorLinear {
    float r = 0.0F;
    float g = 0.0F;
    float b = 0.0F;
    float a = 1.0F;
};

enum class TextureDataRole : std::uint8_t {
    BaseColor,
    Emissive,
    MetallicRoughness,
    Normal,
    Occlusion,
    PickingId,
    Depth,
    ShadowLike,
};

enum class TextureColorEncoding : std::uint8_t {
    LinearData,
    Srgb,
};

[[nodiscard]] ColorLinear srgb_to_linear(ColorSrgb color) noexcept;
[[nodiscard]] ColorSrgb linear_to_srgb(ColorLinear color) noexcept;
[[nodiscard]] TextureColorEncoding texture_color_encoding(TextureDataRole role) noexcept;
[[nodiscard]] bool texture_role_uses_srgb(TextureDataRole role) noexcept;

} // namespace crimson
