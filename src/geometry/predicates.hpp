#pragma once

#include "math/scalar.hpp"
#include "math/vec3.hpp"

#include <span>

namespace quader::geometry {

[[nodiscard]] inline bool all_points_finite(std::span<const quader::math::Vec3> points) noexcept
{
    for (const auto point : points) {
        if (!quader::math::is_finite(point)) {
            return false;
        }
    }

    return true;
}

} // namespace quader::geometry
