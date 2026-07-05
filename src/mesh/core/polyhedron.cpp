/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
#include "mesh/core/polyhedron.hpp"

#include "geometry/normals.hpp"
#include "geometry/predicates.hpp"
#include "mesh/core/detail/openmesh_storage.hpp"
#include "mesh/core/mesh_traversal.hpp"
#include "mesh/core/mesh_validation.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <map>
#include <set>
#include <string>
#include <utility>

namespace quader::mesh {
namespace {

constexpr quader::foundation::IdIndex kNoSlot = quader::foundation::kInvalidIdIndex;

[[nodiscard]] quader::foundation::IdGeneration next_generation(
		quader::foundation::IdGeneration generation) noexcept {
	auto next = static_cast<quader::foundation::IdGeneration>(generation + 1U);
	if (next == 0U) {
		next = 1U;
	}
	return next;
}

template <class Handle>
[[nodiscard]] int handle_index(Handle handle) noexcept {
	return handle.is_valid() ? handle.idx() : -1;
}

template <class Handle>
[[nodiscard]] Handle invalid_handle() noexcept {
	return Handle{ -1 };
}

template <class Handle, class SmartHandle>
[[nodiscard]] Handle plain_handle(SmartHandle handle) noexcept {
	return Handle{ handle.idx() };
}

template <class Handle>
void set_reverse(std::vector<quader::foundation::IdIndex> &reverse,
		Handle handle,
		quader::foundation::IdIndex slot) {
	const auto kIndex = handle_index(handle);
	if (kIndex < 0) {
		return;
	}

	const auto kReverseIndex = static_cast<std::size_t>(kIndex);
	if (reverse.size() <= kReverseIndex) {
		reverse.resize(kReverseIndex + 1U, kNoSlot);
	}
	reverse[kReverseIndex] = slot;
}

template <class Handle>
void clear_reverse(std::vector<quader::foundation::IdIndex> &reverse, Handle handle) {
	const auto kIndex = handle_index(handle);
	if (kIndex < 0) {
		return;
	}

	const auto kReverseIndex = static_cast<std::size_t>(kIndex);
	if (kReverseIndex < reverse.size()) {
		reverse[kReverseIndex] = kNoSlot;
	}
}

template <class Handle>
[[nodiscard]] quader::foundation::IdIndex reverse_slot(const std::vector<quader::foundation::IdIndex> &reverse,
		Handle handle) noexcept {
	const auto kIndex = handle_index(handle);
	if (kIndex < 0) {
		return kNoSlot;
	}

	const auto kReverseIndex = static_cast<std::size_t>(kIndex);
	if (kReverseIndex >= reverse.size()) {
		return kNoSlot;
	}

	return reverse[kReverseIndex];
}

[[nodiscard]] detail::QuaderOpenMesh::Point to_openmesh_point(quader::math::Vec3 position) noexcept {
	return detail::QuaderOpenMesh::Point{ position.x, position.y, position.z };
}

[[nodiscard]] quader::math::Vec3 from_openmesh_point(detail::QuaderOpenMesh::Point point) noexcept {
	return quader::math::Vec3{ point[0], point[1], point[2] };
}

template <class Slot>
[[nodiscard]] std::size_t alive_count(const std::vector<Slot> &slots) noexcept {
	return static_cast<std::size_t>(
			std::count_if(slots.begin(), slots.end(), [](const auto &slot) { return slot.alive; }));
}

template <class Id, class Slot>
[[nodiscard]] std::vector<Id> live_ids(const std::vector<Slot> &slots) {
	std::vector<Id> ids;
	ids.reserve(alive_count(slots));
	for (quader::foundation::IdIndex index = 0; index < slots.size(); ++index) {
		const auto &slot = slots[index];
		if (slot.alive) {
			ids.push_back(Id{ index, slot.generation });
		}
	}
	return ids;
}

template <class Handle>
[[nodiscard]] bool live_backend_handle(const detail::QuaderOpenMesh &backend, Handle handle) noexcept {
	return handle.is_valid() && backend.is_valid_handle(handle) && !backend.status(handle).deleted();
}

[[nodiscard]] bool has_duplicate_vertices(std::span<const VertexId> vertices) noexcept {
	for (std::size_t i = 0; i < vertices.size(); ++i) {
		for (std::size_t j = i + 1U; j < vertices.size(); ++j) {
			if (vertices[i] == vertices[j]) {
				return true;
			}
		}
	}
	return false;
}

struct FaceLoopSnapshot final {
	FaceId face;
	std::vector<VertexId> vertices;
};

struct EdgeVertexKey final {
	VertexId a;
	VertexId b;

	friend auto operator<=>(const EdgeVertexKey &, const EdgeVertexKey &) = default;
};

struct DirectedHalfedgeKey final {
	VertexId origin;
	VertexId target;

