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

#include <cmath>

namespace quader::math {

/// Default tolerance for scalar and vector approximate comparisons.
constexpr float kDefaultEpsilon = 1.0e-6F;

/**
 * Compare two scalar values with an absolute tolerance.
 *
 * @param left First value to compare.
 * @param right Second value to compare.
 * @param epsilon Maximum absolute difference accepted as equal.
 * @return True when the absolute difference is no greater than `epsilon`.
 */
[[nodiscard]] inline bool nearly_equal(float left,
		float right,
		float epsilon = kDefaultEpsilon) noexcept {
	return std::fabs(left - right) <= epsilon;
}

/**
 * Check whether a scalar is within tolerance of zero.
 *
 * @param value Value to test.
 * @param epsilon Maximum absolute value accepted as zero.
 * @return True when `value` is in `[-epsilon, epsilon]`.
 */
[[nodiscard]] inline bool is_near_zero(float value, float epsilon = kDefaultEpsilon) noexcept {
	return std::fabs(value) <= epsilon;
}

/**
 * Check whether a scalar is finite.
 *
 * @param value Value to test.
 * @return True when `value` is neither NaN nor infinity.
 */
[[nodiscard]] inline bool is_finite(float value) noexcept {
	return std::isfinite(value);
}

} // namespace quader::math
