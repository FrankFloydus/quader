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
#include "commands/command_history.hpp"
#include "document/document.hpp"
#include "document/object_id.hpp"
#include "document/transform.hpp"

#include <functional>
#include <memory>
#include <span>

namespace quader::tools {

/// One transient transform preview edit applied outside undo history.
struct ToolContextTransformPreviewEdit {
	/// Object to preview-transform.
	quader::document::ObjectId object;
	/// Preview transform to apply.
	quader::document::Transform transform;
};

/// Services exposed to editor tools without giving tools UI ownership.
class ToolContext final {
public:
	/**
	 * Construct a tool context over a document and command history.
	 *
	 * @param document Document that tools read and mutate through commands.
	 * @param command_history Command history used to apply undoable tool commands.
	 */
	ToolContext(quader::document::Document &document,
			quader::commands::CommandHistory &command_history) noexcept;

	/// Return the document visible to tools.
	[[nodiscard]] const quader::document::Document &document() const noexcept;
	/// Return the command history used for undoable tool commits.
	[[nodiscard]] quader::commands::CommandHistory &command_history() noexcept;
	/// Return the command history used for undoable tool commits.
	[[nodiscard]] const quader::commands::CommandHistory &command_history() const noexcept;

	/**
	 * Execute a tool command through the command history.
	 *
	 * @param command Command to execute against the document.
	 * @return Command result from the history; the after-apply callback runs only when applied.
	 */
	[[nodiscard]] quader::commands::CommandResult execute_command(
			std::unique_ptr<quader::commands::ICommand> command);
	/**
	 * Apply transient object transforms for a live viewport preview.
	 *
	 * Preview transforms mutate the document state used by renderers and overlays,
	 * but they do not create undo entries or mark the document dirty.
	 */
	[[nodiscard]] quader::commands::CommandResult preview_object_transforms(
			std::span<const ToolContextTransformPreviewEdit> edits);
	/// Set a callback invoked after a command is applied successfully.
	void set_after_command_applied(std::function<void()> callback);
	/// Set a callback invoked after a preview mutation changes document state.
	void set_after_preview_mutation_applied(std::function<void()> callback);

private:
	quader::document::Document &document_;
	quader::commands::CommandHistory &command_history_;
	std::function<void()> after_command_applied_;
	std::function<void()> after_preview_mutation_applied_;
};

} // namespace quader::tools
