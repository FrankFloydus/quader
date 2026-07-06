/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
#include "ui/actions/action_registry.hpp"
#include "ui/actions/action_state_updater.hpp"
#include "ui/actions/editor_state_snapshot.hpp"
#include "ui/models/diagnostics_item_model.hpp"
#include "ui/models/document_item_model.hpp"
#include "ui/models/property_item_model.hpp"
#include "ui/panels/diagnostics_panel.hpp"
#include "ui/panels/panel_host.hpp"
#include "ui/panels/properties_panel.hpp"
#include "ui/panels/scene_panel.hpp"
#include "ui/qt_app/main_window.hpp"
#include "ui/qt_app/ui_context.hpp"
#include "ui/services/document_ui_controller.hpp"
#include "ui/services/file_dialog_service.hpp"
#include "ui/services/import_ui_controller.hpp"
#include "ui/services/notification_service.hpp"
#include "ui/services/settings_service.hpp"
#include "ui/services/viewport_diagnostics_service.hpp"

#include <gtest/gtest.h>

#include "commands/command_history.hpp"
#include "document/document.hpp"
#include "io/import_export_registry.hpp"
#include "io/import_service.hpp"
#include "tools/tool_manager.hpp"

#include <QAbstractItemModel>
#include <QApplication>
#include <QDockWidget>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QPointer>
#include <QSettings>
#include <QTableView>
#include <QTemporaryDir>
#include <QTreeView>
#include <QWidget>

#include <array>
#include <cstdio>
#include <memory>
#include <string>

namespace {

constexpr std::array<quader::ui::ActionId, 32> kStandardActions = {
	quader::ui::ActionId::NewScene,
	quader::ui::ActionId::OpenScene,
	quader::ui::ActionId::SaveScene,
	quader::ui::ActionId::SaveSceneAs,
	quader::ui::ActionId::Exit,
	quader::ui::ActionId::Undo,
	quader::ui::ActionId::Redo,
	quader::ui::ActionId::SelectAll,
	quader::ui::ActionId::ClearSelection,
	quader::ui::ActionId::InvertSelection,
	quader::ui::ActionId::DuplicateSelection,
	quader::ui::ActionId::DeleteSelection,
	quader::ui::ActionId::SelectTool,
	quader::ui::ActionId::MoveTool,
	quader::ui::ActionId::RotateTool,
	quader::ui::ActionId::ScaleTool,
	quader::ui::ActionId::BoxTool,
	quader::ui::ActionId::SelectObjectMode,
	quader::ui::ActionId::SelectVertexMode,
	quader::ui::ActionId::SelectEdgeMode,
	quader::ui::ActionId::SelectFaceMode,
	quader::ui::ActionId::FlipMeshNormals,
	quader::ui::ActionId::CreateLight,
	quader::ui::ActionId::CreateCamera,
	quader::ui::ActionId::ViewPerspective,
	quader::ui::ActionId::ViewShaded,
	quader::ui::ActionId::ToggleQuadViewports,
	quader::ui::ActionId::FocusViewport,
	quader::ui::ActionId::ShowScenePanel,
	quader::ui::ActionId::ShowPropertiesPanel,
	quader::ui::ActionId::ShowDiagnosticsPanel,
	quader::ui::ActionId::ResetWorkspaceLayout,
};

struct UiFixture {
	QTemporaryDir settings_dir;
	QSettings settings_store;
	quader::document::Document document;
	quader::commands::CommandHistory command_history;
	quader::tools::ToolManager tool_manager;
	quader::io::ImportExportRegistry io_registry;
	quader::io::ImportService import_service;
	quader::ui::ActionRegistry actions;
	quader::ui::NullEditorStateProvider editor_state;
	quader::ui::ActionStateUpdater action_state_updater;
	quader::ui::SettingsService settings;
	quader::ui::QtFileDialogService file_dialogs;
	quader::ui::NotificationService notifications;
	quader::ui::ViewportDiagnosticsService viewport_diagnostics;
	quader::ui::DocumentUiController document_ui;
	quader::ui::ImportUiController import_ui;
	quader::ui::UiContext context;

