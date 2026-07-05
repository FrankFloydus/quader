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

namespace crimson::gpu {

/// Screen-space origin convention reported by the graphics runtime.
enum class ScreenOrigin {
	/// Texture coordinate origin is the top-left corner.
	TopLeft,
	/// Texture coordinate origin is the bottom-left corner.
	BottomLeft,
};

/// UV range used by the fullscreen triangle shader.
struct FullscreenTriangleUvRange {
	/// Minimum U coordinate.
	float min_u;
	/// Maximum U coordinate.
	float max_u;
	/// Minimum V coordinate.
	float min_v;
	/// Maximum V coordinate.
	float max_v;
};

/**
 * Convert bgfx origin policy into Crimson screen origin.
 *
 * @param origin_bottom_left bgfx origin flag.
 * @return Matching screen origin.
 */
[[nodiscard]] constexpr ScreenOrigin screen_origin_from_bgfx(bool origin_bottom_left) noexcept {
	return origin_bottom_left ? ScreenOrigin::BottomLeft : ScreenOrigin::TopLeft;
}

/**
 * Return fullscreen-triangle UVs for a screen-origin convention.
 *
 * @param origin Screen origin convention.
 * @return UV range for the fullscreen triangle.
 */
[[nodiscard]] constexpr FullscreenTriangleUvRange fullscreen_triangle_uv_range(ScreenOrigin origin) noexcept {
	if (origin == ScreenOrigin::BottomLeft) {
		return FullscreenTriangleUvRange{
			.min_u = -1.0F,
			.max_u = 1.0F,
			.min_v = 1.0F,
			.max_v = -1.0F,
		};
	}

	return FullscreenTriangleUvRange{
		.min_u = -1.0F,
		.max_u = 1.0F,
		.min_v = 0.0F,
		.max_v = 2.0F,
	};
}

} // namespace crimson::gpu
