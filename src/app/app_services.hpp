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

struct AppServices final {
	AppServices();

	QSettings settings_store;
	quader::document::Document document;
	quader::commands::CommandHistory command_history;
	quader::tools::ToolManager tool_manager;
	quader::io::ImportExportRegistry io_registry;
	quader::io::ImportService import_service;
	quader::io::ExportService export_service;
	ui::ActionRegistry actions;
	ui::EditorActionStateProvider editor_state;
	ui::ActionStateUpdater action_state_updater;
	ui::SettingsService settings;
	ui::QtFileDialogService file_dialogs;
	ui::NotificationService notifications;
	ui::ViewportDiagnosticsService viewport_diagnostics;
	ui::DocumentUiController document_ui;
	ui::ImportUiController import_ui;
	ui::EditorActionController editor_actions;
	ui::UiContext ui_context;
};

} // namespace quader::app
