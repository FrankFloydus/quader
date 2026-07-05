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
#include "document/object_id.hpp"
#include "document/scene_object.hpp"
#include "document/selection.hpp"
#include "document/transform.hpp"
#include "mesh/core/polyhedron.hpp"

#include <optional>
#include <string>
#include <string_view>

namespace quader::commands {

/**
 * Undoable command that creates a validated mesh object and selects it.
 *
 * The command owns the input mesh until first execution, then stores the
 * created object snapshot and prior selection on undo so redo can restore the
 * same object id as the only selected object.
 */
class CreateMeshObjectCommand final : public ICommand {
public:
	/**
	 * Construct a create-object command.
	 *
	 * @param name Object name to create.
	 * @param mesh Mesh payload to move into the document on execute.
	 * @param transform Initial transform.
	 * @param material Initial material.
	 */
	CreateMeshObjectCommand(std::string name,
			quader::mesh::Polyhedron mesh,
			quader::document::Transform transform = {},
			quader::document::PbrMaterial material = quader::document::default_mesh_material());

	/// Return the command display name.
	[[nodiscard]] std::string_view name() const noexcept override;
	/// Create or restore the mesh object in `document`.
	[[nodiscard]] CommandResult execute(quader::document::Document &document) override;
	/// Extract the created object for redo.
	[[nodiscard]] CommandResult undo(quader::document::Document &document) override;

	/// Return the created object id after successful execution.
	[[nodiscard]] std::optional<quader::document::ObjectId> object_id() const noexcept;

private:
	std::string name_;
	std::optional<quader::mesh::Polyhedron> mesh_;
	quader::document::Transform transform_;
	quader::document::PbrMaterial material_;
	std::optional<quader::document::ObjectId> object_id_;
	std::optional<quader::document::MeshObject> created_object_;
	std::optional<quader::document::Selection> previous_selection_;
};

/// Undoable command that removes one object and restores prior selection on undo.
class DeleteObjectCommand final : public ICommand {
public:
	/// Construct a delete command for `object`.
	explicit DeleteObjectCommand(quader::document::ObjectId object);

	/// Return the command display name.
	[[nodiscard]] std::string_view name() const noexcept override;
	/// Remove the object and capture its payload.
	[[nodiscard]] CommandResult execute(quader::document::Document &document) override;
	/// Restore the removed object and previous selection.
	[[nodiscard]] CommandResult undo(quader::document::Document &document) override;

private:
	quader::document::ObjectId object_;
	std::optional<quader::document::MeshObject> removed_object_;
	std::optional<quader::document::Selection> selection_before_;
};

/// Undoable command that changes one object's name.
class RenameObjectCommand final : public ICommand {
public:
	/// Construct a rename command for `object`.
	RenameObjectCommand(quader::document::ObjectId object, std::string new_name);

	/// Return the command display name.
	[[nodiscard]] std::string_view name() const noexcept override;
	/// Apply the new name and capture the previous name.
	[[nodiscard]] CommandResult execute(quader::document::Document &document) override;
	/// Restore the previous name.
	[[nodiscard]] CommandResult undo(quader::document::Document &document) override;

private:
	quader::document::ObjectId object_;
	std::string new_name_;
	std::optional<std::string> previous_name_;
};

/// Undoable command that replaces one object's transform.
class SetObjectTransformCommand final : public ICommand {
public:
	/// Construct a transform command for `object`.
	SetObjectTransformCommand(quader::document::ObjectId object,
			quader::document::Transform transform);

	/// Return the command display name.
	[[nodiscard]] std::string_view name() const noexcept override;
	/// Apply the new transform and capture the previous transform.
	[[nodiscard]] CommandResult execute(quader::document::Document &document) override;
	/// Restore the previous transform.
	[[nodiscard]] CommandResult undo(quader::document::Document &document) override;

private:
	quader::document::ObjectId object_;
	quader::document::Transform transform_;
	std::optional<quader::document::Transform> previous_transform_;
};

} // namespace quader::commands
