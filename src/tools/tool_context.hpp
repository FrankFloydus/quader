#pragma once

#include "commands/command.hpp"
#include "commands/command_history.hpp"
#include "document/document.hpp"

#include <memory>

namespace quader::tools {

class ToolContext final {
public:
	ToolContext(quader::document::Document &document,
			quader::commands::CommandHistory &command_history) noexcept;

	[[nodiscard]] const quader::document::Document &document() const noexcept;
	[[nodiscard]] quader::commands::CommandHistory &command_history() noexcept;
	[[nodiscard]] const quader::commands::CommandHistory &command_history() const noexcept;

	[[nodiscard]] quader::commands::CommandResult execute_command(
			std::unique_ptr<quader::commands::ICommand> command);

private:
	quader::document::Document &document_;
	quader::commands::CommandHistory &command_history_;
};

} // namespace quader::tools
