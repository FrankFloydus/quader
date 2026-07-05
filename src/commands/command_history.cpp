#include "commands/command_history.hpp"

#include <utility>

namespace quader::commands {

CommandResult CommandHistory::execute(std::unique_ptr<ICommand> command,
                                      quader::document::Document& document)
{
    if (!command) {
        return CommandResult::rejected("cannot execute a null command");
    }

    auto result = command->execute(document);
    if (!result.is_applied()) {
        return result;
    }

    undo_stack_.push_back(std::move(command));
    redo_stack_.clear();
    return result;
}

CommandResult CommandHistory::undo(quader::document::Document& document)
{
    if (!can_undo()) {
        return CommandResult::rejected("there is no command to undo");
    }

    auto result = undo_stack_.back()->undo(document);
    if (!result.is_applied()) {
        return result;
    }

    redo_stack_.push_back(std::move(undo_stack_.back()));
    undo_stack_.pop_back();
    return result;
}

CommandResult CommandHistory::redo(quader::document::Document& document)
{
    if (!can_redo()) {
        return CommandResult::rejected("there is no command to redo");
    }

    auto result = redo_stack_.back()->execute(document);
    if (!result.is_applied()) {
        return result;
    }

    undo_stack_.push_back(std::move(redo_stack_.back()));
    redo_stack_.pop_back();
    return result;
}

bool CommandHistory::can_undo() const noexcept
{
    return !undo_stack_.empty();
}

bool CommandHistory::can_redo() const noexcept
{
    return !redo_stack_.empty();
}

std::string_view CommandHistory::undo_name() const noexcept
{
    if (!can_undo()) {
        return {};
    }

    return undo_stack_.back()->name();
}

std::string_view CommandHistory::redo_name() const noexcept
{
    if (!can_redo()) {
        return {};
    }

    return redo_stack_.back()->name();
}

void CommandHistory::clear() noexcept
{
    undo_stack_.clear();
    redo_stack_.clear();
}

} // namespace quader::commands
