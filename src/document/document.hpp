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
#include "document/document_events.hpp"
#include "document/object_store.hpp"
#include "document/scene_object.hpp"
#include "document/selection.hpp"
#include "document/transform.hpp"
#include "foundation/result.hpp"

#include <string>
#include <vector>

namespace quader::document {

/**
 * Owns editable scene objects, selection, dirty state, and document events.
 *
 * All mutations validate inputs, update dirty flags, and enqueue document
 * changes for UI/render adapters to drain.
 */
class Document final {
public:
	/**
	 * Create a validated mesh object.
	 *
	 * @param name Object name.
	 * @param mesh Mesh payload to move into the document.
	 * @param transform Initial object transform.
	 * @param material Initial material values.
	 * @return Created object id, or a validation/store error.
	 */
	[[nodiscard]] quader::foundation::Result<ObjectId, DocumentError> create_mesh_object(
			std::string name,
			quader::mesh::Polyhedron mesh,
			Transform transform = {},
			PbrMaterial material = default_box_material());
	/**
	 * Restore a previously extracted mesh object.
	 *
	 * @param object Object snapshot to restore with its original id.
	 * @return Success, or a validation/store error.
	 */
	[[nodiscard]] quader::foundation::Result<void, DocumentError> restore_mesh_object(
			MeshObject object);
	/**
	 * Delete an object from the document.
	 *
	 * @param id Object id to delete.
	 * @return Success, or `InvalidObject` when `id` is not live.
	 */
	[[nodiscard]] quader::foundation::Result<void, DocumentError> delete_object(ObjectId id);
	/**
	 * Remove an object and return its payload for undo storage.
	 *
	 * Selection references to the object are pruned.
	 *
	 * @param id Object id to extract.
	 * @return Removed object, or `InvalidObject` when `id` is not live.
	 */
	[[nodiscard]] quader::foundation::Result<MeshObject, DocumentError> extract_mesh_object(
			ObjectId id);
	/**
	 * Rename a live object.
	 *
	 * @param id Object id to rename.
	 * @param name New object name.
	 * @return Success, or `InvalidObject` when `id` is not live.
	 */
	[[nodiscard]] quader::foundation::Result<void, DocumentError> rename_object(ObjectId id,
			std::string name);
	/**
	 * Replace a live object's transform.
	 *
	 * @param id Object id to mutate.
	 * @param transform Finite transform to store.
	 * @return Success, `InvalidObject`, or `InvalidTransform`.
	 */
	[[nodiscard]] quader::foundation::Result<void, DocumentError> set_transform(ObjectId id,
			Transform transform);

	/// Return a mutable mesh object pointer, or `nullptr` for invalid/stale ids.
	[[nodiscard]] MeshObject *find_mesh_object(ObjectId id) noexcept;
	/// Return a const mesh object pointer, or `nullptr` for invalid/stale ids.
	[[nodiscard]] const MeshObject *find_mesh_object(ObjectId id) const noexcept;
	/**
	 * Validate that an object id resolves in this document.
	 *
	 * @param id Object id to validate.
	 * @return Success, or `InvalidObject`.
	 */
	[[nodiscard]] quader::foundation::Result<void, DocumentError> validate_object_id(
			ObjectId id) const;
	/**
	 * Validate that a component reference resolves in this document.
	 *
	 * @param component Component reference to validate.
	 * @return Success, or the matching object/component error.
	 */
	[[nodiscard]] quader::foundation::Result<void, DocumentError> validate_component_ref(
			const ComponentRef &component) const;

	/// Return the current document selection.
	[[nodiscard]] const Selection &selection() const noexcept;
	/**
	 * Replace the document selection after resolving all ids.
	 *
	 * @param selection Selection to store.
	 * @return Success, or a validation error for unresolved ids.
	 */
	[[nodiscard]] quader::foundation::Result<void, DocumentError> set_selection(
			Selection selection);
	/// Clear selection and emit a selection change only when it was non-empty.
	void clear_selection();

	/// Mark mesh topology dirty for a live object.
	[[nodiscard]] quader::foundation::Result<void, DocumentError> mark_mesh_topology_changed(
			ObjectId id);
	/// Mark mesh geometry dirty for a live object.
	[[nodiscard]] quader::foundation::Result<void, DocumentError> mark_mesh_geometry_changed(
			ObjectId id);

	/// Return true when any dirty flag is set.
	[[nodiscard]] bool is_dirty() const noexcept;
	/// Return the current dirty flag bitmask.
	[[nodiscard]] DocumentDirtyFlags dirty_flags() const noexcept;
	/// Return true when `flag` is present in the current dirty bitmask.
	[[nodiscard]] bool has_dirty_flag(DocumentDirtyFlag flag) const noexcept;
	/// Clear all dirty flags and emit a dirty-state change when needed.
	void clear_dirty();

	/// Move out and clear queued document changes.
	[[nodiscard]] std::vector<DocumentChange> take_pending_changes();
	/// Return the number of live objects.
	[[nodiscard]] std::size_t object_count() const noexcept;
	/// Return ids for all live objects in document slot order.
	[[nodiscard]] std::vector<ObjectId> object_ids() const;

private:
	[[nodiscard]] quader::foundation::Result<void, DocumentError> validate_mesh_object(
			const MeshObject &object) const;
	[[nodiscard]] quader::foundation::Result<void, DocumentError> validate_mesh(
			const quader::mesh::Polyhedron &mesh) const;
	[[nodiscard]] quader::foundation::Result<void, DocumentError> validate_selection(
			const Selection &selection) const;

	void emit_change(DocumentChangeKind kind,
			ObjectId object = ObjectId::invalid(),
			DocumentDirtyFlags flags = kDocumentDirtyNone);
	void mark_dirty(DocumentDirtyFlags flags);

	ObjectStore objects_;
	Selection selection_;
	std::vector<DocumentChange> pending_changes_;
	DocumentDirtyFlags dirty_flags_ = kDocumentDirtyNone;
};

} // namespace quader::document
