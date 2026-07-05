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

struct MeshCorruptionAccess {
	static void set_vertex_position_unchecked(quader::mesh::Polyhedron &mesh,
			quader::mesh::VertexId vertex,
			quader::math::Vec3 position) {
		mesh.test_corrupt_vertex_position(vertex, position);
	}

	static void remove_vertex_backend_handle(quader::mesh::Polyhedron &mesh,
			quader::mesh::VertexId vertex) {
		mesh.test_corrupt_vertex_backend_handle(vertex);
	}

	static void break_halfedge_origin_mapping(quader::mesh::Polyhedron &mesh,
			quader::mesh::HalfedgeId halfedge) {
		mesh.test_corrupt_halfedge_origin_mapping(halfedge);
	}

	static void break_halfedge_reverse_mapping(quader::mesh::Polyhedron &mesh,
			quader::mesh::HalfedgeId halfedge) {
		mesh.test_corrupt_halfedge_reverse_mapping(halfedge);
	}

	static void remove_face_backend_handle(quader::mesh::Polyhedron &mesh,
			quader::mesh::FaceId face) {
		mesh.test_corrupt_face_backend_handle(face);
	}
};

inline quader::mesh::Polyhedron make_invalid_triangle_with_nonfinite_vertex_position(
		quader::math::Vec3 position) {
	auto fixture = make_single_triangle();
	MeshCorruptionAccess::set_vertex_position_unchecked(fixture.mesh, fixture.vertices[0], position);
	return std::move(fixture.mesh);
}

} // namespace quader::tests::mesh_fixtures
