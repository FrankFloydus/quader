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

#include "crimson/diagnostics/renderer_diagnostics_snapshot.hpp"
#include "crimson/frame/frame_render_result.hpp"
#include "crimson/frame/frame_snapshot.hpp"
#include "crimson/gpu/gpu_capabilities.hpp"
#include "crimson/graph/render_graph.hpp"
#include "crimson/native_surface.hpp"
#include "crimson/renderer_config.hpp"
#include "crimson/renderer_diagnostics.hpp"
#include "foundation/result.hpp"

#include <memory>
#include <optional>
#include <vector>

namespace crimson::gpu {

/// Owns the selected graphics backend and per-frame GPU execution path.
class GpuDevice final {
public:
	/// Create an uninitialized GPU device wrapper.
	GpuDevice();
	/// Shut down and release GPU resources.
	~GpuDevice();

	/// GPU devices cannot be copied because they own native runtime state.
	GpuDevice(const GpuDevice &) = delete;
	/// GPU devices cannot be copied because they own native runtime state.
	GpuDevice &operator=(const GpuDevice &) = delete;

	/**
	 * Initialize the graphics runtime for a native surface.
	 *
	 * @param config Renderer startup configuration.
	 * @param surface Native host surface descriptor.
	 * @return True when initialization succeeds.
	 */
	bool initialize(const RendererConfig &config, const NativeSurfaceDescriptor &surface);
	/// Shut down the graphics runtime if initialized.
	void shutdown() noexcept;
	/**
	 * Resize runtime targets.
	 *
	 * @param extent New viewport extent.
	 * @return True when resize succeeds.
	 */
	bool resize(const ViewportExtent &extent);

	/**
	 * Render one frame snapshot through the active graph.
	 *
	 * @param snapshot Immutable frame snapshot.
	 * @param graph Validated render graph for the frame.
	 * @return Frame result, or renderer diagnostic.
	 */
	[[nodiscard]] quader::foundation::Result<FrameRenderResult, RendererDiagnostic> render_frame(
			const FrameSnapshot &snapshot,
			const RenderGraph &graph);
	/**
	 * Return stats for the latest rendered frame.
	 *
	 * @return Frame stats, or empty before a frame completes.
	 */
	[[nodiscard]] std::optional<FrameStats> latest_frame_stats() const noexcept;
	/**
	 * Return latest per-pass stats.
	 *
	 * @return Per-pass stats copied from the latest frame.
	 */
	[[nodiscard]] std::vector<RenderPassStats> latest_pass_stats() const;

	/// Return true when the graphics runtime is initialized.
	bool initialized() const noexcept;
	/// Return the selected backend, or empty before initialization.
	std::optional<GraphicsBackend> selected_backend() const noexcept;
	/// Return the current renderer status owned by the device.
	const RendererStatus &status() const noexcept;

private:
	struct Impl;

	RendererStatus status_;
	std::optional<GraphicsBackend> selected_backend_;
	RendererConfig config_;
	ViewportExtent extent_;
	std::unique_ptr<Impl> impl_;
};

} // namespace crimson::gpu
