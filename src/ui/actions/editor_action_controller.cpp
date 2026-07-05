#include "ui/actions/editor_action_controller.hpp"

#include "commands/command_result.hpp"
#include "commands/document_commands.hpp"
#include "document/document.hpp"
#include "tools/tool_id.hpp"
#include "tools/tool_manager.hpp"
#include "ui/actions/action_registry.hpp"
#include "ui/actions/action_state_updater.hpp"
#include "ui/services/document_ui_controller.hpp"
#include "ui/services/notification_service.hpp"

#include <QObject>

#include <memory>
#include <optional>

namespace quader::ui {
namespace {

[[nodiscard]] std::optional<quader::tools::ToolId> tool_for_action(ActionId id) noexcept {
	switch (id) {
		case ActionId::SelectTool:
			return quader::tools::ToolId::Select;
		case ActionId::MoveTool:
			return quader::tools::ToolId::Move;
		case ActionId::RotateTool:
			return quader::tools::ToolId::Rotate;
		case ActionId::ScaleTool:
			return quader::tools::ToolId::Scale;
		default:
			return std::nullopt;
	}
}

} // namespace

EditorActionController::EditorActionController(ActionRegistry &actions,
		DocumentUiController &document_ui,
		quader::tools::ToolManager &tool_manager,
		ActionStateUpdater &action_state_updater,
		NotificationService &notifications) : actions_(actions),
											  document_ui_(document_ui),
											  tool_manager_(tool_manager),
											  action_state_updater_(action_state_updater),
											  notifications_(notifications) {
	QObject::connect(&actions_,
			&ActionRegistry::action_triggered,
			&actions_,
			[this](ActionId id) {
				handle_action_triggered(id);
			});
	QObject::connect(&actions_,
			&ActionRegistry::action_toggled,
			&actions_,
			[this](ActionId id, bool checked) {
				handle_action_toggled(id, checked);
			});
}

void EditorActionController::handle_action_triggered(ActionId id) {
	switch (id) {
		case ActionId::Undo:
			undo();
			break;
		case ActionId::Redo:
			redo();
			break;
		case ActionId::DeleteSelection:
			delete_selection();
			break;
		default:
			break;
	}
}

void EditorActionController::handle_action_toggled(ActionId id, bool checked) {
	if (!checked) {
		return;
	}

	const auto kTool = tool_for_action(id);
	if (!kTool.has_value()) {
		return;
	}

	if (!tool_manager_.set_active_tool(*kTool)) {
		notifications_.show_warning(NotificationMessage{
				NotificationSeverity::Warning,
				QStringLiteral("Tool unavailable"),
				QStringLiteral("The requested tool is not registered."),
				4000,
		});
	}

	refresh_actions();
}

void EditorActionController::undo() {
	if (!document_ui_.command_history().can_undo()) {
		return;
	}

	(void)document_ui_.undo();
}

void EditorActionController::redo() {
	if (!document_ui_.command_history().can_redo()) {
		return;
	}

	(void)document_ui_.redo();
}

void EditorActionController::delete_selection() {
	const auto &document = document_ui_.document();
	const auto kSelectedObjects = document.selection().selected_objects();
	if (kSelectedObjects.size() != 1U || !document.selection().selected_components().empty()) {
		notifications_.show_warning(NotificationMessage{
				NotificationSeverity::Warning,
				QStringLiteral("Delete unavailable"),
				QStringLiteral("Only a single selected object can be deleted by the current command shell."),
				4000,
		});
		refresh_actions();
		return;
	}

	(void)document_ui_.execute_command(
			std::make_unique<quader::commands::DeleteObjectCommand>(kSelectedObjects.front()));
}

void EditorActionController::refresh_actions() {
	action_state_updater_.refresh();
}

} // namespace quader::ui
