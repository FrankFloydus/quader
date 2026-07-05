#pragma once

#include "math/vec3.hpp"

namespace crimson {

enum class RenderLightKind {
    Directional,
    Point,
    Spot,
};

struct RenderLight {
    RenderLightKind kind = RenderLightKind::Directional;
    quader::math::Vec3 position_world{};
    quader::math::Vec3 direction_world{-0.45F, -0.80F, -0.38F};
    quader::math::Vec3 color_linear{1.0F, 1.0F, 1.0F};
    float intensity = 1.0F;
    float range_m = 0.0F;
};

} // namespace crimson
