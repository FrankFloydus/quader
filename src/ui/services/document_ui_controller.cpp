#include "ui/services/document_ui_controller.hpp"

#include "commands/command.hpp"
#include "commands/command_history.hpp"
#include "commands/command_result.hpp"
#include "document/document.hpp"
#include "ui/actions/action_state_updater.hpp"
#include "ui/services/notification_service.hpp"

#include <QString>

#include <utility>

namespace quader::ui {
namespace {

[[nodiscard]] QString first_diagnostic_message(const quader::commands::CommandResult &result) {
	if (result.diagnostics.empty()) {
		return QStringLiteral("The command could not be applied.");
	}

	return QString::fromStdString(result.diagnostics[0].message);
}

} // namespace

DocumentUiController::DocumentUiController(
		quader::document::Document &document,
		quader::commands::CommandHistory &command_history,
		ActionStateUpdater &action_state_updater,
		NotificationService &notifications,
		QObject *parent) : QObject(parent),
						   document_(document),
						   command_history_(command_history),
						   action_state_updater_(action_state_updater),
						   notifications_(notifications) {
}

const quader::document::Document &DocumentUiController::document() const noexcept {
	return document_;
}

const quader::commands::CommandHistory &DocumentUiController::command_history() const noexcept {
	return command_history_;
}

quader::commands::CommandResult DocumentUiController::execute_command(
		std::unique_ptr<quader::commands::ICommand> command,
		CommandFeedback feedback) {
	auto result = command_history_.execute(std::move(command), document_);
	if (result.is_applied()) {
		drain_pending_changes();
		Q_EMIT command_history_changed();
	} else if (feedback == CommandFeedback::NotifyOnReject) {
		report_rejected_command(result);
	}

	refresh_actions();
	return result;
}

quader::commands::CommandResult DocumentUiController::undo(CommandFeedback feedback) {
	auto result = command_history_.undo(document_);
	if (result.is_applied()) {
		drain_pending_changes();
		Q_EMIT command_history_changed();
	} else if (feedback == CommandFeedback::NotifyOnReject) {
		report_rejected_command(result);
	}

	refresh_actions();
	return result;
}

quader::commands::CommandResult DocumentUiController::redo(CommandFeedback feedback) {
	auto result = command_history_.redo(document_);
	if (result.is_applied()) {
		drain_pending_changes();
		Q_EMIT command_history_changed();
	} else if (feedback == CommandFeedback::NotifyOnReject) {
		report_rejected_command(result);
	}

	refresh_actions();
	return result;
}

void DocumentUiController::refresh_from_document() {
	drain_pending_changes();
	refresh_actions();
}

void DocumentUiController::drain_pending_changes() {
	const auto kChanges = document_.take_pending_changes();
	if (kChanges.empty()) {
		return;
	}

	for (const auto &change : kChanges) {
		emit_change(change);
	}

	Q_EMIT document_events_drained();
}

void DocumentUiController::emit_change(const quader::document::DocumentChange &change) {
	using quader::document::DocumentChangeKind;

	switch (change.kind) {
		case DocumentChangeKind::ObjectCreated:
		case DocumentChangeKind::ObjectDeleted:
			Q_EMIT object_list_changed();
			break;
		case DocumentChangeKind::ObjectRenamed:
		case DocumentChangeKind::TransformChanged:
		case DocumentChangeKind::MeshTopologyChanged:
		case DocumentChangeKind::MeshGeometryChanged:
			Q_EMIT object_changed(change.object);
			break;
		case DocumentChangeKind::SelectionChanged:
			Q_EMIT selection_changed();
			break;
		case DocumentChangeKind::DirtyStateChanged:
			Q_EMIT dirty_state_changed(change.dirty_flags);
			break;
	}
}

void DocumentUiController::report_rejected_command(
		const quader::commands::CommandResult &result) {
	notifications_.show_warning(NotificationMessage{
			NotificationSeverity::Warning,
			QStringLiteral("Command rejected"),
			first_diagnostic_message(result),
			4000,
	});
}

void DocumentUiController::refresh_actions() {
	action_state_updater_.refresh();
}

} // namespace quader::ui
