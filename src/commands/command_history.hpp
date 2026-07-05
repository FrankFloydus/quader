#pragma once

#include "commands/command.hpp"

#include <memory>
#include <string_view>
#include <vector>

namespace quader::commands {

class CommandHistory final {
public:
    [[nodiscard]] CommandResult execute(std::unique_ptr<ICommand> command,
                                        quader::document::Document& document);
    [[nodiscard]] CommandResult undo(quader::document::Document& document);
    [[nodiscard]] CommandResult redo(quader::document::Document& document);

    [[nodiscard]] bool can_undo() const noexcept;
    [[nodiscard]] bool can_redo() const noexcept;
    [[nodiscard]] std::string_view undo_name() const noexcept;
    [[nodiscard]] std::string_view redo_name() const noexcept;

    void clear() noexcept;

private:
    std::vector<std::unique_ptr<ICommand>> undo_stack_;
    std::vector<std::unique_ptr<ICommand>> redo_stack_;
};

} // namespace quader::commands
