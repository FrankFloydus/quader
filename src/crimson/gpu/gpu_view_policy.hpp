#pragma once

namespace crimson::gpu {

enum class ScreenOrigin {
	TopLeft,
	BottomLeft,
};

struct FullscreenTriangleUvRange {
	float min_u;
	float max_u;
	float min_v;
	float max_v;
};

[[nodiscard]] constexpr ScreenOrigin screen_origin_from_bgfx(bool origin_bottom_left) noexcept {
	return origin_bottom_left ? ScreenOrigin::BottomLeft : ScreenOrigin::TopLeft;
}

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
