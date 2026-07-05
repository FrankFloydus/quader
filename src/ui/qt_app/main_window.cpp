/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
#include "ui/qt_app/main_window.hpp"

#include "ui/actions/action_registry.hpp"
#include "ui/actions/action_state_updater.hpp"
#include "ui/panels/diagnostics_panel.hpp"
#include "ui/panels/panel_host.hpp"
#include "ui/panels/properties_panel.hpp"
#include "ui/panels/scene_panel.hpp"
#include "ui/services/document_ui_controller.hpp"
#include "ui/services/import_ui_controller.hpp"
#include "ui/services/notification_service.hpp"
#include "ui/services/settings_service.hpp"
#include "ui/services/viewport_diagnostics_service.hpp"
#include "ui/viewport/crimson_viewport_render_host.hpp"
#include "ui/viewport/viewport_controller.hpp"
#include "ui/viewport/viewport_widget.hpp"

#include <QAction>
#include <QCloseEvent>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QStatusBar>
#include <QWidget>

namespace quader::ui {
namespace {

QString notification_text(const NotificationMessage &message) {
	if (message.detail.isEmpty()) {
		return message.summary;
	}

	return QStringLiteral("%1: %2").arg(message.summary, message.detail);
}

} // namespace

MainWindow::MainWindow(UiContext &context, QWidget *parent) : QMainWindow(parent), context_(context) {
	setObjectName(QStringLiteral("quader.main_window"));
	setWindowTitle(QStringLiteral("Quader"));
	resize(1280, 800);

	build_central_area();
	build_menus();
	build_dock_hosts();
	build_status_ui();
	connect_notifications();
	connect_actions();
	connect_viewport();
	restore_workspace();

	context_.action_state_updater.refresh_from_snapshot(editor_state_with_viewport());
}

MainWindow::~MainWindow() {
	if (QWidget *central = takeCentralWidget()) {
		delete central;
		viewport_ = nullptr;
	} else if (viewport_controller_ != nullptr) {
		viewport_controller_->shutdown_surface();
	}
	context_.viewport_diagnostics.clear_render_host();
}

void MainWindow::closeEvent(QCloseEvent *event) {
	save_workspace();
	QMainWindow::closeEvent(event);
}

void MainWindow::build_menus() {
	auto *file_menu = menuBar()->addMenu(QStringLiteral("&File"));
	file_menu->addAction(&context_.actions.action(ActionId::NewScene));
	file_menu->addAction(&context_.actions.action(ActionId::OpenScene));
	file_menu->addSeparator();
	file_menu->addAction(&context_.actions.action(ActionId::Exit));

	auto *edit_menu = menuBar()->addMenu(QStringLiteral("&Edit"));
	edit_menu->addAction(&context_.actions.action(ActionId::Undo));
	edit_menu->addAction(&context_.actions.action(ActionId::Redo));
	edit_menu->addSeparator();
	edit_menu->addAction(&context_.actions.action(ActionId::DuplicateSelection));
	edit_menu->addAction(&context_.actions.action(ActionId::DeleteSelection));

	auto *tools_menu = menuBar()->addMenu(QStringLiteral("&Tools"));
	tools_menu->addAction(&context_.actions.action(ActionId::SelectTool));
	tools_menu->addAction(&context_.actions.action(ActionId::MoveTool));
	tools_menu->addAction(&context_.actions.action(ActionId::RotateTool));
	tools_menu->addAction(&context_.actions.action(ActionId::ScaleTool));
	tools_menu->addAction(&context_.actions.action(ActionId::BoxTool));

	auto *create_menu = menuBar()->addMenu(QStringLiteral("&Create"));
	create_menu->addAction(&context_.actions.action(ActionId::CreateCube));
	create_menu->addAction(&context_.actions.action(ActionId::CreateLight));
	create_menu->addAction(&context_.actions.action(ActionId::CreateCamera));

	auto *view_menu = menuBar()->addMenu(QStringLiteral("&View"));
	view_menu->addAction(&context_.actions.action(ActionId::ViewPerspective));
	view_menu->addAction(&context_.actions.action(ActionId::ViewShaded));
	view_menu->addSeparator();
	view_menu->addAction(&context_.actions.action(ActionId::ToggleQuadViewports));
	auto *panels_menu = view_menu->addMenu(QStringLiteral("Panels"));
	panels_menu->addAction(&context_.actions.action(ActionId::ShowScenePanel));
	panels_menu->addAction(&context_.actions.action(ActionId::ShowPropertiesPanel));
	panels_menu->addAction(&context_.actions.action(ActionId::ShowDiagnosticsPanel));

	auto *layout_menu = menuBar()->addMenu(QStringLiteral("&Layout"));
	layout_menu->addAction(&context_.actions.action(ActionId::ResetWorkspaceLayout));
}

void MainWindow::build_central_area() {
	viewport_render_host_ = create_crimson_viewport_render_host(context_.documents.document(), context_.tools);
	context_.viewport_diagnostics.attach_render_host(*viewport_render_host_);
	viewport_controller_ = std::make_unique<ViewportController>(*viewport_render_host_, context_.tools);
	viewport_ = new ViewportWidget(*viewport_controller_, this);
	setCentralWidget(viewport_);
}

void MainWindow::build_dock_hosts() {
	auto panel_context = PanelContext{ context_ };
	panel_host_ = new PanelHost(*this, panel_context, this);

	auto *scene_panel = new ScenePanel(panel_context);
	panel_host_->add_panel(
			PanelSpec{
					PanelId::Scene,
					QStringLiteral("Scene"),
					Qt::RightDockWidgetArea,
					Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea,
			},
			scene_panel);

	auto *properties_panel = new PropertiesPanel(panel_context);
	panel_host_->add_panel(
			PanelSpec{
					PanelId::Properties,
					QStringLiteral("Properties"),
					Qt::RightDockWidgetArea,
					Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea,
			},
			properties_panel);

	auto *diagnostics_panel = new DiagnosticsPanel(panel_context);
	panel_host_->add_panel(
			PanelSpec{
					PanelId::Diagnostics,
					QStringLiteral("Diagnostics"),
					Qt::BottomDockWidgetArea,
					Qt::BottomDockWidgetArea | Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea,
					false,
			},
			diagnostics_panel);

	panel_host_->bind_panel_visibility_action(PanelId::Scene, context_.actions, ActionId::ShowScenePanel);
	panel_host_->bind_panel_visibility_action(PanelId::Properties, context_.actions, ActionId::ShowPropertiesPanel);
	panel_host_->bind_panel_visibility_action(PanelId::Diagnostics, context_.actions, ActionId::ShowDiagnosticsPanel);
}

void MainWindow::build_status_ui() {
	renderer_label_ = new QLabel(QStringLiteral("Renderer: starting"), this);
	stats_label_ = new QLabel(QStringLiteral("Waiting for first frame"), this);
	diagnostics_label_ = new QLabel(QStringLiteral("Diagnostics: unavailable"), this);
	statusBar()->addPermanentWidget(renderer_label_);
	statusBar()->addPermanentWidget(stats_label_);
	statusBar()->addPermanentWidget(diagnostics_label_);
	statusBar()->showMessage(QStringLiteral("Ready"));
	update_diagnostics_label();
}

void MainWindow::connect_actions() {
	connect(&context_.actions, &ActionRegistry::action_triggered, this, [this](ActionId id) {
		switch (id) {
			case ActionId::Exit:
				close();
				break;
			case ActionId::OpenScene:
				(void)context_.imports.open_scene(this);
				break;
			case ActionId::FocusViewport:
				if (viewport_ != nullptr) {
					viewport_->setFocus(Qt::OtherFocusReason);
				}
				break;
			case ActionId::ResetWorkspaceLayout:
				reset_workspace();
				break;
			default:
				break;
		}
	});

	connect(&context_.actions, &ActionRegistry::action_toggled, this, [this](ActionId id, bool checked) {
		if (id == ActionId::ToggleQuadViewports && viewport_controller_ != nullptr) {
			viewport_controller_->set_quad_viewports_enabled(checked);
		}
	});
}

void MainWindow::connect_notifications() {
	connect(&context_.notifications,
			&NotificationService::notification_posted,
			this,
			[this](const NotificationMessage &message) {
				const QString kText = notification_text(message);
				const int kTimeout = message.timeout_ms < 0 ? 0 : message.timeout_ms;

				if (message.severity == NotificationSeverity::Error && renderer_label_ != nullptr) {
					renderer_label_->setText(QStringLiteral("Renderer: error"));
				}

				statusBar()->showMessage(kText, kTimeout);
			});
}

void MainWindow::connect_viewport() {
	connect(viewport_controller_.get(), &ViewportController::renderer_ready, this, [this](const QString &renderer_name) {
		renderer_label_->setText(QStringLiteral("Renderer: %1").arg(renderer_name));
		context_.notifications.show_status(QStringLiteral("Crimson initialized with %1").arg(renderer_name), 3000);
		context_.viewport_diagnostics.refresh();
	});

	connect(viewport_controller_.get(), &ViewportController::frame_stats_changed, this, [this](const ViewportFrameStats &stats) {
		update_stats_label(stats);
		context_.viewport_diagnostics.refresh();
	});

	connect(viewport_controller_.get(), &ViewportController::render_error, this, [this](const QString &summary, const QString &detail) {
		context_.notifications.show_error(NotificationMessage{
				NotificationSeverity::Error,
				summary,
				detail,
				0,
		});
		context_.viewport_diagnostics.refresh();
	});

	connect(viewport_controller_.get(), &ViewportController::viewport_layout_changed, this, [this](bool quad_enabled, const QString &layout_name) {
		quad_viewports_enabled_ = quad_enabled;
		context_.action_state_updater.refresh_from_snapshot(editor_state_with_viewport());
		context_.notifications.show_status(layout_name, 2000);
	});

	connect(&context_.viewport_diagnostics,
			&ViewportDiagnosticsService::summary_changed,
			this,
			&MainWindow::update_diagnostics_label);
	connect(&context_.viewport_diagnostics,
			&ViewportDiagnosticsService::diagnostics_unavailable,
			this,
			&MainWindow::update_diagnostics_label);
}

void MainWindow::restore_workspace() {
	const QByteArray kGeometry = context_.settings.main_window_geometry();
	if (!kGeometry.isEmpty()) {
		restoreGeometry(kGeometry);
	}

	const QByteArray kState = context_.settings.main_window_state();
	bool state_restored = false;
	if (!kState.isEmpty()) {
		state_restored = restoreState(kState, SettingsService::kWorkspaceStateVersion);
	}

	if (!state_restored && panel_host_ != nullptr) {
		panel_host_->reset_to_default_layout();
	}

	if (panel_host_ != nullptr) {
		panel_host_->restore_workspace(context_.settings);
	}
}

void MainWindow::save_workspace() {
	context_.settings.set_main_window_geometry(saveGeometry());
	context_.settings.set_main_window_state(saveState(SettingsService::kWorkspaceStateVersion));
	if (panel_host_ != nullptr) {
		panel_host_->save_workspace(context_.settings);
	}
	context_.settings.sync();
}

void MainWindow::reset_workspace() {
	context_.settings.reset_workspace();

	if (panel_host_ != nullptr) {
		panel_host_->reset_to_default_layout();
	}
	if (viewport_ != nullptr) {
		viewport_controller_->set_quad_viewports_enabled(false);
	}

	resize(1280, 800);
	context_.notifications.show_status(QStringLiteral("Workspace layout reset"), 2000);
}

EditorStateSnapshot MainWindow::editor_state_with_viewport() const {
	EditorStateSnapshot snapshot = context_.editor_state.editor_state_snapshot();
	snapshot.viewport_available = viewport_ != nullptr;
	snapshot.viewport_state_known = true;
	snapshot.quad_viewports_enabled = quad_viewports_enabled_;
	return snapshot;
}

void MainWindow::update_stats_label(const ViewportFrameStats &stats) {
	if (stats_label_ == nullptr) {
		return;
	}

	stats_label_->setText(QStringLiteral("%1x%2 | %3 view%4 | %5 fps")
					.arg(stats.width)
					.arg(stats.height)
					.arg(stats.viewport_count)
					.arg(stats.viewport_count == 1 ? QString() : QStringLiteral("s"))
					.arg(stats.fps, 0, 'f', 0));
}

void MainWindow::update_diagnostics_label() {
	if (diagnostics_label_ == nullptr) {
		return;
	}

	const ViewportDiagnosticsSummary kSummary = context_.viewport_diagnostics.summary();
	diagnostics_label_->setText(kSummary.status_text);
	diagnostics_label_->setToolTip(kSummary.tooltip_text);
}

} // namespace quader::ui
