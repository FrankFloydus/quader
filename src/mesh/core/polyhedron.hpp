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
/**
 * Validate mesh topology, mappings, attributes, and geometry invariants.
 *
 * @param mesh Mesh to inspect.
 * @return Validation report containing every issue detected by the pass.
 */
[[nodiscard]] MeshValidationReport validate_mesh(const Polyhedron &mesh);

/// Counts returned after storage compaction.
struct MeshCompactionReport {
	/// Number of deleted vertex slots removed.
	std::size_t removed_vertices = 0;
	/// Number of deleted halfedge slots removed.
	std::size_t removed_halfedges = 0;
	/// Number of deleted edge slots removed.
	std::size_t removed_edges = 0;
	/// Number of deleted face slots removed.
	std::size_t removed_faces = 0;
	/// Number of live vertex slots after compaction.
	std::size_t live_vertices = 0;
	/// Number of live halfedge slots after compaction.
	std::size_t live_halfedges = 0;
	/// Number of live edge slots after compaction.
	std::size_t live_edges = 0;
	/// Number of live face slots after compaction.
	std::size_t live_faces = 0;
};

/**
 * Editable polygon mesh with stable generational element IDs.
 *
 * `Polyhedron` owns topology, vertex positions, and mesh attributes. Expected
 * failures are returned as `Result` errors; invalid identifiers do not throw.
 */
class Polyhedron final {
public:
	/// Create an empty mesh.
	Polyhedron();
	/// Destroy the mesh and its backend storage.
	~Polyhedron();

	/// Deep-copy mesh topology, positions, mappings, and attributes.
	Polyhedron(const Polyhedron &other);
	/// Deep-copy mesh topology, positions, mappings, and attributes.
	Polyhedron &operator=(const Polyhedron &other);
	/// Move mesh storage and attributes.
	Polyhedron(Polyhedron &&) noexcept;
	/// Move mesh storage and attributes.
	Polyhedron &operator=(Polyhedron &&) noexcept;

	/**
	 * Create a vertex.
	 *
	 * @param position Vertex position.
	 * @return Stable vertex identifier.
	 */
	[[nodiscard]] VertexId create_vertex(quader::math::Vec3 position);
	/**
	 * Delete an unreferenced vertex.
	 *
	 * @param vertex Vertex to delete.
	 * @return Success, or an error when the vertex is invalid or referenced.
	 */
	[[nodiscard]] quader::foundation::Result<void, MeshError> delete_vertex(VertexId vertex);
	/**
	 * Create a face from existing vertices.
	 *
	 * @param vertices Face vertices in winding order.
	 * @return New face id, or a mesh error.
	 */
	[[nodiscard]] quader::foundation::Result<FaceId, MeshError> create_face(std::span<const VertexId> vertices);
	/**
	 * Delete a face and its face-owned topology.
	 *
	 * @param face Face to delete.
	 * @return Success, or an error when the face is invalid.
	 */
	[[nodiscard]] quader::foundation::Result<void, MeshError> delete_face(FaceId face);
	/**
	 * Reverse one face winding while preserving stable mesh IDs.
	 *
	 * @param face Face whose vertex cycle should be reversed.
	 * @return Success, or an error when the face is invalid or the backend
	 * cannot represent the reversed topology.
	 */
	[[nodiscard]] quader::foundation::Result<void, MeshError> reverse_face_winding(FaceId face);
	/**
	 * Reverse multiple face windings as one topology rebuild.
	 *
	 * The operation is all-or-nothing: invalid face ids or backend rejection
	 * leave the mesh unchanged.
	 *
	 * @param faces Faces whose vertex cycles should be reversed.
	 * @return Success, or an error when no faces are provided, any face is
	 * invalid, or the backend cannot represent the reversed topology.
	 */
	[[nodiscard]] quader::foundation::Result<void, MeshError> reverse_face_windings(
			std::span<const FaceId> faces);

	/**
	 * Check whether a vertex id resolves to a live vertex.
	 *
	 * @param id Vertex id to test.
	 * @return True when the id is live and generation-matched.
	 */
	[[nodiscard]] bool is_valid(VertexId id) const noexcept;
	/**
	 * Check whether a halfedge id resolves to a live halfedge.
	 *
	 * @param id Halfedge id to test.
	 * @return True when the id is live and generation-matched.
	 */
	[[nodiscard]] bool is_valid(HalfedgeId id) const noexcept;
	/**
	 * Check whether an edge id resolves to a live edge.
	 *
	 * @param id Edge id to test.
	 * @return True when the id is live and generation-matched.
	 */
	[[nodiscard]] bool is_valid(EdgeId id) const noexcept;
	/**
	 * Check whether a face id resolves to a live face.
	 *
	 * @param id Face id to test.
	 * @return True when the id is live and generation-matched.
	 */
	[[nodiscard]] bool is_valid(FaceId id) const noexcept;

