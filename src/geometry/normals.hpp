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
 * Compute an unnormalized triangle normal.
 *
 * @param a First triangle vertex.
 * @param b Second triangle vertex.
 * @param c Third triangle vertex.
 * @return Cross product `(b - a) x (c - a)`.
 */
[[nodiscard]] inline quader::math::Vec3 triangle_normal(quader::math::Vec3 a,
		quader::math::Vec3 b,
		quader::math::Vec3 c) noexcept {
	return quader::math::cross(b - a, c - a);
}

/**
 * Compute an unnormalized polygon normal with Newell accumulation.
 *
 * @param points Polygon vertices in winding order.
 * @return Accumulated normal, or the zero vector for fewer than three points.
 */
[[nodiscard]] inline quader::math::Vec3 polygon_normal(std::span<const quader::math::Vec3> points) noexcept {
	quader::math::Vec3 normal;

	if (points.size() < 3) {
		return normal;
	}

	for (std::size_t index = 0; index < points.size(); ++index) {
		const auto kCurrent = points[index];
		const auto kNext = points[(index + 1U) % points.size()];

		normal.x += (kCurrent.y - kNext.y) * (kCurrent.z + kNext.z);
		normal.y += (kCurrent.z - kNext.z) * (kCurrent.x + kNext.x);
		normal.z += (kCurrent.x - kNext.x) * (kCurrent.y + kNext.y);
	}

	return normal;
}

/**
 * Check whether a polygon has degenerate area.
 *
 * @param points Polygon vertices in winding order.
 * @param epsilon Normal-length threshold treated as degenerate.
 * @return True when the accumulated normal length squared is below threshold.
 */
[[nodiscard]] inline bool has_degenerate_area(std::span<const quader::math::Vec3> points,
		float epsilon = quader::math::kDefaultEpsilon) noexcept {
	return quader::math::length_squared(polygon_normal(points)) <= epsilon * epsilon;
}

} // namespace quader::geometry
