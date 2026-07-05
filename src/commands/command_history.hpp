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

#include "commands/command.hpp"

#include <memory>
#include <string_view>
#include <vector>

namespace quader::commands {

/**
 * Owns undo and redo stacks for document commands.
 *
 * `execute()` applies commands immediately. Successful execution pushes the
 * command onto the undo stack and clears redo history unless a merge consumes
 * the command.
 */
class CommandHistory final {
public:
	/**
	 * Execute and record a command.
	 *
	 * @param command Command to take ownership of.
	 * @param document Document passed to the command.
	 * @return Command result from execution or merge.
	 */
	[[nodiscard]] CommandResult execute(std::unique_ptr<ICommand> command,
			quader::document::Document &document);
	/**
	 * Undo the latest applied command.
	 *
	 * @param document Document passed to the command.
	 * @return Command result, or a rejection when no undo is available.
	 */
	[[nodiscard]] CommandResult undo(quader::document::Document &document);
	/**
	 * Redo the latest undone command.
	 *
	 * @param document Document passed to the command.
	 * @return Command result, or a rejection when no redo is available.
	 */
	[[nodiscard]] CommandResult redo(quader::document::Document &document);

	/**
	 * Check whether an undo command is available.
	 *
	 * @return True when the undo stack is not empty.
	 */
	[[nodiscard]] bool can_undo() const noexcept;
	/**
	 * Check whether a redo command is available.
	 *
	 * @return True when the redo stack is not empty.
	 */
	[[nodiscard]] bool can_redo() const noexcept;
	/**
	 * Return the name of the next undo command.
	 *
	 * @return Borrowed command name, or an empty view when undo is unavailable.
	 */
	[[nodiscard]] std::string_view undo_name() const noexcept;
	/**
	 * Return the name of the next redo command.
	 *
	 * @return Borrowed command name, or an empty view when redo is unavailable.
	 */
	[[nodiscard]] std::string_view redo_name() const noexcept;

	/// Drop all undo and redo history without mutating the document.
	void clear() noexcept;

private:
	std::vector<std::unique_ptr<ICommand>> undo_stack_;
	std::vector<std::unique_ptr<ICommand>> redo_stack_;
};

} // namespace quader::commands
