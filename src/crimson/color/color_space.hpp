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

#include <cstdint>

namespace crimson {

/// Color stored in sRGB transfer space.
struct ColorSrgb {
	/// Red channel.
	float r = 0.0F;
	/// Green channel.
	float g = 0.0F;
	/// Blue channel.
	float b = 0.0F;
	/// Alpha channel, passed through by color-space conversions.
	float a = 1.0F;
};

/// Color stored in linear light space.
struct ColorLinear {
	/// Red channel.
	float r = 0.0F;
	/// Green channel.
	float g = 0.0F;
	/// Blue channel.
	float b = 0.0F;
	/// Alpha channel, passed through by color-space conversions.
	float a = 1.0F;
};

/// Semantic texture role used to choose sampling color encoding.
enum class TextureDataRole : std::uint8_t {
	/// Base-color texture data.
	BaseColor,
	/// Emissive texture data.
	Emissive,
	/// Packed metallic/roughness texture data.
	MetallicRoughness,
	/// Normal-map texture data.
	Normal,
	/// Ambient-occlusion texture data.
	Occlusion,
	/// Picking identifier texture data.
	PickingId,
	/// Depth texture data.
	Depth,
	/// Shadow-map-like texture data.
	ShadowLike,
};

/// Texture color encoding requested from the graphics runtime.
enum class TextureColorEncoding : std::uint8_t {
	/// Treat texture channels as linear data.
	LinearData,
	/// Sample texture with sRGB-to-linear conversion.
	Srgb,
};

/**
 * Convert an sRGB color to linear light.
 *
 * @param color Color in sRGB transfer space.
 * @return Linear-light color with alpha preserved.
 */
[[nodiscard]] ColorLinear srgb_to_linear(ColorSrgb color) noexcept;
/**
 * Convert a linear-light color to sRGB.
 *
 * @param color Color in linear light.
 * @return sRGB color with alpha preserved.
 */
[[nodiscard]] ColorSrgb linear_to_srgb(ColorLinear color) noexcept;
/**
 * Return the texture encoding for a semantic data role.
 *
 * @param role Texture data role.
 * @return Required texture color encoding.
 */
[[nodiscard]] TextureColorEncoding texture_color_encoding(TextureDataRole role) noexcept;
/**
 * Check whether a texture role uses sRGB sampling.
 *
 * @param role Texture data role.
 * @return True when the role should be sampled as sRGB.
 */
[[nodiscard]] bool texture_role_uses_srgb(TextureDataRole role) noexcept;

} // namespace crimson
