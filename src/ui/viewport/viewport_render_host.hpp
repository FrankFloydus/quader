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

#include "ui/viewport/viewport_types.hpp"

#include <optional>
#include <vector>

#include <QString>

namespace quader::ui {

/// Result returned by viewport render-host operations.
struct ViewportRenderResult {
	/// True when the operation succeeded.
	bool ok = true;
	/// Short failure summary when `ok` is false.
	QString summary;
	/// Optional failure detail text.
	QString detail;
	/// Renderer display name.
	QString renderer_name;
	/// Picking results completed during the operation.
	std::vector<ViewportPickResult> completed_pick_results;

	/// Build a successful result.
	[[nodiscard]] static ViewportRenderResult success(
			QString renderer_name = {},
			std::vector<ViewportPickResult> completed_pick_results = {});
	/// Build a failed result.
	[[nodiscard]] static ViewportRenderResult failure(QString summary, QString detail = {});
};

/// Abstract render host used by the Qt viewport controller.
class IViewportRenderHost {
public:
	/// Destroy the render host.
	virtual ~IViewportRenderHost() = default;

	/// Initialize rendering for a native viewport surface.
	[[nodiscard]] virtual ViewportRenderResult initialize_surface(
			const NativeViewportSurface &surface,
			ViewportPixelSize size,
			double device_pixel_ratio) = 0;
	/// Resize the native viewport surface.
	[[nodiscard]] virtual ViewportRenderResult resize_surface(
			ViewportPixelSize size,
			double device_pixel_ratio) = 0;
	/// Render one frame from a borrowed request payload.
	[[nodiscard]] virtual ViewportRenderResult render_frame(const ViewportRenderRequest &request) = 0;
	/// Shut down rendering for the current surface.
	virtual void shutdown_surface() = 0;

	/// Return the latest frame stats if available.
	[[nodiscard]] virtual std::optional<ViewportFrameStats> latest_frame_stats() const = 0;
	/// Return the latest diagnostics snapshot if available.
	[[nodiscard]] virtual std::optional<ViewportDiagnosticsSnapshot> latest_diagnostics() const = 0;
	/// Return the renderer display name.
	[[nodiscard]] virtual QString renderer_name() const = 0;
};

} // namespace quader::ui
