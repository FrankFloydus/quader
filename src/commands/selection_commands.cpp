#include "commands/selection_commands.hpp"

#include <utility>

namespace quader::commands {
namespace {

[[nodiscard]] CommandResult rejected_from_document_error(quader::document::DocumentError error)
{
    return CommandResult::rejected(std::move(error.diagnostic));
}

} // namespace

SetSelectionCommand::SetSelectionCommand(quader::document::Selection selection)
    : selection_(std::move(selection))
{
}

std::string_view SetSelectionCommand::name() const noexcept
{
    return "Set Selection";
}

CommandResult SetSelectionCommand::execute(quader::document::Document& document)
{
    previous_selection_ = document.selection();
    if (*previous_selection_ == selection_) {
        previous_selection_.reset();
        return CommandResult::rejected("selection command would not change the active selection");
    }

    auto selected = document.set_selection(selection_);
    if (!selected) {
        previous_selection_.reset();
        return rejected_from_document_error(std::move(selected).error());
    }

    return CommandResult::applied();
}

CommandResult SetSelectionCommand::undo(quader::document::Document& document)
{
    if (!previous_selection_) {
        return CommandResult::rejected("selection command has no previous selection");
    }

    auto selected = document.set_selection(*previous_selection_);
    if (!selected) {
        return rejected_from_document_error(std::move(selected).error());
    }

    return CommandResult::applied();
}

std::string_view ClearSelectionCommand::name() const noexcept
{
    return "Clear Selection";
}

CommandResult ClearSelectionCommand::execute(quader::document::Document& document)
{
    previous_selection_ = document.selection();
    if (previous_selection_->empty()) {
        previous_selection_.reset();
        return CommandResult::rejected("selection is already empty");
    }

    document.clear_selection();
    return CommandResult::applied();
}

CommandResult ClearSelectionCommand::undo(quader::document::Document& document)
{
    if (!previous_selection_) {
        return CommandResult::rejected("clear selection command has no previous selection");
    }

    auto selected = document.set_selection(*previous_selection_);
    if (!selected) {
        return rejected_from_document_error(std::move(selected).error());
    }

    return CommandResult::applied();
}

} // namespace quader::commands
