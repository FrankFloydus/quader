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
#include "document/selection.hpp"

#include <optional>
#include <string_view>

namespace quader::commands {

/// Command that replaces the document selection and can restore the prior selection.
class SetSelectionCommand final : public ICommand {
public:
	/**
	 * Create a selection replacement command.
	 *
	 * @param selection Selection stored and later applied by `execute()`.
	 */
	explicit SetSelectionCommand(quader::document::Selection selection);

	/**
	 * Return the display name used by undo/redo UI.
	 *
	 * @return Static command name.
	 */
	[[nodiscard]] std::string_view name() const noexcept override;
	/**
	 * Replace the document selection.
	 *
	 * @param document Document to mutate.
	 * @return Applied result after storing the previous selection.
	 */
	[[nodiscard]] CommandResult execute(quader::document::Document &document) override;
	/**
	 * Restore the selection captured during `execute()`.
	 *
	 * @param document Document to mutate.
	 * @return Applied result when a previous selection was captured.
	 */
	[[nodiscard]] CommandResult undo(quader::document::Document &document) override;

private:
	quader::document::Selection selection_;
	std::optional<quader::document::Selection> previous_selection_;
};

/// Command that clears the document selection and can restore the prior selection.
class ClearSelectionCommand final : public ICommand {
public:
	/**
	 * Return the display name used by undo/redo UI.
	 *
	 * @return Static command name.
	 */
	[[nodiscard]] std::string_view name() const noexcept override;
	/**
	 * Clear the document selection.
	 *
	 * @param document Document to mutate.
	 * @return Applied result after storing the previous selection.
	 */
	[[nodiscard]] CommandResult execute(quader::document::Document &document) override;
	/**
	 * Restore the selection captured during `execute()`.
	 *
	 * @param document Document to mutate.
	 * @return Applied result when a previous selection was captured.
	 */
	[[nodiscard]] CommandResult undo(quader::document::Document &document) override;

private:
	std::optional<quader::document::Selection> previous_selection_;
};

} // namespace quader::commands
