/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
#include "ui/actions/editor_action_controller.hpp"

#include "commands/document_commands.hpp"
#include "commands/selection_commands.hpp"
#include "document/document.hpp"
#include "document/selection.hpp"
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
		case ActionId::BoxTool:
			return quader::tools::ToolId::Box;
		default:
			return std::nullopt;
	}
}

[[nodiscard]] std::optional<quader::tools::SelectionMode> selection_mode_for_action(
		ActionId id) noexcept {
	switch (id) {
		case ActionId::SelectObjectMode:
			return quader::tools::SelectionMode::Object;
		case ActionId::SelectVertexMode:
			return quader::tools::SelectionMode::Vertex;
		case ActionId::SelectEdgeMode:
			return quader::tools::SelectionMode::Edge;
		case ActionId::SelectFaceMode:
			return quader::tools::SelectionMode::Face;
		default:
			return std::nullopt;
	}
}

[[nodiscard]] quader::document::SelectionMode document_selection_mode_for(
		quader::tools::SelectionMode mode) noexcept {
	switch (mode) {
		case quader::tools::SelectionMode::Object:
			return quader::document::SelectionMode::Object;
		case quader::tools::SelectionMode::Vertex:
			return quader::document::SelectionMode::Vertex;
		case quader::tools::SelectionMode::Edge:
			return quader::document::SelectionMode::Edge;
		case quader::tools::SelectionMode::Face:
			return quader::document::SelectionMode::Face;
	}

	return quader::document::SelectionMode::Object;
}

[[nodiscard]] quader::tools::SelectionMode tool_selection_mode_for(
		quader::document::SelectionMode mode) noexcept {
	switch (mode) {
		case quader::document::SelectionMode::Object:
			return quader::tools::SelectionMode::Object;
		case quader::document::SelectionMode::Vertex:
			return quader::tools::SelectionMode::Vertex;
		case quader::document::SelectionMode::Edge:
			return quader::tools::SelectionMode::Edge;
		case quader::document::SelectionMode::Face:
			return quader::tools::SelectionMode::Face;
	}

	return quader::tools::SelectionMode::Object;
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
		case ActionId::SelectAll:
			select_all();
			break;
		case ActionId::ClearSelection:
			clear_selection();
			break;
		case ActionId::InvertSelection:
			invert_selection();
			break;
		case ActionId::DeleteSelection:
			delete_selection();
			break;
		case ActionId::FlipMeshNormals:
			flip_mesh_normals();
			break;
		default:
			break;
	}
}

void EditorActionController::handle_action_toggled(ActionId id, bool checked) {
	if (!checked) {
		refresh_actions();
		return;
	}

	if (const auto kMode = selection_mode_for_action(id)) {
		set_selection_mode(*kMode);
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

	const auto kResult = document_ui_.undo();
	if (kResult.is_applied()) {
		sync_selection_mode_from_document();
	}
	refresh_actions();
}

void EditorActionController::redo() {
	if (!document_ui_.command_history().can_redo()) {
		return;
	}

	const auto kResult = document_ui_.redo();
	if (kResult.is_applied()) {
		sync_selection_mode_from_document();
	}
	refresh_actions();
}

void EditorActionController::select_all() {
	(void)document_ui_.execute_command(std::make_unique<quader::commands::SelectAllCommand>());
}

void EditorActionController::clear_selection() {
	(void)document_ui_.execute_command(std::make_unique<quader::commands::ClearSelectionCommand>());
}

void EditorActionController::invert_selection() {
	(void)document_ui_.execute_command(std::make_unique<quader::commands::InvertSelectionCommand>());
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

void EditorActionController::flip_mesh_normals() {
	(void)document_ui_.execute_command(
			std::make_unique<quader::commands::FlipMeshNormalsCommand>());
}

void EditorActionController::set_selection_mode(quader::tools::SelectionMode mode) {
	(void)tool_manager_.set_selection_mode(mode);
	if (tool_manager_.has_tool(quader::tools::ToolId::Select)) {
		(void)tool_manager_.set_active_tool(quader::tools::ToolId::Select);
	}

	const auto kDocumentMode = document_selection_mode_for(mode);
	if (document_ui_.document().selection().mode() != kDocumentMode) {
		(void)document_ui_.execute_command(
				std::make_unique<quader::commands::SetSelectionModeCommand>(kDocumentMode));
		sync_selection_mode_from_document();
	}

	refresh_actions();
}

void EditorActionController::sync_selection_mode_from_document() {
	(void)tool_manager_.set_selection_mode(
			tool_selection_mode_for(document_ui_.document().selection().mode()));
}

void EditorActionController::refresh_actions() {
	action_state_updater_.refresh();
}

} // namespace quader::ui
