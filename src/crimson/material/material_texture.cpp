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
