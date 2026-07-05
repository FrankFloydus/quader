/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
#include "document/selection.hpp"

#include <algorithm>
#include <tuple>

namespace quader::document {
namespace {

[[nodiscard]] DocumentError invalid_component_error(ComponentKind kind) {
	switch (kind) {
		case ComponentKind::Vertex:
			return make_document_error(DocumentErrorCode::InvalidVertex,
					"selection contains an invalid vertex id");
		case ComponentKind::Edge:
			return make_document_error(DocumentErrorCode::InvalidEdge,
					"selection contains an invalid edge id");
		case ComponentKind::Face:
			return make_document_error(DocumentErrorCode::InvalidFace,
					"selection contains an invalid face id");
	}

	return make_document_error(DocumentErrorCode::InvalidSelection,
			"selection contains an invalid component id");
}

[[nodiscard]] auto component_key(const ComponentRef &ref) {
	return std::visit(
			[](auto id) {
				return std::tuple{
					id.index(),
					id.generation(),
				};
			},
			ref.component);
}

[[nodiscard]] bool component_less(const ComponentRef &left, const ComponentRef &right) {
	if (left.object != right.object) {
		return left.object < right.object;
	}

	const ComponentKind kLeftKind = component_kind(left);
	const ComponentKind kRightKind = component_kind(right);
	if (kLeftKind != kRightKind) {
		return kLeftKind < kRightKind;
	}

	return component_key(left) < component_key(right);
}

[[nodiscard]] bool component_matches_mode(const ComponentRef &component, SelectionMode mode) noexcept {
	return selection_mode_for_component_kind(component_kind(component)) == mode;
}

[[nodiscard]] quader::foundation::Result<void, DocumentError> validate_object_ids(
		const std::vector<ObjectId> &objects) {
	for (const auto kObject : objects) {
		if (!kObject.is_valid()) {
			return quader::foundation::Result<void, DocumentError>::failure(
					make_document_error(DocumentErrorCode::InvalidObject,
							"selection contains an invalid object id"));
		}
	}

	return quader::foundation::Result<void, DocumentError>::success();
}

} // namespace

ComponentKind component_kind(const ComponentRef &ref) noexcept {
	switch (ref.component.index()) {
		case 0U:
			return ComponentKind::Vertex;
		case 1U:
			return ComponentKind::Edge;
		case 2U:
			return ComponentKind::Face;
		default:
			return ComponentKind::Vertex;
	}
}

SelectionMode selection_mode_for_component_kind(ComponentKind kind) noexcept {
	switch (kind) {
		case ComponentKind::Vertex:
			return SelectionMode::Vertex;
		case ComponentKind::Edge:
			return SelectionMode::Edge;
		case ComponentKind::Face:
			return SelectionMode::Face;
	}

	return SelectionMode::Vertex;
}

bool selection_mode_is_component(SelectionMode mode) noexcept {
	return mode == SelectionMode::Vertex || mode == SelectionMode::Edge ||
			mode == SelectionMode::Face;
}

bool component_id_is_valid(const ComponentRef &ref) {
	return std::visit([](auto id) { return id.is_valid(); }, ref.component);
}

SelectionMode Selection::mode() const noexcept {
	return mode_;
}

void Selection::set_mode(SelectionMode mode) {
	if (mode_ == mode) {
		return;
	}

	mode_ = mode;
	if (mode_ == SelectionMode::Object) {
		components_.clear();
		return;
	}

	components_.erase(std::remove_if(components_.begin(),
							  components_.end(),
							  [this](const ComponentRef &component) {
								  return !component_matches_mode(component, mode_);
							  }),
			components_.end());
}

bool Selection::empty() const noexcept {
	return objects_.empty() && components_.empty();
}

void Selection::clear() {
	mode_ = SelectionMode::Object;
	objects_.clear();
	components_.clear();
}

quader::foundation::Result<void, DocumentError> Selection::set_objects(std::vector<ObjectId> objects) {
	auto validation = validate_object_ids(objects);
	if (!validation) {
		return validation;
	}

	std::sort(objects.begin(), objects.end());
	objects.erase(std::unique(objects.begin(), objects.end()), objects.end());

	mode_ = SelectionMode::Object;
	objects_ = std::move(objects);
	components_.clear();
	return quader::foundation::Result<void, DocumentError>::success();
}

quader::foundation::Result<void, DocumentError> Selection::set_component_selection(
		SelectionMode mode,
		std::vector<ObjectId> targets,
		std::vector<ComponentRef> components) {
	if (!selection_mode_is_component(mode)) {
		return quader::foundation::Result<void, DocumentError>::failure(
				make_document_error(DocumentErrorCode::InvalidSelection,
						"component selection requires a component selection mode"));
	}

	auto target_validation = validate_object_ids(targets);
	if (!target_validation) {
		return target_validation;
	}

	for (const auto &component : components) {
		if (!component.object.is_valid()) {
			return quader::foundation::Result<void, DocumentError>::failure(
					make_document_error(DocumentErrorCode::InvalidObject,
							"selection contains an invalid object id"));
		}

		if (!component_id_is_valid(component)) {
			return quader::foundation::Result<void, DocumentError>::failure(
					invalid_component_error(component_kind(component)));
		}

		if (!component_matches_mode(component, mode)) {
			return quader::foundation::Result<void, DocumentError>::failure(
					make_document_error(DocumentErrorCode::InvalidSelection,
							"component selection contains an id from a different selection mode"));
		}

		targets.push_back(component.object);
	}

	std::sort(targets.begin(), targets.end());
	targets.erase(std::unique(targets.begin(), targets.end()), targets.end());

	std::sort(components.begin(), components.end(), component_less);
	components.erase(std::unique(components.begin(), components.end()), components.end());

	mode_ = mode;
	objects_ = std::move(targets);
	components_ = std::move(components);
	return quader::foundation::Result<void, DocumentError>::success();
}

quader::foundation::Result<void, DocumentError> Selection::set_components(
		std::vector<ComponentRef> components) {
	if (components.empty()) {
		clear();
		return quader::foundation::Result<void, DocumentError>::success();
	}

	const SelectionMode kMode = selection_mode_for_component_kind(component_kind(components.front()));
	return set_component_selection(kMode, {}, std::move(components));
}

std::span<const ObjectId> Selection::selected_objects() const noexcept {
	return objects_;
}

std::span<const ComponentRef> Selection::selected_components() const noexcept {
	return components_;
}

bool Selection::references_object(ObjectId object) const noexcept {
	if (contains_object(object)) {
		return true;
	}

	return std::any_of(components_.begin(), components_.end(), [object](const ComponentRef &component) {
		return component.object == object;
	});
}

bool Selection::contains_object(ObjectId object) const noexcept {
	return std::find(objects_.begin(), objects_.end(), object) != objects_.end();
}

bool Selection::contains_component(const ComponentRef &component) const noexcept {
	return std::find(components_.begin(), components_.end(), component) != components_.end();
}

void Selection::remove_object(ObjectId object) {
	objects_.erase(std::remove(objects_.begin(), objects_.end(), object), objects_.end());
	components_.erase(std::remove_if(components_.begin(),
							  components_.end(),
							  [object](const ComponentRef &component) {
								  return component.object == object;
							  }),
			components_.end());
}

} // namespace quader::document
