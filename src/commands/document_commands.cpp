/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
#include "commands/document_commands.hpp"

#include <algorithm>
#include <utility>

namespace quader::commands {
namespace {

[[nodiscard]] CommandResult rejected_from_document_error(quader::document::DocumentError error) {
	return CommandResult::rejected(std::move(error.diagnostic));
}

} // namespace

CreateMeshObjectCommand::CreateMeshObjectCommand(std::string name,
		quader::mesh::Polyhedron mesh,
		quader::document::Transform transform,
		quader::document::PbrMaterial material) : name_(std::move(name)),
												  mesh_(std::move(mesh)),
												  transform_(transform),
												  material_(material) {
}

std::string_view CreateMeshObjectCommand::name() const noexcept {
	return "Create Mesh Object";
}

CommandResult CreateMeshObjectCommand::execute(quader::document::Document &document) {
	if (!previous_selection_) {
		previous_selection_ = document.selection();
	}

	if (created_object_) {
		if (document.find_mesh_object(created_object_->id) != nullptr) {
			return CommandResult::rejected("cannot redo create because the object is already live");
		}

		const auto kId = created_object_->id;
		auto restored = document.restore_mesh_object(std::move(*created_object_));
		if (!restored) {
			created_object_.reset();
			return rejected_from_document_error(std::move(restored).error());
		}

		created_object_.reset();
		object_id_ = kId;
		quader::document::Selection selection;
		auto selected = selection.set_objects({ kId });
		if (!selected) {
			return rejected_from_document_error(std::move(selected).error());
		}

		auto selection_applied = document.set_selection(std::move(selection));
		if (!selection_applied) {
			return rejected_from_document_error(std::move(selection_applied).error());
		}

		return CommandResult::applied();
	}

	if (!mesh_) {
		return CommandResult::rejected("create command has no mesh data to execute");
	}

	auto created = document.create_mesh_object(name_, std::move(*mesh_), transform_, material_);
	if (!created) {
		mesh_.reset();
		return rejected_from_document_error(std::move(created).error());
	}

	object_id_ = created.value();
	mesh_.reset();
	quader::document::Selection selection;
	auto selected = selection.set_objects({ *object_id_ });
	if (!selected) {
		return rejected_from_document_error(std::move(selected).error());
	}

	auto selection_applied = document.set_selection(std::move(selection));
	if (!selection_applied) {
		return rejected_from_document_error(std::move(selection_applied).error());
	}

	return CommandResult::applied();
}

CommandResult CreateMeshObjectCommand::undo(quader::document::Document &document) {
	if (!object_id_ || !previous_selection_) {
		return CommandResult::rejected("create command has not created an object");
	}

	auto removed = document.extract_mesh_object(*object_id_);
	if (!removed) {
		return rejected_from_document_error(std::move(removed).error());
	}

	created_object_ = std::move(removed).value();
	auto selected = document.set_selection(*previous_selection_);
	if (!selected) {
		return rejected_from_document_error(std::move(selected).error());
	}

	return CommandResult::applied();
}

std::optional<quader::document::ObjectId> CreateMeshObjectCommand::object_id() const noexcept {
	return object_id_;
}

DeleteObjectCommand::DeleteObjectCommand(quader::document::ObjectId object) : object_(object) {
}

std::string_view DeleteObjectCommand::name() const noexcept {
	return "Delete Object";
}

CommandResult DeleteObjectCommand::execute(quader::document::Document &document) {
	selection_before_ = document.selection();

	auto removed = document.extract_mesh_object(object_);
	if (!removed) {
		selection_before_.reset();
		return rejected_from_document_error(std::move(removed).error());
	}

	removed_object_ = std::move(removed).value();
	return CommandResult::applied();
}

CommandResult DeleteObjectCommand::undo(quader::document::Document &document) {
	if (!removed_object_ || !selection_before_) {
		return CommandResult::rejected("delete command has not removed an object");
	}

	auto restored = document.restore_mesh_object(std::move(*removed_object_));
	if (!restored) {
		removed_object_.reset();
		return rejected_from_document_error(std::move(restored).error());
	}

	removed_object_.reset();
	auto selection_restored = document.set_selection(*selection_before_);
	if (!selection_restored) {
		return rejected_from_document_error(std::move(selection_restored).error());
	}

	return CommandResult::applied();
}

RenameObjectCommand::RenameObjectCommand(quader::document::ObjectId object, std::string new_name) : object_(object), new_name_(std::move(new_name)) {
}

std::string_view RenameObjectCommand::name() const noexcept {
	return "Rename Object";
}

CommandResult RenameObjectCommand::execute(quader::document::Document &document) {
	const auto *object = document.find_mesh_object(object_);
	if (object == nullptr) {
		return CommandResult::rejected("cannot rename an unknown object");
	}

	if (object->name == new_name_) {
		return CommandResult::rejected("rename command would not change the object name");
	}

	previous_name_ = object->name;
	auto renamed = document.rename_object(object_, new_name_);
	if (!renamed) {
		previous_name_.reset();
		return rejected_from_document_error(std::move(renamed).error());
	}

	return CommandResult::applied();
}

CommandResult RenameObjectCommand::undo(quader::document::Document &document) {
	if (!previous_name_) {
		return CommandResult::rejected("rename command has no previous object name");
	}

	auto renamed = document.rename_object(object_, *previous_name_);
	if (!renamed) {
		return rejected_from_document_error(std::move(renamed).error());
	}

	return CommandResult::applied();
}

SetObjectTransformCommand::SetObjectTransformCommand(quader::document::ObjectId object,
		quader::document::Transform transform) : object_(object),
												 transform_(transform) {
}

std::string_view SetObjectTransformCommand::name() const noexcept {
	return "Set Object Transform";
}

CommandResult SetObjectTransformCommand::execute(quader::document::Document &document) {
	const auto *object = document.find_mesh_object(object_);
	if (object == nullptr) {
		return CommandResult::rejected("cannot transform an unknown object");
	}

	if (object->transform == transform_) {
		return CommandResult::rejected("transform command would not change the object transform");
	}

	previous_transform_ = object->transform;
	auto transformed = document.set_transform(object_, transform_);
	if (!transformed) {
		previous_transform_.reset();
		return rejected_from_document_error(std::move(transformed).error());
	}

	return CommandResult::applied();
}

CommandResult SetObjectTransformCommand::undo(quader::document::Document &document) {
	if (!previous_transform_) {
		return CommandResult::rejected("transform command has no previous object transform");
	}

	auto transformed = document.set_transform(object_, *previous_transform_);
	if (!transformed) {
		return rejected_from_document_error(std::move(transformed).error());
	}

	return CommandResult::applied();
}

BatchSetObjectTransformsCommand::BatchSetObjectTransformsCommand(
		std::vector<ObjectTransformEdit> edits) : edits_(std::move(edits)) {
}

std::string_view BatchSetObjectTransformsCommand::name() const noexcept {
	return "Transform Objects";
}

CommandResult BatchSetObjectTransformsCommand::execute(quader::document::Document &document) {
	if (edits_.empty()) {
		return CommandResult::rejected("transform batch has no object edits");
	}

	std::vector<quader::document::ObjectId> seen;
	seen.reserve(edits_.size());
	bool changes_any_transform = false;
	std::vector<ObjectTransformEdit> captured_previous;
	captured_previous.reserve(edits_.size());

	for (const ObjectTransformEdit &edit : edits_) {
		if (!edit.object.is_valid()) {
			return CommandResult::rejected("transform batch contains an invalid object id");
		}
		if (!quader::document::is_finite(edit.transform)) {
			return CommandResult::rejected("transform batch contains a non-finite transform");
		}
		if (std::find(seen.begin(), seen.end(), edit.object) != seen.end()) {
			return CommandResult::rejected("transform batch contains duplicate object ids");
		}
		seen.push_back(edit.object);

		const auto *object = document.find_mesh_object(edit.object);
		if (object == nullptr) {
			return CommandResult::rejected("transform batch references an unknown object");
		}

		if (!(object->transform == edit.transform)) {
			changes_any_transform = true;
		}
		captured_previous.push_back(ObjectTransformEdit{
				.object = edit.object,
				.transform = object->transform,
		});
	}

	if (!changes_any_transform) {
		return CommandResult::rejected("transform batch would not change any object transform");
	}

	if (previous_edits_.empty()) {
		previous_edits_ = captured_previous;
	}

	for (const ObjectTransformEdit &edit : edits_) {
		auto transformed = document.set_transform(edit.object, edit.transform);
		if (!transformed) {
			return rejected_from_document_error(std::move(transformed).error());
		}
	}

	return CommandResult::applied();
}

CommandResult BatchSetObjectTransformsCommand::undo(quader::document::Document &document) {
	if (previous_edits_.empty()) {
		return CommandResult::rejected("transform batch has no previous transforms");
	}

	for (const ObjectTransformEdit &edit : previous_edits_) {
		auto transformed = document.set_transform(edit.object, edit.transform);
		if (!transformed) {
			return rejected_from_document_error(std::move(transformed).error());
		}
	}

	return CommandResult::applied();
}

} // namespace quader::commands
