#pragma once

#include <cmath>

namespace quader::math {

constexpr float kDefaultEpsilon = 1.0e-6F;

[[nodiscard]] inline bool nearly_equal(float left,
		float right,
		float epsilon = kDefaultEpsilon) noexcept {
	return std::fabs(left - right) <= epsilon;
}

[[nodiscard]] inline bool is_near_zero(float value, float epsilon = kDefaultEpsilon) noexcept {
	return std::fabs(value) <= epsilon;
}

[[nodiscard]] inline bool is_finite(float value) noexcept {
	return std::isfinite(value);
}

} // namespace quader::math