	friend auto operator<=>(const DirectedHalfedgeKey &, const DirectedHalfedgeKey &) = default;
};

[[nodiscard]] EdgeVertexKey make_edge_vertex_key(VertexId first, VertexId second) noexcept {
	if (second < first) {
		return EdgeVertexKey{ second, first };
	}
	return EdgeVertexKey{ first, second };
}

[[nodiscard]] MeshError backend_mapping_error(std::string message) {
	return make_mesh_error(MeshErrorCode::BackendMappingCorrupt, std::move(message));
}

[[nodiscard]] quader::foundation::Result<std::map<DirectedHalfedgeKey, HalfedgeId>, MeshError>
build_directed_halfedge_map(const Polyhedron &mesh) {
	std::map<DirectedHalfedgeKey, HalfedgeId> result;
	for (const HalfedgeId kHalfedge : mesh.halfedge_ids()) {
		auto origin = mesh.halfedge_origin(kHalfedge);
		if (!origin) {
			return quader::foundation::Result<std::map<DirectedHalfedgeKey, HalfedgeId>, MeshError>::failure(
					std::move(origin).error());
		}
		auto target = mesh.halfedge_target(kHalfedge);
		if (!target) {
			return quader::foundation::Result<std::map<DirectedHalfedgeKey, HalfedgeId>, MeshError>::failure(
					std::move(target).error());
		}

		result.emplace(DirectedHalfedgeKey{ origin.value(), target.value() }, kHalfedge);
	}

	return quader::foundation::Result<std::map<DirectedHalfedgeKey, HalfedgeId>, MeshError>::success(
			std::move(result));
}

[[nodiscard]] quader::foundation::Result<std::map<EdgeVertexKey, EdgeId>, MeshError> build_edge_map(
		const Polyhedron &mesh) {
	std::map<EdgeVertexKey, EdgeId> result;
	for (const EdgeId kEdge : mesh.edge_ids()) {
		auto halfedges = mesh.edge_halfedges(kEdge);
		if (!halfedges) {
			return quader::foundation::Result<std::map<EdgeVertexKey, EdgeId>, MeshError>::failure(
					std::move(halfedges).error());
		}
		auto origin = mesh.halfedge_origin(halfedges.value()[0]);
		if (!origin) {
			return quader::foundation::Result<std::map<EdgeVertexKey, EdgeId>, MeshError>::failure(
					std::move(origin).error());
		}
		auto target = mesh.halfedge_target(halfedges.value()[0]);
		if (!target) {
			return quader::foundation::Result<std::map<EdgeVertexKey, EdgeId>, MeshError>::failure(
					std::move(target).error());
		}

		result.emplace(make_edge_vertex_key(origin.value(), target.value()), kEdge);
	}

	return quader::foundation::Result<std::map<EdgeVertexKey, EdgeId>, MeshError>::success(std::move(result));
}

[[nodiscard]] quader::foundation::Result<std::vector<FaceLoopSnapshot>, MeshError> build_face_loop_snapshots(
		const Polyhedron &mesh,
		const std::vector<FaceId> &faces_to_reverse) {
	std::vector<FaceLoopSnapshot> result;
	result.reserve(mesh.face_count());
	for (const FaceId kFace : mesh.face_ids()) {
		auto vertices = face_vertices(mesh, kFace);
		if (!vertices) {
			return quader::foundation::Result<std::vector<FaceLoopSnapshot>, MeshError>::failure(
					std::move(vertices).error());
		}

		if (std::binary_search(faces_to_reverse.begin(), faces_to_reverse.end(), kFace)) {
			std::reverse(vertices.value().begin(), vertices.value().end());
		}
		auto vertex_loop = std::move(vertices).value();

		result.push_back(FaceLoopSnapshot{
				kFace,
				std::move(vertex_loop),
		});
	}

	return quader::foundation::Result<std::vector<FaceLoopSnapshot>, MeshError>::success(std::move(result));
}

} // namespace

Polyhedron::Polyhedron() : storage_(std::make_unique<detail::OpenMeshStorage>()) {
}

Polyhedron::~Polyhedron() = default;

Polyhedron::Polyhedron(const Polyhedron &other) : storage_(std::make_unique<detail::OpenMeshStorage>(*other.storage_)), attributes_(other.attributes_) {
}

Polyhedron &Polyhedron::operator=(const Polyhedron &other) {
	if (this == &other) {
		return *this;
	}

	storage_ = std::make_unique<detail::OpenMeshStorage>(*other.storage_);
	attributes_ = other.attributes_;
	return *this;
}

Polyhedron::Polyhedron(Polyhedron &&) noexcept = default;
Polyhedron &Polyhedron::operator=(Polyhedron &&) noexcept = default;

VertexId Polyhedron::create_vertex(quader::math::Vec3 position) {
	const auto kHandle =
			plain_handle<detail::QuaderOpenMesh::VertexHandle>(storage_->backend.add_vertex(to_openmesh_point(position)));

	quader::foundation::IdIndex slot_index = 0;
	if (!storage_->free_vertices.empty()) {
		slot_index = storage_->free_vertices.back();
		storage_->free_vertices.pop_back();
		auto &slot = storage_->vertices[slot_index];
		slot.alive = true;
		slot.handle = kHandle;
	} else {
		slot_index = static_cast<quader::foundation::IdIndex>(storage_->vertices.size());
		storage_->vertices.push_back(detail::OpenMeshIdSlot<detail::QuaderOpenMesh::VertexHandle>{});
		auto &slot = storage_->vertices.back();
		slot.alive = true;
		slot.handle = kHandle;
		attributes_.resize_domain(AttributeDomain::Vertex, storage_->vertices.size());
	}

	set_reverse(storage_->vertex_reverse.handle_to_slot, kHandle, slot_index);
	attributes_.reset_slot_to_defaults(AttributeDomain::Vertex, slot_index);
	return VertexId{ slot_index, storage_->vertices[slot_index].generation };
}

quader::foundation::Result<void, MeshError> Polyhedron::delete_vertex(VertexId vertex) {
	if (!is_valid(vertex)) {
		return quader::foundation::Result<void, MeshError>::failure(
				make_mesh_error(MeshErrorCode::InvalidVertex, "cannot delete an invalid vertex id"));
	}

	const auto kHandle = storage_->vertices[vertex.index()].handle;
	if (!storage_->backend.is_isolated(kHandle)) {
		return quader::foundation::Result<void, MeshError>::failure(
				make_mesh_error(MeshErrorCode::VertexStillReferenced,
						"cannot delete a vertex while live halfedges reference it"));
	}

	storage_->backend.delete_vertex(kHandle, false);
	clear_reverse(storage_->vertex_reverse.handle_to_slot, kHandle);

	auto &slot = storage_->vertices[vertex.index()];
	slot.alive = false;
	slot.generation = next_generation(slot.generation);
	slot.handle = invalid_handle<detail::QuaderOpenMesh::VertexHandle>();
	storage_->free_vertices.push_back(vertex.index());

	return quader::foundation::Result<void, MeshError>::success();
}

quader::foundation::Result<FaceId, MeshError> Polyhedron::create_face(std::span<const VertexId> vertices) {
	if (vertices.size() < 3U) {
		return quader::foundation::Result<FaceId, MeshError>::failure(
				make_mesh_error(MeshErrorCode::FaceNeedsAtLeastThreeVertices, "a face requires at least three vertices"));
	}

	if (has_duplicate_vertices(vertices)) {
		return quader::foundation::Result<FaceId, MeshError>::failure(
				make_mesh_error(MeshErrorCode::DegenerateFace, "cannot create a face with duplicate vertices"));
	}

	std::vector<detail::QuaderOpenMesh::VertexHandle> handles;
	std::vector<quader::math::Vec3> positions;
	handles.reserve(vertices.size());
	positions.reserve(vertices.size());

	for (const auto kVertex : vertices) {
		if (!is_valid(kVertex)) {
			return quader::foundation::Result<FaceId, MeshError>::failure(
					make_mesh_error(MeshErrorCode::InvalidVertex, "cannot create a face with an invalid vertex id"));
		}

		const auto kHandle = storage_->vertices[kVertex.index()].handle;
		handles.push_back(kHandle);
		positions.push_back(from_openmesh_point(storage_->backend.point(kHandle)));
	}

	if (!quader::geometry::all_points_finite(positions)) {
		return quader::foundation::Result<FaceId, MeshError>::failure(
				make_mesh_error(MeshErrorCode::NonFinitePosition,
						"cannot create a face with non-finite vertex positions"));
	}

	if (quader::geometry::has_degenerate_area(positions)) {
		return quader::foundation::Result<FaceId, MeshError>::failure(
				make_mesh_error(MeshErrorCode::DegenerateFace, "cannot create a face with degenerate polygon area"));
	}

	const auto kRollback = *storage_;
	const auto kFaceHandle =
			plain_handle<detail::QuaderOpenMesh::FaceHandle>(storage_->backend.add_face(handles));
	if (!kFaceHandle.is_valid()) {
		*storage_ = kRollback;
		return quader::foundation::Result<FaceId, MeshError>::failure(
				make_mesh_error(MeshErrorCode::BackendRejectedFace,
						"OpenMesh rejected the requested face without committing it"));
	}

	auto allocate_face_id = [this](detail::QuaderOpenMesh::FaceHandle handle) {
		const auto kExisting = reverse_slot(storage_->face_reverse.handle_to_slot, handle);
		if (kExisting != kNoSlot && kExisting < storage_->faces.size() && storage_->faces[kExisting].alive) {
			return FaceId{ kExisting, storage_->faces[kExisting].generation };
		}

		quader::foundation::IdIndex slot_index = 0;
		if (!storage_->free_faces.empty()) {
			slot_index = storage_->free_faces.back();
			storage_->free_faces.pop_back();
			auto &slot = storage_->faces[slot_index];
			slot.alive = true;
			slot.handle = handle;
		} else {
			slot_index = static_cast<quader::foundation::IdIndex>(storage_->faces.size());
			storage_->faces.push_back(detail::OpenMeshIdSlot<detail::QuaderOpenMesh::FaceHandle>{});
			auto &slot = storage_->faces.back();
			slot.alive = true;
			slot.handle = handle;
			attributes_.resize_domain(AttributeDomain::Face, storage_->faces.size());
		}
		set_reverse(storage_->face_reverse.handle_to_slot, handle, slot_index);
		attributes_.reset_slot_to_defaults(AttributeDomain::Face, slot_index);
		return FaceId{ slot_index, storage_->faces[slot_index].generation };
	};

	auto allocate_halfedge_id = [this](detail::QuaderOpenMesh::HalfedgeHandle handle) {
		const auto kExisting = reverse_slot(storage_->halfedge_reverse.handle_to_slot, handle);
		if (kExisting != kNoSlot && kExisting < storage_->halfedges.size() && storage_->halfedges[kExisting].alive) {
			return HalfedgeId{ kExisting, storage_->halfedges[kExisting].generation };
		}

		quader::foundation::IdIndex slot_index = 0;
		if (!storage_->free_halfedges.empty()) {
			slot_index = storage_->free_halfedges.back();
			storage_->free_halfedges.pop_back();
			auto &slot = storage_->halfedges[slot_index];
			slot.alive = true;
			slot.handle = handle;
		} else {
			slot_index = static_cast<quader::foundation::IdIndex>(storage_->halfedges.size());
			storage_->halfedges.push_back(detail::OpenMeshIdSlot<detail::QuaderOpenMesh::HalfedgeHandle>{});
			auto &slot = storage_->halfedges.back();
			slot.alive = true;
			slot.handle = handle;
			attributes_.resize_domain(AttributeDomain::Halfedge, storage_->halfedges.size());
		}
		set_reverse(storage_->halfedge_reverse.handle_to_slot, handle, slot_index);
		attributes_.reset_slot_to_defaults(AttributeDomain::Halfedge, slot_index);
		return HalfedgeId{ slot_index, storage_->halfedges[slot_index].generation };
	};

	auto allocate_edge_id = [this](detail::QuaderOpenMesh::EdgeHandle handle, HalfedgeId representative) {
		const auto kExisting = reverse_slot(storage_->edge_reverse.handle_to_slot, handle);
		if (kExisting != kNoSlot && kExisting < storage_->edges.size() && storage_->edges[kExisting].alive) {
			return EdgeId{ kExisting, storage_->edges[kExisting].generation };
		}

		quader::foundation::IdIndex slot_index = 0;
		if (!storage_->free_edges.empty()) {
			slot_index = storage_->free_edges.back();
			storage_->free_edges.pop_back();
			auto &slot = storage_->edges[slot_index];
			slot.alive = true;
			slot.handle = handle;
			slot.representative_halfedge = representative;
		} else {
			slot_index = static_cast<quader::foundation::IdIndex>(storage_->edges.size());
			storage_->edges.push_back(detail::OpenMeshEdgeSlot{});
			auto &slot = storage_->edges.back();
			slot.alive = true;
			slot.handle = handle;
			slot.representative_halfedge = representative;
			attributes_.resize_domain(AttributeDomain::Edge, storage_->edges.size());
		}
		set_reverse(storage_->edge_reverse.handle_to_slot, handle, slot_index);
		attributes_.reset_slot_to_defaults(AttributeDomain::Edge, slot_index);
		return EdgeId{ slot_index, storage_->edges[slot_index].generation };
	};

	const FaceId kFaceId = allocate_face_id(kFaceHandle);

	for (auto halfedge_it = storage_->backend.fh_iter(kFaceHandle); halfedge_it.is_valid(); ++halfedge_it) {
		const auto kHalfedgeHandle = plain_handle<detail::QuaderOpenMesh::HalfedgeHandle>(*halfedge_it);
		const auto kHalfedgeId = allocate_halfedge_id(kHalfedgeHandle);
		const auto kOppositeHandle = plain_handle<detail::QuaderOpenMesh::HalfedgeHandle>(
				storage_->backend.opposite_halfedge_handle(kHalfedgeHandle));
		allocate_halfedge_id(kOppositeHandle);
		allocate_edge_id(plain_handle<detail::QuaderOpenMesh::EdgeHandle>(storage_->backend.edge_handle(kHalfedgeHandle)),
				kHalfedgeId);
	}

	return quader::foundation::Result<FaceId, MeshError>::success(kFaceId);
}

quader::foundation::Result<void, MeshError> Polyhedron::delete_face(FaceId face) {
	if (!is_valid(face)) {
		return quader::foundation::Result<void, MeshError>::failure(
				make_mesh_error(MeshErrorCode::InvalidFace, "cannot delete an invalid face id"));
	}

	auto halfedges = face_halfedges(face);
	if (!halfedges) {
		return quader::foundation::Result<void, MeshError>::failure(std::move(halfedges).error());
	}

	std::vector<HalfedgeId> affected_halfedges;
	std::vector<EdgeId> affected_edges;
	affected_halfedges.reserve(halfedges.value().size() * 2U);
	affected_edges.reserve(halfedges.value().size());

	for (const auto kHalfedge : halfedges.value()) {
		affected_halfedges.push_back(kHalfedge);
		auto opposite = opposite_halfedge(kHalfedge);
		if (opposite) {
			affected_halfedges.push_back(opposite.value());
		}
		auto edge = halfedge_edge(kHalfedge);
		if (edge) {
			affected_edges.push_back(edge.value());
		}
	}

	const auto kFaceHandle = storage_->faces[face.index()].handle;
	storage_->backend.delete_face(kFaceHandle, false);

	clear_reverse(storage_->face_reverse.handle_to_slot, kFaceHandle);
	auto &face_slot = storage_->faces[face.index()];
	face_slot.alive = false;
	face_slot.generation = next_generation(face_slot.generation);
	face_slot.handle = invalid_handle<detail::QuaderOpenMesh::FaceHandle>();
	storage_->free_faces.push_back(face.index());

	for (const auto kEdge : affected_edges) {
		if (kEdge.index() >= storage_->edges.size()) {
			continue;
		}
		auto &slot = storage_->edges[kEdge.index()];
		if (!slot.alive || !storage_->backend.status(slot.handle).deleted()) {
			continue;
		}
		clear_reverse(storage_->edge_reverse.handle_to_slot, slot.handle);
		slot.alive = false;
		slot.generation = next_generation(slot.generation);
		slot.handle = invalid_handle<detail::QuaderOpenMesh::EdgeHandle>();
		slot.representative_halfedge = HalfedgeId::invalid();
		storage_->free_edges.push_back(kEdge.index());
	}

	for (const auto kHalfedge : affected_halfedges) {
		if (kHalfedge.index() >= storage_->halfedges.size()) {
			continue;
		}
		auto &slot = storage_->halfedges[kHalfedge.index()];
		if (!slot.alive || !storage_->backend.status(slot.handle).deleted()) {
			continue;
		}
		clear_reverse(storage_->halfedge_reverse.handle_to_slot, slot.handle);
		slot.alive = false;
		slot.generation = next_generation(slot.generation);
		slot.handle = invalid_handle<detail::QuaderOpenMesh::HalfedgeHandle>();
		storage_->free_halfedges.push_back(kHalfedge.index());
	}

	return quader::foundation::Result<void, MeshError>::success();
}

quader::foundation::Result<void, MeshError> Polyhedron::reverse_face_winding(FaceId face) {
	const std::array<FaceId, 1U> kFaces{ face };
	return reverse_face_windings(kFaces);
}

quader::foundation::Result<void, MeshError> Polyhedron::reverse_face_windings(
		std::span<const FaceId> faces) {
	if (faces.empty()) {
		return quader::foundation::Result<void, MeshError>::failure(
				make_mesh_error(MeshErrorCode::InvalidFace, "cannot reverse face winding without live faces"));
	}

	std::vector<FaceId> faces_to_reverse{ faces.begin(), faces.end() };
	for (const FaceId kFace : faces_to_reverse) {
		if (!is_valid(kFace)) {
			return quader::foundation::Result<void, MeshError>::failure(
					make_mesh_error(MeshErrorCode::InvalidFace,
							"cannot reverse the winding of an invalid face id"));
		}
	}
	std::sort(faces_to_reverse.begin(), faces_to_reverse.end());
	faces_to_reverse.erase(std::unique(faces_to_reverse.begin(), faces_to_reverse.end()),
			faces_to_reverse.end());

	auto face_loops = build_face_loop_snapshots(*this, faces_to_reverse);
	if (!face_loops) {
		return quader::foundation::Result<void, MeshError>::failure(std::move(face_loops).error());
	}

	auto halfedge_map = build_directed_halfedge_map(*this);
	if (!halfedge_map) {
		return quader::foundation::Result<void, MeshError>::failure(std::move(halfedge_map).error());
	}

	auto edge_map = build_edge_map(*this);
	if (!edge_map) {
		return quader::foundation::Result<void, MeshError>::failure(std::move(edge_map).error());
	}

	detail::OpenMeshStorage rebuilt;
	rebuilt.vertices = storage_->vertices;
	rebuilt.halfedges = storage_->halfedges;
	rebuilt.edges = storage_->edges;
	rebuilt.faces = storage_->faces;
	rebuilt.free_vertices = storage_->free_vertices;
	rebuilt.free_halfedges = storage_->free_halfedges;
	rebuilt.free_edges = storage_->free_edges;
	rebuilt.free_faces = storage_->free_faces;

	for (auto &slot : rebuilt.vertices) {
		slot.handle = invalid_handle<detail::QuaderOpenMesh::VertexHandle>();
	}
	for (auto &slot : rebuilt.halfedges) {
		slot.handle = invalid_handle<detail::QuaderOpenMesh::HalfedgeHandle>();
	}
	for (auto &slot : rebuilt.edges) {
		slot.handle = invalid_handle<detail::QuaderOpenMesh::EdgeHandle>();
		slot.representative_halfedge = HalfedgeId::invalid();
	}
	for (auto &slot : rebuilt.faces) {
		slot.handle = invalid_handle<detail::QuaderOpenMesh::FaceHandle>();
	}

	std::map<VertexId, detail::QuaderOpenMesh::VertexHandle> vertex_handles;
	for (const VertexId kVertex : vertex_ids()) {
		auto position = vertex_position(kVertex);
		if (!position) {
			return quader::foundation::Result<void, MeshError>::failure(std::move(position).error());
		}

		const auto kHandle = plain_handle<detail::QuaderOpenMesh::VertexHandle>(
				rebuilt.backend.add_vertex(to_openmesh_point(position.value())));
		auto &slot = rebuilt.vertices[kVertex.index()];
		slot.alive = true;
		slot.generation = kVertex.generation();
		slot.handle = kHandle;
		set_reverse(rebuilt.vertex_reverse.handle_to_slot, kHandle, kVertex.index());
		vertex_handles.emplace(kVertex, kHandle);
	}

	for (const FaceLoopSnapshot &face_loop : face_loops.value()) {
		std::vector<detail::QuaderOpenMesh::VertexHandle> handles;
		handles.reserve(face_loop.vertices.size());
		for (const VertexId kVertex : face_loop.vertices) {
			const auto found = vertex_handles.find(kVertex);
			if (found == vertex_handles.end()) {
				return quader::foundation::Result<void, MeshError>::failure(
						backend_mapping_error("face winding rebuild could not resolve a vertex handle"));
			}
			handles.push_back(found->second);
		}

		const auto kFaceHandle =
				plain_handle<detail::QuaderOpenMesh::FaceHandle>(rebuilt.backend.add_face(handles));
		if (!kFaceHandle.is_valid()) {
			return quader::foundation::Result<void, MeshError>::failure(
					make_mesh_error(MeshErrorCode::BackendRejectedFace,
							"backend rejected the requested reversed face winding"));
		}

		auto &slot = rebuilt.faces[face_loop.face.index()];
		slot.alive = true;
		slot.generation = face_loop.face.generation();
		slot.handle = kFaceHandle;
		set_reverse(rebuilt.face_reverse.handle_to_slot, kFaceHandle, face_loop.face.index());
	}

	std::set<HalfedgeId> assigned_halfedges;
	std::set<EdgeId> assigned_edges;
	for (auto edge_it = rebuilt.backend.edges_begin(); edge_it != rebuilt.backend.edges_end(); ++edge_it) {
		const auto kEdgeHandle = plain_handle<detail::QuaderOpenMesh::EdgeHandle>(*edge_it);
		const std::array<detail::QuaderOpenMesh::HalfedgeHandle, 2U> kHalfedgeHandles{
			plain_handle<detail::QuaderOpenMesh::HalfedgeHandle>(
					rebuilt.backend.halfedge_handle(kEdgeHandle, 0)),
			plain_handle<detail::QuaderOpenMesh::HalfedgeHandle>(
					rebuilt.backend.halfedge_handle(kEdgeHandle, 1)),
		};

		std::array<HalfedgeId, 2U> halfedge_ids{
			HalfedgeId::invalid(),
			HalfedgeId::invalid(),
		};
		DirectedHalfedgeKey first_key{};
		for (std::size_t index = 0; index < kHalfedgeHandles.size(); ++index) {
			const auto kHalfedgeHandle = kHalfedgeHandles[index];
			const auto kOriginHandle = rebuilt.backend.from_vertex_handle(kHalfedgeHandle);
			const auto kTargetHandle = rebuilt.backend.to_vertex_handle(kHalfedgeHandle);
			const auto kOriginSlot = reverse_slot(rebuilt.vertex_reverse.handle_to_slot, kOriginHandle);
			const auto kTargetSlot = reverse_slot(rebuilt.vertex_reverse.handle_to_slot, kTargetHandle);
			if (kOriginSlot == kNoSlot || kTargetSlot == kNoSlot ||
					kOriginSlot >= rebuilt.vertices.size() || kTargetSlot >= rebuilt.vertices.size()) {
				return quader::foundation::Result<void, MeshError>::failure(
						backend_mapping_error("reversed topology contains an unmapped halfedge vertex"));
			}

			const DirectedHalfedgeKey kKey{
				VertexId{ kOriginSlot, rebuilt.vertices[kOriginSlot].generation },
				VertexId{ kTargetSlot, rebuilt.vertices[kTargetSlot].generation },
			};
			if (index == 0U) {
				first_key = kKey;
			}
			const auto found_halfedge = halfedge_map.value().find(kKey);
			if (found_halfedge == halfedge_map.value().end()) {
				return quader::foundation::Result<void, MeshError>::failure(
						backend_mapping_error("reversed topology could not preserve a halfedge id"));
			}

			const HalfedgeId kHalfedge = found_halfedge->second;
			auto &slot = rebuilt.halfedges[kHalfedge.index()];
			slot.alive = true;
			slot.generation = kHalfedge.generation();
			slot.handle = kHalfedgeHandle;
			set_reverse(rebuilt.halfedge_reverse.handle_to_slot, kHalfedgeHandle, kHalfedge.index());
			assigned_halfedges.insert(kHalfedge);
			halfedge_ids[index] = kHalfedge;
		}

		const auto found_edge = edge_map.value().find(make_edge_vertex_key(first_key.origin, first_key.target));
		if (found_edge == edge_map.value().end()) {
			return quader::foundation::Result<void, MeshError>::failure(
					backend_mapping_error("reversed topology could not preserve an edge id"));
		}

		const EdgeId kEdge = found_edge->second;
		auto &slot = rebuilt.edges[kEdge.index()];
		slot.alive = true;
		slot.generation = kEdge.generation();
		slot.handle = kEdgeHandle;
		slot.representative_halfedge = halfedge_ids[0].is_valid() ? halfedge_ids[0] : halfedge_ids[1];
		set_reverse(rebuilt.edge_reverse.handle_to_slot, kEdgeHandle, kEdge.index());
		assigned_edges.insert(kEdge);
	}

	if (assigned_halfedges.size() != halfedge_count() || assigned_edges.size() != edge_count()) {
		return quader::foundation::Result<void, MeshError>::failure(
				backend_mapping_error("reversed topology did not preserve every live edge and halfedge id"));
	}

	auto previous_storage = std::move(storage_);
	storage_ = std::make_unique<detail::OpenMeshStorage>(std::move(rebuilt));
	const auto kValidation = validate_mesh(*this);
	if (!kValidation.ok()) {
		storage_ = std::move(previous_storage);
		return quader::foundation::Result<void, MeshError>::failure(
				make_mesh_error(MeshErrorCode::InvalidGeometry,
						"mesh validation failed after reversing face winding"));
	}

	return quader::foundation::Result<void, MeshError>::success();
}

bool Polyhedron::is_valid(VertexId id) const noexcept {
	if (!id.is_valid() || id.index() >= storage_->vertices.size()) {
		return false;
	}
	const auto &slot = storage_->vertices[id.index()];
	return slot.alive && slot.generation == id.generation() && live_backend_handle(storage_->backend, slot.handle) && reverse_slot(storage_->vertex_reverse.handle_to_slot, slot.handle) == id.index();
}

bool Polyhedron::is_valid(HalfedgeId id) const noexcept {
	if (!id.is_valid() || id.index() >= storage_->halfedges.size()) {
		return false;
	}
	const auto &slot = storage_->halfedges[id.index()];
	return slot.alive && slot.generation == id.generation() && live_backend_handle(storage_->backend, slot.handle) && reverse_slot(storage_->halfedge_reverse.handle_to_slot, slot.handle) == id.index();
}

bool Polyhedron::is_valid(EdgeId id) const noexcept {
	if (!id.is_valid() || id.index() >= storage_->edges.size()) {
		return false;
	}
	const auto &slot = storage_->edges[id.index()];
	return slot.alive && slot.generation == id.generation() && live_backend_handle(storage_->backend, slot.handle) && reverse_slot(storage_->edge_reverse.handle_to_slot, slot.handle) == id.index();
}

bool Polyhedron::is_valid(FaceId id) const noexcept {
	if (!id.is_valid() || id.index() >= storage_->faces.size()) {
		return false;
	}
	const auto &slot = storage_->faces[id.index()];
	return slot.alive && slot.generation == id.generation() && live_backend_handle(storage_->backend, slot.handle) && reverse_slot(storage_->face_reverse.handle_to_slot, slot.handle) == id.index();
}

std::size_t Polyhedron::vertex_count() const noexcept {
	return alive_count(storage_->vertices);
}

std::size_t Polyhedron::halfedge_count() const noexcept {
	return alive_count(storage_->halfedges);
}

std::size_t Polyhedron::edge_count() const noexcept {
	return alive_count(storage_->edges);
}

std::size_t Polyhedron::face_count() const noexcept {
	return alive_count(storage_->faces);
}

std::vector<VertexId> Polyhedron::vertex_ids() const {
	return live_ids<VertexId>(storage_->vertices);
}

std::vector<HalfedgeId> Polyhedron::halfedge_ids() const {
	return live_ids<HalfedgeId>(storage_->halfedges);
}

std::vector<EdgeId> Polyhedron::edge_ids() const {
	return live_ids<EdgeId>(storage_->edges);
}

std::vector<FaceId> Polyhedron::face_ids() const {
	return live_ids<FaceId>(storage_->faces);
}

quader::foundation::Result<quader::math::Vec3, MeshError> Polyhedron::vertex_position(VertexId vertex) const {
	if (!is_valid(vertex)) {
		return quader::foundation::Result<quader::math::Vec3, MeshError>::failure(
				make_mesh_error(MeshErrorCode::InvalidVertex, "cannot query an invalid vertex id"));
	}
	return quader::foundation::Result<quader::math::Vec3, MeshError>::success(
			from_openmesh_point(storage_->backend.point(storage_->vertices[vertex.index()].handle)));
}

quader::foundation::Result<void, MeshError> Polyhedron::set_vertex_position(VertexId vertex,
		quader::math::Vec3 position) {
	if (!is_valid(vertex)) {
		return quader::foundation::Result<void, MeshError>::failure(
				make_mesh_error(MeshErrorCode::InvalidVertex, "cannot move an invalid vertex id"));
	}
	if (!quader::math::is_finite(position)) {
		return quader::foundation::Result<void, MeshError>::failure(
				make_mesh_error(MeshErrorCode::NonFinitePosition, "cannot assign a non-finite vertex position"));
	}

	storage_->backend.set_point(storage_->vertices[vertex.index()].handle, to_openmesh_point(position));
	return quader::foundation::Result<void, MeshError>::success();
}

quader::foundation::Result<HalfedgeId, MeshError> Polyhedron::vertex_outgoing_halfedge(VertexId vertex) const {
	if (!is_valid(vertex)) {
		return quader::foundation::Result<HalfedgeId, MeshError>::failure(
				make_mesh_error(MeshErrorCode::InvalidVertex, "cannot query an invalid vertex id"));
	}

	const auto kHandle = storage_->backend.halfedge_handle(storage_->vertices[vertex.index()].handle);
	const auto kSlot = reverse_slot(storage_->halfedge_reverse.handle_to_slot, kHandle);
	if (kSlot == kNoSlot || kSlot >= storage_->halfedges.size()) {
		return quader::foundation::Result<HalfedgeId, MeshError>::failure(
				make_mesh_error(MeshErrorCode::InvalidHalfedge, "vertex does not have a valid outgoing halfedge"));
	}
	return quader::foundation::Result<HalfedgeId, MeshError>::success(
			HalfedgeId{ kSlot, storage_->halfedges[kSlot].generation });
}

quader::foundation::Result<VertexId, MeshError> Polyhedron::halfedge_origin(HalfedgeId halfedge) const {
	if (!is_valid(halfedge)) {
		return quader::foundation::Result<VertexId, MeshError>::failure(
				make_mesh_error(MeshErrorCode::InvalidHalfedge, "cannot query an invalid halfedge id"));
	}

	const auto kVertexHandle = storage_->backend.from_vertex_handle(storage_->halfedges[halfedge.index()].handle);
	const auto kSlot = reverse_slot(storage_->vertex_reverse.handle_to_slot, kVertexHandle);
	if (kSlot == kNoSlot || kSlot >= storage_->vertices.size()) {
		return quader::foundation::Result<VertexId, MeshError>::failure(
				make_mesh_error(MeshErrorCode::InvalidVertex, "halfedge origin is an invalid vertex"));
	}
	return quader::foundation::Result<VertexId, MeshError>::success(VertexId{ kSlot, storage_->vertices[kSlot].generation });
}

quader::foundation::Result<VertexId, MeshError> Polyhedron::halfedge_target(HalfedgeId halfedge) const {
	if (!is_valid(halfedge)) {
		return quader::foundation::Result<VertexId, MeshError>::failure(
				make_mesh_error(MeshErrorCode::InvalidHalfedge, "cannot query an invalid halfedge id"));
	}

	const auto kVertexHandle = storage_->backend.to_vertex_handle(storage_->halfedges[halfedge.index()].handle);
	const auto kSlot = reverse_slot(storage_->vertex_reverse.handle_to_slot, kVertexHandle);
	if (kSlot == kNoSlot || kSlot >= storage_->vertices.size()) {
		return quader::foundation::Result<VertexId, MeshError>::failure(
				make_mesh_error(MeshErrorCode::InvalidVertex, "halfedge target is an invalid vertex"));
	}
	return quader::foundation::Result<VertexId, MeshError>::success(VertexId{ kSlot, storage_->vertices[kSlot].generation });
}

quader::foundation::Result<HalfedgeId, MeshError> Polyhedron::opposite_halfedge(HalfedgeId halfedge) const {
	if (!is_valid(halfedge)) {
		return quader::foundation::Result<HalfedgeId, MeshError>::failure(
				make_mesh_error(MeshErrorCode::InvalidHalfedge, "cannot query an invalid halfedge id"));
	}

	const auto kOppositeHandle = storage_->backend.opposite_halfedge_handle(storage_->halfedges[halfedge.index()].handle);
	const auto kSlot = reverse_slot(storage_->halfedge_reverse.handle_to_slot, kOppositeHandle);
	if (kSlot == kNoSlot || kSlot >= storage_->halfedges.size()) {
		return quader::foundation::Result<HalfedgeId, MeshError>::failure(
				make_mesh_error(MeshErrorCode::InvalidHalfedge, "halfedge has an invalid opposite"));
	}
	return quader::foundation::Result<HalfedgeId, MeshError>::success(
			HalfedgeId{ kSlot, storage_->halfedges[kSlot].generation });
}

quader::foundation::Result<HalfedgeId, MeshError> Polyhedron::next_halfedge(HalfedgeId halfedge) const {
	if (!is_valid(halfedge)) {
		return quader::foundation::Result<HalfedgeId, MeshError>::failure(
				make_mesh_error(MeshErrorCode::InvalidHalfedge, "cannot query an invalid halfedge id"));
	}

	const auto kHandle = storage_->halfedges[halfedge.index()].handle;
	if (storage_->backend.is_boundary(kHandle)) {
		return quader::foundation::Result<HalfedgeId, MeshError>::failure(
				make_mesh_error(MeshErrorCode::InvalidHalfedge, "boundary halfedge does not expose a public next link"));
	}

	const auto kNextHandle = storage_->backend.next_halfedge_handle(kHandle);
	const auto kSlot = reverse_slot(storage_->halfedge_reverse.handle_to_slot, kNextHandle);
	if (kSlot == kNoSlot || kSlot >= storage_->halfedges.size()) {
		return quader::foundation::Result<HalfedgeId, MeshError>::failure(
				make_mesh_error(MeshErrorCode::InvalidHalfedge, "halfedge has an invalid next link"));
	}
	return quader::foundation::Result<HalfedgeId, MeshError>::success(
			HalfedgeId{ kSlot, storage_->halfedges[kSlot].generation });
}

quader::foundation::Result<HalfedgeId, MeshError> Polyhedron::previous_halfedge(HalfedgeId halfedge) const {
	if (!is_valid(halfedge)) {
		return quader::foundation::Result<HalfedgeId, MeshError>::failure(
				make_mesh_error(MeshErrorCode::InvalidHalfedge, "cannot query an invalid halfedge id"));
	}

	const auto kHandle = storage_->halfedges[halfedge.index()].handle;
	if (storage_->backend.is_boundary(kHandle)) {
		return quader::foundation::Result<HalfedgeId, MeshError>::failure(
				make_mesh_error(MeshErrorCode::InvalidHalfedge,
						"boundary halfedge does not expose a public previous link"));
	}

	const auto kPrevHandle = storage_->backend.prev_halfedge_handle(kHandle);
	const auto kSlot = reverse_slot(storage_->halfedge_reverse.handle_to_slot, kPrevHandle);
	if (kSlot == kNoSlot || kSlot >= storage_->halfedges.size()) {
		return quader::foundation::Result<HalfedgeId, MeshError>::failure(
				make_mesh_error(MeshErrorCode::InvalidHalfedge, "halfedge has an invalid previous link"));
	}
	return quader::foundation::Result<HalfedgeId, MeshError>::success(
			HalfedgeId{ kSlot, storage_->halfedges[kSlot].generation });
}

quader::foundation::Result<EdgeId, MeshError> Polyhedron::halfedge_edge(HalfedgeId halfedge) const {
	if (!is_valid(halfedge)) {
		return quader::foundation::Result<EdgeId, MeshError>::failure(
				make_mesh_error(MeshErrorCode::InvalidHalfedge, "cannot query an invalid halfedge id"));
	}

	const auto kEdgeHandle = storage_->backend.edge_handle(storage_->halfedges[halfedge.index()].handle);
	const auto kSlot = reverse_slot(storage_->edge_reverse.handle_to_slot, kEdgeHandle);
	if (kSlot == kNoSlot || kSlot >= storage_->edges.size()) {
		return quader::foundation::Result<EdgeId, MeshError>::failure(
				make_mesh_error(MeshErrorCode::InvalidEdge, "halfedge edge is invalid"));
	}
	return quader::foundation::Result<EdgeId, MeshError>::success(EdgeId{ kSlot, storage_->edges[kSlot].generation });
}

quader::foundation::Result<FaceId, MeshError> Polyhedron::halfedge_face(HalfedgeId halfedge) const {
	if (!is_valid(halfedge)) {
		return quader::foundation::Result<FaceId, MeshError>::failure(
				make_mesh_error(MeshErrorCode::InvalidHalfedge, "cannot query an invalid halfedge id"));
	}

	const auto kHandle = storage_->halfedges[halfedge.index()].handle;
	if (storage_->backend.is_boundary(kHandle)) {
		return quader::foundation::Result<FaceId, MeshError>::failure(
				make_mesh_error(MeshErrorCode::InvalidFace, "halfedge face is invalid"));
	}

	const auto kFaceHandle = storage_->backend.face_handle(kHandle);
	const auto kSlot = reverse_slot(storage_->face_reverse.handle_to_slot, kFaceHandle);
	if (kSlot == kNoSlot || kSlot >= storage_->faces.size()) {
		return quader::foundation::Result<FaceId, MeshError>::failure(
				make_mesh_error(MeshErrorCode::InvalidFace, "halfedge face is invalid"));
	}
	return quader::foundation::Result<FaceId, MeshError>::success(FaceId{ kSlot, storage_->faces[kSlot].generation });
}

quader::foundation::Result<bool, MeshError> Polyhedron::is_boundary_halfedge(HalfedgeId halfedge) const {
	if (!is_valid(halfedge)) {
		return quader::foundation::Result<bool, MeshError>::failure(
				make_mesh_error(MeshErrorCode::InvalidHalfedge, "cannot query an invalid halfedge id"));
	}
	return quader::foundation::Result<bool, MeshError>::success(
			storage_->backend.is_boundary(storage_->halfedges[halfedge.index()].handle));
}

quader::foundation::Result<std::vector<HalfedgeId>, MeshError> Polyhedron::face_halfedges(FaceId face) const {
	if (!is_valid(face)) {
		return quader::foundation::Result<std::vector<HalfedgeId>, MeshError>::failure(
				make_mesh_error(MeshErrorCode::InvalidFace, "cannot traverse an invalid face id"));
	}

	std::vector<HalfedgeId> result;
	const auto kFaceHandle = storage_->faces[face.index()].handle;
	for (auto halfedge_it = storage_->backend.cfh_iter(kFaceHandle); halfedge_it.is_valid(); ++halfedge_it) {
		const auto kSlot = reverse_slot(storage_->halfedge_reverse.handle_to_slot, *halfedge_it);
		if (kSlot == kNoSlot || kSlot >= storage_->halfedges.size()) {
			return quader::foundation::Result<std::vector<HalfedgeId>, MeshError>::failure(
					make_mesh_error(MeshErrorCode::InvalidHalfedge, "face cycle references an invalid halfedge"));
		}
		result.push_back(HalfedgeId{ kSlot, storage_->halfedges[kSlot].generation });
	}

	if (result.empty()) {
		return quader::foundation::Result<std::vector<HalfedgeId>, MeshError>::failure(
				make_mesh_error(MeshErrorCode::BrokenFaceCycle, "face cycle did not close"));
	}

	return quader::foundation::Result<std::vector<HalfedgeId>, MeshError>::success(std::move(result));
}

quader::foundation::Result<std::array<HalfedgeId, 2>, MeshError> Polyhedron::edge_halfedges(EdgeId edge) const {
	if (!is_valid(edge)) {
		return quader::foundation::Result<std::array<HalfedgeId, 2>, MeshError>::failure(
				make_mesh_error(MeshErrorCode::InvalidEdge, "cannot traverse an invalid edge id"));
	}

	const auto kEdgeHandle = storage_->edges[edge.index()].handle;
	const auto kFirstHandle = storage_->backend.halfedge_handle(kEdgeHandle, 0);
	const auto kSecondHandle = storage_->backend.halfedge_handle(kEdgeHandle, 1);
	const auto kFirstSlot = reverse_slot(storage_->halfedge_reverse.handle_to_slot, kFirstHandle);
	const auto kSecondSlot = reverse_slot(storage_->halfedge_reverse.handle_to_slot, kSecondHandle);
	if (kFirstSlot == kNoSlot || kSecondSlot == kNoSlot || kFirstSlot >= storage_->halfedges.size() || kSecondSlot >= storage_->halfedges.size()) {
		return quader::foundation::Result<std::array<HalfedgeId, 2>, MeshError>::failure(
				make_mesh_error(MeshErrorCode::InvalidHalfedge, "edge references an invalid halfedge"));
	}

	return quader::foundation::Result<std::array<HalfedgeId, 2>, MeshError>::success(
			{ HalfedgeId{ kFirstSlot, storage_->halfedges[kFirstSlot].generation },
					HalfedgeId{ kSecondSlot, storage_->halfedges[kSecondSlot].generation } });
}

quader::foundation::Result<MeshCompactionReport, MeshError> Polyhedron::compact_storage() {
	MeshCompactionReport report;
	report.removed_vertices = storage_->backend.n_vertices() > vertex_count()
			? storage_->backend.n_vertices() - vertex_count()
			: 0U;
	report.removed_halfedges = storage_->backend.n_halfedges() > halfedge_count()
			? storage_->backend.n_halfedges() - halfedge_count()
			: 0U;
	report.removed_edges = storage_->backend.n_edges() > edge_count() ? storage_->backend.n_edges() - edge_count()
																	  : 0U;
	report.removed_faces = storage_->backend.n_faces() > face_count() ? storage_->backend.n_faces() - face_count()
																	  : 0U;

	std::vector<detail::QuaderOpenMesh::VertexHandle *> vertex_handles;
	std::vector<detail::QuaderOpenMesh::HalfedgeHandle *> halfedge_handles;
	std::vector<detail::QuaderOpenMesh::FaceHandle *> face_handles;
	vertex_handles.reserve(vertex_count());
	halfedge_handles.reserve(halfedge_count());
	face_handles.reserve(face_count());

	for (auto &slot : storage_->vertices) {
		if (slot.alive) {
			vertex_handles.push_back(&slot.handle);
		}
	}
	for (auto &slot : storage_->halfedges) {
		if (slot.alive) {
			halfedge_handles.push_back(&slot.handle);
		}
	}
	for (auto &slot : storage_->faces) {
		if (slot.alive) {
			face_handles.push_back(&slot.handle);
		}
	}

	storage_->backend.garbage_collection(vertex_handles, halfedge_handles, face_handles, true, true, true);

	storage_->vertex_reverse.handle_to_slot.clear();
	storage_->halfedge_reverse.handle_to_slot.clear();
	storage_->edge_reverse.handle_to_slot.clear();
	storage_->face_reverse.handle_to_slot.clear();

	for (quader::foundation::IdIndex index = 0; index < storage_->vertices.size(); ++index) {
		if (storage_->vertices[index].alive) {
			set_reverse(storage_->vertex_reverse.handle_to_slot, storage_->vertices[index].handle, index);
		}
	}
	for (quader::foundation::IdIndex index = 0; index < storage_->halfedges.size(); ++index) {
		if (storage_->halfedges[index].alive) {
			set_reverse(storage_->halfedge_reverse.handle_to_slot, storage_->halfedges[index].handle, index);
		}
	}
	for (quader::foundation::IdIndex index = 0; index < storage_->edges.size(); ++index) {
		auto &edge_slot = storage_->edges[index];
		if (!edge_slot.alive || !is_valid(edge_slot.representative_halfedge)) {
			continue;
		}
		edge_slot.handle = storage_->backend.edge_handle(storage_->halfedges[edge_slot.representative_halfedge.index()].handle);
		set_reverse(storage_->edge_reverse.handle_to_slot, edge_slot.handle, index);
	}
	for (quader::foundation::IdIndex index = 0; index < storage_->faces.size(); ++index) {
		if (storage_->faces[index].alive) {
			set_reverse(storage_->face_reverse.handle_to_slot, storage_->faces[index].handle, index);
		}
	}

	report.live_vertices = vertex_count();
	report.live_halfedges = halfedge_count();
	report.live_edges = edge_count();
	report.live_faces = face_count();

	const auto kValidation = validate_mesh(*this);
	if (!kValidation.ok()) {
		return quader::foundation::Result<MeshCompactionReport, MeshError>::failure(
				make_mesh_error(MeshErrorCode::CompactionFailed, "mesh validation failed after storage compaction"));
	}

	return quader::foundation::Result<MeshCompactionReport, MeshError>::success(report);
}

MeshAttributes &Polyhedron::attributes() noexcept {
	return attributes_;
}

const MeshAttributes &Polyhedron::attributes() const noexcept {
	return attributes_;
}

void Polyhedron::test_corrupt_vertex_position(VertexId vertex, quader::math::Vec3 position) {
	if (vertex.index() < storage_->vertices.size() && storage_->vertices[vertex.index()].handle.is_valid()) {
		storage_->backend.set_point(storage_->vertices[vertex.index()].handle, to_openmesh_point(position));
	}
}

void Polyhedron::test_corrupt_vertex_backend_handle(VertexId vertex) {
	if (vertex.index() < storage_->vertices.size()) {
		clear_reverse(storage_->vertex_reverse.handle_to_slot, storage_->vertices[vertex.index()].handle);
		storage_->vertices[vertex.index()].handle = invalid_handle<detail::QuaderOpenMesh::VertexHandle>();
	}
}

void Polyhedron::test_corrupt_halfedge_origin_mapping(HalfedgeId halfedge) {
	if (halfedge.index() < storage_->halfedges.size() && storage_->halfedges[halfedge.index()].handle.is_valid()) {
		const auto kVertex = storage_->backend.from_vertex_handle(storage_->halfedges[halfedge.index()].handle);
		const auto kIndex = handle_index(kVertex);
		if (kIndex >= 0) {
			const auto kReverseIndex = static_cast<std::size_t>(kIndex);
			if (storage_->vertex_reverse.handle_to_slot.size() <= kReverseIndex) {
				storage_->vertex_reverse.handle_to_slot.resize(kReverseIndex + 1U, kNoSlot);
			}
			storage_->vertex_reverse.handle_to_slot[kReverseIndex] =
					static_cast<quader::foundation::IdIndex>(storage_->vertices.size() + 16U);
		}
	}
}

void Polyhedron::test_corrupt_halfedge_reverse_mapping(HalfedgeId halfedge) {
	if (halfedge.index() < storage_->halfedges.size()) {
		clear_reverse(storage_->halfedge_reverse.handle_to_slot, storage_->halfedges[halfedge.index()].handle);
	}
}

void Polyhedron::test_corrupt_face_backend_handle(FaceId face) {
	if (face.index() < storage_->faces.size()) {
		clear_reverse(storage_->face_reverse.handle_to_slot, storage_->faces[face.index()].handle);
		storage_->faces[face.index()].handle = invalid_handle<detail::QuaderOpenMesh::FaceHandle>();
	}
}

} // namespace quader::mesh
