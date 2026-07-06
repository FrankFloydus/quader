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

#include "crimson/frame/frame_stats.hpp"
#include "crimson/mesh/render_mesh.hpp"
#include "crimson/overlays/overlay_command.hpp"
#include "crimson/picking/picking.hpp"
#include "crimson/renderer_types.hpp"
#include "crimson/scene/render_camera.hpp"
#include "crimson/scene/render_object.hpp"
#include "math/vec3.hpp"

#include <cstdint>
#include <span>

namespace crimson {

/// Projection mode used by viewport frame input.
enum class ViewportCameraProjection {
	/// Perspective projection.
	Perspective,
	/// Orthographic projection.
	Orthographic,
};

/// Camera state consumed by viewport frame building.
struct ViewportCamera {
	/// Camera eye position in world space.
	quader::math::Vec3 eye{};
	/// Camera target position in world space.
	quader::math::Vec3 target{};
	/// Camera up direction.
	quader::math::Vec3 up{ 0.0F, 1.0F, 0.0F };
	/// Camera forward direction.
	quader::math::Vec3 forward{ 0.0F, 0.0F, -1.0F };
	/// Projection mode.
	ViewportCameraProjection projection = ViewportCameraProjection::Perspective;
	/// Near clip distance in meters.
	float near_plane_m = kDefaultCameraNearPlaneM;
	/// Far clip distance in meters.
	float far_plane_m = kDefaultCameraFarPlaneM;
	/// Perspective vertical field of view in degrees.
	float fov_degrees = 60.0F;
	/// Orthographic view height in world units.
	float orthographic_size = 24.0F;
};

/// Pixel rectangle for one viewport pane.
struct ViewportFrameRect {
	/// Left coordinate in pixels.
	std::uint16_t x = 0;
	/// Top coordinate in pixels.
	std::uint16_t y = 0;
	/// Width in pixels.
	std::uint16_t width = 1;
	/// Height in pixels.
	std::uint16_t height = 1;
};

/// One view submitted through the viewport path.
struct ViewportFrameView {
	/// View rectangle inside the target extent.
	ViewportFrameRect rect;
	/// Index into `ViewportFrameInput::cameras`.
	std::uint8_t camera_index = 0;
	/// Optional debug name borrowed by the renderer for the frame.
	const char *debug_name = "Viewport";
};

/// Per-frame payload submitted by a viewport host.
struct ViewportFrameInput {
	/// Target extent for the frame.
	ViewportExtent target_extent;
	/// View list borrowed for the duration of the call.
	std::span<const ViewportFrameView> views;
	/// Camera list borrowed for the duration of the call.
	std::span<const ViewportCamera> cameras;
	/// Mesh uploads borrowed for the duration of the call.
	std::span<const RenderMeshUpload> mesh_uploads;
	/// Render objects borrowed for the duration of the call.
	std::span<const RenderObject> objects;
	/// Extra overlay commands borrowed for the duration of the call.
	std::span<const OverlayCommand> overlays;
	/// Line overlay payloads borrowed for the duration of the call.
	std::span<const LineOverlaySegment> line_overlay_payloads;
	/// Triangle overlay payloads borrowed for the duration of the call.
	std::span<const TriangleOverlayPrimitive> triangle_overlay_payloads;
	/// Point overlay payloads borrowed for the duration of the call.
	std::span<const PointOverlayPrimitive> point_overlay_payloads;
	/// Picking requests borrowed for the duration of the call.
	std::span<const PickingRequest> picking_requests;
	/// Whether the viewport scene animation is enabled.
	bool animation_enabled = true;
	/// Elapsed animation time in seconds.
	double elapsed_seconds = 0.0;
};

/**
 * Check whether a viewport frame view references a valid pane and camera slot.
 *
 * @param view View to validate.
 * @return True when dimensions are non-zero and `camera_index` is within the viewport pane limit.
 */
[[nodiscard]] constexpr bool is_valid_viewport_frame_view(const ViewportFrameView &view) noexcept {
	return view.rect.width > 0 && view.rect.height > 0 && view.camera_index < 4;
}

} // namespace crimson
