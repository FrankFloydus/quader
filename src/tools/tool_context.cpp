#include "tools/tool_context.hpp"

#include <utility>

namespace quader::tools {

ToolContext::ToolContext(quader::document::Document &document,
		quader::commands::CommandHistory &command_history) noexcept
		: document_(document),
		  command_history_(command_history) {
}

const quader::document::Document &ToolContext::document() const noexcept {
	return document_;
}

quader::commands::CommandHistory &ToolContext::command_history() noexcept {
	return command_history_;
}

const quader::commands::CommandHistory &ToolContext::command_history() const noexcept {
	return command_history_;
}

quader::commands::CommandResult ToolContext::execute_command(
		std::unique_ptr<quader::commands::ICommand> command) {
	return command_history_.execute(std::move(command), document_);
}

} // namespace quader::tools
