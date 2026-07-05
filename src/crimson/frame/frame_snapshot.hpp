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
#include "crimson/scene/render_environment.hpp"
#include "crimson/scene/render_light.hpp"
#include "crimson/scene/render_object.hpp"
#include "crimson/scene/viewport_settings.hpp"

#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace crimson {

/// Renderer debug visualization selected for the frame.
enum class DebugViewMode {
	FinalColor, ///< Final shaded and composited viewport color.
};

/// Pixel rectangle occupied by a render view inside the frame target.
struct RenderViewportRect {
	/// Left edge in physical pixels.
	std::uint16_t x = 0;
	/// Top edge in physical pixels.
	std::uint16_t y = 0;
	/// Width in physical pixels.
	std::uint16_t width = 1;
	/// Height in physical pixels.
	std::uint16_t height = 1;
};

/// One camera/view rectangle rendered into a frame target.
struct RenderView {
	/// Stable view index used by overlays and picking requests.
	std::uint32_t view_index = 0;
	/// Viewport rectangle in the render target.
	RenderViewportRect rect;
	/// Camera used to render this view.
	RenderCamera camera;
	/// Developer-facing view label.
	std::string debug_name = "Viewport";
};

/**
 * Immutable renderer input for one frame.
 *
 * The snapshot owns all spans returned by accessors. It is CPU-side frame data;
 * GPU resource ownership remains in the renderer GPU layer.
 */
class FrameSnapshot final {
public:
	/// Construct a snapshot by taking ownership of all frame payloads.
	FrameSnapshot(
			std::uint64_t frame_index,
			double elapsed_seconds,
			ViewportExtent target_extent,
			std::vector<RenderView> views,
			std::vector<RenderMeshUpload> mesh_uploads,
			std::vector<RenderObject> objects,
			std::vector<RenderLight> lights,
			RenderEnvironment environment,
			std::vector<OverlayCommand> overlays,
			std::vector<GridOverlayCommand> grid_overlay_payloads,
			std::vector<LineOverlaySegment> line_overlay_payloads,
			std::vector<PickingRequest> picking_requests,
			ViewportSettings viewport_settings,
			DebugViewMode debug_view = DebugViewMode::FinalColor);

	/// Monotonic frame index supplied by the caller.
	[[nodiscard]] std::uint64_t frame_index() const noexcept;
	/// Elapsed application time in seconds.
	[[nodiscard]] double elapsed_seconds() const noexcept;
	/// Render target extent for the frame.
	[[nodiscard]] ViewportExtent target_extent() const noexcept;
	/// Borrow render views owned by this snapshot.
	[[nodiscard]] std::span<const RenderView> views() const noexcept;
	/// Borrow mesh uploads owned by this snapshot.
	[[nodiscard]] std::span<const RenderMeshUpload> mesh_uploads() const noexcept;
	/// Borrow render objects owned by this snapshot.
	[[nodiscard]] std::span<const RenderObject> objects() const noexcept;
	/// Borrow render lights owned by this snapshot.
	[[nodiscard]] std::span<const RenderLight> lights() const noexcept;
	/// Return the frame environment.
	[[nodiscard]] const RenderEnvironment &environment() const noexcept;
	/// Borrow overlay command records owned by this snapshot.
	[[nodiscard]] std::span<const OverlayCommand> overlays() const noexcept;
	/// Borrow grid overlay payloads owned by this snapshot.
	[[nodiscard]] std::span<const GridOverlayCommand> grid_overlay_payloads() const noexcept;
	/// Borrow line overlay payloads owned by this snapshot.
	[[nodiscard]] std::span<const LineOverlaySegment> line_overlay_payloads() const noexcept;
	/// Borrow picking requests owned by this snapshot.
	[[nodiscard]] std::span<const PickingRequest> picking_requests() const noexcept;
	/// Return viewport render settings.
	[[nodiscard]] ViewportSettings viewport_settings() const noexcept;
	/// Return the requested debug view mode.
	[[nodiscard]] DebugViewMode debug_view() const noexcept;

private:
	std::uint64_t frame_index_ = 0;
	double elapsed_seconds_ = 0.0;
	ViewportExtent target_extent_;
	std::vector<RenderView> views_;
	std::vector<RenderMeshUpload> mesh_uploads_;
	std::vector<RenderObject> objects_;
	std::vector<RenderLight> lights_;
	RenderEnvironment environment_;
	std::vector<OverlayCommand> overlays_;
	std::vector<GridOverlayCommand> grid_overlay_payloads_;
	std::vector<LineOverlaySegment> line_overlay_payloads_;
	std::vector<PickingRequest> picking_requests_;
	ViewportSettings viewport_settings_;
	DebugViewMode debug_view_ = DebugViewMode::FinalColor;
};

/**
 * Validate a render view rectangle and camera payload.
 *
 * @param view View to validate.
 * @return True when the view has a non-empty rectangle and usable camera data.
 */
[[nodiscard]] bool is_valid_render_view(const RenderView &view) noexcept;

} // namespace crimson
