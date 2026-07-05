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

#include <cstdint>
#include <memory>

#include <bgfx/bgfx.h>

namespace crimson::gpu {

/// Owns GPU state needed to submit unlit overlay grid and line primitives.
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
			const PreparedLineOverlayCommand &lines,
			bgfx::ProgramHandle program) const noexcept;

private:
	struct Impl;

	std::unique_ptr<Impl> impl_;
};

} // namespace crimson::gpu
