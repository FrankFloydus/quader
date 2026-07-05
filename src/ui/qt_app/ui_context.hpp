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

struct UiContext {
    ActionRegistry& actions;
    ActionStateUpdater& action_state_updater;
    IEditorStateProvider& editor_state;
    SettingsService& settings;
    NotificationService& notifications;
    DocumentUiController& documents;
    ImportUiController& imports;
    ViewportDiagnosticsService& viewport_diagnostics;
};

} // namespace quader::ui
