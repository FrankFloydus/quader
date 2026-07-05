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
#include "crimson/native_surface.hpp"
#include "crimson/renderer_config.hpp"
#include "crimson/renderer_diagnostics.hpp"
#include "foundation/result.hpp"

#include <memory>
#include <optional>
#include <string>

namespace crimson {

/**
 * Public Crimson renderer facade.
 *
 * The facade owns the GPU device implementation and consumes immutable frame
 * snapshots. It does not own document or modeling state.
 */
class Renderer final {
public:
	/**
	 * Create and initialize a renderer for a native surface.
	 *
	 * @param config Renderer startup configuration.
	 * @param surface Native host surface descriptor.
	 * @return Renderer instance, or a startup diagnostic.
	 */
	[[nodiscard]] static quader::foundation::Result<std::unique_ptr<Renderer>, RendererDiagnostic> create(
			RendererConfig config,
			const NativeSurfaceDescriptor &surface);

	/// Shut down the renderer and release GPU resources.
	~Renderer();

	/// Renderers are move-blocked because the implementation owns native resources.
	Renderer(const Renderer &) = delete;
	/// Renderers are move-blocked because the implementation owns native resources.
	Renderer &operator=(const Renderer &) = delete;

	/**
	 * Resize the renderer's target extent.
	 *
	 * @param extent New viewport extent.
	 * @return Success, or a resize diagnostic.
	 */
	[[nodiscard]] quader::foundation::Result<void, RendererDiagnostic> resize(const ViewportExtent &extent);
	/**
	 * Render one immutable frame snapshot.
	 *
	 * @param snapshot Frame data consumed by the renderer.
	 * @return Frame result, or a renderer diagnostic.
	 */
	[[nodiscard]] quader::foundation::Result<FrameRenderResult, RendererDiagnostic> render(
			const FrameSnapshot &snapshot);

	/**
	 * Return renderer status by value.
	 *
	 * @return Current status snapshot.
	 */
	[[nodiscard]] RendererStatus status() const;
	/**
	 * Return stats for the latest rendered frame.
	 *
	 * @return Frame stats, or empty when no frame has completed.
	 */
	[[nodiscard]] std::optional<FrameStats> latest_frame_stats() const;
	/**
	 * Build a diagnostics snapshot for UI/test consumers.
	 *
	 * @return Snapshot containing status, counters, recent diagnostics, and graph dump.
	 */
	[[nodiscard]] RendererDiagnosticsSnapshot diagnostics_snapshot() const;
	/**
	 * Dump the latest frame graph for debugging.
	 *
	 * @return Human-readable graph dump, or an empty string before rendering.
	 */
	[[nodiscard]] std::string debug_dump_last_frame_graph() const;

private:
	struct Impl;

	explicit Renderer(std::unique_ptr<Impl> impl);

	std::unique_ptr<Impl> impl_;
};

} // namespace crimson
