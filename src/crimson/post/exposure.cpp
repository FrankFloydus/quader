/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
#include "crimson/post/exposure.hpp"

#include <algorithm>
#include <cmath>

namespace crimson {

float exposure_multiplier_from_ev100(float ev100, float compensation_ev) noexcept {
	const float kSafeEv100 = std::isfinite(ev100) ? std::clamp(ev100, -24.0F, 24.0F) : 0.0F;
	const float kSafeCompensation = std::isfinite(compensation_ev)
			? std::clamp(compensation_ev, -16.0F, 16.0F)
			: 0.0F;
	const float kMaxLuminance = 1.2F * std::pow(2.0F, kSafeEv100);
	return std::pow(2.0F, kSafeCompensation) / std::max(kMaxLuminance, 0.000001F);
}

ColorLinear apply_exposure(ColorLinear hdr, float exposure_multiplier) noexcept {
	const float kMultiplier = std::isfinite(exposure_multiplier) ? std::max(0.0F, exposure_multiplier) : 0.0F;
	return ColorLinear{
		.r = hdr.r * kMultiplier,
		.g = hdr.g * kMultiplier,
		.b = hdr.b * kMultiplier,
		.a = hdr.a,
	};
}

} // namespace crimson
