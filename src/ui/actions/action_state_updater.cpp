#include "ui/actions/action_state_updater.hpp"

#include <QSignalBlocker>

namespace quader::ui {
namespace {

void set_enabled(ActionRegistry &registry, ActionId id, bool enabled) {
	registry.action(id).setEnabled(enabled);
}

void set_checked(ActionRegistry &registry, ActionId id, bool checked) {
	QAction &action = registry.action(id);
	const QSignalBlocker kBlocker(&action);
	action.setChecked(checked);
}

void set_text(ActionRegistry &registry, ActionId id, const QString &text) {
	registry.action(id).setText(text);
}

} // namespace

ActionStateUpdater::ActionStateUpdater(ActionRegistry &actions,
		IEditorStateProvider &state_provider,
		QObject *parent) : QObject(parent),
						   actions_(actions),
						   state_provider_(state_provider) {
}

void ActionStateUpdater::refresh() {
	refresh_from_snapshot(state_provider_.editor_state_snapshot());
}

void ActionStateUpdater::refresh_from_snapshot(const EditorStateSnapshot &snapshot) {
	set_enabled(actions_, ActionId::NewScene, snapshot.new_scene_available);
	set_enabled(actions_, ActionId::OpenScene, snapshot.document_services_available && snapshot.import_available);
	set_enabled(actions_,
			ActionId::SaveScene,
			snapshot.has_active_document && snapshot.document_dirty && snapshot.export_available);
	set_enabled(actions_, ActionId::SaveSceneAs, snapshot.has_active_document && snapshot.export_available);

	set_enabled(actions_, ActionId::Exit, true);

	set_enabled(actions_, ActionId::Undo, snapshot.can_undo);
	set_enabled(actions_, ActionId::Redo, snapshot.can_redo);
	set_text(actions_, ActionId::Undo, snapshot.undo_text.isEmpty() ? QStringLiteral("&Undo") : snapshot.undo_text);
	set_text(actions_, ActionId::Redo, snapshot.redo_text.isEmpty() ? QStringLiteral("&Redo") : snapshot.redo_text);

	set_enabled(actions_, ActionId::DuplicateSelection, snapshot.can_duplicate_selection);
	set_enabled(actions_, ActionId::DeleteSelection, snapshot.can_delete_selection);

	const bool kToolsEnabled = snapshot.tools_available;
	set_enabled(actions_, ActionId::SelectTool, kToolsEnabled);
	set_enabled(actions_, ActionId::MoveTool, kToolsEnabled);
	set_enabled(actions_, ActionId::RotateTool, kToolsEnabled);
	set_enabled(actions_, ActionId::ScaleTool, kToolsEnabled);

	set_checked(actions_, ActionId::SelectTool, kToolsEnabled && snapshot.active_tool == ActionId::SelectTool);
	set_checked(actions_, ActionId::MoveTool, kToolsEnabled && snapshot.active_tool == ActionId::MoveTool);
	set_checked(actions_, ActionId::RotateTool, kToolsEnabled && snapshot.active_tool == ActionId::RotateTool);
	set_checked(actions_, ActionId::ScaleTool, kToolsEnabled && snapshot.active_tool == ActionId::ScaleTool);

	const bool kCreationEnabled = snapshot.has_active_document && snapshot.creation_available;
	set_enabled(actions_, ActionId::CreateCube, kCreationEnabled);
	set_enabled(actions_, ActionId::CreateLight, kCreationEnabled);
	set_enabled(actions_, ActionId::CreateCamera, kCreationEnabled);

	if (snapshot.viewport_state_known) {
		set_enabled(actions_, ActionId::ViewPerspective, snapshot.viewport_available);
		set_enabled(actions_, ActionId::ViewShaded, snapshot.viewport_available);
		set_enabled(actions_, ActionId::ToggleQuadViewports, snapshot.viewport_available);
		set_checked(actions_, ActionId::ToggleQuadViewports, snapshot.quad_viewports_enabled);
		set_enabled(actions_, ActionId::FocusViewport, snapshot.viewport_available);
	}

	set_enabled(actions_, ActionId::ResetWorkspaceLayout, true);
	set_enabled(actions_, ActionId::ShowScenePanel, true);
	set_enabled(actions_, ActionId::ShowPropertiesPanel, true);
	set_enabled(actions_, ActionId::ShowDiagnosticsPanel, true);
}

} // namespace quader::ui
