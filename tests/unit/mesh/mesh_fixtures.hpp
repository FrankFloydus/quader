#pragma once

#include "foundation/assert.hpp"
#include "math/vec3.hpp"
#include "mesh/core/mesh_ids.hpp"
#include "mesh/core/polyhedron.hpp"

#include <array>
#include <utility>

namespace quader::tests::mesh_fixtures {

struct SingleTriangleMesh {
	quader::mesh::Polyhedron mesh;
	std::array<quader::mesh::VertexId, 3> vertices;
	quader::mesh::FaceId face;
};

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
