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

#include "crimson/renderer_types.hpp"
#include "math/aabb.hpp"

#include <array>
#include <cstdint>

namespace crimson {

/// Stable renderer object identifier used for drawing and picking.
using RenderObjectId = std::uint64_t;

/**
 * Create a column-major identity transform.
 *
 * @return Sixteen floats representing an identity matrix.
 */
[[nodiscard]] constexpr std::array<float, 16> identity_transform() noexcept {
	return {
		1.0F,
		0.0F,
		0.0F,
		0.0F,
		0.0F,
		1.0F,
		0.0F,
		0.0F,
		0.0F,
		0.0F,
		1.0F,
		0.0F,
		0.0F,
		0.0F,
		0.0F,
		1.0F,
	};
}

/// CPU-side object submitted to the render world.
struct RenderObject {
	/// Stable renderer object id; zero is reserved for unset objects.
	RenderObjectId object_id = 0;
	/// Mesh resource used by this object.
	RenderMeshHandle mesh;
	/// Material resource used by this object.
	RenderMaterialHandle material;
	/// Base shader schema used for material interpretation.
	BaseShaderId base_shader = BaseShaderId::OpaquePbr;
	/// Object-to-world transform matrix.
	std::array<float, 16> world_from_object = identity_transform();
	/// World-space bounds used for culling and sorting.
	quader::math::Aabb world_bounds;
	/// Render queue used for packet routing.
	RenderQueue queue = RenderQueue::Opaque;
	/// Submesh index inside the mesh resource.
	std::uint32_t submesh_index = 0;
	/// Whether the object participates in rendering.
	bool visible = true;
	/// Whether the object participates in picking.
	bool pickable = true;
};

} // namespace crimson
