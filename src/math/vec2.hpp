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

#include "math/detail/glm_adapter.hpp"
#include "math/scalar.hpp"

namespace quader::math {

/// Two-component single-precision vector.
struct Vec2 {
	/// X component.
	float x = 0.0F;
	/// Y component.
	float y = 0.0F;
};

/**
 * Add two vectors component-wise.
 *
 * @param left First addend.
 * @param right Second addend.
 * @return Component-wise sum.
 */
[[nodiscard]] inline Vec2 operator+(Vec2 left, Vec2 right) noexcept {
	const auto kResult = detail::add(detail::Components2{ left.x, left.y }, detail::Components2{ right.x, right.y });
	return Vec2{ kResult.x, kResult.y };
}

/**
 * Subtract two vectors component-wise.
 *
 * @param left Minuend.
 * @param right Subtrahend.
 * @return Component-wise difference.
 */
[[nodiscard]] inline Vec2 operator-(Vec2 left, Vec2 right) noexcept {
	const auto kResult = detail::subtract(detail::Components2{ left.x, left.y }, detail::Components2{ right.x, right.y });
	return Vec2{ kResult.x, kResult.y };
}

/**
 * Scale a vector.
 *
 * @param value Vector to scale.
 * @param scale Scalar multiplier.
 * @return Scaled vector.
 */
[[nodiscard]] inline Vec2 operator*(Vec2 value, float scale) noexcept {
	const auto kResult = detail::scale(detail::Components2{ value.x, value.y }, scale);
	return Vec2{ kResult.x, kResult.y };
}

/**
 * Compute a dot product.
 *
 * @param left First vector.
 * @param right Second vector.
 * @return Sum of component-wise products.
 */
[[nodiscard]] inline float dot(Vec2 left, Vec2 right) noexcept {
	return detail::dot(detail::Components2{ left.x, left.y }, detail::Components2{ right.x, right.y });
}

/**
 * Compute squared vector length.
 *
 * @param value Vector to measure.
 * @return Dot product of `value` with itself.
 */
[[nodiscard]] inline float length_squared(Vec2 value) noexcept {
	return detail::length_squared(detail::Components2{ value.x, value.y });
}

/**
 * Check whether every component is finite.
 *
 * @param value Vector to test.
 * @return True when both components are finite.
 */
[[nodiscard]] inline bool is_finite(Vec2 value) noexcept {
	return is_finite(value.x) && is_finite(value.y);
}

/**
 * Compare two vectors with an absolute component tolerance.
 *
 * @param left First vector.
 * @param right Second vector.
 * @param epsilon Maximum per-component absolute difference.
 * @return True when both component pairs are nearly equal.
 */
[[nodiscard]] inline bool nearly_equal(Vec2 left,
		Vec2 right,
		float epsilon = kDefaultEpsilon) noexcept {
	return nearly_equal(left.x, right.x, epsilon) && nearly_equal(left.y, right.y, epsilon);
}

} // namespace quader::math
