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

#include "ui/actions/action_registry.hpp"
#include "ui/actions/action_state_updater.hpp"
#include "ui/actions/editor_action_controller.hpp"
#include "ui/actions/editor_action_state_provider.hpp"
#include "ui/qt_app/ui_context.hpp"
#include "ui/services/document_ui_controller.hpp"
#include "ui/services/file_dialog_service.hpp"
#include "ui/services/import_ui_controller.hpp"
#include "ui/services/notification_service.hpp"
#include "ui/services/settings_service.hpp"
#include "ui/services/viewport_diagnostics_service.hpp"

#include "commands/command_history.hpp"
#include "document/document.hpp"
#include "io/export_service.hpp"
#include "io/import_export_registry.hpp"
#include "io/import_service.hpp"
#include "tools/tool_manager.hpp"

#include <QSettings>

namespace quader::app {

/// Owns the long-lived services shared by the Qt application shell.
struct AppServices final {
	/// Construct services in dependency order and wire action/tool callbacks.
	AppServices();

	/// Qt settings backend owned by the application service graph.
	QSettings settings_store;
	/// Editable document state.
	quader::document::Document document;
	/// Undo/redo command history for document mutations.
	quader::commands::CommandHistory command_history;
	/// Active editor tool registry and dispatcher.
	quader::tools::ToolManager tool_manager;
	/// Import/export format registry.
	quader::io::ImportExportRegistry io_registry;
	/// Import service bound to `io_registry`.
	quader::io::ImportService import_service;
	/// Export service bound to `io_registry`.
	quader::io::ExportService export_service;
	/// Canonical UI action registry.
	ui::ActionRegistry actions;
	/// State provider used to derive action enabled/checked state.
	ui::EditorActionStateProvider editor_state;
	/// Applies editor state to registered actions.
	ui::ActionStateUpdater action_state_updater;
	/// Workspace and UI settings facade.
	ui::SettingsService settings;
	/// Qt-backed file dialog service.
	ui::QtFileDialogService file_dialogs;
	/// Notification/status broadcaster.
	ui::NotificationService notifications;
	/// Latest viewport diagnostics bridge.
	ui::ViewportDiagnosticsService viewport_diagnostics;
	/// UI controller for command/document mutation.
	ui::DocumentUiController document_ui;
	/// UI controller for import dialogs and notifications.
	ui::ImportUiController import_ui;
	/// Action-to-command/tool dispatcher.
	ui::EditorActionController editor_actions;
	/// Non-owning context passed into the main window and panels.
	ui::UiContext ui_context;
};

} // namespace quader::app
