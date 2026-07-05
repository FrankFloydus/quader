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
#include "crimson/scene/render_object.hpp"
#include "math/vec3.hpp"

#include <cstdint>
#include <span>

namespace crimson {

/// Projection mode used by the legacy prototype viewport adapter.
enum class PrototypeCameraProjection {
	/// Perspective projection.
	Perspective,
	/// Orthographic projection.
	Orthographic,
};

/// Camera state consumed by the legacy prototype viewport path.
struct PrototypeCamera {
	/// Camera eye position in world space.
	quader::math::Vec3 eye{};
	/// Camera target position in world space.
	quader::math::Vec3 target{};
	/// Camera up direction.
	quader::math::Vec3 up{ 0.0F, 1.0F, 0.0F };
	/// Camera forward direction.
	quader::math::Vec3 forward{ 0.0F, 0.0F, -1.0F };
	/// Projection mode.
	PrototypeCameraProjection projection = PrototypeCameraProjection::Perspective;
	/// Perspective vertical field of view in degrees.
	float fov_degrees = 60.0F;
	/// Orthographic view height in world units.
	float orthographic_size = 24.0F;
};

/// Pixel rectangle for one prototype viewport pane.
struct PrototypeViewportRect {
	/// Left coordinate in pixels.
	std::uint16_t x = 0;
	/// Top coordinate in pixels.
	std::uint16_t y = 0;
	/// Width in pixels.
	std::uint16_t width = 1;
	/// Height in pixels.
	std::uint16_t height = 1;
};

/// One view submitted through the prototype viewport path.
struct PrototypeViewportView {
	/// View rectangle inside the target extent.
	PrototypeViewportRect rect;
	/// Index into `PrototypeViewportFrame::cameras`.
	std::uint8_t camera_index = 0;
	/// Optional debug name borrowed by the renderer for the frame.
	const char *debug_name = "Viewport";
};

/// Per-frame payload for the legacy prototype viewport renderer.
struct PrototypeViewportFrame {
	/// Target extent for the frame.
	ViewportExtent target_extent;
	/// View list borrowed for the duration of the call.
	std::span<const PrototypeViewportView> views;
	/// Camera list borrowed for the duration of the call.
	std::span<const PrototypeCamera> cameras;
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
	/// Whether the prototype scene animation is enabled.
	bool animation_enabled = true;
	/// Elapsed animation time in seconds.
	double elapsed_seconds = 0.0;
};

/**
 * Check whether a prototype view references a valid pane and camera slot.
 *
 * @param view View to validate.
 * @return True when dimensions are non-zero and `camera_index` is within the prototype limit.
 */
[[nodiscard]] constexpr bool is_valid_prototype_view(const PrototypeViewportView &view) noexcept {
	return view.rect.width > 0 && view.rect.height > 0 && view.camera_index < 4;
}

} // namespace crimson
