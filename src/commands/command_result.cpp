/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
#include "commands/command_result.hpp"

#include <utility>

namespace quader::commands {

CommandResult CommandResult::applied() {
	return CommandResult{
		CommandStatus::Applied,
		{},
	};
}

CommandResult CommandResult::rejected(quader::foundation::Diagnostic diagnostic) {
	CommandResult result{
		CommandStatus::Rejected,
		{},
	};
	result.diagnostics.add(std::move(diagnostic));
	return result;
}

CommandResult CommandResult::rejected(const std::string &message) {
	return rejected(quader::foundation::Diagnostic{
			quader::foundation::Severity::Error,
			message,
	});
}

CommandResult CommandResult::not_mergeable() {
	return CommandResult{
		CommandStatus::NotMergeable,
		{},
	};
}

} // namespace quader::commands
