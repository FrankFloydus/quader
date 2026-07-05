#pragma once

#include "math/detail/glm_adapter.hpp"
#include "math/scalar.hpp"

namespace quader::math {

struct Vec2 {
    float x = 0.0F;
    float y = 0.0F;
};

[[nodiscard]] inline Vec2 operator+(Vec2 left, Vec2 right) noexcept
{
    const auto result = detail::add(detail::Components2{left.x, left.y}, detail::Components2{right.x, right.y});
    return Vec2{result.x, result.y};
}

[[nodiscard]] inline Vec2 operator-(Vec2 left, Vec2 right) noexcept
{
    const auto result = detail::subtract(detail::Components2{left.x, left.y}, detail::Components2{right.x, right.y});
    return Vec2{result.x, result.y};
}

[[nodiscard]] inline Vec2 operator*(Vec2 value, float scale) noexcept
{
    const auto result = detail::scale(detail::Components2{value.x, value.y}, scale);
    return Vec2{result.x, result.y};
}

[[nodiscard]] inline float dot(Vec2 left, Vec2 right) noexcept
{
    return detail::dot(detail::Components2{left.x, left.y}, detail::Components2{right.x, right.y});
}

[[nodiscard]] inline float length_squared(Vec2 value) noexcept
{
    return detail::length_squared(detail::Components2{value.x, value.y});
}

[[nodiscard]] inline bool is_finite(Vec2 value) noexcept
{
    return is_finite(value.x) && is_finite(value.y);
}

[[nodiscard]] inline bool nearly_equal(Vec2 left,
                                       Vec2 right,
                                       float epsilon = kDefaultEpsilon) noexcept
{
    return nearly_equal(left.x, right.x, epsilon) && nearly_equal(left.y, right.y, epsilon);
}

} // namespace quader::math
