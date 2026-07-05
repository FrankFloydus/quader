/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
#pragma once

#include "crimson/color/color_space.hpp"
#include "crimson/renderer_types.hpp"
#include "math/vec3.hpp"

#include <cstdint>

namespace crimson {

/// Depth policy used when compositing editor overlays.
enum class OverlayDepthMode : std::uint8_t {
	DepthTested, ///< Hidden by scene depth.
	XRay,        ///< Blended through scene depth.
	AlwaysOnTop,///< Drawn without depth testing.
};

/// Overlay primitive payload type.
enum class OverlayPrimitive : std::uint8_t {
	Grid,           ///< Grid payload in `GridOverlayCommand`.
	LineList,       ///< Line segment payloads in `LineOverlaySegment`.
	SolidTriangles, ///< Triangle overlay payloads.
};

/// Renderer command describing one overlay draw payload.
struct OverlayCommand {
	/// View index that owns the overlay.
	std::uint32_t view_index = 0;
	/// Payload primitive kind.
	OverlayPrimitive primitive = OverlayPrimitive::LineList;
	/// Overlay depth policy.
	OverlayDepthMode depth_mode = OverlayDepthMode::DepthTested;
	/// Base shader used for the overlay.
	BaseShaderId base_shader = BaseShaderId::OverlayUnlit;
	/// Overlay color authored in sRGB UI space.
	ColorSrgb color_srgb{ 1.0F, 1.0F, 1.0F, 1.0F };
	/// Extra opacity multiplier.
	float opacity = 1.0F;
	/// Line thickness in physical pixels.
	float thickness_px = 1.0F;
	/// Offset into the matching overlay payload array.
	std::uint32_t payload_offset = 0;
	/// Number of payload records consumed by this command.
	std::uint32_t payload_count = 0;
};

/// Grid overlay payload consumed by the overlay renderer.
struct GridOverlayCommand {
	/// View index that owns the grid.
	std::uint32_t view_index = 0;
	/// Grid plane origin in world space.
	quader::math::Vec3 origin{};
	/// First grid axis.
	quader::math::Vec3 u_axis{ 1.0F, 0.0F, 0.0F };
	/// Second grid axis.
	quader::math::Vec3 v_axis{ 0.0F, 0.0F, -1.0F };
	/// Grid plane width in world units.
	float plane_width = 1.0F;
	/// Grid plane height in world units.
	float plane_height = 1.0F;
	/// X-axis rotation in radians for shader-facing grid orientation.
	float rotation_x = 0.0F;
	/// Y-axis rotation in radians for shader-facing grid orientation.
	float rotation_y = 0.0F;
	/// Z-axis rotation in radians for shader-facing grid orientation.
	float rotation_z = 0.0F;
	/// Minor grid spacing in meters.
	float minor_spacing_m = 1.0F;
	/// Major grid spacing in meters.
	float major_spacing_m = 2.0F;
	/// Distance where grid fade begins.
	float fade_start_m = 96.0F;
	/// Distance where grid fade ends.
	float fade_end_m = 1536.0F;
	/// Minor line thickness scale.
	float minor_line_scale = 0.325F;
	/// Major line thickness scale.
	float major_line_scale = 0.250F;
	/// Axis line thickness scale.
	float axis_line_scale = 1.0F;
	/// Orthographic view height in meters.
	float orthographic_height_m = 1.0F;
	/// Viewport height in physical pixels.
	float viewport_height_px = 1.0F;
	/// Edge fade softness in meters.
	float edge_softness_m = 0.001F;
	/// Minor line color in sRGB UI space.
	ColorSrgb minor_color{ 150.0F / 255.0F, 150.0F / 255.0F, 150.0F / 255.0F, 1.0F };
	/// Major line color in sRGB UI space.
	ColorSrgb major_color{ 210.0F / 255.0F, 210.0F / 255.0F, 210.0F / 255.0F, 1.0F };
	/// U-axis color in sRGB UI space.
	ColorSrgb axis_u_color{ 1.0F, 0.239F, 0.0F, 0.72F };
	/// V-axis color in sRGB UI space.
	ColorSrgb axis_v_color{ 0.059F, 0.612F, 1.0F, 0.72F };
};

/// World-space line segment payload for overlay line lists.
struct LineOverlaySegment {
	/// Segment start point.
	quader::math::Vec3 start;
	/// Segment end point.
	quader::math::Vec3 end;
};

/// Return the render queue corresponding to an overlay depth mode.
[[nodiscard]] RenderQueue render_queue_for_overlay_depth_mode(OverlayDepthMode depth_mode) noexcept;

} // namespace crimson
