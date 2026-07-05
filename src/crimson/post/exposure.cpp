#include "crimson/post/exposure.hpp"

#include <algorithm>
#include <cmath>

namespace crimson {

float exposure_multiplier_from_ev100(float ev100, float compensation_ev) noexcept
{
    const float safe_ev100 = std::isfinite(ev100) ? std::clamp(ev100, -24.0F, 24.0F) : 0.0F;
    const float safe_compensation = std::isfinite(compensation_ev)
        ? std::clamp(compensation_ev, -16.0F, 16.0F)
        : 0.0F;
    const float max_luminance = 1.2F * std::pow(2.0F, safe_ev100);
    return std::pow(2.0F, safe_compensation) / std::max(max_luminance, 0.000001F);
}

ColorLinear apply_exposure(ColorLinear hdr, float exposure_multiplier) noexcept
{
    const float multiplier = std::isfinite(exposure_multiplier) ? std::max(0.0F, exposure_multiplier) : 0.0F;
    return ColorLinear{
        .r = hdr.r * multiplier,
        .g = hdr.g * multiplier,
        .b = hdr.b * multiplier,
        .a = hdr.a,
    };
}

} // namespace crimson