	/// Return the number of live vertices.
	[[nodiscard]] std::size_t vertex_count() const noexcept;
	/// Return the number of live halfedges.
	[[nodiscard]] std::size_t halfedge_count() const noexcept;
	/// Return the number of live edges.
	[[nodiscard]] std::size_t edge_count() const noexcept;
	/// Return the number of live faces.
	[[nodiscard]] std::size_t face_count() const noexcept;

	/// Return all live vertex IDs.
	[[nodiscard]] std::vector<VertexId> vertex_ids() const;
	/// Return all live halfedge IDs.
	[[nodiscard]] std::vector<HalfedgeId> halfedge_ids() const;
	/// Return all live edge IDs.
	[[nodiscard]] std::vector<EdgeId> edge_ids() const;
	/// Return all live face IDs.
	[[nodiscard]] std::vector<FaceId> face_ids() const;

	/**
	 * Return a vertex position.
	 *
	 * @param vertex Vertex to query.
	 * @return Position, or an invalid-vertex error.
	 */
	[[nodiscard]] quader::foundation::Result<quader::math::Vec3, MeshError> vertex_position(VertexId vertex) const;
	/**
	 * Set a vertex position.
	 *
	 * @param vertex Vertex to mutate.
	 * @param position New position.
	 * @return Success, or an invalid-vertex error.
	 */
	[[nodiscard]] quader::foundation::Result<void, MeshError> set_vertex_position(VertexId vertex,
			quader::math::Vec3 position);

	/// Return one outgoing halfedge for a vertex.
	[[nodiscard]] quader::foundation::Result<HalfedgeId, MeshError> vertex_outgoing_halfedge(VertexId vertex) const;
	/// Return the origin vertex of a halfedge.
	[[nodiscard]] quader::foundation::Result<VertexId, MeshError> halfedge_origin(HalfedgeId halfedge) const;
	/// Return the target vertex of a halfedge.
	[[nodiscard]] quader::foundation::Result<VertexId, MeshError> halfedge_target(HalfedgeId halfedge) const;
	/// Return the opposite halfedge.
	[[nodiscard]] quader::foundation::Result<HalfedgeId, MeshError> opposite_halfedge(HalfedgeId halfedge) const;
	/// Return the next halfedge in a cycle.
	[[nodiscard]] quader::foundation::Result<HalfedgeId, MeshError> next_halfedge(HalfedgeId halfedge) const;
	/// Return the previous halfedge in a cycle.
	[[nodiscard]] quader::foundation::Result<HalfedgeId, MeshError> previous_halfedge(HalfedgeId halfedge) const;
	/// Return the undirected edge owning a halfedge.
	[[nodiscard]] quader::foundation::Result<EdgeId, MeshError> halfedge_edge(HalfedgeId halfedge) const;
	/// Return the face associated with a halfedge.
	[[nodiscard]] quader::foundation::Result<FaceId, MeshError> halfedge_face(HalfedgeId halfedge) const;
	/// Check whether a halfedge belongs to a boundary cycle.
	[[nodiscard]] quader::foundation::Result<bool, MeshError> is_boundary_halfedge(HalfedgeId halfedge) const;

	/// Return the halfedge cycle for a face.
	[[nodiscard]] quader::foundation::Result<std::vector<HalfedgeId>, MeshError> face_halfedges(FaceId face) const;
	/// Return the two halfedges associated with an edge.
	[[nodiscard]] quader::foundation::Result<std::array<HalfedgeId, 2>, MeshError> edge_halfedges(EdgeId edge) const;

	/**
	 * Remove deleted storage slots and rebuild ID mappings.
	 *
	 * @return Compaction counts, or an error when mappings cannot be preserved.
	 */
	[[nodiscard]] quader::foundation::Result<MeshCompactionReport, MeshError> compact_storage();

	/**
	 * Access mutable mesh attributes.
	 *
	 * @return Attribute container owned by this mesh.
	 */
	[[nodiscard]] MeshAttributes &attributes() noexcept;
	/**
	 * Access immutable mesh attributes.
	 *
	 * @return Attribute container owned by this mesh.
	 */
	[[nodiscard]] const MeshAttributes &attributes() const noexcept;

	/**
	 * Access a mutable vertex attribute value.
	 *
	 * @tparam T Expected attribute value type.
	 * @param attribute Attribute id to resolve.
	 * @param vertex Vertex whose slot is accessed.
	 * @return Pointer valid until attribute storage mutation, or a mesh error.
	 */
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

	/**
	 * Access an immutable vertex attribute value.
	 *
	 * @tparam T Expected attribute value type.
	 * @param attribute Attribute id to resolve.
	 * @param vertex Vertex whose slot is accessed.
	 * @return Pointer valid until attribute storage mutation, or a mesh error.
	 */
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
