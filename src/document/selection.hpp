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

#include "document/document_error.hpp"
#include "document/object_id.hpp"
#include "foundation/result.hpp"
#include "mesh/core/mesh_ids.hpp"

#include <span>
#include <variant>
#include <vector>

namespace quader::document {

/**
 * Selection mode used by document, command, tool, and UI layers.
 *
 * Object mode stores whole-object selections. Component modes store object
 * targets plus component references of the matching kind.
 */
enum class SelectionMode {
	Object, ///< Whole mesh object selection.
	Vertex, ///< Vertex component selection.
	Edge,   ///< Edge component selection.
	Face,   ///< Face component selection.
};

/// Mesh component category held by a component selection reference.
enum class ComponentKind {
	Vertex, ///< Vertex component.
	Edge,   ///< Edge component.
	Face,   ///< Face component.
};

/// Reference to one mesh component on a document object.
struct ComponentRef {
	/// Object that owns the component.
	ObjectId object;
	/// Stable mesh component id within `object`.
	std::variant<quader::mesh::VertexId, quader::mesh::EdgeId, quader::mesh::FaceId> component;

	friend bool operator==(const ComponentRef &, const ComponentRef &) = default;
};

/**
 * Return the component kind stored by a component reference.
 *
 * @param ref Component reference to inspect.
 * @return Variant kind represented by `ref`.
 */
[[nodiscard]] ComponentKind component_kind(const ComponentRef &ref) noexcept;
/**
 * Return the component selection mode for a component kind.
 *
 * @param kind Component kind to map.
 * @return Matching selection mode.
 */
[[nodiscard]] SelectionMode selection_mode_for_component_kind(ComponentKind kind) noexcept;
/**
 * Check whether a selection mode stores mesh component references.
 *
 * @param mode Selection mode to inspect.
 * @return True for vertex, edge, and face modes.
 */
[[nodiscard]] bool selection_mode_is_component(SelectionMode mode) noexcept;
/**
 * Check whether the component id itself is non-null.
 *
 * @param ref Component reference to inspect.
 * @return True when the contained mesh id is structurally valid.
 */
[[nodiscard]] bool component_id_is_valid(const ComponentRef &ref);

/**
 * Stores object and component selection state for a document.
 *
 * Object mode treats object ids as selected objects. Component modes treat
 * object ids as active edit targets and component refs as the selected
 * components for the current mode. Setters validate structural id validity,
 * sort inputs, and remove duplicates.
 */
class Selection final {
public:
	/// Return the current selection mode.
	[[nodiscard]] SelectionMode mode() const noexcept;
	/**
	 * Change mode and clear incompatible component references.
	 *
	 * Object ids are preserved as object selection or component edit targets.
	 *
	 * @param mode New selection mode.
	 */
	void set_mode(SelectionMode mode);

	/// Return true when no objects or components are selected or targeted.
	[[nodiscard]] bool empty() const noexcept;
	/// Clear object targets, component references, and restore object mode.
	void clear();

	/**
	 * Replace selection with object ids.
	 *
	 * @param objects Object ids to select.
	 * @return Success, or `InvalidObject` when any id is structurally invalid.
	 */
	[[nodiscard]] quader::foundation::Result<void, DocumentError> set_objects(
			std::vector<ObjectId> objects);
	/**
	 * Replace component-mode selection with object targets and component refs.
	 *
	 * Component refs must match `mode`. Component owners are always retained as
	 * edit targets so component-mode overlays can resolve the owning mesh.
	 *
	 * @param mode Component selection mode to store.
	 * @param targets Object ids used as component edit targets.
	 * @param components Component refs to select.
	 * @return Success, or a document error for structurally invalid ids or
	 * mismatched component kinds.
	 */
	[[nodiscard]] quader::foundation::Result<void, DocumentError> set_component_selection(
			SelectionMode mode,
			std::vector<ObjectId> targets,
			std::vector<ComponentRef> components);
	/**
	 * Replace selection with component references.
	 *
	 * @param components Component references to select.
	 * @return Success, or a document error for any structurally invalid id or
	 * mixed component kinds.
	 */
	[[nodiscard]] quader::foundation::Result<void, DocumentError> set_components(
			std::vector<ComponentRef> components);

	/// Borrow selected object ids or component edit targets until the next mutation.
	[[nodiscard]] std::span<const ObjectId> selected_objects() const noexcept;
	/// Borrow selected component refs until the next mutation of this selection.
	[[nodiscard]] std::span<const ComponentRef> selected_components() const noexcept;
	/// Return true when object or component selection references `object`.
	[[nodiscard]] bool references_object(ObjectId object) const noexcept;
	/// Return true when object ids contain `object`.
	[[nodiscard]] bool contains_object(ObjectId object) const noexcept;
	/// Return true when component refs contain `component`.
	[[nodiscard]] bool contains_component(const ComponentRef &component) const noexcept;

	/// Remove selected object refs and component refs owned by `object`.
	void remove_object(ObjectId object);

	friend bool operator==(const Selection &, const Selection &) = default;

private:
	SelectionMode mode_ = SelectionMode::Object;
	std::vector<ObjectId> objects_;
	std::vector<ComponentRef> components_;
};

} // namespace quader::document
