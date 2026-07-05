#pragma once

#include "commands/command.hpp"
#include "document/selection.hpp"

#include <optional>
#include <string_view>

namespace quader::commands {

class SetSelectionCommand final : public ICommand {
public:
    explicit SetSelectionCommand(quader::document::Selection selection);

    [[nodiscard]] std::string_view name() const noexcept override;
    [[nodiscard]] CommandResult execute(quader::document::Document& document) override;
    [[nodiscard]] CommandResult undo(quader::document::Document& document) override;

private:
    quader::document::Selection selection_;
    std::optional<quader::document::Selection> previous_selection_;
};

class ClearSelectionCommand final : public ICommand {
public:
    [[nodiscard]] std::string_view name() const noexcept override;
    [[nodiscard]] CommandResult execute(quader::document::Document& document) override;
    [[nodiscard]] CommandResult undo(quader::document::Document& document) override;

private:
    std::optional<quader::document::Selection> previous_selection_;
};

} // namespace quader::commands
