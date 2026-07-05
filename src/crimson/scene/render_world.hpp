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

#include "crimson/scene/render_object.hpp"

#include <span>
#include <vector>

namespace crimson {

/// Mutable CPU-side collection of renderer objects.
class RenderWorld final {
public:
	/**
	 * Add an object and assign a renderer object id.
	 *
	 * @param object Object payload to store.
	 * @return Assigned object id.
	 */
	RenderObjectId add_object(RenderObject object);
	/**
	 * Replace an existing object payload.
	 *
	 * @param id Object id to update.
	 * @param object Replacement payload.
	 * @return True when `id` was present.
	 */
	[[nodiscard]] bool update_object(RenderObjectId id, RenderObject object);
	/**
	 * Remove an object by id.
	 *
	 * @param id Object id to remove.
	 * @return True when `id` was present.
	 */
	[[nodiscard]] bool remove_object(RenderObjectId id);

	/**
	 * Return all stored objects.
	 *
	 * @return Borrowed span valid until the world is mutated.
	 */
	[[nodiscard]] std::span<const RenderObject> objects() const noexcept;
	/// Remove all objects and reset the next object id.
	void clear() noexcept;

private:
	std::vector<RenderObject> objects_;
	RenderObjectId next_object_id_ = 1;
};

} // namespace crimson
