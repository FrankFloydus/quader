#pragma once

#include "crimson/renderer_types.hpp"

#include <cstdint>
#include <string>

namespace crimson {

enum class TextureColorSpace : std::uint8_t {
	Srgb,
	Linear,
	Data,
};

struct MaterialTextureSlotDesc {
	std::string name;
	TextureColorSpace color_space = TextureColorSpace::Linear;
	RenderTextureHandle default_texture;
	bool required = false;
};

struct MaterialTextureBinding {
	std::string name;
	RenderTextureHandle texture;
};

[[nodiscard]] const char *texture_color_space_name(TextureColorSpace color_space) noexcept;

} // namespace crimson
