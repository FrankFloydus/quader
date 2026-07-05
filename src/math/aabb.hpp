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

[[nodiscard]] inline bool empty(Aabb bounds) noexcept {
	return bounds.min.x > bounds.max.x || bounds.min.y > bounds.max.y || bounds.min.z > bounds.max.z;
}

inline void expand(Aabb &bounds, Vec3 point) noexcept {
	const auto kMinResult = detail::component_min(detail::Components3{ bounds.min.x, bounds.min.y, bounds.min.z },
			detail::Components3{ point.x, point.y, point.z });
	const auto kMaxResult = detail::component_max(detail::Components3{ bounds.max.x, bounds.max.y, bounds.max.z },
			detail::Components3{ point.x, point.y, point.z });
	bounds.min = Vec3{ kMinResult.x, kMinResult.y, kMinResult.z };
	bounds.max = Vec3{ kMaxResult.x, kMaxResult.y, kMaxResult.z };
}

[[nodiscard]] inline Vec3 center(Aabb bounds) noexcept {
	return (bounds.min + bounds.max) * 0.5F;
}

} // namespace quader::math
