#pragma once

#include "crimson/frame/frame_stats.hpp"
#include "crimson/picking/picking.hpp"
#include "crimson/renderer_types.hpp"
#include "math/vec3.hpp"

#include <cstdint>
#include <span>

namespace crimson {

enum class PrototypeCameraProjection {
    Perspective,
    Orthographic,
};

struct PrototypeCamera {
    quader::math::Vec3 eye{};
    quader::math::Vec3 target{};
    quader::math::Vec3 up{0.0F, 1.0F, 0.0F};
    quader::math::Vec3 forward{0.0F, 0.0F, -1.0F};
    PrototypeCameraProjection projection = PrototypeCameraProjection::Perspective;
    float fov_degrees = 60.0F;
    float orthographic_size = 24.0F;
};

struct PrototypeViewportRect {
    std::uint16_t x = 0;
    std::uint16_t y = 0;
    std::uint16_t width = 1;
    std::uint16_t height = 1;
};

struct PrototypeViewportView {
    PrototypeViewportRect rect;
    std::uint8_t camera_index = 0;
    const char* debug_name = "Viewport";
};

struct PrototypeViewportFrame {
    ViewportExtent target_extent;
    std::span<const PrototypeViewportView> views;
    std::span<const PrototypeCamera> cameras;
    std::span<const PickingRequest> picking_requests;
    bool animation_enabled = true;
    double elapsed_seconds = 0.0;
};

[[nodiscard]] constexpr bool is_valid_prototype_view(const PrototypeViewportView& view) noexcept
{
    return view.rect.width > 0 && view.rect.height > 0 && view.camera_index < 4;
}

} // namespace crimson
