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

class CreateMeshObjectCommand final : public ICommand {
public:
	CreateMeshObjectCommand(std::string name,
			quader::mesh::Polyhedron mesh,
			quader::document::Transform transform = {});

	[[nodiscard]] std::string_view name() const noexcept override;
	[[nodiscard]] CommandResult execute(quader::document::Document &document) override;
	[[nodiscard]] CommandResult undo(quader::document::Document &document) override;

	[[nodiscard]] std::optional<quader::document::ObjectId> object_id() const noexcept;

private:
	std::string name_;
	std::optional<quader::mesh::Polyhedron> mesh_;
	quader::document::Transform transform_;
	std::optional<quader::document::ObjectId> object_id_;
	std::optional<quader::document::MeshObject> created_object_;
};

class DeleteObjectCommand final : public ICommand {
public:
	explicit DeleteObjectCommand(quader::document::ObjectId object);

	[[nodiscard]] std::string_view name() const noexcept override;
	[[nodiscard]] CommandResult execute(quader::document::Document &document) override;
	[[nodiscard]] CommandResult undo(quader::document::Document &document) override;

private:
	quader::document::ObjectId object_;
	std::optional<quader::document::MeshObject> removed_object_;
	std::optional<quader::document::Selection> selection_before_;
};

class RenameObjectCommand final : public ICommand {
public:
	RenameObjectCommand(quader::document::ObjectId object, std::string new_name);

	[[nodiscard]] std::string_view name() const noexcept override;
	[[nodiscard]] CommandResult execute(quader::document::Document &document) override;
	[[nodiscard]] CommandResult undo(quader::document::Document &document) override;

private:
	quader::document::ObjectId object_;
	std::string new_name_;
	std::optional<std::string> previous_name_;
};

class SetObjectTransformCommand final : public ICommand {
public:
	SetObjectTransformCommand(quader::document::ObjectId object,
			quader::document::Transform transform);

	[[nodiscard]] std::string_view name() const noexcept override;
	[[nodiscard]] CommandResult execute(quader::document::Document &document) override;
	[[nodiscard]] CommandResult undo(quader::document::Document &document) override;

private:
	quader::document::ObjectId object_;
	quader::document::Transform transform_;
	std::optional<quader::document::Transform> previous_transform_;
};

} // namespace quader::commands
