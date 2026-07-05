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

#include "ui/panels/panel_context.hpp"
#include "ui/panels/panel_id.hpp"

#include <QDockWidget>
#include <QMainWindow>
#include <QObject>

#include <unordered_map>

namespace quader::ui {

class ActionRegistry;
class SettingsService;
enum class ActionId : std::uint16_t;

/// Declarative dock-panel registration metadata.
struct PanelSpec {
	/// Stable panel id.
	PanelId id = PanelId::Scene;
	/// Dock title.
	QString title;
	/// Default dock area.
	Qt::DockWidgetArea default_area = Qt::RightDockWidgetArea;
	/// Areas where the dock may be placed.
	Qt::DockWidgetAreas allowed_areas = Qt::AllDockWidgetAreas;
	/// Initial visibility after default-layout reset.
	bool default_visible = true;
};

/// Owns dock widgets and workspace persistence for application panels.
class PanelHost final : public QObject {
	Q_OBJECT

public:
	/// Construct a panel host for `main_window`.
	explicit PanelHost(QMainWindow &main_window, PanelContext context, QObject *parent = nullptr);

	/// Add a panel widget wrapped in a dock widget.
	QDockWidget &add_panel(PanelSpec spec, QWidget *panel_widget);
	/// Return the dock for `id`, or `nullptr` when not registered.
	[[nodiscard]] QDockWidget *dock(PanelId id) const;

	/// Show a registered panel.
	void show_panel(PanelId id);
	/// Hide a registered panel.
	void hide_panel(PanelId id);
	/// Bind a checkable action to one panel's visibility.
	void bind_panel_visibility_action(PanelId panel, ActionRegistry &actions, ActionId action);
	/// Save dock visibility and main-window workspace state.
	void save_workspace(SettingsService &settings) const;
	/// Restore dock visibility and main-window workspace state.
	void restore_workspace(SettingsService &settings);
	/// Reset all docks to their default areas and visibility.
	void reset_to_default_layout();

private:
	QMainWindow &main_window_;
	PanelContext context_;
	std::unordered_map<PanelId, QDockWidget *> docks_;
	std::unordered_map<PanelId, PanelSpec> specs_;
};

} // namespace quader::ui
