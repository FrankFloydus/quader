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
#include "document/scene_object.hpp"
#include "foundation/result.hpp"

#include <cstddef>
#include <vector>

namespace quader::document {

/**
 * Owns mesh object slots and stable object id generation.
 *
 * Removed slots are reused with incremented generations so stale `ObjectId`
 * values stop resolving after deletion.
 */
class ObjectStore final {
public:
	/**
	 * Create a mesh object in a live slot.
	 *
	 * @param name Object name to store.
	 * @param mesh Mesh payload to move into the object.
	 * @param transform Object transform.
	 * @param material Object material.
	 * @return New object id, or an object-store error.
	 */
	[[nodiscard]] quader::foundation::Result<ObjectId, DocumentError> create_mesh_object(
			std::string name,
			quader::mesh::Polyhedron mesh,
			Transform transform,
			PbrMaterial material);

	/**
	 * Restore a previously removed mesh object into its original slot.
	 *
	 * @param object Object snapshot with a valid, currently free id.
	 * @return Success, or an error when the slot is unavailable.
	 */
	[[nodiscard]] quader::foundation::Result<void, DocumentError> restore_mesh_object(
			MeshObject object);
	/**
	 * Remove a mesh object and return its owned payload.
	 *
	 * @param id Live object id to remove.
	 * @return Removed object, or `InvalidObject` when `id` is not live.
	 */
	[[nodiscard]] quader::foundation::Result<MeshObject, DocumentError> remove_mesh_object(
			ObjectId id);

	/// Return a mutable object pointer, or `nullptr` for invalid or stale ids.
	[[nodiscard]] MeshObject *find_mesh_object(ObjectId id) noexcept;
	/// Return a const object pointer, or `nullptr` for invalid or stale ids.
	[[nodiscard]] const MeshObject *find_mesh_object(ObjectId id) const noexcept;
	/// Return true when `id` resolves to a live object.
	[[nodiscard]] bool contains(ObjectId id) const noexcept;

	/// Return ids for all live objects in slot order.
	[[nodiscard]] std::vector<ObjectId> object_ids() const;
	/// Return the number of live objects.
	[[nodiscard]] std::size_t size() const noexcept;

private:
	struct ObjectSlot {
		quader::foundation::IdGeneration generation = 1U;
		bool alive = false;
		MeshObject object;
	};

	[[nodiscard]] quader::foundation::IdIndex allocate_slot();
	[[nodiscard]] static quader::foundation::IdGeneration next_generation(
			quader::foundation::IdGeneration generation) noexcept;

	std::vector<ObjectSlot> slots_;
	std::vector<quader::foundation::IdIndex> free_slots_;
};

} // namespace quader::document
