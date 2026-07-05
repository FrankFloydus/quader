/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
#include "ui/actions/editor_action_state_provider.hpp"

#include "commands/command_history.hpp"
#include "document/document.hpp"
#include "io/import_export_registry.hpp"
#include "tools/tool_id.hpp"
#include "tools/tool_manager.hpp"

#include <QString>

#include <string>
#include <string_view>

namespace quader::ui {
namespace {

[[nodiscard]] QString command_action_text(const QString &verb, std::string_view command_name) {
	if (command_name.empty()) {
		return {};
	}

	return QStringLiteral("%1 %2").arg(verb, QString::fromStdString(std::string(command_name)));
}

[[nodiscard]] bool has_any_registered_tool(const quader::tools::ToolManager &tool_manager) {
	return tool_manager.has_tool(quader::tools::ToolId::Select) || tool_manager.has_tool(quader::tools::ToolId::Move) || tool_manager.has_tool(quader::tools::ToolId::Rotate) || tool_manager.has_tool(quader::tools::ToolId::Scale) || tool_manager.has_tool(quader::tools::ToolId::Box);
}

[[nodiscard]] ActionId action_for_tool(quader::tools::ToolId id) noexcept {
	switch (id) {
		case quader::tools::ToolId::Select:
			return ActionId::SelectTool;
		case quader::tools::ToolId::Move:
			return ActionId::MoveTool;
		case quader::tools::ToolId::Rotate:
			return ActionId::RotateTool;
		case quader::tools::ToolId::Scale:
			return ActionId::ScaleTool;
		case quader::tools::ToolId::Box:
			return ActionId::BoxTool;
	}

	return ActionId::SelectTool;
}

} // namespace

EditorActionStateProvider::EditorActionStateProvider(
		const quader::document::Document &document,
		const quader::commands::CommandHistory &command_history,
		const quader::tools::ToolManager &tool_manager,
		const quader::io::ImportExportRegistry &io_registry) noexcept
		: document_(document),
		  command_history_(command_history),
		  tool_manager_(tool_manager),
		  io_registry_(io_registry) {
}

EditorStateSnapshot EditorActionStateProvider::editor_state_snapshot() const {
	EditorStateSnapshot snapshot;
	snapshot.document_services_available = true;
	snapshot.import_available = !io_registry_.import_formats().empty();
	snapshot.export_available = !io_registry_.export_formats().empty();
	snapshot.file_io_available = snapshot.import_available || snapshot.export_available;
	snapshot.has_active_document = true;
	snapshot.document_dirty = document_.is_dirty();
	snapshot.has_selection = !document_.selection().empty();
	snapshot.can_delete_selection = document_.selection().selected_objects().size() == 1U && document_.selection().selected_components().empty();

	snapshot.can_undo = command_history_.can_undo();
	snapshot.undo_text = command_action_text(QStringLiteral("Undo"), command_history_.undo_name());
	snapshot.can_redo = command_history_.can_redo();
	snapshot.redo_text = command_action_text(QStringLiteral("Redo"), command_history_.redo_name());

	snapshot.tools_available = has_any_registered_tool(tool_manager_);
	if (const auto kActiveTool = tool_manager_.active_tool_id()) {
		snapshot.active_tool = action_for_tool(*kActiveTool);
	}

	return snapshot;
}

} // namespace quader::ui
