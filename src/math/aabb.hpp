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
#include "math/vec3.hpp"

#include <limits>

namespace quader::math {

/// Axis-aligned bounding box with an empty default state.
struct Aabb {
	/// Minimum corner; starts at positive infinity for empty bounds.
	Vec3 min{
		std::numeric_limits<float>::infinity(),
		std::numeric_limits<float>::infinity(),
		std::numeric_limits<float>::infinity(),
	};
	/// Maximum corner; starts at negative infinity for empty bounds.
	Vec3 max{
		-std::numeric_limits<float>::infinity(),
		-std::numeric_limits<float>::infinity(),
		-std::numeric_limits<float>::infinity(),
	};
};

/**
 * Check whether bounds contain no points.
 *
 * @param bounds Bounds to test.
 * @return True when any minimum component is greater than its maximum.
 */
[[nodiscard]] inline bool empty(Aabb bounds) noexcept {
	return bounds.min.x > bounds.max.x || bounds.min.y > bounds.max.y || bounds.min.z > bounds.max.z;
}

/**
 * Expand bounds to include a point.
 *
 * @param[in, out] bounds Bounds to mutate.
 * @param point Point included in the resulting bounds.
 */
inline void expand(Aabb &bounds, Vec3 point) noexcept {
	const auto kMinResult = detail::component_min(detail::Components3{ bounds.min.x, bounds.min.y, bounds.min.z },
			detail::Components3{ point.x, point.y, point.z });
	const auto kMaxResult = detail::component_max(detail::Components3{ bounds.max.x, bounds.max.y, bounds.max.z },
			detail::Components3{ point.x, point.y, point.z });
	bounds.min = Vec3{ kMinResult.x, kMinResult.y, kMinResult.z };
	bounds.max = Vec3{ kMaxResult.x, kMaxResult.y, kMaxResult.z };
}

/**
 * Compute the center point of bounds.
 *
 * @param bounds Bounds to sample.
 * @return Midpoint between `bounds.min` and `bounds.max`.
 */
[[nodiscard]] inline Vec3 center(Aabb bounds) noexcept {
	return (bounds.min + bounds.max) * 0.5F;
}

} // namespace quader::math
