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

#include "foundation/result.hpp"
#include "math/aabb.hpp"
#include "math/vec3.hpp"
#include "mesh/core/mesh_error.hpp"
#include "mesh/core/polyhedron.hpp"

#include <array>

namespace quader::mesh::ops {

/// Axis-aligned box bounds used by the mesh box builder.
struct BoxDimensions {
	/// Minimum corner before normalization.
	quader::math::Vec3 min;
	/// Maximum corner before normalization.
	quader::math::Vec3 max;
};

/// Oriented box corner set consumed by the mesh box builder.
struct BoxCorners {
	/// Eight box corners in the builder's stable face-index order.
	std::array<quader::math::Vec3, 8> points;
};

/**
 * Build a closed six-face polyhedron from axis-aligned bounds.
 *
 * @param bounds Minimum and maximum bounds; reversed axes are normalized.
 * @return Valid box mesh, or a mesh error for non-finite or degenerate bounds.
 */
[[nodiscard]] quader::foundation::Result<quader::mesh::Polyhedron, quader::mesh::MeshError>
make_box_from_bounds(BoxDimensions bounds);

/**
 * Build a closed six-face polyhedron from an axis-aligned bounding box.
 *
 * @param bounds Bounding box to convert into a mesh.
 * @return Valid box mesh, or a mesh error for non-finite or degenerate bounds.
 */
[[nodiscard]] quader::foundation::Result<quader::mesh::Polyhedron, quader::mesh::MeshError>
make_box_from_bounds(quader::math::Aabb bounds);

/**
 * Build a closed six-face polyhedron from oriented box corners.
 *
 * @param corners Eight finite, non-degenerate corners.
 * @return Valid box mesh, or a mesh error when any face degenerates.
 */
[[nodiscard]] quader::foundation::Result<quader::mesh::Polyhedron, quader::mesh::MeshError>
make_box_from_corners(const BoxCorners &corners);

} // namespace quader::mesh::ops
