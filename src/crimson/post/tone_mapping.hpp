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
#include "crimson/renderer_config.hpp"

#include <optional>

namespace crimson {

/// Final display conversion path selected for the active backend.
enum class DisplayConversionPath {
	/// Hardware sRGB conversion on the backbuffer.
	SrgbBackbuffer,
	/// Manual final shader conversion from linear to sRGB.
	ManualFinalShader,
};

/**
 * Apply a tone mapper to a linear HDR color.
 *
 * @param mapper Tone mapper to apply.
 * @param hdr Linear HDR input color.
 * @return Tone-mapped linear SDR color with alpha preserved.
 */
[[nodiscard]] ColorLinear apply_tone_mapper(ToneMapper mapper, ColorLinear hdr) noexcept;
/**
 * Choose the display conversion path for backend capabilities and settings.
 *
 * @param srgb_backbuffer_supported Whether the active backend supports sRGB backbuffers.
 * @param settings Display conversion preferences.
 * @return Selected path, or empty when neither path is allowed.
 */
[[nodiscard]] std::optional<DisplayConversionPath> choose_display_conversion_path(
		bool srgb_backbuffer_supported,
		DisplayConversionSettings settings) noexcept;

} // namespace crimson
