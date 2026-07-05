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
#include "crimson/frame/frame_targets.hpp"
#include "crimson/gpu/gpu_mesh_cache.hpp"
#include "crimson/gpu/gpu_program_cache.hpp"
#include "crimson/picking/picking.hpp"
#include "crimson/renderer_diagnostics.hpp"

#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

#include <bgfx/bgfx.h>

namespace crimson::gpu {

/// Picking results and readback counts completed during one GPU frame submission.
struct GpuPickingFrameResult {
	/// Picking results whose delayed readbacks completed.
	std::vector<PickingResult> completed_results;
	/// New readbacks scheduled by this frame.
	std::uint32_t scheduled_readbacks = 0;
};

/// Owns GPU picking target submission and delayed readback resolution.
class GpuPicking final {
public:
	/// Construct an empty picking subsystem.
	GpuPicking();
	/// Release GPU resources.
	~GpuPicking();

	/// Copying is disabled because the implementation owns GPU handles/readbacks.
	GpuPicking(const GpuPicking &) = delete;
	/// Copying is disabled because the implementation owns GPU handles/readbacks.
	GpuPicking &operator=(const GpuPicking &) = delete;

	/// Initialize picking resources for the selected target format.
	[[nodiscard]] bool initialize(PickingIdTargetFormat target_format, RendererStatus &status);
	/// Release all owned picking resources and pending readbacks.
	void shutdown() noexcept;

	/// Return true after successful initialization and before shutdown.
	[[nodiscard]] bool ready() const noexcept;

	/**
	 * Submit all picking work for one frame.
	 *
	 * @param snapshot Frame snapshot containing picking requests.
	 * @param mesh_cache Mesh cache used to draw pickable objects.
	 * @param program_cache Program cache containing the picking shader.
	 * @param unit_box_mesh Fallback mesh handle used for bounding proxies.
	 * @param picking_program Program handle selected for the picking pass.
	 * @param completed_bgfx_frame Latest bgfx frame index known complete.
	 * @return Completed readback results and count of new readbacks.
	 */
	[[nodiscard]] GpuPickingFrameResult submit_frame_requests(
			const FrameSnapshot &snapshot,
			const GpuMeshCache &mesh_cache,
			const GpuProgramCache &program_cache,
			RenderMeshHandle unit_box_mesh,
			RenderProgramHandle picking_program,
			std::uint32_t completed_bgfx_frame);

private:
	struct Impl;

	std::unique_ptr<Impl> impl_;
};

} // namespace crimson::gpu
