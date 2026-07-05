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

#include "ui/actions/action_id.hpp"

namespace quader::tools {
class ToolManager;
}

namespace quader::ui {

class ActionRegistry;
class ActionStateUpdater;
class DocumentUiController;
class NotificationService;

/// Routes triggered actions to document commands, tool activation, and UI services.
class EditorActionController final {
public:
	/// Connect to the action registry and store non-owning service references.
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
