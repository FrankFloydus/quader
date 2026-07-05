#include "crimson/color/color_space.hpp"

#include <algorithm>
#include <cmath>

namespace crimson {
namespace {

[[nodiscard]] float finite_or_zero(float value) noexcept {
	return std::isfinite(value) ? value : 0.0F;
}

[[nodiscard]] float clamp01(float value) noexcept {
	return std::clamp(finite_or_zero(value), 0.0F, 1.0F);
}

[[nodiscard]] float srgb_channel_to_linear(float channel) noexcept {
	const float kSrgb = clamp01(channel);
	if (kSrgb <= 0.04045F) {
		return kSrgb / 12.92F;
	}

	return std::pow((kSrgb + 0.055F) / 1.055F, 2.4F);
}

[[nodiscard]] float linear_channel_to_srgb(float channel) noexcept {
	const float kLinear = clamp01(channel);
	if (kLinear <= 0.0031308F) {
		return kLinear * 12.92F;
	}

	return 1.055F * std::pow(kLinear, 1.0F / 2.4F) - 0.055F;
}

} // namespace

ColorLinear srgb_to_linear(ColorSrgb color) noexcept {
	return ColorLinear{
		.r = srgb_channel_to_linear(color.r),
		.g = srgb_channel_to_linear(color.g),
		.b = srgb_channel_to_linear(color.b),
		.a = clamp01(color.a),
	};
}

ColorSrgb linear_to_srgb(ColorLinear color) noexcept {
	return ColorSrgb{
		.r = linear_channel_to_srgb(color.r),
		.g = linear_channel_to_srgb(color.g),
		.b = linear_channel_to_srgb(color.b),
		.a = clamp01(color.a),
	};
}

TextureColorEncoding texture_color_encoding(TextureDataRole role) noexcept {
	switch (role) {
		case TextureDataRole::BaseColor:
		case TextureDataRole::Emissive:
			return TextureColorEncoding::Srgb;
		case TextureDataRole::MetallicRoughness:
		case TextureDataRole::Normal:
		case TextureDataRole::Occlusion:
		case TextureDataRole::PickingId:
		case TextureDataRole::Depth:
		case TextureDataRole::ShadowLike:
			return TextureColorEncoding::LinearData;
	}

	return TextureColorEncoding::LinearData;
}

bool texture_role_uses_srgb(TextureDataRole role) noexcept {
	return texture_color_encoding(role) == TextureColorEncoding::Srgb;
}

} // namespace crimson
