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

#include "foundation/assert.hpp"
#include "math/vec3.hpp"
#include "mesh/core/mesh_ids.hpp"
#include "mesh/core/polyhedron.hpp"

#include <array>
#include <utility>

namespace quader::tests::mesh_fixtures {

/// Mesh fixture containing one triangular face.
struct SingleTriangleMesh {
	/// Mesh containing the triangle.
	quader::mesh::Polyhedron mesh;
	/// Vertex ids in creation order.
	std::array<quader::mesh::VertexId, 3> vertices;
	/// Face id for the triangle.
	quader::mesh::FaceId face;
};

/**
 * Create a single-triangle mesh fixture.
 *
 * @return Mesh with three vertices and one triangular face.
 */
inline SingleTriangleMesh make_single_triangle() {
	quader::mesh::Polyhedron mesh;
	std::array<quader::mesh::VertexId, 3> vertices{
		mesh.create_vertex(quader::math::Vec3{ 0.0F, 0.0F, 0.0F }),
		mesh.create_vertex(quader::math::Vec3{ 1.0F, 0.0F, 0.0F }),
		mesh.create_vertex(quader::math::Vec3{ 0.0F, 1.0F, 0.0F }),
	};

	auto face = mesh.create_face(vertices);
	QUADER_ASSERT(face);

	return SingleTriangleMesh{ std::move(mesh), vertices, face.value() };
}

} // namespace quader::tests::mesh_fixtures
