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

#include "math/scalar.hpp"
#include "math/vec3.hpp"

#include <span>

namespace quader::geometry {

/**
 * Check whether all points contain finite coordinates.
 *
 * @param points Points to test.
 * @return True when every point has finite `x`, `y`, and `z` components.
 */
[[nodiscard]] inline bool all_points_finite(std::span<const quader::math::Vec3> points) noexcept {
	for (const auto kPoint : points) {
		if (!quader::math::is_finite(kPoint)) {
			return false;
		}
	}

	return true;
}

} // namespace quader::geometry
