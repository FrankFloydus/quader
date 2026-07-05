/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
#include "crimson/overlays/grid_overlay.hpp"

#include "math/scalar.hpp"

#include <algorithm>
#include <cmath>

namespace crimson {
namespace {

using quader::math::length;
using quader::math::normalized;
using quader::math::Vec3;

constexpr float kHalfPi = 1.57079632679F;
constexpr float kViewportGridDefaultPlaneSize = 8192.0F;
constexpr float kViewportGridPlaneOffset = -0.002F;
constexpr float kViewportGridOrthographicMargin = 1.25F;
constexpr float kGridWorldSpacing = 1.0F;
constexpr float kParentGridWorldSize = 2.0F;
constexpr float kGroundGridFadeStart = 96.0F;
constexpr float kGroundGridFadeEnd = 1536.0F;
constexpr float kDefaultViewportGridMinorLineSize = 0.325F;
constexpr float kDefaultViewportGridMajorLineSize = 0.250F;
constexpr float kDefaultViewportGridAxisLineSize = 1.0F;

enum class GridPlane {
	XZ,
	YZ,
	XY,
};

[[nodiscard]] GridPlane grid_plane_for_camera(const RenderCamera &camera) {
	if (camera.projection != CameraProjection::Orthographic) {
		return GridPlane::XZ;
	}

	Vec3 forward = normalized(camera.target - camera.eye);
	if (length(forward) <= quader::math::kDefaultEpsilon) {
		forward = normalized(camera.forward);
	}
	if (length(forward) <= quader::math::kDefaultEpsilon) {
		forward = { 0.0F, 0.0F, 1.0F };
	}

	const float kAbsX = std::abs(forward.x);
	const float kAbsY = std::abs(forward.y);
	const float kAbsZ = std::abs(forward.z);
	if (kAbsX >= kAbsY && kAbsX >= kAbsZ) {
		return GridPlane::YZ;
	}
	if (kAbsZ >= kAbsX && kAbsZ >= kAbsY) {
		return GridPlane::XY;
	}
	return GridPlane::XZ;
}

} // namespace

GridOverlayCommand make_grid_overlay_for_view(const RenderView &view) {
	GridOverlayCommand command;
	command.view_index = view.view_index;

	const RenderCamera &camera = view.camera;
	command.depth_mode = camera.projection == CameraProjection::Orthographic
			? OverlayDepthMode::DepthTested
			: OverlayDepthMode::AlwaysOnTop;
	const GridPlane kPlane = grid_plane_for_camera(camera);
	const float kAspect = std::max(0.0001F, static_cast<float>(std::max<std::uint16_t>(1, view.rect.width)) / static_cast<float>(std::max<std::uint16_t>(1, view.rect.height)));

	command.plane_width = kViewportGridDefaultPlaneSize;
	command.plane_height = kViewportGridDefaultPlaneSize;
	if (camera.projection == CameraProjection::Orthographic) {
		const float kPlaneHeight = std::max(0.01F, camera.orthographic_height_m) * kViewportGridOrthographicMargin;
		if (kPlane == GridPlane::YZ) {
			command.plane_width = kPlaneHeight;
			command.plane_height = kPlaneHeight * kAspect;
		} else {
			command.plane_width = kPlaneHeight * kAspect;
			command.plane_height = kPlaneHeight;
		}
	}

	if (kPlane == GridPlane::YZ) {
		command.origin = { kViewportGridPlaneOffset, camera.eye.y, camera.eye.z };
		command.rotation_z = -kHalfPi;
		command.u_axis = { 0.0F, 0.0F, -1.0F };
		command.v_axis = { 0.0F, 1.0F, 0.0F };
		command.axis_u_color = { 0.059F, 0.612F, 1.0F, 0.72F };
		command.axis_v_color = { 0.29F, 0.58F, 0.0F, 0.0F };
	} else if (kPlane == GridPlane::XY) {
		command.origin = { camera.eye.x, camera.eye.y, kViewportGridPlaneOffset };
		command.rotation_x = kHalfPi;
		command.u_axis = { 1.0F, 0.0F, 0.0F };
		command.v_axis = { 0.0F, 1.0F, 0.0F };
		command.axis_u_color = { 1.0F, 0.239F, 0.0F, 0.72F };
		command.axis_v_color = { 0.29F, 0.58F, 0.0F, 0.0F };
	} else {
		command.origin = { camera.eye.x, kViewportGridPlaneOffset, camera.eye.z };
	}

	command.minor_spacing_m = kGridWorldSpacing;
	command.major_spacing_m = std::max(kParentGridWorldSize, kGridWorldSpacing);
	command.fade_start_m = kGroundGridFadeStart;
	command.fade_end_m = kGroundGridFadeEnd;
	command.minor_line_scale = kDefaultViewportGridMinorLineSize;
	command.major_line_scale = kDefaultViewportGridMajorLineSize;
	command.axis_line_scale = kDefaultViewportGridAxisLineSize;
	command.orthographic_height_m = std::max(0.0001F, camera.orthographic_height_m);
	command.viewport_height_px = static_cast<float>(std::max<std::uint16_t>(1, view.rect.height));
	command.edge_softness_m = 0.001F;
	return command;
}

OverlayCommand make_grid_overlay_command(const GridOverlayCommand &grid, std::uint32_t payload_offset) {
	return OverlayCommand{
		.view_index = grid.view_index,
		.primitive = OverlayPrimitive::Grid,
		.depth_mode = grid.depth_mode,
		.base_shader = BaseShaderId::OverlayUnlit,
		.color_srgb = ColorSrgb{ 1.0F, 1.0F, 1.0F, 1.0F },
		.opacity = 1.0F,
		.thickness_px = grid.minor_line_scale,
		.payload_offset = payload_offset,
		.payload_count = 1,
	};
}

} // namespace crimson
