#pragma once

#include "math/vec3.hpp"

namespace crimson {

enum class CameraProjection {
    Perspective,
    Orthographic,
};

struct RenderCamera {
    quader::math::Vec3 eye{};
    quader::math::Vec3 target{};
    quader::math::Vec3 up{0.0F, 1.0F, 0.0F};
    quader::math::Vec3 forward{0.0F, 0.0F, -1.0F};
    CameraProjection projection = CameraProjection::Perspective;
    float near_plane_m = 0.05F;
    float far_plane_m = 1000.0F;
    float vertical_fov_degrees = 60.0F;
    float orthographic_height_m = 24.0F;
    float exposure_ev100 = 12.0F;
    float exposure_compensation_ev = 0.0F;
};

} // namespace crimson
