#pragma once

#include "foundation/result.hpp"
#include "math/vec3.hpp"
#include "mesh/core/mesh_attributes.hpp"
#include "mesh/core/mesh_error.hpp"
#include "mesh/core/mesh_ids.hpp"

#include <array>
#include <cstddef>
#include <memory>
#include <span>
#include <vector>

#ifdef QUADER_MESH_CORE_TEST_HOOKS
namespace quader::tests::mesh_fixtures {
struct MeshCorruptionAccess;
} // namespace quader::tests::mesh_fixtures
#endif

namespace quader::mesh {

namespace detail {
class OpenMeshStorage;
} // namespace detail

class Polyhedron;
class MeshValidationReport;
[[nodiscard]] MeshValidationReport validate_mesh(const Polyhedron &mesh);

struct MeshCompactionReport {
	std::size_t removed_vertices = 0;
	std::size_t removed_halfedges = 0;
	std::size_t removed_edges = 0;
	std::size_t removed_faces = 0;
	std::size_t live_vertices = 0;
	std::size_t live_halfedges = 0;
	std::size_t live_edges = 0;
	std::size_t live_faces = 0;
};

class Polyhedron final {
public:
	Polyhedron();
	~Polyhedron();

	Polyhedron(const Polyhedron &other);
	Polyhedron &operator=(const Polyhedron &other);
	Polyhedron(Polyhedron &&) noexcept;
	Polyhedron &operator=(Polyhedron &&) noexcept;

	[[nodiscard]] VertexId create_vertex(quader::math::Vec3 position);
	[[nodiscard]] quader::foundation::Result<void, MeshError> delete_vertex(VertexId vertex);
	[[nodiscard]] quader::foundation::Result<FaceId, MeshError> create_face(std::span<const VertexId> vertices);
	[[nodiscard]] quader::foundation::Result<void, MeshError> delete_face(FaceId face);

	[[nodiscard]] bool is_valid(VertexId id) const noexcept;
	[[nodiscard]] bool is_valid(HalfedgeId id) const noexcept;
	[[nodiscard]] bool is_valid(EdgeId id) const noexcept;
	[[nodiscard]] bool is_valid(FaceId id) const noexcept;

	[[nodiscard]] std::size_t vertex_count() const noexcept;
	[[nodiscard]] std::size_t halfedge_count() const noexcept;
	[[nodiscard]] std::size_t edge_count() const noexcept;
	[[nodiscard]] std::size_t face_count() const noexcept;

	[[nodiscard]] std::vector<VertexId> vertex_ids() const;
	[[nodiscard]] std::vector<HalfedgeId> halfedge_ids() const;
	[[nodiscard]] std::vector<EdgeId> edge_ids() const;
	[[nodiscard]] std::vector<FaceId> face_ids() const;

	[[nodiscard]] quader::foundation::Result<quader::math::Vec3, MeshError> vertex_position(VertexId vertex) const;
	[[nodiscard]] quader::foundation::Result<void, MeshError> set_vertex_position(VertexId vertex,
			quader::math::Vec3 position);

	[[nodiscard]] quader::foundation::Result<HalfedgeId, MeshError> vertex_outgoing_halfedge(VertexId vertex) const;
	[[nodiscard]] quader::foundation::Result<VertexId, MeshError> halfedge_origin(HalfedgeId halfedge) const;
	[[nodiscard]] quader::foundation::Result<VertexId, MeshError> halfedge_target(HalfedgeId halfedge) const;
	[[nodiscard]] quader::foundation::Result<HalfedgeId, MeshError> opposite_halfedge(HalfedgeId halfedge) const;
	[[nodiscard]] quader::foundation::Result<HalfedgeId, MeshError> next_halfedge(HalfedgeId halfedge) const;
	[[nodiscard]] quader::foundation::Result<HalfedgeId, MeshError> previous_halfedge(HalfedgeId halfedge) const;
	[[nodiscard]] quader::foundation::Result<EdgeId, MeshError> halfedge_edge(HalfedgeId halfedge) const;
	[[nodiscard]] quader::foundation::Result<FaceId, MeshError> halfedge_face(HalfedgeId halfedge) const;
	[[nodiscard]] quader::foundation::Result<bool, MeshError> is_boundary_halfedge(HalfedgeId halfedge) const;

	[[nodiscard]] quader::foundation::Result<std::vector<HalfedgeId>, MeshError> face_halfedges(FaceId face) const;
	[[nodiscard]] quader::foundation::Result<std::array<HalfedgeId, 2>, MeshError> edge_halfedges(EdgeId edge) const;

	[[nodiscard]] quader::foundation::Result<MeshCompactionReport, MeshError> compact_storage();

	[[nodiscard]] MeshAttributes &attributes() noexcept;
	[[nodiscard]] const MeshAttributes &attributes() const noexcept;

	template <class T>
	[[nodiscard]] quader::foundation::Result<T *, MeshError> vertex_attribute(AttributeId attribute,
			VertexId vertex) {
		if (!is_valid(vertex)) {
			return quader::foundation::Result<T *, MeshError>::failure(
					make_mesh_error(MeshErrorCode::InvalidVertex,
							"cannot access a vertex attribute through an invalid vertex id"));
		}

		return attributes_.value<T>(attribute, vertex.index());
	}

	template <class T>
	[[nodiscard]] quader::foundation::Result<const T *, MeshError> vertex_attribute(AttributeId attribute,
			VertexId vertex) const {
		if (!is_valid(vertex)) {
			return quader::foundation::Result<const T *, MeshError>::failure(
					make_mesh_error(MeshErrorCode::InvalidVertex,
							"cannot access a vertex attribute through an invalid vertex id"));
		}

		return attributes_.value<T>(attribute, vertex.index());
	}

private:
	friend MeshValidationReport validate_mesh(const Polyhedron &mesh);

#ifdef QUADER_MESH_CORE_TEST_HOOKS
	friend struct ::quader::tests::mesh_fixtures::MeshCorruptionAccess;
#endif

	void test_corrupt_vertex_position(VertexId vertex, quader::math::Vec3 position);
	void test_corrupt_vertex_backend_handle(VertexId vertex);
	void test_corrupt_halfedge_origin_mapping(HalfedgeId halfedge);
	void test_corrupt_halfedge_reverse_mapping(HalfedgeId halfedge);
	void test_corrupt_face_backend_handle(FaceId face);

	std::unique_ptr<detail::OpenMeshStorage> storage_;
	MeshAttributes attributes_;
};

} // namespace quader::mesh
