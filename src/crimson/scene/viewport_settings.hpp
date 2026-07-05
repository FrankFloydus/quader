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

#include "crimson/renderer_config.hpp"

#include <cstdint>

namespace crimson {

/// Per-viewport render settings.
struct ViewportSettings {
	/// Clear color encoded as RGBA8.
	std::uint32_t clear_color_rgba8 = 0x020202ff;
	/// Tone mapper used for display conversion.
	ToneMapper tone_mapper = ToneMapper::AcesFitted;
	/// Exposure value at ISO 100.
	float exposure_ev100 = 12.0F;
	/// Exposure compensation in stops.
	float exposure_compensation_ev = 0.0F;
	/// Whether lit surface queues should be drawn.
	bool draw_lit_surfaces = true;
	/// Whether the grid overlay should be drawn.
	bool draw_grid_overlay = true;
	/// Whether editor overlays should be drawn.
	bool draw_overlays = true;
};

} // namespace crimson
