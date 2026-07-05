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

#ifndef QUADER_MESH_CORE_TEST_HOOKS
#error "mesh_corruption_fixtures.hpp requires QUADER_MESH_CORE_TEST_HOOKS on the including test target."
#endif

#include "math/vec3.hpp"
#include "mesh/core/mesh_ids.hpp"
#include "mesh/core/polyhedron.hpp"
#include "mesh_fixtures.hpp"

#include <utility>

namespace quader::tests::mesh_fixtures {

/// Test-only accessors for intentionally corrupting mesh internals.
struct MeshCorruptionAccess {
	/**
	 * Replace a vertex position without validation.
	 *
	 * @param mesh Mesh to corrupt.
	 * @param vertex Vertex to mutate.
	 * @param position Position written directly into the test hook.
	 */
	static void set_vertex_position_unchecked(quader::mesh::Polyhedron &mesh,
			quader::mesh::VertexId vertex,
			quader::math::Vec3 position) {
		mesh.test_corrupt_vertex_position(vertex, position);
	}

	/**
	 * Remove the backend handle for a vertex.
	 *
	 * @param mesh Mesh to corrupt.
	 * @param vertex Vertex whose backend handle is removed.
	 */
	static void remove_vertex_backend_handle(quader::mesh::Polyhedron &mesh,
			quader::mesh::VertexId vertex) {
		mesh.test_corrupt_vertex_backend_handle(vertex);
	}

	/**
	 * Break the stored origin mapping for a halfedge.
	 *
	 * @param mesh Mesh to corrupt.
	 * @param halfedge Halfedge whose origin mapping is corrupted.
	 */
	static void break_halfedge_origin_mapping(quader::mesh::Polyhedron &mesh,
			quader::mesh::HalfedgeId halfedge) {
		mesh.test_corrupt_halfedge_origin_mapping(halfedge);
	}

	/**
	 * Break the reverse mapping for a halfedge.
	 *
	 * @param mesh Mesh to corrupt.
	 * @param halfedge Halfedge whose reverse mapping is corrupted.
	 */
	static void break_halfedge_reverse_mapping(quader::mesh::Polyhedron &mesh,
			quader::mesh::HalfedgeId halfedge) {
		mesh.test_corrupt_halfedge_reverse_mapping(halfedge);
	}

	/**
	 * Remove the backend handle for a face.
	 *
	 * @param mesh Mesh to corrupt.
	 * @param face Face whose backend handle is removed.
	 */
	static void remove_face_backend_handle(quader::mesh::Polyhedron &mesh,
			quader::mesh::FaceId face) {
		mesh.test_corrupt_face_backend_handle(face);
	}
};

/**
 * Create an invalid triangle fixture with a corrupted vertex position.
 *
 * @param position Position written to the first vertex through test hooks.
 * @return Mesh containing the intentionally invalid vertex position.
 */
inline quader::mesh::Polyhedron make_invalid_triangle_with_nonfinite_vertex_position(
		quader::math::Vec3 position) {
	auto fixture = make_single_triangle();
	MeshCorruptionAccess::set_vertex_position_unchecked(fixture.mesh, fixture.vertices[0], position);
	return std::move(fixture.mesh);
}

} // namespace quader::tests::mesh_fixtures
