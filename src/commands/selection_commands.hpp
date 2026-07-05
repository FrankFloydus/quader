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
#include "mesh/core/polyhedron.hpp"

#include <optional>
#include <string_view>
#include <vector>

namespace quader::commands {

/// Selection-set operation applied by `EditSelectionCommand`.
enum class SelectionEditOperation {
	Replace, ///< Replace/select-only with the provided selection.
	Add,     ///< Add the provided refs to the current selection.
	Remove,  ///< Remove the provided refs from the current selection.
	Toggle,  ///< Toggle the provided refs in the current selection.
};

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

/// Command that applies add, remove, toggle, or replace selection verbs.
class EditSelectionCommand final : public ICommand {
public:
	/**
	 * Create a selection edit command.
	 *
	 * @param operation Selection-set operation to apply.
	 * @param selection Object or component refs used as the command operand.
	 */
	EditSelectionCommand(SelectionEditOperation operation,
			quader::document::Selection selection);

	/// Return the display name used by undo/redo UI.
	[[nodiscard]] std::string_view name() const noexcept override;
	/// Apply the requested selection edit.
	[[nodiscard]] CommandResult execute(quader::document::Document &document) override;
	/// Restore the selection captured during `execute()`.
	[[nodiscard]] CommandResult undo(quader::document::Document &document) override;

private:
	SelectionEditOperation operation_ = SelectionEditOperation::Replace;
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

/// Command that changes selection mode and preserves object edit targets.
class SetSelectionModeCommand final : public ICommand {
public:
	/**
	 * Create a mode-change command.
	 *
	 * @param mode New selection mode.
	 */
	explicit SetSelectionModeCommand(quader::document::SelectionMode mode);

	/// Return the display name used by undo/redo UI.
	[[nodiscard]] std::string_view name() const noexcept override;
	/// Change selection mode.
	[[nodiscard]] CommandResult execute(quader::document::Document &document) override;
	/// Restore the prior selection mode and refs.
	[[nodiscard]] CommandResult undo(quader::document::Document &document) override;

private:
	quader::document::SelectionMode mode_ = quader::document::SelectionMode::Object;
	std::optional<quader::document::Selection> previous_selection_;
};

/// Command that selects every live object or component in the active mode.
class SelectAllCommand final : public ICommand {
public:
	/// Return the display name used by undo/redo UI.
	[[nodiscard]] std::string_view name() const noexcept override;
	/// Select every live object or component for the current mode.
	[[nodiscard]] CommandResult execute(quader::document::Document &document) override;
	/// Restore the previous selection.
	[[nodiscard]] CommandResult undo(quader::document::Document &document) override;

private:
	std::optional<quader::document::Selection> previous_selection_;
};

/// Command that inverts object or component selection in the active mode.
class InvertSelectionCommand final : public ICommand {
public:
	/// Return the display name used by undo/redo UI.
	[[nodiscard]] std::string_view name() const noexcept override;
	/// Invert live refs for the current selection mode.
	[[nodiscard]] CommandResult execute(quader::document::Document &document) override;
	/// Restore the previous selection.
	[[nodiscard]] CommandResult undo(quader::document::Document &document) override;

private:
	std::optional<quader::document::Selection> previous_selection_;
};

/// Command that flips actual mesh face winding for selected objects or faces.
class FlipMeshNormalsCommand final : public ICommand {
public:
	/// Return the display name used by undo/redo UI.
	[[nodiscard]] std::string_view name() const noexcept override;
	/// Flip all selected-object faces or selected face components.
	[[nodiscard]] CommandResult execute(quader::document::Document &document) override;
	/// Restore exact pre-flip meshes and selection.
	[[nodiscard]] CommandResult undo(quader::document::Document &document) override;

private:
	struct MeshSnapshot {
		quader::document::ObjectId object;
		quader::mesh::Polyhedron mesh;
	};

	[[nodiscard]] CommandResult restore_snapshot(quader::document::Document &document,
			const std::vector<MeshSnapshot> &meshes,
			const quader::document::Selection &selection) const;

	std::vector<MeshSnapshot> before_meshes_;
	std::vector<MeshSnapshot> after_meshes_;
	std::optional<quader::document::Selection> before_selection_;
	std::optional<quader::document::Selection> after_selection_;
};

} // namespace quader::commands
