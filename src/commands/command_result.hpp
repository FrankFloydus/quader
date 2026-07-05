/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
#pragma once

#include "foundation/diagnostic.hpp"

#include <string>

namespace quader::commands {

/// Outcome status returned by command execution and merge operations.
enum class CommandStatus {
	/// Command applied successfully.
	Applied,
	/// Command was rejected and did not apply its requested mutation.
	Rejected,
	/// Command merge was rejected because the pair is incompatible.
	NotMergeable,
};

/// Result payload returned by command operations.
struct CommandResult final {
	/// High-level command outcome.
	CommandStatus status = CommandStatus::Rejected;
	/// Diagnostics explaining rejected commands or merge failures.
	quader::foundation::DiagnosticList diagnostics;

	/**
	 * Check whether the command applied successfully.
	 *
	 * @return True when `status` is `CommandStatus::Applied`.
	 */
	[[nodiscard]] bool is_applied() const noexcept {
		return status == CommandStatus::Applied;
	}

	/**
	 * Create a successful command result.
	 *
	 * @return Result with `CommandStatus::Applied`.
	 */
	[[nodiscard]] static CommandResult applied();
	/**
	 * Create a rejected command result from a diagnostic.
	 *
	 * @param diagnostic Diagnostic explaining the rejection.
	 * @return Result with `CommandStatus::Rejected`.
	 */
	[[nodiscard]] static CommandResult rejected(quader::foundation::Diagnostic diagnostic);
	/**
	 * Create a rejected command result from an error message.
	 *
	 * @param message Message explaining the rejection.
	 * @return Result with `CommandStatus::Rejected`.
	 */
	[[nodiscard]] static CommandResult rejected(const std::string &message);
	/**
	 * Create a non-mergeable command result.
	 *
	 * @return Result with `CommandStatus::NotMergeable`.
	 */
	[[nodiscard]] static CommandResult not_mergeable();
};

} // namespace quader::commands
