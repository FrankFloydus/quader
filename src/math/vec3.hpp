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

/// Three-component single-precision vector.
struct Vec3 {
	/// X component.
	float x = 0.0F;
	/// Y component.
	float y = 0.0F;
	/// Z component.
	float z = 0.0F;
};

/**
 * Add two vectors component-wise.
 *
 * @param left First addend.
 * @param right Second addend.
 * @return Component-wise sum.
 */
[[nodiscard]] inline Vec3 operator+(Vec3 left, Vec3 right) noexcept {
	const auto kResult = detail::add(detail::Components3{ left.x, left.y, left.z },
			detail::Components3{ right.x, right.y, right.z });
	return Vec3{ kResult.x, kResult.y, kResult.z };
}

/**
 * Subtract two vectors component-wise.
 *
 * @param left Minuend.
 * @param right Subtrahend.
 * @return Component-wise difference.
 */
[[nodiscard]] inline Vec3 operator-(Vec3 left, Vec3 right) noexcept {
	const auto kResult = detail::subtract(detail::Components3{ left.x, left.y, left.z },
			detail::Components3{ right.x, right.y, right.z });
	return Vec3{ kResult.x, kResult.y, kResult.z };
}

/**
 * Scale a vector.
 *
 * @param value Vector to scale.
 * @param scale Scalar multiplier.
 * @return Scaled vector.
 */
[[nodiscard]] inline Vec3 operator*(Vec3 value, float scale) noexcept {
	const auto kResult = detail::scale(detail::Components3{ value.x, value.y, value.z }, scale);
	return Vec3{ kResult.x, kResult.y, kResult.z };
}

/**
 * Divide a vector by a scalar.
 *
 * @param value Vector to divide.
 * @param scale Scalar divisor.
 * @return Component-wise quotient.
 */
[[nodiscard]] inline Vec3 operator/(Vec3 value, float scale) noexcept {
	const auto kResult = detail::divide(detail::Components3{ value.x, value.y, value.z }, scale);
	return Vec3{ kResult.x, kResult.y, kResult.z };
}

/**
 * Compute a dot product.
 *
 * @param left First vector.
 * @param right Second vector.
 * @return Sum of component-wise products.
 */
[[nodiscard]] inline float dot(Vec3 left, Vec3 right) noexcept {
	return detail::dot(detail::Components3{ left.x, left.y, left.z },
			detail::Components3{ right.x, right.y, right.z });
}

/**
 * Compute a cross product.
 *
 * @param left First vector.
 * @param right Second vector.
 * @return Vector perpendicular to `left` and `right` using right-handed order.
 */
[[nodiscard]] inline Vec3 cross(Vec3 left, Vec3 right) noexcept {
	const auto kResult = detail::cross(detail::Components3{ left.x, left.y, left.z },
			detail::Components3{ right.x, right.y, right.z });
	return Vec3{ kResult.x, kResult.y, kResult.z };
}

/**
 * Compute squared vector length.
 *
 * @param value Vector to measure.
 * @return Dot product of `value` with itself.
 */
[[nodiscard]] inline float length_squared(Vec3 value) noexcept {
	return detail::length_squared(detail::Components3{ value.x, value.y, value.z });
}

/**
 * Compute vector length.
 *
 * @param value Vector to measure.
 * @return Euclidean length.
 */
[[nodiscard]] inline float length(Vec3 value) noexcept {
	return detail::length(detail::Components3{ value.x, value.y, value.z });
}

/**
 * Normalize a vector with a zero fallback.
 *
 * @param value Vector to normalize.
 * @param epsilon Length threshold treated as degenerate.
 * @return Unit vector, or the zero vector when `value` is degenerate.
 */
[[nodiscard]] inline Vec3 normalized(Vec3 value, float epsilon = kDefaultEpsilon) noexcept {
	const float kLen = length(value);
	if (kLen <= epsilon) {
		return {};
	}

	const auto kResult = detail::normalized(detail::Components3{ value.x, value.y, value.z });
	return Vec3{ kResult.x, kResult.y, kResult.z };
}

/**
 * Check whether every component is finite.
 *
 * @param value Vector to test.
 * @return True when all components are finite.
 */
[[nodiscard]] inline bool is_finite(Vec3 value) noexcept {
	return is_finite(value.x) && is_finite(value.y) && is_finite(value.z);
}

/**
 * Compare two vectors with an absolute component tolerance.
 *
 * @param left First vector.
 * @param right Second vector.
 * @param epsilon Maximum per-component absolute difference.
 * @return True when all component pairs are nearly equal.
 */
[[nodiscard]] inline bool nearly_equal(Vec3 left,
		Vec3 right,
		float epsilon = kDefaultEpsilon) noexcept {
	return nearly_equal(left.x, right.x, epsilon) && nearly_equal(left.y, right.y, epsilon) && nearly_equal(left.z, right.z, epsilon);
}

} // namespace quader::math
