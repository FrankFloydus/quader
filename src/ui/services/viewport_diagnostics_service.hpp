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

#include "ui/viewport/viewport_render_host.hpp"

#include <QObject>
#include <QString>

#include <optional>

namespace quader::ui {

/// Compact summary derived from the latest viewport diagnostics snapshot.
struct ViewportDiagnosticsSummary {
	/// True when diagnostics data is available.
	bool available = false;
	/// Renderer display name.
	QString renderer_name;
	/// Last frame stats.
	ViewportFrameStats frame;
	/// Number of render pass rows.
	int pass_count = 0;
	/// Number of counter rows.
	int counter_count = 0;
	/// Number of diagnostic rows.
	int diagnostic_count = 0;
	/// Number of warning diagnostics.
	int warning_count = 0;
	/// Number of error diagnostics.
	int error_count = 0;
	/// Short status text for the shell.
	QString status_text;
	/// Detail text for tooltips.
	QString tooltip_text;
};

/// Tracks the active viewport render host and exposes latest diagnostics.
class ViewportDiagnosticsService final : public QObject {
	Q_OBJECT

public:
	/// Construct an empty diagnostics service.
	explicit ViewportDiagnosticsService(QObject *parent = nullptr);

	/// Attach a render host as the diagnostics provider.
	void attach_render_host(IViewportRenderHost &render_host) noexcept;
	/// Detach the render host if it is the current provider.
	void detach_render_host(const IViewportRenderHost &render_host) noexcept;
	/// Clear the current render host provider.
	void clear_render_host() noexcept;

	/// Refresh the latest snapshot from the provider.
	void refresh();

	/// Return the latest diagnostics snapshot, if available.
	[[nodiscard]] const std::optional<ViewportDiagnosticsSnapshot> &latest_snapshot() const noexcept;
	/// Return a compact summary of the latest snapshot.
	[[nodiscard]] ViewportDiagnosticsSummary summary() const;
	/// Return true when a render host provider is attached.
	[[nodiscard]] bool has_provider() const noexcept;

Q_SIGNALS:
	/// Emitted when diagnostics data changes.
	void diagnostics_changed();
	/// Emitted when diagnostics become unavailable.
	void diagnostics_unavailable();
	/// Emitted when the summary may have changed.
	void summary_changed();

private:
	void clear_snapshot_and_notify(bool emit_unavailable);

	IViewportRenderHost *render_host_ = nullptr;
	std::optional<ViewportDiagnosticsSnapshot> latest_snapshot_;
};

} // namespace quader::ui
