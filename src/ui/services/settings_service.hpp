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

#include "ui/panels/panel_id.hpp"
#include "ui/viewport/viewport_types.hpp"

#include <QByteArray>
#include <QSettings>
#include <QStringView>
#include <QVariant>

namespace quader::ui {

/// Scoped wrapper around `QSettings` for workspace and viewport state.
class SettingsService final {
public:
	/// Version for persisted workspace layout state.
	static constexpr int kWorkspaceStateVersion = 2;

	/// Construct a settings facade over an externally owned `QSettings`.
	explicit SettingsService(QSettings &settings);

	/// Read a raw settings value.
	[[nodiscard]] QVariant value(QStringView key, const QVariant &fallback = {}) const;
	/// Write a raw settings value.
	void set_value(QStringView key, const QVariant &value);
	/// Remove a settings group.
	void remove_group(QStringView group);
	/// Flush pending settings writes.
	void sync();

	/// Return saved main-window geometry.
	[[nodiscard]] QByteArray main_window_geometry() const;
	/// Store main-window geometry.
	void set_main_window_geometry(const QByteArray &geometry);

	/// Return saved main-window dock state.
	[[nodiscard]] QByteArray main_window_state() const;
	/// Store main-window dock state.
	void set_main_window_state(const QByteArray &state);

	/// Read a viewport-scoped setting.
	[[nodiscard]] QVariant viewport_value(QStringView key, const QVariant &fallback = {}) const;
	/// Write a viewport-scoped setting.
	void set_viewport_value(QStringView key, const QVariant &value);
	/// Return typed viewport display settings.
	[[nodiscard]] ViewportDisplaySettings viewport_display_settings() const;
	/// Store typed viewport display settings.
	void set_viewport_display_settings(const ViewportDisplaySettings &settings);

	/// Read a workspace-scoped setting.
	[[nodiscard]] QVariant workspace_value(QStringView key, const QVariant &fallback = {}) const;
	/// Write a workspace-scoped setting.
	void set_workspace_value(QStringView key, const QVariant &value);

	/// Return saved panel visibility.
	[[nodiscard]] bool panel_visible(PanelId id, bool fallback) const;
	/// Store panel visibility.
	void set_panel_visible(PanelId id, bool visible);

	/// Remove persisted workspace state.
	void reset_workspace();

private:
	QSettings &settings_;
};

} // namespace quader::ui
