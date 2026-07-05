/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
#include "crimson/material/material_texture.hpp"

namespace crimson {

const char *texture_color_space_name(TextureColorSpace color_space) noexcept {
	switch (color_space) {
		case TextureColorSpace::Srgb:
			return "Srgb";
		case TextureColorSpace::Linear:
			return "Linear";
		case TextureColorSpace::Data:
			return "Data";
	}

	return "Unknown";
}

} // namespace crimson
