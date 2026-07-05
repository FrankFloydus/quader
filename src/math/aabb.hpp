#pragma once

#include "math/detail/glm_adapter.hpp"
#include "math/vec3.hpp"

#include <limits>

namespace quader::math {

struct Aabb {
    Vec3 min{
        std::numeric_limits<float>::infinity(),
        std::numeric_limits<float>::infinity(),
        std::numeric_limits<float>::infinity(),
    };
    Vec3 max{
        -std::numeric_limits<float>::infinity(),
        -std::numeric_limits<float>::infinity(),
        -std::numeric_limits<float>::infinity(),
    };
};

[[nodiscard]] inline bool empty(Aabb bounds) noexcept
{
    return bounds.min.x > bounds.max.x || bounds.min.y > bounds.max.y || bounds.min.z > bounds.max.z;
}

inline void expand(Aabb& bounds, Vec3 point) noexcept
{
    const auto min_result = detail::component_min(detail::Components3{bounds.min.x, bounds.min.y, bounds.min.z},
                                                  detail::Components3{point.x, point.y, point.z});
    const auto max_result = detail::component_max(detail::Components3{bounds.max.x, bounds.max.y, bounds.max.z},
                                                  detail::Components3{point.x, point.y, point.z});
    bounds.min = Vec3{min_result.x, min_result.y, min_result.z};
    bounds.max = Vec3{max_result.x, max_result.y, max_result.z};
}

[[nodiscard]] inline Vec3 center(Aabb bounds) noexcept
{
    return (bounds.min + bounds.max) * 0.5F;
}

} // namespace quader::math
