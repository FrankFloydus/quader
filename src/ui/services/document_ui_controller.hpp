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

#include "document/document_events.hpp"
#include "document/object_id.hpp"

#include <QObject>

#include <memory>

namespace quader::commands {
class CommandHistory;
class ICommand;
struct CommandResult;
} //namespace quader::commands

namespace quader::document {
class Document;
}

namespace quader::ui {

class ActionStateUpdater;
class NotificationService;

/// User-feedback policy for command execution failures.
enum class CommandFeedback {
	Silent,        ///< Do not post rejection notifications.
	NotifyOnReject, ///< Post rejection notifications.
};

/**
 * UI-facing document mutation controller.
 *
 * Commands are executed through `CommandHistory`; document changes are drained
 * into Qt signals for models, panels, and action state.
 */
class DocumentUiController final : public QObject {
	Q_OBJECT

public:
	/// Construct the controller over non-owning document/service references.
	DocumentUiController(quader::document::Document &document,
			quader::commands::CommandHistory &command_history,
			ActionStateUpdater &action_state_updater,
			NotificationService &notifications,
			QObject *parent = nullptr);

	/// Return the document as read-only UI state.
	[[nodiscard]] const quader::document::Document &document() const noexcept;
	/// Return the command history as read-only UI state.
	[[nodiscard]] const quader::commands::CommandHistory &command_history() const noexcept;

	/// Execute a command, drain document changes, and refresh actions.
	[[nodiscard]] quader::commands::CommandResult execute_command(
			std::unique_ptr<quader::commands::ICommand> command,
			CommandFeedback feedback = CommandFeedback::NotifyOnReject);
	/// Undo through command history, drain document changes, and refresh actions.
	[[nodiscard]] quader::commands::CommandResult undo(
			CommandFeedback feedback = CommandFeedback::NotifyOnReject);
	/// Redo through command history, drain document changes, and refresh actions.
	[[nodiscard]] quader::commands::CommandResult redo(
			CommandFeedback feedback = CommandFeedback::NotifyOnReject);

	/// Drain pending document events and refresh dependent UI state.
	void refresh_from_document();

Q_SIGNALS:
	/// Emitted when the object list changes.
	void object_list_changed();
	/// Emitted when one object's displayable data changes.
	void object_changed(quader::document::ObjectId object);
	/// Emitted when document selection changes.
	void selection_changed();
	/// Emitted when document dirty flags change.
	void dirty_state_changed(quader::document::DocumentDirtyFlags flags);
	/// Emitted when undo/redo availability may have changed.
	void command_history_changed();
	/// Emitted after a drain cycle completes.
	void document_events_drained();

private:
	void drain_pending_changes();
	void emit_change(const quader::document::DocumentChange &change);
	void report_rejected_command(const quader::commands::CommandResult &result);
	void refresh_actions();

	quader::document::Document &document_;
	quader::commands::CommandHistory &command_history_;
	ActionStateUpdater &action_state_updater_;
	NotificationService &notifications_;
};

} // namespace quader::ui
