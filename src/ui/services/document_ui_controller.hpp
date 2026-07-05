#pragma once

#include "document/document_events.hpp"
#include "document/object_id.hpp"

#include <QObject>

#include <memory>

namespace quader::commands {
class CommandHistory;
class ICommand;
struct CommandResult;
}

namespace quader::document {
class Document;
}

namespace quader::ui {

class ActionStateUpdater;
class NotificationService;

enum class CommandFeedback {
    Silent,
    NotifyOnReject,
};

class DocumentUiController final : public QObject {
    Q_OBJECT

public:
    DocumentUiController(quader::document::Document& document,
                         quader::commands::CommandHistory& command_history,
                         ActionStateUpdater& action_state_updater,
                         NotificationService& notifications,
                         QObject* parent = nullptr);

    [[nodiscard]] const quader::document::Document& document() const noexcept;
    [[nodiscard]] const quader::commands::CommandHistory& command_history() const noexcept;

    [[nodiscard]] quader::commands::CommandResult execute_command(
        std::unique_ptr<quader::commands::ICommand> command,
        CommandFeedback feedback = CommandFeedback::NotifyOnReject);
    [[nodiscard]] quader::commands::CommandResult undo(
        CommandFeedback feedback = CommandFeedback::NotifyOnReject);
    [[nodiscard]] quader::commands::CommandResult redo(
        CommandFeedback feedback = CommandFeedback::NotifyOnReject);

    void refresh_from_document();

Q_SIGNALS:
    void object_list_changed();
    void object_changed(quader::document::ObjectId object);
    void selection_changed();
    void dirty_state_changed(quader::document::DocumentDirtyFlags flags);
    void command_history_changed();
    void document_events_drained();

private:
    void drain_pending_changes();
    void emit_change(const quader::document::DocumentChange& change);
    void report_rejected_command(const quader::commands::CommandResult& result);
    void refresh_actions();

    quader::document::Document& document_;
    quader::commands::CommandHistory& command_history_;
    ActionStateUpdater& action_state_updater_;
    NotificationService& notifications_;
};

} // namespace quader::ui
