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

#include "crimson/frame/frame_snapshot.hpp"
#include "crimson/gpu/gpu_program_cache.hpp"
#include "crimson/overlays/overlay_system.hpp"
#include "crimson/renderer_diagnostics.hpp"
#include "math/vec2.hpp"

#include <array>
#include <cstdint>
#include <memory>
#include <optional>

#include <bgfx/bgfx.h>

namespace crimson::gpu {

/// Return the bgfx submit state used for the reference-style ground grid.
[[nodiscard]] std::uint64_t grid_overlay_submit_state(OverlayDepthMode depth_mode) noexcept;

/// Expanded overlay quad corners in the coordinate space consumed by the target overlay shader.
struct OverlayScreenSpaceQuad {
	/// Quad corners ordered for the shared overlay index pattern.
	std::array<quader::math::Vec3, 4> corners;
};

/// Device-space line vertex carrying signed distance from the line center.
struct OverlayLineDeviceVertex {
	/// Clip/device-space position consumed directly by the line shader.
	quader::math::Vec3 position;
	/// Signed distance from the line center in physical pixels.
	float line_distance_pixels = 0.0F;
	/// Device-space depth before edit-wire bias; used for manual scene-depth rejection.
	float original_device_z = 0.0F;
	/// Device-space position on the original line center used for depth sampling.
	quader::math::Vec2 center_device;
};

/// Expanded overlay line quad in device space.
struct OverlayLineDeviceQuad {
	/// Quad vertices ordered for the shared overlay index pattern.
	std::array<OverlayLineDeviceVertex, 4> vertices;
};

/// Build a reference-style device-space line quad after CPU near-plane clipping.
[[nodiscard]] std::optional<OverlayLineDeviceQuad> make_overlay_line_device_quad(
		const RenderView &view,
		const LineOverlaySegment &segment,
		float width_px,
		float depth_bias_units,
		bool homogeneous_depth,
		bool edit_wire_depth_bias = false) noexcept;

/// Build a reference-style world-space line quad after CPU near-plane clipping.
[[nodiscard]] std::optional<OverlayScreenSpaceQuad> make_overlay_line_screen_space_quad(
		const RenderView &view,
		const LineOverlaySegment &segment,
		float width_px,
		float depth_bias_units,
		bool homogeneous_depth) noexcept;

/// Build a reference-style device-space point quad after CPU projection.
[[nodiscard]] std::optional<OverlayScreenSpaceQuad> make_overlay_point_screen_space_quad(
		const RenderView &view,
		quader::math::Vec3 position,
		float size_px,
		float depth_bias_units,
		bool homogeneous_depth) noexcept;

/// Owns GPU state needed to submit unlit overlay grid, line, triangle, and point primitives.
class GpuOverlayRenderer final {
public:
	/// Construct an empty overlay renderer.
	GpuOverlayRenderer();
	/// Release GPU resources.
	~GpuOverlayRenderer();

	/// Copying is disabled because the implementation owns GPU handles.
	GpuOverlayRenderer(const GpuOverlayRenderer &) = delete;
	/// Copying is disabled because the implementation owns GPU handles.
	GpuOverlayRenderer &operator=(const GpuOverlayRenderer &) = delete;

	/// Initialize GPU overlay resources and append diagnostics on failure.
	[[nodiscard]] bool initialize(RendererStatus &status);
	/// Release all owned GPU overlay resources.
	void shutdown() noexcept;

	/// Return true after successful initialization and before shutdown.
	[[nodiscard]] bool ready() const noexcept;
	/// Submit one prepared grid overlay command.
	[[nodiscard]] std::uint32_t submit_grid(
			bgfx::ViewId view_id,
			const RenderView &view,
			const PreparedGridOverlayCommand &grid,
			bgfx::ProgramHandle program) const noexcept;
	/// Submit one prepared line overlay command.
	[[nodiscard]] std::uint32_t submit_lines(
			bgfx::ViewId view_id,
			const RenderView &view,
			const PreparedLineOverlayCommand &lines,
			bgfx::ProgramHandle program) const noexcept;
	/// Submit one depth-aware component edit-line overlay command.
	[[nodiscard]] std::uint32_t submit_edit_lines(
			bgfx::ViewId view_id,
			const RenderView &view,
			ViewportExtent target_extent,
			const PreparedLineOverlayCommand &lines,
			bgfx::TextureHandle scene_depth_texture,
			bgfx::ProgramHandle program) const noexcept;
	/// Submit one prepared solid-triangle overlay command.
	[[nodiscard]] std::uint32_t submit_triangles(
			bgfx::ViewId view_id,
			const PreparedTriangleOverlayCommand &triangles,
			bgfx::ProgramHandle program) const noexcept;
	/// Submit one prepared point overlay command.
	[[nodiscard]] std::uint32_t submit_points(
			bgfx::ViewId view_id,
			const RenderView &view,
			const PreparedPointOverlayCommand &points,
			bgfx::ProgramHandle program) const noexcept;

private:
	struct Impl;

	std::unique_ptr<Impl> impl_;
};

} // namespace crimson::gpu
