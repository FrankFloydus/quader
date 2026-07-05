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

#include "crimson/renderer_types.hpp"

#include <cstdint>
#include <string>

namespace crimson {

/// Color-space interpretation for material texture sampling.
enum class TextureColorSpace : std::uint8_t {
	Srgb,   ///< sRGB color data.
	Linear, ///< Linear scalar/vector data.
	Data,   ///< Non-color data such as normal maps or ids.
};

/// Schema entry for one material texture slot.
struct MaterialTextureSlotDesc {
	/// Slot name used for lookup and material bindings.
	std::string name;
	/// Required texture color-space interpretation.
	TextureColorSpace color_space = TextureColorSpace::Linear;
	/// Default texture handle used when the binding is omitted.
	RenderTextureHandle default_texture;
	/// True when material instances must bind this slot.
	bool required = false;
};

/// Concrete texture binding keyed by slot name.
struct MaterialTextureBinding {
	/// Texture slot name.
	std::string name;
	/// Texture handle bound to the slot.
	RenderTextureHandle texture;
};

/// Return the stable debug name for a texture color-space policy.
[[nodiscard]] const char *texture_color_space_name(TextureColorSpace color_space) noexcept;

} // namespace crimson
