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

#include "math/vec3.hpp"

namespace quader::document {

/// Document object transform stored as translation, Euler rotation, and scale.
struct Transform {
	/// Translation in world units.
	quader::math::Vec3 translation;
	/// Euler rotation in degrees.
	quader::math::Vec3 rotation_euler;
	/// Non-uniform scale factors.
	quader::math::Vec3 scale{ 1.0F, 1.0F, 1.0F };
};

/**
 * Compare two vectors with exact component equality.
 *
 * @param left, right Vectors to compare.
 * @return True when all components match exactly.
 */
[[nodiscard]] constexpr bool vec3_exactly_equal(quader::math::Vec3 left,
		quader::math::Vec3 right) noexcept {
	return left.x == right.x && left.y == right.y && left.z == right.z;
}

/**
 * Compare two transforms with exact component equality.
 *
 * @param left, right Transforms to compare.
 * @return True when translation, rotation, and scale match exactly.
 */
[[nodiscard]] constexpr bool operator==(Transform left, Transform right) noexcept {
	return vec3_exactly_equal(left.translation, right.translation) && vec3_exactly_equal(left.rotation_euler, right.rotation_euler) && vec3_exactly_equal(left.scale, right.scale);
}

/**
 * Check whether every transform component is finite.
 *
 * @param transform Transform to validate.
 * @return True when translation, rotation, and scale contain only finite values.
 */
[[nodiscard]] inline bool is_finite(Transform transform) noexcept {
	return quader::math::is_finite(transform.translation) && quader::math::is_finite(transform.rotation_euler) && quader::math::is_finite(transform.scale);
}

} // namespace quader::document
