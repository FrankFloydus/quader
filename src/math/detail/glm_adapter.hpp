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

#include "math/detail/glm_config.hpp"

#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace quader::math::detail {

/// Two-component scalar payload used at the Quader/GLM boundary.
struct Components2 {
	/// X component.
	float x = 0.0F;
	/// Y component.
	float y = 0.0F;
};

/// Three-component scalar payload used at the Quader/GLM boundary.
struct Components3 {
	/// X component.
	float x = 0.0F;
	/// Y component.
	float y = 0.0F;
	/// Z component.
	float z = 0.0F;
};

/**
 * Convert two scalar components to a GLM vector.
 *
 * @param value Components to convert.
 * @return GLM vector with matching coordinates.
 */
[[nodiscard]] inline glm::vec2 to_glm(Components2 value) noexcept {
	return glm::vec2{ value.x, value.y };
}

/**
 * Convert three scalar components to a GLM vector.
 *
 * @param value Components to convert.
 * @return GLM vector with matching coordinates.
 */
[[nodiscard]] inline glm::vec3 to_glm(Components3 value) noexcept {
	return glm::vec3{ value.x, value.y, value.z };
}

/**
 * Convert a GLM two-vector to scalar components.
 *
 * @param value GLM vector to convert.
 * @return Components with matching coordinates.
 */
[[nodiscard]] inline Components2 from_glm(glm::vec2 value) noexcept {
	return Components2{ value.x, value.y };
}

/**
 * Convert a GLM three-vector to scalar components.
 *
 * @param value GLM vector to convert.
 * @return Components with matching coordinates.
 */
[[nodiscard]] inline Components3 from_glm(glm::vec3 value) noexcept {
	return Components3{ value.x, value.y, value.z };
}

/**
 * Add two component vectors.
 *
 * @param left First addend.
 * @param right Second addend.
 * @return Component-wise sum.
 */
[[nodiscard]] inline Components2 add(Components2 left, Components2 right) noexcept {
	return from_glm(to_glm(left) + to_glm(right));
}

/**
 * Add two component vectors.
 *
 * @param left First addend.
 * @param right Second addend.
 * @return Component-wise sum.
 */
[[nodiscard]] inline Components3 add(Components3 left, Components3 right) noexcept {
	return from_glm(to_glm(left) + to_glm(right));
}

/**
 * Subtract one component vector from another.
 *
 * @param left Minuend.
 * @param right Subtrahend.
 * @return Component-wise difference.
 */
[[nodiscard]] inline Components2 subtract(Components2 left, Components2 right) noexcept {
	return from_glm(to_glm(left) - to_glm(right));
}

/**
 * Subtract one component vector from another.
 *
 * @param left Minuend.
 * @param right Subtrahend.
 * @return Component-wise difference.
 */
[[nodiscard]] inline Components3 subtract(Components3 left, Components3 right) noexcept {
	return from_glm(to_glm(left) - to_glm(right));
}

/**
 * Multiply a component vector by a scalar.
 *
 * @param value Vector to scale.
 * @param scalar Scale factor.
 * @return Scaled vector.
 */
[[nodiscard]] inline Components2 scale(Components2 value, float scalar) noexcept {
	return from_glm(to_glm(value) * scalar);
}

/**
 * Multiply a component vector by a scalar.
 *
 * @param value Vector to scale.
 * @param scalar Scale factor.
 * @return Scaled vector.
 */
[[nodiscard]] inline Components3 scale(Components3 value, float scalar) noexcept {
	return from_glm(to_glm(value) * scalar);
}

/**
 * Divide a component vector by a scalar.
 *
 * @param value Vector to divide.
 * @param scalar Divisor.
 * @return Divided vector.
 */
[[nodiscard]] inline Components3 divide(Components3 value, float scalar) noexcept {
	return from_glm(to_glm(value) / scalar);
}

/**
 * Compute the dot product of two component vectors.
 *
 * @param left First vector.
 * @param right Second vector.
 * @return Dot product.
 */
[[nodiscard]] inline float dot(Components2 left, Components2 right) noexcept {
	return glm::dot(to_glm(left), to_glm(right));
}

/**
 * Compute the dot product of two component vectors.
 *
 * @param left First vector.
 * @param right Second vector.
 * @return Dot product.
 */
[[nodiscard]] inline float dot(Components3 left, Components3 right) noexcept {
	return glm::dot(to_glm(left), to_glm(right));
}

/**
 * Compute the cross product of two three-component vectors.
 *
 * @param left First vector.
 * @param right Second vector.
 * @return Cross product.
 */
[[nodiscard]] inline Components3 cross(Components3 left, Components3 right) noexcept {
	return from_glm(glm::cross(to_glm(left), to_glm(right)));
}

/**
 * Compute squared vector length.
 *
 * @param value Vector to measure.
 * @return Squared Euclidean length.
 */
[[nodiscard]] inline float length_squared(Components2 value) noexcept {
	return dot(value, value);
}

/**
 * Compute squared vector length.
 *
 * @param value Vector to measure.
 * @return Squared Euclidean length.
 */
[[nodiscard]] inline float length_squared(Components3 value) noexcept {
	return dot(value, value);
}

/**
 * Compute vector length.
 *
 * @param value Vector to measure.
 * @return Euclidean length.
 */
[[nodiscard]] inline float length(Components3 value) noexcept {
	return glm::length(to_glm(value));
}

/**
 * Normalize a vector using GLM's normalization semantics.
 *
 * @param value Vector to normalize.
 * @return Normalized vector.
 */
[[nodiscard]] inline Components3 normalized(Components3 value) noexcept {
	return from_glm(glm::normalize(to_glm(value)));
}

/**
 * Compute component-wise minima.
 *
 * @param left First vector.
 * @param right Second vector.
 * @return Component-wise minimum.
 */
[[nodiscard]] inline Components3 component_min(Components3 left, Components3 right) noexcept {
	return from_glm(glm::min(to_glm(left), to_glm(right)));
}

/**
 * Compute component-wise maxima.
 *
 * @param left First vector.
 * @param right Second vector.
 * @return Component-wise maximum.
 */
[[nodiscard]] inline Components3 component_max(Components3 left, Components3 right) noexcept {
	return from_glm(glm::max(to_glm(left), to_glm(right)));
}

} // namespace quader::math::detail
