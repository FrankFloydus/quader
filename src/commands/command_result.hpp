#pragma once

#include "foundation/diagnostic.hpp"

#include <string>

namespace quader::commands {

enum class CommandStatus {
    Applied,
    Rejected,
    NotMergeable,
};

struct CommandResult final {
    CommandStatus status = CommandStatus::Rejected;
    quader::foundation::DiagnosticList diagnostics;

    [[nodiscard]] bool is_applied() const noexcept
    {
        return status == CommandStatus::Applied;
    }

    [[nodiscard]] static CommandResult applied();
    [[nodiscard]] static CommandResult rejected(quader::foundation::Diagnostic diagnostic);
    [[nodiscard]] static CommandResult rejected(std::string message);
    [[nodiscard]] static CommandResult not_mergeable();
};

} // namespace quader::commands
