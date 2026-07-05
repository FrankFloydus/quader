#pragma once

#include "crimson/color/color_space.hpp"

namespace crimson {

[[nodiscard]] float exposure_multiplier_from_ev100(float ev100, float compensation_ev) noexcept;
[[nodiscard]] ColorLinear apply_exposure(ColorLinear hdr, float exposure_multiplier) noexcept;

} // namespace crimson
