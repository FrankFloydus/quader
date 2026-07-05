#pragma once

#include "math/vec3.hpp"

namespace quader::document {

struct Transform {
	quader::math::Vec3 translation;
	quader::math::Vec3 rotation_euler;
	quader::math::Vec3 scale{ 1.0F, 1.0F, 1.0F };
};

[[nodiscard]] constexpr bool vec3_exactly_equal(quader::math::Vec3 left,
		quader::math::Vec3 right) noexcept {
	return left.x == right.x && left.y == right.y && left.z == right.z;
}

[[nodiscard]] constexpr bool operator==(Transform left, Transform right) noexcept {
	return vec3_exactly_equal(left.translation, right.translation) && vec3_exactly_equal(left.rotation_euler, right.rotation_euler) && vec3_exactly_equal(left.scale, right.scale);
}

[[nodiscard]] inline bool is_finite(Transform transform) noexcept {
	return quader::math::is_finite(transform.translation) && quader::math::is_finite(transform.rotation_euler) && quader::math::is_finite(transform.scale);
}

} // namespace quader::document
