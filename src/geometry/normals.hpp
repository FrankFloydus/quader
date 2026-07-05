#pragma once

#include "math/scalar.hpp"
#include "math/vec3.hpp"

#include <span>

namespace quader::geometry {

[[nodiscard]] inline quader::math::Vec3 triangle_normal(quader::math::Vec3 a,
		quader::math::Vec3 b,
		quader::math::Vec3 c) noexcept {
	return quader::math::cross(b - a, c - a);
}

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

[[nodiscard]] inline bool has_degenerate_area(std::span<const quader::math::Vec3> points,
		float epsilon = quader::math::kDefaultEpsilon) noexcept {
	return quader::math::length_squared(polygon_normal(points)) <= epsilon * epsilon;
}

} // namespace quader::geometry
