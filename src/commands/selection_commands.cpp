/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
#include "commands/selection_commands.hpp"

#include <algorithm>
#include <span>
#include <utility>
#include <variant>
#include <vector>

namespace quader::commands {
namespace {

[[nodiscard]] CommandResult rejected_from_document_error(quader::document::DocumentError error) {
	return CommandResult::rejected(std::move(error.diagnostic));
}

[[nodiscard]] CommandResult rejected_from_mesh_error(quader::mesh::MeshError error) {
	return CommandResult::rejected(std::move(error.diagnostic));
}

template <class T>
void append_unique(std::vector<T> &values, T value) {
	if (std::find(values.begin(), values.end(), value) == values.end()) {
		values.push_back(value);
	}
}

template <class T>
bool remove_value(std::vector<T> &values, const T &value) {
	const auto old_size = values.size();
	values.erase(std::remove(values.begin(), values.end(), value), values.end());
	return values.size() != old_size;
}

void sort_unique_objects(std::vector<quader::document::ObjectId> &objects) {
	std::sort(objects.begin(), objects.end());
	objects.erase(std::unique(objects.begin(), objects.end()), objects.end());
}

void sort_unique_components(std::vector<quader::document::ComponentRef> &components) {
	quader::document::Selection normalized;
	if (normalized.set_components(std::move(components))) {
		components.assign(normalized.selected_components().begin(),
				normalized.selected_components().end());
	}
}

[[nodiscard]] std::vector<quader::document::ObjectId> object_vector(
		std::span<const quader::document::ObjectId> objects) {
	return std::vector<quader::document::ObjectId>{ objects.begin(), objects.end() };
}

[[nodiscard]] std::vector<quader::document::ComponentRef> component_vector(
		std::span<const quader::document::ComponentRef> components) {
	return std::vector<quader::document::ComponentRef>{ components.begin(), components.end() };
}

[[nodiscard]] std::vector<quader::document::ObjectId> component_targets(
		const quader::document::Selection &selection,
		const quader::document::Document &document) {
	std::vector<quader::document::ObjectId> targets = object_vector(selection.selected_objects());
	for (const auto &component : selection.selected_components()) {
		append_unique(targets, component.object);
	}
	if (targets.empty()) {
		targets = document.object_ids();
	}
	sort_unique_objects(targets);
	return targets;
}

[[nodiscard]] std::vector<quader::document::ComponentRef> all_component_refs_for_mode(
		const quader::document::Document &document,
		quader::document::SelectionMode mode,
		std::span<const quader::document::ObjectId> objects) {
	std::vector<quader::document::ComponentRef> components;
	for (const auto kObject : objects) {
		const auto *object = document.find_mesh_object(kObject);
		if (object == nullptr) {
			continue;
		}

		switch (mode) {
			case quader::document::SelectionMode::Vertex:
				for (const auto kVertex : object->mesh.vertex_ids()) {
					components.push_back(quader::document::ComponentRef{ kObject, kVertex });
				}
				break;
			case quader::document::SelectionMode::Edge:
				for (const auto kEdge : object->mesh.edge_ids()) {
					components.push_back(quader::document::ComponentRef{ kObject, kEdge });
				}
				break;
			case quader::document::SelectionMode::Face:
				for (const auto kFace : object->mesh.face_ids()) {
					components.push_back(quader::document::ComponentRef{ kObject, kFace });
				}
				break;
			case quader::document::SelectionMode::Object:
				break;
		}
	}
	return components;
}

[[nodiscard]] quader::document::Selection make_mode_selection(
		const quader::document::Selection &current,
		quader::document::SelectionMode mode) {
	std::vector<quader::document::ObjectId> targets = object_vector(current.selected_objects());
	for (const auto &component : current.selected_components()) {
		append_unique(targets, component.object);
	}
	sort_unique_objects(targets);

	quader::document::Selection next;
	if (mode == quader::document::SelectionMode::Object) {
		[[maybe_unused]] const auto selected = next.set_objects(std::move(targets));
		return next;
	}

	[[maybe_unused]] const auto selected = next.set_component_selection(mode, std::move(targets), {});
	return next;
}

[[nodiscard]] quader::document::Selection edit_object_selection(
		const quader::document::Selection &current,
		const quader::document::Selection &operand,
		SelectionEditOperation operation) {
	if (operation == SelectionEditOperation::Replace) {
		return operand;
	}

	quader::document::Selection next;
	if (current.mode() == quader::document::SelectionMode::Object) {
		std::vector<quader::document::ObjectId> objects = object_vector(current.selected_objects());
		for (const auto kObject : operand.selected_objects()) {
			if (operation == SelectionEditOperation::Add) {
				append_unique(objects, kObject);
			} else if (operation == SelectionEditOperation::Remove) {
				[[maybe_unused]] const bool removed = remove_value(objects, kObject);
			} else if (operation == SelectionEditOperation::Toggle) {
				if (!remove_value(objects, kObject)) {
					objects.push_back(kObject);
				}
			}
		}
		[[maybe_unused]] const auto selected = next.set_objects(std::move(objects));
		return next;
	}

	std::vector<quader::document::ObjectId> targets = object_vector(current.selected_objects());
	std::vector<quader::document::ComponentRef> components = component_vector(current.selected_components());
	for (const auto kObject : operand.selected_objects()) {
		if (operation == SelectionEditOperation::Add) {
			append_unique(targets, kObject);
		} else if (operation == SelectionEditOperation::Remove) {
			[[maybe_unused]] const bool removed_target = remove_value(targets, kObject);
			components.erase(std::remove_if(components.begin(),
									  components.end(),
									  [kObject](const quader::document::ComponentRef &component) {
										  return component.object == kObject;
									  }),
					components.end());
		} else if (operation == SelectionEditOperation::Toggle) {
			if (remove_value(targets, kObject)) {
				components.erase(std::remove_if(components.begin(),
										  components.end(),
										  [kObject](const quader::document::ComponentRef &component) {
											  return component.object == kObject;
										  }),
						components.end());
			} else {
				targets.push_back(kObject);
			}
		}
	}
	[[maybe_unused]] const auto selected = next.set_component_selection(current.mode(),
			std::move(targets),
			std::move(components));
	return next;
}

[[nodiscard]] quader::document::Selection edit_component_selection(
		const quader::document::Selection &current,
		const quader::document::Selection &operand,
		SelectionEditOperation operation) {
	const auto kMode = operand.mode();
	if (operation == SelectionEditOperation::Replace) {
		return operand;
	}

	std::vector<quader::document::ObjectId> targets;
	std::vector<quader::document::ComponentRef> components;
	if (current.mode() == kMode) {
		targets = object_vector(current.selected_objects());
		components = component_vector(current.selected_components());
	} else if (current.mode() == quader::document::SelectionMode::Object) {
		targets = object_vector(current.selected_objects());
	}

	for (const auto kObject : operand.selected_objects()) {
		append_unique(targets, kObject);
	}

	for (const auto &component : operand.selected_components()) {
		if (operation == SelectionEditOperation::Add) {
			append_unique(components, component);
			append_unique(targets, component.object);
		} else if (operation == SelectionEditOperation::Remove) {
			[[maybe_unused]] const bool removed = remove_value(components, component);
		} else if (operation == SelectionEditOperation::Toggle) {
			if (!remove_value(components, component)) {
				components.push_back(component);
				append_unique(targets, component.object);
			}
		}
	}

	sort_unique_components(components);
	quader::document::Selection next;
	[[maybe_unused]] const auto selected = next.set_component_selection(kMode,
			std::move(targets),
			std::move(components));
	return next;
}

[[nodiscard]] quader::document::Selection apply_selection_edit(
		const quader::document::Selection &current,
		const quader::document::Selection &operand,
		SelectionEditOperation operation) {
	if (operation == SelectionEditOperation::Replace) {
		return operand;
	}

	if (operand.mode() == quader::document::SelectionMode::Object) {
		return edit_object_selection(current, operand, operation);
	}

	return edit_component_selection(current, operand, operation);
}

struct FaceFlipGroup final {
	quader::document::ObjectId object;
	std::vector<quader::mesh::FaceId> faces;
};

[[nodiscard]] FaceFlipGroup *find_or_add_group(std::vector<FaceFlipGroup> &groups,
		quader::document::ObjectId object) {
	const auto found = std::find_if(groups.begin(), groups.end(), [object](const FaceFlipGroup &group) {
		return group.object == object;
	});
	if (found != groups.end()) {
		return &(*found);
	}

	groups.push_back(FaceFlipGroup{ object, {} });
	return &groups.back();
}

} // namespace

SetSelectionCommand::SetSelectionCommand(quader::document::Selection selection) : selection_(std::move(selection)) {
}

std::string_view SetSelectionCommand::name() const noexcept {
	return "Set Selection";
}

CommandResult SetSelectionCommand::execute(quader::document::Document &document) {
	previous_selection_ = document.selection();
	auto current_liveness = document.validate_current_selection_liveness();
	if (!current_liveness) {
		previous_selection_.reset();
		return rejected_from_document_error(std::move(current_liveness).error());
	}
	if (*previous_selection_ == selection_) {
		previous_selection_.reset();
		return CommandResult::rejected("selection command would not change the active selection");
	}

	auto selected = document.set_selection(selection_);
	if (!selected) {
		previous_selection_.reset();
		return rejected_from_document_error(std::move(selected).error());
	}

	return CommandResult::applied();
}

CommandResult SetSelectionCommand::undo(quader::document::Document &document) {
	if (!previous_selection_) {
		return CommandResult::rejected("selection command has no previous selection");
	}

	auto selected = document.set_selection(*previous_selection_);
	if (!selected) {
		return rejected_from_document_error(std::move(selected).error());
	}

	return CommandResult::applied();
}

EditSelectionCommand::EditSelectionCommand(SelectionEditOperation operation,
		quader::document::Selection selection)
		: operation_(operation),
		  selection_(std::move(selection)) {
}

std::string_view EditSelectionCommand::name() const noexcept {
	switch (operation_) {
		case SelectionEditOperation::Replace:
			return "Set Selection";
		case SelectionEditOperation::Add:
			return "Add Selection";
		case SelectionEditOperation::Remove:
			return "Remove Selection";
		case SelectionEditOperation::Toggle:
			return "Toggle Selection";
	}

	return "Edit Selection";
}

CommandResult EditSelectionCommand::execute(quader::document::Document &document) {
	previous_selection_ = document.selection();
	auto current_liveness = document.validate_current_selection_liveness();
	if (!current_liveness) {
		previous_selection_.reset();
		return rejected_from_document_error(std::move(current_liveness).error());
	}
	auto operand_liveness = document.validate_selection_liveness(selection_);
	if (!operand_liveness) {
		previous_selection_.reset();
		return rejected_from_document_error(std::move(operand_liveness).error());
	}

	const quader::document::Selection next =
			apply_selection_edit(document.selection(), selection_, operation_);
	if (next == document.selection()) {
		previous_selection_.reset();
		return CommandResult::rejected("selection edit would not change the active selection");
	}

	auto selected = document.set_selection(next);
	if (!selected) {
		previous_selection_.reset();
		return rejected_from_document_error(std::move(selected).error());
	}

	return CommandResult::applied();
}

CommandResult EditSelectionCommand::undo(quader::document::Document &document) {
	if (!previous_selection_) {
		return CommandResult::rejected("selection edit command has no previous selection");
	}

	auto selected = document.set_selection(*previous_selection_);
	if (!selected) {
		return rejected_from_document_error(std::move(selected).error());
	}

	return CommandResult::applied();
}

std::string_view ClearSelectionCommand::name() const noexcept {
	return "Clear Selection";
}

CommandResult ClearSelectionCommand::execute(quader::document::Document &document) {
	previous_selection_ = document.selection();
	auto current_liveness = document.validate_current_selection_liveness();
	if (!current_liveness) {
		previous_selection_.reset();
		return rejected_from_document_error(std::move(current_liveness).error());
	}
	if (previous_selection_->empty()) {
		previous_selection_.reset();
		return CommandResult::rejected("selection is already empty");
	}

	document.clear_selection();
	return CommandResult::applied();
}

CommandResult ClearSelectionCommand::undo(quader::document::Document &document) {
	if (!previous_selection_) {
		return CommandResult::rejected("clear selection command has no previous selection");
	}

	auto selected = document.set_selection(*previous_selection_);
	if (!selected) {
		return rejected_from_document_error(std::move(selected).error());
	}

	return CommandResult::applied();
}

SetSelectionModeCommand::SetSelectionModeCommand(quader::document::SelectionMode mode) : mode_(mode) {
}

std::string_view SetSelectionModeCommand::name() const noexcept {
	return "Set Selection Mode";
}

CommandResult SetSelectionModeCommand::execute(quader::document::Document &document) {
	previous_selection_ = document.selection();
	auto current_liveness = document.validate_current_selection_liveness();
	if (!current_liveness) {
		previous_selection_.reset();
		return rejected_from_document_error(std::move(current_liveness).error());
	}
	if (previous_selection_->mode() == mode_) {
		previous_selection_.reset();
		return CommandResult::rejected("selection mode command would not change the active mode");
	}

	auto next = make_mode_selection(*previous_selection_, mode_);
	auto selected = document.set_selection(next);
	if (!selected) {
		previous_selection_.reset();
		return rejected_from_document_error(std::move(selected).error());
	}

	return CommandResult::applied();
}

CommandResult SetSelectionModeCommand::undo(quader::document::Document &document) {
	if (!previous_selection_) {
		return CommandResult::rejected("selection mode command has no previous selection");
	}

	auto selected = document.set_selection(*previous_selection_);
	if (!selected) {
		return rejected_from_document_error(std::move(selected).error());
	}

	return CommandResult::applied();
}

std::string_view SelectAllCommand::name() const noexcept {
	return "Select All";
}

CommandResult SelectAllCommand::execute(quader::document::Document &document) {
	previous_selection_ = document.selection();
	auto current_liveness = document.validate_current_selection_liveness();
	if (!current_liveness) {
		previous_selection_.reset();
		return rejected_from_document_error(std::move(current_liveness).error());
	}

	quader::document::Selection next;
	if (previous_selection_->mode() == quader::document::SelectionMode::Object) {
		auto objects = document.object_ids();
		[[maybe_unused]] const auto selected = next.set_objects(std::move(objects));
	} else {
		auto targets = component_targets(*previous_selection_, document);
		auto components = all_component_refs_for_mode(document, previous_selection_->mode(), targets);
		[[maybe_unused]] const auto selected = next.set_component_selection(previous_selection_->mode(),
				std::move(targets),
				std::move(components));
	}

	if (next == document.selection()) {
		previous_selection_.reset();
		return CommandResult::rejected("select all would not change the active selection");
	}

	auto selected = document.set_selection(next);
	if (!selected) {
		previous_selection_.reset();
		return rejected_from_document_error(std::move(selected).error());
	}

	return CommandResult::applied();
}

CommandResult SelectAllCommand::undo(quader::document::Document &document) {
	if (!previous_selection_) {
		return CommandResult::rejected("select all command has no previous selection");
	}

	auto selected = document.set_selection(*previous_selection_);
	if (!selected) {
		return rejected_from_document_error(std::move(selected).error());
	}

	return CommandResult::applied();
}

std::string_view InvertSelectionCommand::name() const noexcept {
	return "Invert Selection";
}

CommandResult InvertSelectionCommand::execute(quader::document::Document &document) {
	previous_selection_ = document.selection();
	auto current_liveness = document.validate_current_selection_liveness();
	if (!current_liveness) {
		previous_selection_.reset();
		return rejected_from_document_error(std::move(current_liveness).error());
	}

	quader::document::Selection next;
	if (previous_selection_->mode() == quader::document::SelectionMode::Object) {
		std::vector<quader::document::ObjectId> inverted;
		for (const auto kObject : document.object_ids()) {
			if (!previous_selection_->contains_object(kObject)) {
				inverted.push_back(kObject);
			}
		}
		[[maybe_unused]] const auto selected = next.set_objects(std::move(inverted));
	} else {
		auto targets = component_targets(*previous_selection_, document);
		const auto universe = all_component_refs_for_mode(document, previous_selection_->mode(), targets);
		std::vector<quader::document::ComponentRef> inverted;
		for (const auto &component : universe) {
			if (!previous_selection_->contains_component(component)) {
				inverted.push_back(component);
			}
		}
		[[maybe_unused]] const auto selected = next.set_component_selection(previous_selection_->mode(),
				std::move(targets),
				std::move(inverted));
	}

	if (next == document.selection()) {
		previous_selection_.reset();
		return CommandResult::rejected("invert selection would not change the active selection");
	}

	auto selected = document.set_selection(next);
	if (!selected) {
		previous_selection_.reset();
		return rejected_from_document_error(std::move(selected).error());
	}

	return CommandResult::applied();
}

CommandResult InvertSelectionCommand::undo(quader::document::Document &document) {
	if (!previous_selection_) {
		return CommandResult::rejected("invert selection command has no previous selection");
	}

	auto selected = document.set_selection(*previous_selection_);
	if (!selected) {
		return rejected_from_document_error(std::move(selected).error());
	}

	return CommandResult::applied();
}

std::string_view FlipMeshNormalsCommand::name() const noexcept {
	return "Flip Mesh Normals";
}

CommandResult FlipMeshNormalsCommand::execute(quader::document::Document &document) {
	if (!after_meshes_.empty() && after_selection_) {
		return restore_snapshot(document, after_meshes_, *after_selection_);
	}

	before_selection_ = document.selection();
	auto current_liveness = document.validate_current_selection_liveness();
	if (!current_liveness) {
		before_selection_.reset();
		return rejected_from_document_error(std::move(current_liveness).error());
	}

	std::vector<FaceFlipGroup> groups;
	if (before_selection_->mode() == quader::document::SelectionMode::Object) {
		for (const auto kObject : before_selection_->selected_objects()) {
			const auto *object = document.find_mesh_object(kObject);
			if (object == nullptr) {
				before_selection_.reset();
				return CommandResult::rejected("flip normals selection contains an unknown object");
			}
			FaceFlipGroup *group = find_or_add_group(groups, kObject);
			group->faces = object->mesh.face_ids();
		}
	} else if (before_selection_->mode() == quader::document::SelectionMode::Face) {
		for (const auto &component : before_selection_->selected_components()) {
			if (quader::document::component_kind(component) != quader::document::ComponentKind::Face) {
				continue;
			}
			FaceFlipGroup *group = find_or_add_group(groups, component.object);
			append_unique(group->faces, std::get<quader::mesh::FaceId>(component.component));
		}
	} else {
		before_selection_.reset();
		return CommandResult::rejected("flip mesh normals requires object or face selection");
	}

	groups.erase(std::remove_if(groups.begin(),
						 groups.end(),
						 [](const FaceFlipGroup &group) {
							 return group.faces.empty();
						 }),
			groups.end());
	if (groups.empty()) {
		before_selection_.reset();
		return CommandResult::rejected("flip mesh normals found no live selected faces");
	}

	before_meshes_.clear();
	before_meshes_.reserve(groups.size());
	for (const FaceFlipGroup &group : groups) {
		const auto *object = document.find_mesh_object(group.object);
		if (object == nullptr) {
			before_selection_.reset();
			before_meshes_.clear();
			return CommandResult::rejected("flip normals selection contains an unknown object");
		}
		before_meshes_.push_back(MeshSnapshot{ group.object, object->mesh });
	}

	for (const FaceFlipGroup &group : groups) {
		auto *object = document.find_mesh_object(group.object);
		if (object == nullptr) {
			for (const MeshSnapshot &snapshot : before_meshes_) {
				if (auto *restore_object = document.find_mesh_object(snapshot.object)) {
					restore_object->mesh = snapshot.mesh;
				}
			}
			before_selection_.reset();
			before_meshes_.clear();
			return CommandResult::rejected("flip normals selection contains an unknown object");
		}

		auto flipped = object->mesh.reverse_face_windings(group.faces);
		if (!flipped) {
			for (const MeshSnapshot &snapshot : before_meshes_) {
				if (auto *restore_object = document.find_mesh_object(snapshot.object)) {
					restore_object->mesh = snapshot.mesh;
				}
			}
			before_selection_.reset();
			before_meshes_.clear();
			return rejected_from_mesh_error(std::move(flipped).error());
		}
	}

	after_meshes_.clear();
	after_meshes_.reserve(groups.size());
	for (const FaceFlipGroup &group : groups) {
		const auto *object = document.find_mesh_object(group.object);
		if (object != nullptr) {
			after_meshes_.push_back(MeshSnapshot{ group.object, object->mesh });
			auto dirty = document.mark_mesh_topology_changed(group.object);
			if (!dirty) {
				return rejected_from_document_error(std::move(dirty).error());
			}
		}
	}

	after_selection_ = document.selection();
	return CommandResult::applied();
}

CommandResult FlipMeshNormalsCommand::undo(quader::document::Document &document) {
	if (before_meshes_.empty() || !before_selection_) {
		return CommandResult::rejected("flip mesh normals command has no previous mesh state");
	}

	return restore_snapshot(document, before_meshes_, *before_selection_);
}

CommandResult FlipMeshNormalsCommand::restore_snapshot(quader::document::Document &document,
		const std::vector<MeshSnapshot> &meshes,
		const quader::document::Selection &selection) const {
	auto selection_liveness = document.validate_selection_liveness(selection);
	if (!selection_liveness) {
		return rejected_from_document_error(std::move(selection_liveness).error());
	}

	for (const MeshSnapshot &snapshot : meshes) {
		if (document.find_mesh_object(snapshot.object) == nullptr) {
			return CommandResult::rejected("cannot restore flipped normals for an unknown object");
		}
	}

	for (const MeshSnapshot &snapshot : meshes) {
		auto *object = document.find_mesh_object(snapshot.object);
		object->mesh = snapshot.mesh;
		auto dirty = document.mark_mesh_topology_changed(snapshot.object);
		if (!dirty) {
			return rejected_from_document_error(std::move(dirty).error());
		}
	}

	auto selected = document.set_selection(selection);
	if (!selected) {
		return rejected_from_document_error(std::move(selected).error());
	}

	return CommandResult::applied();
}

} // namespace quader::commands
