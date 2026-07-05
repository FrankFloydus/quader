#pragma once

#include "crimson/renderer_types.hpp"
#include "math/vec3.hpp"

namespace crimson {

struct RenderEnvironment {
    RenderEnvironmentHandle environment;
    quader::math::Vec3 clear_color_linear{0.02F, 0.02F, 0.02F};
    float intensity = 1.0F;
};

} // namespace crimson
