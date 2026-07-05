#include "crimson/post/tone_mapping.hpp"

#include <algorithm>
#include <cmath>

namespace crimson {
namespace {

[[nodiscard]] float safe_hdr(float value) noexcept
{
    return std::isfinite(value) ? std::max(0.0F, value) : 0.0F;
}

[[nodiscard]] float saturate(float value) noexcept
{
    return std::clamp(std::isfinite(value) ? value : 0.0F, 0.0F, 1.0F);
}

[[nodiscard]] float aces_fitted(float value) noexcept
{
    const float x = safe_hdr(value);
    constexpr float a = 2.51F;
    constexpr float b = 0.03F;
    constexpr float c = 2.43F;
    constexpr float d = 0.59F;
    constexpr float e = 0.14F;
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

[[nodiscard]] float agx_approx(float value) noexcept
{
    const float x = safe_hdr(value);
    return saturate(1.0F - std::exp(-x * 0.84F));
}

[[nodiscard]] float reinhard(float value) noexcept
{
    const float x = safe_hdr(value);
    return saturate(x / (1.0F + x));
}

[[nodiscard]] float tone_map_channel(ToneMapper mapper, float value) noexcept
{
    switch (mapper) {
    case ToneMapper::AcesFitted:
        return aces_fitted(value);
    case ToneMapper::AgxApprox:
        return agx_approx(value);
    case ToneMapper::Reinhard:
        return reinhard(value);
    case ToneMapper::Linear:
        return saturate(value);
    }

    return saturate(value);
}

} // namespace

ColorLinear apply_tone_mapper(ToneMapper mapper, ColorLinear hdr) noexcept
{
    return ColorLinear{
        .r = tone_map_channel(mapper, hdr.r),
        .g = tone_map_channel(mapper, hdr.g),
        .b = tone_map_channel(mapper, hdr.b),
        .a = saturate(hdr.a),
    };
}

std::optional<DisplayConversionPath> choose_display_conversion_path(
    bool srgb_backbuffer_supported,
    DisplayConversionSettings settings) noexcept
{
    if (srgb_backbuffer_supported && settings.prefer_srgb_backbuffer) {
        return DisplayConversionPath::SrgbBackbuffer;
    }
    if (settings.allow_manual_final_srgb) {
        return DisplayConversionPath::ManualFinalShader;
    }
    if (srgb_backbuffer_supported) {
        return DisplayConversionPath::SrgbBackbuffer;
    }
    return std::nullopt;
}

} // namespace crimson
