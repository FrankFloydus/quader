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

#include "ui/actions/editor_state_snapshot.hpp"
#include "ui/qt_app/ui_context.hpp"
#include "ui/viewport/viewport_types.hpp"

#include <memory>

#include <QMainWindow>

class QLabel;
class QCloseEvent;

namespace quader::ui {

class IViewportRenderHost;
class PanelHost;
class ViewportController;
class ViewportWidget;

/// Top-level Qt shell for menus, panels, status UI, and viewport hosting.
class MainWindow final : public QMainWindow {
	Q_OBJECT

public:
	/// Construct the main window over a non-owning UI context.
	explicit MainWindow(UiContext &context, QWidget *parent = nullptr);
	/// Destroy the viewport host and child widgets.
	~MainWindow() override;

protected:
	/// Save workspace state before accepting close.
	void closeEvent(QCloseEvent *event) override;

private:
	void build_menus();
	void build_central_area();
	void build_dock_hosts();
	void build_status_ui();
	void connect_actions();
	void connect_notifications();
	void connect_viewport();
	void restore_workspace();
	void save_workspace();
	void reset_workspace();
	[[nodiscard]] EditorStateSnapshot editor_state_with_viewport() const;
	void update_stats_label(const ViewportFrameStats &stats);
	void update_diagnostics_label();

	UiContext &context_;
	std::unique_ptr<IViewportRenderHost> viewport_render_host_;
	std::unique_ptr<ViewportController> viewport_controller_;
	PanelHost *panel_host_ = nullptr;
	ViewportWidget *viewport_ = nullptr;
	QLabel *renderer_label_ = nullptr;
	QLabel *stats_label_ = nullptr;
	QLabel *diagnostics_label_ = nullptr;
	bool quad_viewports_enabled_ = false;
	ViewportDisplaySettings viewport_display_settings_;
};

} // namespace quader::ui
