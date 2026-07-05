#pragma once

#include "math/detail/glm_config.hpp"

#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace quader::math::detail {

struct Components2 {
    float x = 0.0F;
    float y = 0.0F;
};

struct Components3 {
    float x = 0.0F;
    float y = 0.0F;
    float z = 0.0F;
};

[[nodiscard]] inline glm::vec2 to_glm(Components2 value) noexcept
{
    return glm::vec2{value.x, value.y};
}

[[nodiscard]] inline glm::vec3 to_glm(Components3 value) noexcept
{
    return glm::vec3{value.x, value.y, value.z};
}

[[nodiscard]] inline Components2 from_glm(glm::vec2 value) noexcept
{
    return Components2{value.x, value.y};
}

[[nodiscard]] inline Components3 from_glm(glm::vec3 value) noexcept
{
    return Components3{value.x, value.y, value.z};
}

[[nodiscard]] inline Components2 add(Components2 left, Components2 right) noexcept
{
    return from_glm(to_glm(left) + to_glm(right));
}

[[nodiscard]] inline Components3 add(Components3 left, Components3 right) noexcept
{
    return from_glm(to_glm(left) + to_glm(right));
}

[[nodiscard]] inline Components2 subtract(Components2 left, Components2 right) noexcept
{
    return from_glm(to_glm(left) - to_glm(right));
}

[[nodiscard]] inline Components3 subtract(Components3 left, Components3 right) noexcept
{
    return from_glm(to_glm(left) - to_glm(right));
}

[[nodiscard]] inline Components2 scale(Components2 value, float scalar) noexcept
{
    return from_glm(to_glm(value) * scalar);
}

[[nodiscard]] inline Components3 scale(Components3 value, float scalar) noexcept
{
    return from_glm(to_glm(value) * scalar);
}

[[nodiscard]] inline Components3 divide(Components3 value, float scalar) noexcept
{
    return from_glm(to_glm(value) / scalar);
}

[[nodiscard]] inline float dot(Components2 left, Components2 right) noexcept
{
    return glm::dot(to_glm(left), to_glm(right));
}

[[nodiscard]] inline float dot(Components3 left, Components3 right) noexcept
{
    return glm::dot(to_glm(left), to_glm(right));
}

[[nodiscard]] inline Components3 cross(Components3 left, Components3 right) noexcept
{
    return from_glm(glm::cross(to_glm(left), to_glm(right)));
}

[[nodiscard]] inline float length_squared(Components2 value) noexcept
{
    return dot(value, value);
}

[[nodiscard]] inline float length_squared(Components3 value) noexcept
{
    return dot(value, value);
}

[[nodiscard]] inline float length(Components3 value) noexcept
{
    return glm::length(to_glm(value));
}

[[nodiscard]] inline Components3 normalized(Components3 value) noexcept
{
    return from_glm(glm::normalize(to_glm(value)));
}

[[nodiscard]] inline Components3 component_min(Components3 left, Components3 right) noexcept
{
    return from_glm(glm::min(to_glm(left), to_glm(right)));
}

[[nodiscard]] inline Components3 component_max(Components3 left, Components3 right) noexcept
{
    return from_glm(glm::max(to_glm(left), to_glm(right)));
}

} // namespace quader::math::detail