	UiFixture() : settings_store(settings_dir.path() + QStringLiteral("/settings.ini"), QSettings::IniFormat), tool_manager(quader::tools::ToolContext{ document, command_history }), import_service(io_registry), action_state_updater(actions, editor_state), settings(settings_store), document_ui(document, command_history, action_state_updater, notifications), import_ui(file_dialogs, io_registry, import_service, notifications), context{
				actions,
				action_state_updater,
				editor_state,
				settings,
				notifications,
				document_ui,
				import_ui,
				viewport_diagnostics,
				tool_manager,
			} {
		quader::ui::register_standard_actions(actions);
		action_state_updater.refresh();
	}
};

TEST(UiShell, ActionRegistryRegistersCanonicalStandardActions) {
	quader::ui::ActionRegistry registry;
	quader::ui::register_standard_actions(registry);

	EXPECT_EQ(registry.size(), kStandardActions.size());
	for (const quader::ui::ActionId kId : kStandardActions) {
		EXPECT_TRUE(registry.contains(kId));
		EXPECT_NE(quader::ui::action_id_name(kId), std::string_view("Unknown"));
	}

	QAction &first_lookup = registry.action(quader::ui::ActionId::ToggleQuadViewports);
	QAction &second_lookup = registry.action(quader::ui::ActionId::ToggleQuadViewports);
	EXPECT_TRUE(&first_lookup == &second_lookup);
	EXPECT_TRUE(first_lookup.isCheckable());
	EXPECT_EQ(first_lookup.shortcut(), QKeySequence(Qt::Key_F1));

	quader::ui::register_standard_actions(registry);
	EXPECT_EQ(registry.size(), kStandardActions.size());
}

TEST(UiShell, ActionStateUpdaterDisablesUnavailableDocumentToolAndCreateActions) {
	UiFixture fixture;

	EXPECT_FALSE(fixture.actions.action(quader::ui::ActionId::NewScene).isEnabled());
	EXPECT_FALSE(fixture.actions.action(quader::ui::ActionId::OpenScene).isEnabled());
	EXPECT_FALSE(fixture.actions.action(quader::ui::ActionId::SaveScene).isEnabled());
	EXPECT_FALSE(fixture.actions.action(quader::ui::ActionId::SaveSceneAs).isEnabled());
	EXPECT_FALSE(fixture.actions.action(quader::ui::ActionId::Undo).isEnabled());
	EXPECT_FALSE(fixture.actions.action(quader::ui::ActionId::Redo).isEnabled());
	EXPECT_FALSE(fixture.actions.action(quader::ui::ActionId::SelectAll).isEnabled());
	EXPECT_FALSE(fixture.actions.action(quader::ui::ActionId::ClearSelection).isEnabled());
	EXPECT_FALSE(fixture.actions.action(quader::ui::ActionId::InvertSelection).isEnabled());
	EXPECT_FALSE(fixture.actions.action(quader::ui::ActionId::DuplicateSelection).isEnabled());
	EXPECT_FALSE(fixture.actions.action(quader::ui::ActionId::DeleteSelection).isEnabled());
	EXPECT_FALSE(fixture.actions.action(quader::ui::ActionId::SelectTool).isEnabled());
	EXPECT_FALSE(fixture.actions.action(quader::ui::ActionId::MoveTool).isEnabled());
	EXPECT_FALSE(fixture.actions.action(quader::ui::ActionId::RotateTool).isEnabled());
	EXPECT_FALSE(fixture.actions.action(quader::ui::ActionId::ScaleTool).isEnabled());
	EXPECT_FALSE(fixture.actions.action(quader::ui::ActionId::SelectObjectMode).isEnabled());
	EXPECT_FALSE(fixture.actions.action(quader::ui::ActionId::SelectVertexMode).isEnabled());
	EXPECT_FALSE(fixture.actions.action(quader::ui::ActionId::SelectEdgeMode).isEnabled());
	EXPECT_FALSE(fixture.actions.action(quader::ui::ActionId::SelectFaceMode).isEnabled());
	EXPECT_FALSE(fixture.actions.action(quader::ui::ActionId::FlipMeshNormals).isEnabled());
	EXPECT_FALSE(fixture.actions.action(quader::ui::ActionId::CreateLight).isEnabled());
	EXPECT_FALSE(fixture.actions.action(quader::ui::ActionId::CreateCamera).isEnabled());

	EXPECT_TRUE(fixture.actions.action(quader::ui::ActionId::Exit).isEnabled());
	EXPECT_TRUE(fixture.actions.action(quader::ui::ActionId::ToggleQuadViewports).isEnabled());
	EXPECT_TRUE(fixture.actions.action(quader::ui::ActionId::FocusViewport).isEnabled());
	EXPECT_TRUE(fixture.actions.action(quader::ui::ActionId::ShowScenePanel).isEnabled());
	EXPECT_TRUE(fixture.actions.action(quader::ui::ActionId::ShowPropertiesPanel).isEnabled());
	EXPECT_TRUE(fixture.actions.action(quader::ui::ActionId::ShowDiagnosticsPanel).isEnabled());
	EXPECT_TRUE(fixture.actions.action(quader::ui::ActionId::ResetWorkspaceLayout).isEnabled());
}

TEST(UiShell, ActionStateUpdaterUsesEditorSnapshot) {
	UiFixture fixture;

	quader::ui::EditorStateSnapshot snapshot;
	snapshot.document_services_available = true;
	snapshot.new_scene_available = true;
	snapshot.file_io_available = true;
	snapshot.import_available = true;
	snapshot.export_available = true;
	snapshot.has_active_document = true;
	snapshot.document_dirty = true;
	snapshot.has_selection = true;
	snapshot.can_select_all = true;
	snapshot.can_clear_selection = true;
	snapshot.can_invert_selection = true;
	snapshot.can_duplicate_selection = true;
	snapshot.can_delete_selection = true;
	snapshot.can_flip_mesh_normals = true;
	snapshot.can_undo = true;
	snapshot.undo_text = QStringLiteral("Undo Move Selection");
	snapshot.can_redo = true;
	snapshot.redo_text = QStringLiteral("Redo Move Selection");
	snapshot.tools_available = true;
	snapshot.creation_available = true;
	snapshot.active_tool = quader::ui::ActionId::MoveTool;
	snapshot.active_selection_mode = quader::ui::ActionId::SelectFaceMode;
	snapshot.viewport_available = true;
	snapshot.viewport_state_known = true;
	snapshot.quad_viewports_enabled = true;

	fixture.action_state_updater.refresh_from_snapshot(snapshot);

	EXPECT_TRUE(fixture.actions.action(quader::ui::ActionId::NewScene).isEnabled());
	EXPECT_TRUE(fixture.actions.action(quader::ui::ActionId::SaveScene).isEnabled());
	EXPECT_TRUE(fixture.actions.action(quader::ui::ActionId::Undo).isEnabled());
	EXPECT_EQ(fixture.actions.action(quader::ui::ActionId::Undo).text(), QStringLiteral("Undo Move Selection"));
	EXPECT_TRUE(fixture.actions.action(quader::ui::ActionId::SelectAll).isEnabled());
	EXPECT_TRUE(fixture.actions.action(quader::ui::ActionId::ClearSelection).isEnabled());
	EXPECT_TRUE(fixture.actions.action(quader::ui::ActionId::InvertSelection).isEnabled());
	EXPECT_TRUE(fixture.actions.action(quader::ui::ActionId::DuplicateSelection).isEnabled());
	EXPECT_TRUE(fixture.actions.action(quader::ui::ActionId::DeleteSelection).isEnabled());
	EXPECT_TRUE(fixture.actions.action(quader::ui::ActionId::MoveTool).isChecked());
	EXPECT_FALSE(fixture.actions.action(quader::ui::ActionId::SelectTool).isChecked());
	EXPECT_TRUE(fixture.actions.action(quader::ui::ActionId::SelectFaceMode).isChecked());
	EXPECT_FALSE(fixture.actions.action(quader::ui::ActionId::SelectObjectMode).isChecked());
	EXPECT_TRUE(fixture.actions.action(quader::ui::ActionId::FlipMeshNormals).isEnabled());
	EXPECT_TRUE(fixture.actions.action(quader::ui::ActionId::ToggleQuadViewports).isChecked());
}

TEST(UiShell, SettingsServiceUsesWorkspaceKeysAndResetScope) {
	QTemporaryDir settings_dir;
	QSettings settings_store(settings_dir.path() + QStringLiteral("/settings.ini"), QSettings::IniFormat);
	quader::ui::SettingsService settings(settings_store);

	const QByteArray kGeometry("geometry");
	const QByteArray kState("state");
	settings.set_main_window_geometry(kGeometry);
	settings.set_main_window_state(kState);
	settings.set_value(QStringLiteral("app/unrelated"), QStringLiteral("keep"));
	settings.set_value(QStringLiteral("ui/workspace/v1/legacy"), QStringLiteral("keep"));
	settings.set_viewport_value(QStringLiteral("layout_mode"), QStringLiteral("quad"));
	settings.set_panel_visible(quader::ui::PanelId::Scene, false);
	settings.set_panel_visible(quader::ui::PanelId::Diagnostics, true);
	settings.set_workspace_value(QStringLiteral("custom/value"), QStringLiteral("workspace"));

	EXPECT_EQ(settings.main_window_geometry(), kGeometry);
	EXPECT_EQ(settings.main_window_state(), kState);
	EXPECT_EQ(settings_store.value(QStringLiteral("ui/workspace/v2/main_window/state_version")).toInt(),
			quader::ui::SettingsService::kWorkspaceStateVersion);
	EXPECT_EQ(settings.viewport_value(QStringLiteral("layout_mode")).toString(), QStringLiteral("quad"));
	EXPECT_FALSE(settings.panel_visible(quader::ui::PanelId::Scene, true));
	EXPECT_TRUE(settings.panel_visible(quader::ui::PanelId::Diagnostics, false));
	EXPECT_EQ(settings.workspace_value(QStringLiteral("custom/value")).toString(), QStringLiteral("workspace"));

	settings.reset_workspace();
	EXPECT_TRUE(settings.main_window_geometry().isEmpty());
	EXPECT_TRUE(settings.main_window_state().isEmpty());
	EXPECT_TRUE(settings.panel_visible(quader::ui::PanelId::Scene, true));
	EXPECT_FALSE(settings.panel_visible(quader::ui::PanelId::Diagnostics, false));
	EXPECT_EQ(settings.value(QStringLiteral("app/unrelated")).toString(), QStringLiteral("keep"));
	EXPECT_EQ(settings.value(QStringLiteral("ui/workspace/v1/legacy")).toString(), QStringLiteral("keep"));
}

TEST(UiShell, NotificationServiceEmitsTypedMessages) {
	quader::ui::NotificationService notifications;
	quader::ui::NotificationMessage last_message;
	int posted_count = 0;

	QObject::connect(&notifications,
			&quader::ui::NotificationService::notification_posted,
			&notifications,
			[&last_message, &posted_count](const quader::ui::NotificationMessage &message) {
				last_message = message;
				++posted_count;
			});

	notifications.show_status(QStringLiteral("Ready"), 1200);
	EXPECT_EQ(posted_count, 1);
	EXPECT_EQ(last_message.severity, quader::ui::NotificationSeverity::Status);
	EXPECT_EQ(last_message.summary, QStringLiteral("Ready"));
	EXPECT_EQ(last_message.timeout_ms, 1200);

	notifications.show_error(quader::ui::NotificationMessage{
			quader::ui::NotificationSeverity::Status,
			QStringLiteral("Renderer failed"),
			QStringLiteral("No adapter"),
			-1,
	});
	EXPECT_EQ(posted_count, 2);
	EXPECT_EQ(last_message.severity, quader::ui::NotificationSeverity::Error);
	EXPECT_EQ(last_message.timeout_ms, 0);
}

TEST(UiShell, SceneAndPropertiesPanelsConstructWithoutGpuSurface) {
	UiFixture fixture;
	quader::ui::PanelContext panel_context{ fixture.context };

	quader::ui::ScenePanel scene_panel(panel_context);
	quader::ui::PropertiesPanel properties_panel(panel_context);
	quader::ui::DiagnosticsPanel diagnostics_panel(panel_context);

	EXPECT_TRUE(&scene_panel.model() != nullptr);
	EXPECT_TRUE(&scene_panel.tree_view() != nullptr);
	EXPECT_TRUE(scene_panel.tree_view().model() == static_cast<QAbstractItemModel *>(&scene_panel.model()));
	EXPECT_TRUE(&properties_panel.model() != nullptr);
	EXPECT_TRUE(&properties_panel.table_view() != nullptr);
	EXPECT_TRUE(properties_panel.table_view().model() == static_cast<QAbstractItemModel *>(&properties_panel.model()));
	EXPECT_TRUE(&diagnostics_panel.model() != nullptr);
	EXPECT_TRUE(&diagnostics_panel.tree_view() != nullptr);
	EXPECT_TRUE(diagnostics_panel.tree_view().model() == static_cast<QAbstractItemModel *>(&diagnostics_panel.model()));
}

TEST(UiShell, PanelHostCreatesStableDocksAndVisibilityActions) {
	UiFixture fixture;
	QMainWindow host_window;
	quader::ui::PanelHost panel_host(host_window, quader::ui::PanelContext{ fixture.context }, &host_window);

	auto *scene_panel = new QWidget;
	QDockWidget &scene_dock = panel_host.add_panel(
			quader::ui::PanelSpec{
					quader::ui::PanelId::Scene,
					QStringLiteral("Scene"),
					Qt::RightDockWidgetArea,
					Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea,
			},
			scene_panel);
	auto *properties_panel = new QWidget;
	QDockWidget &properties_dock = panel_host.add_panel(
			quader::ui::PanelSpec{
					quader::ui::PanelId::Properties,
					QStringLiteral("Properties"),
					Qt::RightDockWidgetArea,
					Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea,
			},
			properties_panel);
	auto *diagnostics_panel = new QWidget;
	QDockWidget &diagnostics_dock = panel_host.add_panel(
			quader::ui::PanelSpec{
					quader::ui::PanelId::Diagnostics,
					QStringLiteral("Diagnostics"),
					Qt::BottomDockWidgetArea,
					Qt::BottomDockWidgetArea | Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea,
					false,
			},
			diagnostics_panel);

	panel_host.bind_panel_visibility_action(
			quader::ui::PanelId::Scene, fixture.actions, quader::ui::ActionId::ShowScenePanel);
	panel_host.bind_panel_visibility_action(
			quader::ui::PanelId::Properties, fixture.actions, quader::ui::ActionId::ShowPropertiesPanel);
	panel_host.bind_panel_visibility_action(
			quader::ui::PanelId::Diagnostics, fixture.actions, quader::ui::ActionId::ShowDiagnosticsPanel);

	EXPECT_TRUE(panel_host.dock(quader::ui::PanelId::Scene) == &scene_dock);
	EXPECT_TRUE(panel_host.dock(quader::ui::PanelId::Properties) == &properties_dock);
	EXPECT_TRUE(panel_host.dock(quader::ui::PanelId::Diagnostics) == &diagnostics_dock);
	EXPECT_EQ(scene_dock.objectName(), QStringLiteral("quader.panel.scene"));
	EXPECT_EQ(properties_dock.objectName(), QStringLiteral("quader.panel.properties"));
	EXPECT_EQ(diagnostics_dock.objectName(), QStringLiteral("quader.panel.diagnostics"));
	EXPECT_TRUE(scene_dock.widget() == scene_panel);
	EXPECT_TRUE(properties_dock.widget() == properties_panel);
	EXPECT_TRUE(diagnostics_dock.widget() == diagnostics_panel);
	EXPECT_TRUE(scene_dock.parent() == &host_window);
	EXPECT_TRUE(diagnostics_dock.isHidden());
	EXPECT_FALSE(fixture.actions.action(quader::ui::ActionId::ShowDiagnosticsPanel).isChecked());

	QAction &scene_action = fixture.actions.action(quader::ui::ActionId::ShowScenePanel);
	EXPECT_TRUE(scene_action.isCheckable());
	EXPECT_TRUE(scene_action.isChecked());
	scene_action.trigger();
	EXPECT_TRUE(scene_dock.isHidden());
	EXPECT_FALSE(scene_action.isChecked());
	scene_action.trigger();
	EXPECT_FALSE(scene_dock.isHidden());
	EXPECT_TRUE(scene_action.isChecked());

	panel_host.hide_panel(quader::ui::PanelId::Properties);
	EXPECT_TRUE(properties_dock.isHidden());
	EXPECT_FALSE(fixture.actions.action(quader::ui::ActionId::ShowPropertiesPanel).isChecked());

	panel_host.reset_to_default_layout();
	EXPECT_FALSE(scene_dock.isHidden());
	EXPECT_FALSE(properties_dock.isHidden());
	EXPECT_TRUE(diagnostics_dock.isHidden());

	panel_host.save_workspace(fixture.settings);
	EXPECT_TRUE(fixture.settings.panel_visible(quader::ui::PanelId::Scene, false));
	EXPECT_TRUE(fixture.settings.panel_visible(quader::ui::PanelId::Properties, false));
	EXPECT_FALSE(fixture.settings.panel_visible(quader::ui::PanelId::Diagnostics, true));

	panel_host.hide_panel(quader::ui::PanelId::Scene);
	fixture.settings.set_panel_visible(quader::ui::PanelId::Scene, false);
	fixture.settings.set_panel_visible(quader::ui::PanelId::Diagnostics, true);
	panel_host.restore_workspace(fixture.settings);
	EXPECT_TRUE(scene_dock.isHidden());
	EXPECT_FALSE(diagnostics_dock.isHidden());
}

TEST(UiShell, MainWindowConstructsWithServicesWithoutShowingGpuSurface) {
	UiFixture fixture;
	quader::ui::MainWindow window(fixture.context);

	EXPECT_EQ(window.objectName(), QStringLiteral("quader.main_window"));
	EXPECT_TRUE(window.centralWidget() != nullptr);
	EXPECT_FALSE(fixture.actions.action(quader::ui::ActionId::NewScene).isEnabled());

	QMenu *file_menu = window.menuBar()->actions().at(0)->menu();
	EXPECT_TRUE(file_menu->actions().contains(&fixture.actions.action(quader::ui::ActionId::NewScene)));
	EXPECT_TRUE(file_menu->actions().contains(&fixture.actions.action(quader::ui::ActionId::OpenScene)));

	QMenu *edit_menu = window.menuBar()->actions().at(1)->menu();
	EXPECT_TRUE(edit_menu->actions().contains(&fixture.actions.action(quader::ui::ActionId::SelectAll)));
	EXPECT_TRUE(edit_menu->actions().contains(&fixture.actions.action(quader::ui::ActionId::ClearSelection)));
	EXPECT_TRUE(edit_menu->actions().contains(&fixture.actions.action(quader::ui::ActionId::InvertSelection)));

	QMenu *tools_menu = window.menuBar()->actions().at(2)->menu();
	EXPECT_TRUE(tools_menu->actions().contains(&fixture.actions.action(quader::ui::ActionId::SelectObjectMode)));
	EXPECT_TRUE(tools_menu->actions().contains(&fixture.actions.action(quader::ui::ActionId::SelectVertexMode)));
	EXPECT_TRUE(tools_menu->actions().contains(&fixture.actions.action(quader::ui::ActionId::SelectEdgeMode)));
	EXPECT_TRUE(tools_menu->actions().contains(&fixture.actions.action(quader::ui::ActionId::SelectFaceMode)));
	EXPECT_TRUE(tools_menu->actions().contains(&fixture.actions.action(quader::ui::ActionId::FlipMeshNormals)));

	const auto kDocks = window.findChildren<QDockWidget *>();
	bool found_scene = false;
	bool found_properties = false;
	bool found_diagnostics = false;
	bool found_inspector = false;
	for (const auto *dock : kDocks) {
		found_scene = found_scene || dock->objectName() == QStringLiteral("quader.panel.scene");
		found_properties = found_properties || dock->objectName() == QStringLiteral("quader.panel.properties");
		found_diagnostics = found_diagnostics || dock->objectName() == QStringLiteral("quader.panel.diagnostics");
		found_inspector = found_inspector || dock->objectName() == QStringLiteral("quader.panel.inspector");
		if (dock->objectName() == QStringLiteral("quader.panel.diagnostics")) {
			EXPECT_TRUE(dock->isHidden());
		}
	}
	EXPECT_TRUE(found_scene);
	EXPECT_TRUE(found_properties);
	EXPECT_TRUE(found_diagnostics);
	EXPECT_FALSE(found_inspector);

	QMenu *view_menu = window.menuBar()->actions().at(4)->menu();
	QMenu *panels_menu = nullptr;
	for (QAction *action : view_menu->actions()) {
		if (action->menu() != nullptr && action->menu()->title() == QStringLiteral("Panels")) {
			panels_menu = action->menu();
			break;
		}
	}
	EXPECT_TRUE(panels_menu != nullptr);
	EXPECT_TRUE(panels_menu->actions().contains(&fixture.actions.action(quader::ui::ActionId::ShowScenePanel)));
	EXPECT_TRUE(panels_menu->actions().contains(&fixture.actions.action(quader::ui::ActionId::ShowPropertiesPanel)));
	EXPECT_TRUE(panels_menu->actions().contains(&fixture.actions.action(quader::ui::ActionId::ShowDiagnosticsPanel)));
}

TEST(UiShell, MainWindowTearsDownViewportBeforeControllerMembers) {
	UiFixture fixture;
	QPointer<QWidget> central_widget;

	{
		auto window = std::make_unique<quader::ui::MainWindow>(fixture.context);
		central_widget = window->centralWidget();
		EXPECT_TRUE(!central_widget.isNull());
	}

	EXPECT_TRUE(central_widget.isNull());
}

} // namespace
