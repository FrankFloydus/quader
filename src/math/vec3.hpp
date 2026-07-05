#pragma once

#include "math/detail/glm_adapter.hpp"
#include "math/scalar.hpp"

namespace quader::math {

struct Vec3 {
    float x = 0.0F;
    float y = 0.0F;
    float z = 0.0F;
};

[[nodiscard]] inline Vec3 operator+(Vec3 left, Vec3 right) noexcept
{
    const auto result = detail::add(detail::Components3{left.x, left.y, left.z},
                                    detail::Components3{right.x, right.y, right.z});
    return Vec3{result.x, result.y, result.z};
}

[[nodiscard]] inline Vec3 operator-(Vec3 left, Vec3 right) noexcept
{
    const auto result = detail::subtract(detail::Components3{left.x, left.y, left.z},
                                         detail::Components3{right.x, right.y, right.z});
    return Vec3{result.x, result.y, result.z};
}

[[nodiscard]] inline Vec3 operator*(Vec3 value, float scale) noexcept
{
    const auto result = detail::scale(detail::Components3{value.x, value.y, value.z}, scale);
    return Vec3{result.x, result.y, result.z};
}

[[nodiscard]] inline Vec3 operator/(Vec3 value, float scale) noexcept
{
    const auto result = detail::divide(detail::Components3{value.x, value.y, value.z}, scale);
    return Vec3{result.x, result.y, result.z};
}

[[nodiscard]] inline float dot(Vec3 left, Vec3 right) noexcept
{
    return detail::dot(detail::Components3{left.x, left.y, left.z},
                       detail::Components3{right.x, right.y, right.z});
}

[[nodiscard]] inline Vec3 cross(Vec3 left, Vec3 right) noexcept
{
    const auto result = detail::cross(detail::Components3{left.x, left.y, left.z},
                                      detail::Components3{right.x, right.y, right.z});
    return Vec3{result.x, result.y, result.z};
}

[[nodiscard]] inline float length_squared(Vec3 value) noexcept
{
    return detail::length_squared(detail::Components3{value.x, value.y, value.z});
}

[[nodiscard]] inline float length(Vec3 value) noexcept
{
    return detail::length(detail::Components3{value.x, value.y, value.z});
}

[[nodiscard]] inline Vec3 normalized(Vec3 value, float epsilon = kDefaultEpsilon) noexcept
{
    const float len = length(value);
    if (len <= epsilon) {
        return {};
    }

    const auto result = detail::normalized(detail::Components3{value.x, value.y, value.z});
    return Vec3{result.x, result.y, result.z};
}

[[nodiscard]] inline bool is_finite(Vec3 value) noexcept
{
    return is_finite(value.x) && is_finite(value.y) && is_finite(value.z);
}

[[nodiscard]] inline bool nearly_equal(Vec3 left,
                                       Vec3 right,
                                       float epsilon = kDefaultEpsilon) noexcept
{
    return nearly_equal(left.x, right.x, epsilon) && nearly_equal(left.y, right.y, epsilon)
        && nearly_equal(left.z, right.z, epsilon);
}

} // namespace quader::math
