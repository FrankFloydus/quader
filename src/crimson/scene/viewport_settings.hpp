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

#include <cstdint>

namespace crimson {

/// Per-viewport render settings.
struct ViewportSettings {
	/// Clear color encoded as RGBA8.
	std::uint32_t clear_color_rgba8 = 0x020202ff;
	/// Tone mapper used for display conversion.
	ToneMapper tone_mapper = ToneMapper::Linear;
	/// Exposure value at ISO 100.
	float exposure_ev100 = 0.0F;
	/// Exposure compensation in stops.
	float exposure_compensation_ev = 0.0F;
	/// Whether lit surface queues should be drawn.
	bool draw_lit_surfaces = true;
	/// Whether the grid overlay should be drawn.
	bool draw_grid_overlay = true;
	/// Whether editor overlays should be drawn.
	bool draw_overlays = true;
	/// Whether the mesh material surface grid should be mixed into mesh color.
	bool draw_mesh_grid = false;
	/// Minor mesh surface grid color in sRGB UI space.
	ColorSrgb surface_grid_minor_color{ 0.02F, 0.02F, 0.02F, 1.0F };
	/// Major mesh surface grid color in sRGB UI space.
	ColorSrgb surface_grid_major_color{ 0.02F, 0.02F, 0.02F, 1.0F };
	/// Minor mesh surface grid spacing in world units.
	float surface_grid_size_m = 1.0F;
	/// Major mesh surface grid spacing in world units.
	float surface_grid_major_size_m = 4.0F;
	/// Minor mesh surface grid line thickness multiplier.
	float surface_grid_minor_line_thickness = 0.325F;
	/// Major mesh surface grid line thickness multiplier.
	float surface_grid_major_line_thickness = 0.250F;
};

} // namespace crimson
