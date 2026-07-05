/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
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
	auto result = command_history_.execute(std::move(command), document_);
	if (result.is_applied() && after_command_applied_) {
		after_command_applied_();
	}
	return result;
}

void ToolContext::set_after_command_applied(std::function<void()> callback) {
	after_command_applied_ = std::move(callback);
}

} // namespace quader::tools
