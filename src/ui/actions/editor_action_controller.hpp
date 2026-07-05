#pragma once

#include "ui/actions/action_id.hpp"

namespace quader::tools {
class ToolManager;
}

namespace quader::ui {

class ActionRegistry;
class ActionStateUpdater;
class DocumentUiController;
class NotificationService;

class EditorActionController final {
public:
	EditorActionController(ActionRegistry &actions,
			DocumentUiController &document_ui,
			quader::tools::ToolManager &tool_manager,
			ActionStateUpdater &action_state_updater,
			NotificationService &notifications);

private:
	void handle_action_triggered(ActionId id);
	void handle_action_toggled(ActionId id, bool checked);
	void undo();
	void redo();
	void delete_selection();
	void refresh_actions();

	ActionRegistry &actions_;
	DocumentUiController &document_ui_;
	quader::tools::ToolManager &tool_manager_;
	ActionStateUpdater &action_state_updater_;
	NotificationService &notifications_;
};

} // namespace quader::ui
