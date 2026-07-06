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

#include "commands/command_result.hpp"
#include "document/document.hpp"

#include <memory>
#include <string_view>

namespace quader::commands {

/**
 * Base interface for undoable document commands.
 *
 * Commands are the mutation boundary for user-visible document edits. Concrete
 * commands mutate a `Document` in `execute()` and restore prior state in
 * `undo()`.
 */
class ICommand {
public:
	/// Destroy a command through the base interface.
	virtual ~ICommand() = default;

	/**
	 * Return the display name used by undo/redo UI.
	 *
	 * @return Borrowed command name valid for the command lifetime.
	 */
	[[nodiscard]] virtual std::string_view name() const noexcept = 0;
	/**
	 * Apply the command to a document.
	 *
	 * @param document Document to mutate.
	 * @return Command result with diagnostics for rejected operations.
	 */
	[[nodiscard]] virtual CommandResult execute(quader::document::Document &document) = 0;
	/**
	 * Undo a previously applied command.
	 *
	 * @param document Document to mutate back toward its previous state.
	 * @return Command result with diagnostics for undo failures.
	 */
	[[nodiscard]] virtual CommandResult undo(quader::document::Document &document) = 0;

	/**
	 * Check whether a later command can be merged into this command.
	 *
	 * @param next Later command proposed for merge.
	 * @return True when `merge_with()` may consume `next`.
	 */
	[[nodiscard]] virtual bool can_merge_with(const ICommand &next) const noexcept;
	/**
	 * Merge a later command into this command.
	 *
	 * `CommandHistory` calls this only after `can_merge_with()` returns true
	 * for a command that has already executed successfully. Implementations
	 * that accept a merge must preserve undo of the original pre-merge state.
	 *
	 * @param next Later command to consume.
	 * @return `CommandStatus::NotMergeable` when the merge is rejected.
	 */
	[[nodiscard]] virtual CommandResult merge_with(std::unique_ptr<ICommand> next);
};

} // namespace quader::commands
