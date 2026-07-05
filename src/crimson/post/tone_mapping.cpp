/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
#include "crimson/post/tone_mapping.hpp"

#include <algorithm>
#include <cmath>

namespace crimson {
namespace {

[[nodiscard]] float safe_hdr(float value) noexcept {
	return std::isfinite(value) ? std::max(0.0F, value) : 0.0F;
}

[[nodiscard]] float saturate(float value) noexcept {
	return std::clamp(std::isfinite(value) ? value : 0.0F, 0.0F, 1.0F);
}

[[nodiscard]] float aces_fitted(float value) noexcept {
	const float kX = safe_hdr(value);
	constexpr float kA = 2.51F;
	constexpr float kB = 0.03F;
	constexpr float kC = 2.43F;
	constexpr float kD = 0.59F;
	constexpr float kE = 0.14F;
	return saturate((kX * (kA * kX + kB)) / (kX * (kC * kX + kD) + kE));
}

[[nodiscard]] float agx_approx(float value) noexcept {
	const float kX = safe_hdr(value);
	return saturate(1.0F - std::exp(-kX * 0.84F));
}

[[nodiscard]] float reinhard(float value) noexcept {
	const float kX = safe_hdr(value);
	return saturate(kX / (1.0F + kX));
}

[[nodiscard]] float tone_map_channel(ToneMapper mapper, float value) noexcept {
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

ColorLinear apply_tone_mapper(ToneMapper mapper, ColorLinear hdr) noexcept {
	return ColorLinear{
		.r = tone_map_channel(mapper, hdr.r),
		.g = tone_map_channel(mapper, hdr.g),
		.b = tone_map_channel(mapper, hdr.b),
		.a = saturate(hdr.a),
	};
}

std::optional<DisplayConversionPath> choose_display_conversion_path(
		bool srgb_backbuffer_supported,
		DisplayConversionSettings settings) noexcept {
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
