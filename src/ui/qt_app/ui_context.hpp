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

namespace quader::ui {

class ActionRegistry;
class ActionStateUpdater;
class DocumentUiController;
class IEditorStateProvider;
class ImportUiController;
class NotificationService;
class SettingsService;
class ViewportDiagnosticsService;

} // namespace quader::ui

namespace quader::tools {
class ToolManager;
}

namespace quader::ui {

/// Non-owning service bundle passed to the Qt shell and panels.
struct UiContext {
	/// Canonical action registry.
	ActionRegistry &actions;
	/// Action state updater.
	ActionStateUpdater &action_state_updater;
	/// Editor state provider.
	IEditorStateProvider &editor_state;
	/// Settings facade.
	SettingsService &settings;
	/// Notification service.
	NotificationService &notifications;
	/// Document UI controller.
	DocumentUiController &documents;
	/// Import UI controller.
	ImportUiController &imports;
	/// Viewport diagnostics service.
	ViewportDiagnosticsService &viewport_diagnostics;
	/// Editor tool manager.
	quader::tools::ToolManager &tools;
};

} // namespace quader::ui
