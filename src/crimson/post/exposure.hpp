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

#include "crimson/color/color_space.hpp"

namespace crimson {

/**
 * Convert camera EV100 and exposure compensation into a multiplier.
 *
 * @param ev100 Camera exposure value at ISO 100.
 * @param compensation_ev Exposure compensation in stops.
 * @return Linear multiplier applied to HDR scene color.
 */
[[nodiscard]] float exposure_multiplier_from_ev100(float ev100, float compensation_ev) noexcept;
/**
 * Apply exposure to an HDR color.
 *
 * @param hdr Linear HDR color.
 * @param exposure_multiplier Linear exposure multiplier.
 * @return Exposed linear color with alpha preserved.
 */
[[nodiscard]] ColorLinear apply_exposure(ColorLinear hdr, float exposure_multiplier) noexcept;

} // namespace crimson
