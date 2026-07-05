#pragma once

#include "crimson/color/color_space.hpp"
#include "crimson/renderer_types.hpp"
#include "math/vec3.hpp"

#include <cstdint>

namespace crimson {

enum class OverlayDepthMode : std::uint8_t {
    DepthTested,
    XRay,
    AlwaysOnTop,
};

enum class OverlayPrimitive : std::uint8_t {
    Grid,
    LineList,
    SolidTriangles,
};

struct OverlayCommand {
    std::uint32_t view_index = 0;
    OverlayPrimitive primitive = OverlayPrimitive::LineList;
    OverlayDepthMode depth_mode = OverlayDepthMode::DepthTested;
    BaseShaderId base_shader = BaseShaderId::OverlayUnlit;
    ColorSrgb color_srgb{1.0F, 1.0F, 1.0F, 1.0F};
    float opacity = 1.0F;
    float thickness_px = 1.0F;
    std::uint32_t payload_offset = 0;
    std::uint32_t payload_count = 0;
};

struct GridOverlayCommand {
    std::uint32_t view_index = 0;
    quader::math::Vec3 origin{};
    quader::math::Vec3 u_axis{1.0F, 0.0F, 0.0F};
    quader::math::Vec3 v_axis{0.0F, 0.0F, -1.0F};
    float plane_width = 1.0F;
    float plane_height = 1.0F;
    float rotation_x = 0.0F;
    float rotation_y = 0.0F;
    float rotation_z = 0.0F;
    float minor_spacing_m = 1.0F;
    float major_spacing_m = 2.0F;
    float fade_start_m = 96.0F;
    float fade_end_m = 1536.0F;
    float minor_line_scale = 0.325F;
    float major_line_scale = 0.250F;
    float axis_line_scale = 1.0F;
    float orthographic_height_m = 1.0F;
    float viewport_height_px = 1.0F;
    float edge_softness_m = 0.001F;
    ColorSrgb minor_color{150.0F / 255.0F, 150.0F / 255.0F, 150.0F / 255.0F, 1.0F};
    ColorSrgb major_color{210.0F / 255.0F, 210.0F / 255.0F, 210.0F / 255.0F, 1.0F};
    ColorSrgb axis_u_color{1.0F, 0.239F, 0.0F, 0.72F};
    ColorSrgb axis_v_color{0.059F, 0.612F, 1.0F, 0.72F};
};

[[nodiscard]] RenderQueue render_queue_for_overlay_depth_mode(OverlayDepthMode depth_mode) noexcept;

} // namespace crimson
