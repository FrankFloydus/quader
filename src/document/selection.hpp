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
 * Check whether the component id itself is non-null.
 *
 * @param ref Component reference to inspect.
 * @return True when the contained mesh id is structurally valid.
 */
[[nodiscard]] bool component_id_is_valid(const ComponentRef &ref);

/**
 * Stores either object selection or component selection for a document.
 *
 * Object and component selections are mutually exclusive. Setters validate
 * structural id validity, sort inputs, and remove duplicates.
 */
class Selection final {
public:
	/// Return true when no objects or components are selected.
	[[nodiscard]] bool empty() const noexcept;
	/// Clear object and component selection.
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
	 * Replace selection with component references.
	 *
	 * @param components Component references to select.
	 * @return Success, or a document error for any structurally invalid id.
	 */
	[[nodiscard]] quader::foundation::Result<void, DocumentError> set_components(
			std::vector<ComponentRef> components);

	/// Borrow selected object ids until the next mutation of this selection.
	[[nodiscard]] std::span<const ObjectId> selected_objects() const noexcept;
	/// Borrow selected component refs until the next mutation of this selection.
	[[nodiscard]] std::span<const ComponentRef> selected_components() const noexcept;
	/// Return true when object or component selection references `object`.
	[[nodiscard]] bool references_object(ObjectId object) const noexcept;

	/// Remove selected object refs and component refs owned by `object`.
	void remove_object(ObjectId object);

	friend bool operator==(const Selection &, const Selection &) = default;

private:
	std::vector<ObjectId> objects_;
	std::vector<ComponentRef> components_;
};

} // namespace quader::document
