#pragma once

#include "crimson/renderer_config.hpp"

#include <cstdint>

namespace crimson {

struct ViewportSettings {
	std::uint32_t clear_color_rgba8 = 0x020202ff;
	ToneMapper tone_mapper = ToneMapper::AcesFitted;
	float exposure_ev100 = 12.0F;
	float exposure_compensation_ev = 0.0F;
	bool draw_lit_surfaces = true;
	bool draw_grid_overlay = true;
	bool draw_overlays = true;
};

} // namespace crimson
