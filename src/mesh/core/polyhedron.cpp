#include "mesh/core/polyhedron.hpp"

#include "geometry/normals.hpp"
#include "geometry/predicates.hpp"
#include "mesh/core/detail/openmesh_storage.hpp"
#include "mesh/core/mesh_validation.hpp"

#include <algorithm>
#include <cstddef>
#include <string>
#include <utility>

namespace quader::mesh {
namespace {

constexpr quader::foundation::IdIndex kNoSlot = quader::foundation::kInvalidIdIndex;

[[nodiscard]] quader::foundation::IdGeneration next_generation(
    quader::foundation::IdGeneration generation) noexcept
{
    auto next = static_cast<quader::foundation::IdGeneration>(generation + 1U);
    if (next == 0U) {
        next = 1U;
    }
    return next;
}

template <class Handle>
[[nodiscard]] int handle_index(Handle handle) noexcept
{
    return handle.is_valid() ? handle.idx() : -1;
}

template <class Handle>
void set_reverse(std::vector<quader::foundation::IdIndex>& reverse,
                 Handle handle,
                 quader::foundation::IdIndex slot)
{
    const auto index = handle_index(handle);
    if (index < 0) {
        return;
    }

    const auto reverse_index = static_cast<std::size_t>(index);
    if (reverse.size() <= reverse_index) {
        reverse.resize(reverse_index + 1U, kNoSlot);
    }
    reverse[reverse_index] = slot;
}

template <class Handle>
void clear_reverse(std::vector<quader::foundation::IdIndex>& reverse, Handle handle)
{
    const auto index = handle_index(handle);
    if (index < 0) {
        return;
    }

    const auto reverse_index = static_cast<std::size_t>(index);
    if (reverse_index < reverse.size()) {
        reverse[reverse_index] = kNoSlot;
    }
}

template <class Handle>
[[nodiscard]] quader::foundation::IdIndex reverse_slot(const std::vector<quader::foundation::IdIndex>& reverse,
                                                       Handle handle) noexcept
{
    const auto index = handle_index(handle);
    if (index < 0) {
        return kNoSlot;
    }

    const auto reverse_index = static_cast<std::size_t>(index);
    if (reverse_index >= reverse.size()) {
        return kNoSlot;
    }

    return reverse[reverse_index];
}

[[nodiscard]] detail::QuaderOpenMesh::Point to_openmesh_point(quader::math::Vec3 position) noexcept
{
    return detail::QuaderOpenMesh::Point{position.x, position.y, position.z};
}

[[nodiscard]] quader::math::Vec3 from_openmesh_point(detail::QuaderOpenMesh::Point point) noexcept
{
    return quader::math::Vec3{point[0], point[1], point[2]};
}

template <class Slot>
[[nodiscard]] std::size_t alive_count(const std::vector<Slot>& slots) noexcept
{
    return static_cast<std::size_t>(
        std::count_if(slots.begin(), slots.end(), [](const auto& slot) { return slot.alive; }));
}

template <class Id, class Slot>
[[nodiscard]] std::vector<Id> live_ids(const std::vector<Slot>& slots)
{
    std::vector<Id> ids;
    ids.reserve(alive_count(slots));
    for (quader::foundation::IdIndex index = 0; index < slots.size(); ++index) {
        const auto& slot = slots[index];
        if (slot.alive) {
            ids.push_back(Id{index, slot.generation});
        }
    }
    return ids;
}

template <class Handle>
[[nodiscard]] bool live_backend_handle(const detail::QuaderOpenMesh& backend, Handle handle) noexcept
{
    return handle.is_valid() && backend.is_valid_handle(handle) && !backend.status(handle).deleted();
}

[[nodiscard]] bool has_duplicate_vertices(std::span<const VertexId> vertices) noexcept
{
    for (std::size_t i = 0; i < vertices.size(); ++i) {
        for (std::size_t j = i + 1U; j < vertices.size(); ++j) {
            if (vertices[i] == vertices[j]) {
                return true;
            }
        }
    }
    return false;
}

} // namespace

Polyhedron::Polyhedron()
    : storage_(std::make_unique<detail::OpenMeshStorage>())
{
}

Polyhedron::~Polyhedron() = default;

Polyhedron::Polyhedron(const Polyhedron& other)
    : storage_(std::make_unique<detail::OpenMeshStorage>(*other.storage_))
    , attributes_(other.attributes_)
{
}

Polyhedron& Polyhedron::operator=(const Polyhedron& other)
{
    if (this == &other) {
        return *this;
    }

    storage_ = std::make_unique<detail::OpenMeshStorage>(*other.storage_);
    attributes_ = other.attributes_;
    return *this;
}

Polyhedron::Polyhedron(Polyhedron&&) noexcept = default;
Polyhedron& Polyhedron::operator=(Polyhedron&&) noexcept = default;

VertexId Polyhedron::create_vertex(quader::math::Vec3 position)
{
    const auto handle = storage_->backend.add_vertex(to_openmesh_point(position));

    quader::foundation::IdIndex slot_index = 0;
    if (!storage_->free_vertices.empty()) {
        slot_index = storage_->free_vertices.back();
        storage_->free_vertices.pop_back();
        auto& slot = storage_->vertices[slot_index];
        slot.alive = true;
        slot.handle = handle;
    } else {
        slot_index = static_cast<quader::foundation::IdIndex>(storage_->vertices.size());
        storage_->vertices.push_back(detail::OpenMeshIdSlot<detail::QuaderOpenMesh::VertexHandle>{});
        auto& slot = storage_->vertices.back();
        slot.alive = true;
        slot.handle = handle;
        attributes_.resize_domain(AttributeDomain::Vertex, storage_->vertices.size());
    }

    set_reverse(storage_->vertex_reverse.handle_to_slot, handle, slot_index);
    attributes_.reset_slot_to_defaults(AttributeDomain::Vertex, slot_index);
    return VertexId{slot_index, storage_->vertices[slot_index].generation};
}

quader::foundation::Result<void, MeshError> Polyhedron::delete_vertex(VertexId vertex)
{
    if (!is_valid(vertex)) {
        return quader::foundation::Result<void, MeshError>::failure(
            make_mesh_error(MeshErrorCode::InvalidVertex, "cannot delete an invalid vertex id"));
    }

    const auto handle = storage_->vertices[vertex.index()].handle;
    if (!storage_->backend.is_isolated(handle)) {
        return quader::foundation::Result<void, MeshError>::failure(
            make_mesh_error(MeshErrorCode::VertexStillReferenced,
                            "cannot delete a vertex while live halfedges reference it"));
    }

    storage_->backend.delete_vertex(handle, false);
    clear_reverse(storage_->vertex_reverse.handle_to_slot, handle);

    auto& slot = storage_->vertices[vertex.index()];
    slot.alive = false;
    slot.generation = next_generation(slot.generation);
    slot.handle = {};
    storage_->free_vertices.push_back(vertex.index());

    return quader::foundation::Result<void, MeshError>::success();
}

quader::foundation::Result<FaceId, MeshError> Polyhedron::create_face(std::span<const VertexId> vertices)
{
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

    for (const auto vertex : vertices) {
        if (!is_valid(vertex)) {
            return quader::foundation::Result<FaceId, MeshError>::failure(
                make_mesh_error(MeshErrorCode::InvalidVertex, "cannot create a face with an invalid vertex id"));
        }

        const auto handle = storage_->vertices[vertex.index()].handle;
        handles.push_back(handle);
        positions.push_back(from_openmesh_point(storage_->backend.point(handle)));
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

    const auto rollback = *storage_;
    const auto face_handle = storage_->backend.add_face(handles);
    if (!face_handle.is_valid()) {
        *storage_ = rollback;
        return quader::foundation::Result<FaceId, MeshError>::failure(
            make_mesh_error(MeshErrorCode::BackendRejectedFace,
                            "OpenMesh rejected the requested face without committing it"));
    }

    auto allocate_face_id = [this](detail::QuaderOpenMesh::FaceHandle handle) {
        const auto existing = reverse_slot(storage_->face_reverse.handle_to_slot, handle);
        if (existing != kNoSlot && existing < storage_->faces.size() && storage_->faces[existing].alive) {
            return FaceId{existing, storage_->faces[existing].generation};
        }

        quader::foundation::IdIndex slot_index = 0;
        if (!storage_->free_faces.empty()) {
            slot_index = storage_->free_faces.back();
            storage_->free_faces.pop_back();
            auto& slot = storage_->faces[slot_index];
            slot.alive = true;
            slot.handle = handle;
        } else {
            slot_index = static_cast<quader::foundation::IdIndex>(storage_->faces.size());
            storage_->faces.push_back(detail::OpenMeshIdSlot<detail::QuaderOpenMesh::FaceHandle>{});
            auto& slot = storage_->faces.back();
            slot.alive = true;
            slot.handle = handle;
            attributes_.resize_domain(AttributeDomain::Face, storage_->faces.size());
        }
        set_reverse(storage_->face_reverse.handle_to_slot, handle, slot_index);
        attributes_.reset_slot_to_defaults(AttributeDomain::Face, slot_index);
        return FaceId{slot_index, storage_->faces[slot_index].generation};
    };

    auto allocate_halfedge_id = [this](detail::QuaderOpenMesh::HalfedgeHandle handle) {
        const auto existing = reverse_slot(storage_->halfedge_reverse.handle_to_slot, handle);
        if (existing != kNoSlot && existing < storage_->halfedges.size() && storage_->halfedges[existing].alive) {
            return HalfedgeId{existing, storage_->halfedges[existing].generation};
        }

        quader::foundation::IdIndex slot_index = 0;
        if (!storage_->free_halfedges.empty()) {
            slot_index = storage_->free_halfedges.back();
            storage_->free_halfedges.pop_back();
            auto& slot = storage_->halfedges[slot_index];
            slot.alive = true;
            slot.handle = handle;
        } else {
            slot_index = static_cast<quader::foundation::IdIndex>(storage_->halfedges.size());
            storage_->halfedges.push_back(detail::OpenMeshIdSlot<detail::QuaderOpenMesh::HalfedgeHandle>{});
            auto& slot = storage_->halfedges.back();
            slot.alive = true;
            slot.handle = handle;
            attributes_.resize_domain(AttributeDomain::Halfedge, storage_->halfedges.size());
        }
        set_reverse(storage_->halfedge_reverse.handle_to_slot, handle, slot_index);
        attributes_.reset_slot_to_defaults(AttributeDomain::Halfedge, slot_index);
        return HalfedgeId{slot_index, storage_->halfedges[slot_index].generation};
    };

    auto allocate_edge_id = [this](detail::QuaderOpenMesh::EdgeHandle handle, HalfedgeId representative) {
        const auto existing = reverse_slot(storage_->edge_reverse.handle_to_slot, handle);
        if (existing != kNoSlot && existing < storage_->edges.size() && storage_->edges[existing].alive) {
            return EdgeId{existing, storage_->edges[existing].generation};
        }

        quader::foundation::IdIndex slot_index = 0;
        if (!storage_->free_edges.empty()) {
            slot_index = storage_->free_edges.back();
            storage_->free_edges.pop_back();
            auto& slot = storage_->edges[slot_index];
            slot.alive = true;
            slot.handle = handle;
            slot.representative_halfedge = representative;
        } else {
            slot_index = static_cast<quader::foundation::IdIndex>(storage_->edges.size());
            storage_->edges.push_back(detail::OpenMeshEdgeSlot{});
            auto& slot = storage_->edges.back();
            slot.alive = true;
            slot.handle = handle;
            slot.representative_halfedge = representative;
            attributes_.resize_domain(AttributeDomain::Edge, storage_->edges.size());
        }
        set_reverse(storage_->edge_reverse.handle_to_slot, handle, slot_index);
        attributes_.reset_slot_to_defaults(AttributeDomain::Edge, slot_index);
        return EdgeId{slot_index, storage_->edges[slot_index].generation};
    };

    const FaceId face_id = allocate_face_id(face_handle);

    for (auto halfedge_it = storage_->backend.fh_iter(face_handle); halfedge_it.is_valid(); ++halfedge_it) {
        const auto halfedge_handle = *halfedge_it;
        const auto halfedge_id = allocate_halfedge_id(halfedge_handle);
        const auto opposite_handle = storage_->backend.opposite_halfedge_handle(halfedge_handle);
        allocate_halfedge_id(opposite_handle);
        allocate_edge_id(storage_->backend.edge_handle(halfedge_handle), halfedge_id);
    }

    return quader::foundation::Result<FaceId, MeshError>::success(face_id);
}

quader::foundation::Result<void, MeshError> Polyhedron::delete_face(FaceId face)
{
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

    for (const auto halfedge : halfedges.value()) {
        affected_halfedges.push_back(halfedge);
        auto opposite = opposite_halfedge(halfedge);
        if (opposite) {
            affected_halfedges.push_back(opposite.value());
        }
        auto edge = halfedge_edge(halfedge);
        if (edge) {
            affected_edges.push_back(edge.value());
        }
    }

    const auto face_handle = storage_->faces[face.index()].handle;
    storage_->backend.delete_face(face_handle, false);

    clear_reverse(storage_->face_reverse.handle_to_slot, face_handle);
    auto& face_slot = storage_->faces[face.index()];
    face_slot.alive = false;
    face_slot.generation = next_generation(face_slot.generation);
    face_slot.handle = {};
    storage_->free_faces.push_back(face.index());

    for (const auto edge : affected_edges) {
        if (edge.index() >= storage_->edges.size()) {
            continue;
        }
        auto& slot = storage_->edges[edge.index()];
        if (!slot.alive || !storage_->backend.status(slot.handle).deleted()) {
            continue;
        }
        clear_reverse(storage_->edge_reverse.handle_to_slot, slot.handle);
        slot.alive = false;
        slot.generation = next_generation(slot.generation);
        slot.handle = {};
        slot.representative_halfedge = HalfedgeId::invalid();
        storage_->free_edges.push_back(edge.index());
    }

    for (const auto halfedge : affected_halfedges) {
        if (halfedge.index() >= storage_->halfedges.size()) {
            continue;
        }
        auto& slot = storage_->halfedges[halfedge.index()];
        if (!slot.alive || !storage_->backend.status(slot.handle).deleted()) {
            continue;
        }
        clear_reverse(storage_->halfedge_reverse.handle_to_slot, slot.handle);
        slot.alive = false;
        slot.generation = next_generation(slot.generation);
        slot.handle = {};
        storage_->free_halfedges.push_back(halfedge.index());
    }

    return quader::foundation::Result<void, MeshError>::success();
}

bool Polyhedron::is_valid(VertexId id) const noexcept
{
    if (!id.is_valid() || id.index() >= storage_->vertices.size()) {
        return false;
    }
    const auto& slot = storage_->vertices[id.index()];
    return slot.alive && slot.generation == id.generation()
        && live_backend_handle(storage_->backend, slot.handle)
        && reverse_slot(storage_->vertex_reverse.handle_to_slot, slot.handle) == id.index();
}

bool Polyhedron::is_valid(HalfedgeId id) const noexcept
{
    if (!id.is_valid() || id.index() >= storage_->halfedges.size()) {
        return false;
    }
    const auto& slot = storage_->halfedges[id.index()];
    return slot.alive && slot.generation == id.generation()
        && live_backend_handle(storage_->backend, slot.handle)
        && reverse_slot(storage_->halfedge_reverse.handle_to_slot, slot.handle) == id.index();
}

bool Polyhedron::is_valid(EdgeId id) const noexcept
{
    if (!id.is_valid() || id.index() >= storage_->edges.size()) {
        return false;
    }
    const auto& slot = storage_->edges[id.index()];
    return slot.alive && slot.generation == id.generation()
        && live_backend_handle(storage_->backend, slot.handle)
        && reverse_slot(storage_->edge_reverse.handle_to_slot, slot.handle) == id.index();
}

bool Polyhedron::is_valid(FaceId id) const noexcept
{
    if (!id.is_valid() || id.index() >= storage_->faces.size()) {
        return false;
    }
    const auto& slot = storage_->faces[id.index()];
    return slot.alive && slot.generation == id.generation()
        && live_backend_handle(storage_->backend, slot.handle)
        && reverse_slot(storage_->face_reverse.handle_to_slot, slot.handle) == id.index();
}

std::size_t Polyhedron::vertex_count() const noexcept
{
    return alive_count(storage_->vertices);
}

std::size_t Polyhedron::halfedge_count() const noexcept
{
    return alive_count(storage_->halfedges);
}

std::size_t Polyhedron::edge_count() const noexcept
{
    return alive_count(storage_->edges);
}

std::size_t Polyhedron::face_count() const noexcept
{
    return alive_count(storage_->faces);
}

std::vector<VertexId> Polyhedron::vertex_ids() const
{
    return live_ids<VertexId>(storage_->vertices);
}

std::vector<HalfedgeId> Polyhedron::halfedge_ids() const
{
    return live_ids<HalfedgeId>(storage_->halfedges);
}

std::vector<EdgeId> Polyhedron::edge_ids() const
{
    return live_ids<EdgeId>(storage_->edges);
}

std::vector<FaceId> Polyhedron::face_ids() const
{
    return live_ids<FaceId>(storage_->faces);
}

quader::foundation::Result<quader::math::Vec3, MeshError> Polyhedron::vertex_position(VertexId vertex) const
{
    if (!is_valid(vertex)) {
        return quader::foundation::Result<quader::math::Vec3, MeshError>::failure(
            make_mesh_error(MeshErrorCode::InvalidVertex, "cannot query an invalid vertex id"));
    }
    return quader::foundation::Result<quader::math::Vec3, MeshError>::success(
        from_openmesh_point(storage_->backend.point(storage_->vertices[vertex.index()].handle)));
}

quader::foundation::Result<void, MeshError> Polyhedron::set_vertex_position(VertexId vertex,
                                                                              quader::math::Vec3 position)
{
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

quader::foundation::Result<HalfedgeId, MeshError> Polyhedron::vertex_outgoing_halfedge(VertexId vertex) const
{
    if (!is_valid(vertex)) {
        return quader::foundation::Result<HalfedgeId, MeshError>::failure(
            make_mesh_error(MeshErrorCode::InvalidVertex, "cannot query an invalid vertex id"));
    }

    const auto handle = storage_->backend.halfedge_handle(storage_->vertices[vertex.index()].handle);
    const auto slot = reverse_slot(storage_->halfedge_reverse.handle_to_slot, handle);
    if (slot == kNoSlot || slot >= storage_->halfedges.size()) {
        return quader::foundation::Result<HalfedgeId, MeshError>::failure(
            make_mesh_error(MeshErrorCode::InvalidHalfedge, "vertex does not have a valid outgoing halfedge"));
    }
    return quader::foundation::Result<HalfedgeId, MeshError>::success(
        HalfedgeId{slot, storage_->halfedges[slot].generation});
}

quader::foundation::Result<VertexId, MeshError> Polyhedron::halfedge_origin(HalfedgeId halfedge) const
{
    if (!is_valid(halfedge)) {
        return quader::foundation::Result<VertexId, MeshError>::failure(
            make_mesh_error(MeshErrorCode::InvalidHalfedge, "cannot query an invalid halfedge id"));
    }

    const auto vertex_handle = storage_->backend.from_vertex_handle(storage_->halfedges[halfedge.index()].handle);
    const auto slot = reverse_slot(storage_->vertex_reverse.handle_to_slot, vertex_handle);
    if (slot == kNoSlot || slot >= storage_->vertices.size()) {
        return quader::foundation::Result<VertexId, MeshError>::failure(
            make_mesh_error(MeshErrorCode::InvalidVertex, "halfedge origin is an invalid vertex"));
    }
    return quader::foundation::Result<VertexId, MeshError>::success(VertexId{slot, storage_->vertices[slot].generation});
}

quader::foundation::Result<VertexId, MeshError> Polyhedron::halfedge_target(HalfedgeId halfedge) const
{
    if (!is_valid(halfedge)) {
        return quader::foundation::Result<VertexId, MeshError>::failure(
            make_mesh_error(MeshErrorCode::InvalidHalfedge, "cannot query an invalid halfedge id"));
    }

    const auto vertex_handle = storage_->backend.to_vertex_handle(storage_->halfedges[halfedge.index()].handle);
    const auto slot = reverse_slot(storage_->vertex_reverse.handle_to_slot, vertex_handle);
    if (slot == kNoSlot || slot >= storage_->vertices.size()) {
        return quader::foundation::Result<VertexId, MeshError>::failure(
            make_mesh_error(MeshErrorCode::InvalidVertex, "halfedge target is an invalid vertex"));
    }
    return quader::foundation::Result<VertexId, MeshError>::success(VertexId{slot, storage_->vertices[slot].generation});
}

quader::foundation::Result<HalfedgeId, MeshError> Polyhedron::opposite_halfedge(HalfedgeId halfedge) const
{
    if (!is_valid(halfedge)) {
        return quader::foundation::Result<HalfedgeId, MeshError>::failure(
            make_mesh_error(MeshErrorCode::InvalidHalfedge, "cannot query an invalid halfedge id"));
    }

    const auto opposite_handle = storage_->backend.opposite_halfedge_handle(storage_->halfedges[halfedge.index()].handle);
    const auto slot = reverse_slot(storage_->halfedge_reverse.handle_to_slot, opposite_handle);
    if (slot == kNoSlot || slot >= storage_->halfedges.size()) {
        return quader::foundation::Result<HalfedgeId, MeshError>::failure(
            make_mesh_error(MeshErrorCode::InvalidHalfedge, "halfedge has an invalid opposite"));
    }
    return quader::foundation::Result<HalfedgeId, MeshError>::success(
        HalfedgeId{slot, storage_->halfedges[slot].generation});
}

quader::foundation::Result<HalfedgeId, MeshError> Polyhedron::next_halfedge(HalfedgeId halfedge) const
{
    if (!is_valid(halfedge)) {
        return quader::foundation::Result<HalfedgeId, MeshError>::failure(
            make_mesh_error(MeshErrorCode::InvalidHalfedge, "cannot query an invalid halfedge id"));
    }

    const auto handle = storage_->halfedges[halfedge.index()].handle;
    if (storage_->backend.is_boundary(handle)) {
        return quader::foundation::Result<HalfedgeId, MeshError>::failure(
            make_mesh_error(MeshErrorCode::InvalidHalfedge, "boundary halfedge does not expose a public next link"));
    }

    const auto next_handle = storage_->backend.next_halfedge_handle(handle);
    const auto slot = reverse_slot(storage_->halfedge_reverse.handle_to_slot, next_handle);
    if (slot == kNoSlot || slot >= storage_->halfedges.size()) {
        return quader::foundation::Result<HalfedgeId, MeshError>::failure(
            make_mesh_error(MeshErrorCode::InvalidHalfedge, "halfedge has an invalid next link"));
    }
    return quader::foundation::Result<HalfedgeId, MeshError>::success(
        HalfedgeId{slot, storage_->halfedges[slot].generation});
}

quader::foundation::Result<HalfedgeId, MeshError> Polyhedron::previous_halfedge(HalfedgeId halfedge) const
{
    if (!is_valid(halfedge)) {
        return quader::foundation::Result<HalfedgeId, MeshError>::failure(
            make_mesh_error(MeshErrorCode::InvalidHalfedge, "cannot query an invalid halfedge id"));
    }

    const auto handle = storage_->halfedges[halfedge.index()].handle;
    if (storage_->backend.is_boundary(handle)) {
        return quader::foundation::Result<HalfedgeId, MeshError>::failure(
            make_mesh_error(MeshErrorCode::InvalidHalfedge,
                            "boundary halfedge does not expose a public previous link"));
    }

    const auto prev_handle = storage_->backend.prev_halfedge_handle(handle);
    const auto slot = reverse_slot(storage_->halfedge_reverse.handle_to_slot, prev_handle);
    if (slot == kNoSlot || slot >= storage_->halfedges.size()) {
        return quader::foundation::Result<HalfedgeId, MeshError>::failure(
            make_mesh_error(MeshErrorCode::InvalidHalfedge, "halfedge has an invalid previous link"));
    }
    return quader::foundation::Result<HalfedgeId, MeshError>::success(
        HalfedgeId{slot, storage_->halfedges[slot].generation});
}

quader::foundation::Result<EdgeId, MeshError> Polyhedron::halfedge_edge(HalfedgeId halfedge) const
{
    if (!is_valid(halfedge)) {
        return quader::foundation::Result<EdgeId, MeshError>::failure(
            make_mesh_error(MeshErrorCode::InvalidHalfedge, "cannot query an invalid halfedge id"));
    }

    const auto edge_handle = storage_->backend.edge_handle(storage_->halfedges[halfedge.index()].handle);
    const auto slot = reverse_slot(storage_->edge_reverse.handle_to_slot, edge_handle);
    if (slot == kNoSlot || slot >= storage_->edges.size()) {
        return quader::foundation::Result<EdgeId, MeshError>::failure(
            make_mesh_error(MeshErrorCode::InvalidEdge, "halfedge edge is invalid"));
    }
    return quader::foundation::Result<EdgeId, MeshError>::success(EdgeId{slot, storage_->edges[slot].generation});
}

quader::foundation::Result<FaceId, MeshError> Polyhedron::halfedge_face(HalfedgeId halfedge) const
{
    if (!is_valid(halfedge)) {
        return quader::foundation::Result<FaceId, MeshError>::failure(
            make_mesh_error(MeshErrorCode::InvalidHalfedge, "cannot query an invalid halfedge id"));
    }

    const auto handle = storage_->halfedges[halfedge.index()].handle;
    if (storage_->backend.is_boundary(handle)) {
        return quader::foundation::Result<FaceId, MeshError>::failure(
            make_mesh_error(MeshErrorCode::InvalidFace, "halfedge face is invalid"));
    }

    const auto face_handle = storage_->backend.face_handle(handle);
    const auto slot = reverse_slot(storage_->face_reverse.handle_to_slot, face_handle);
    if (slot == kNoSlot || slot >= storage_->faces.size()) {
        return quader::foundation::Result<FaceId, MeshError>::failure(
            make_mesh_error(MeshErrorCode::InvalidFace, "halfedge face is invalid"));
    }
    return quader::foundation::Result<FaceId, MeshError>::success(FaceId{slot, storage_->faces[slot].generation});
}

quader::foundation::Result<bool, MeshError> Polyhedron::is_boundary_halfedge(HalfedgeId halfedge) const
{
    if (!is_valid(halfedge)) {
        return quader::foundation::Result<bool, MeshError>::failure(
            make_mesh_error(MeshErrorCode::InvalidHalfedge, "cannot query an invalid halfedge id"));
    }
    return quader::foundation::Result<bool, MeshError>::success(
        storage_->backend.is_boundary(storage_->halfedges[halfedge.index()].handle));
}

quader::foundation::Result<std::vector<HalfedgeId>, MeshError> Polyhedron::face_halfedges(FaceId face) const
{
    if (!is_valid(face)) {
        return quader::foundation::Result<std::vector<HalfedgeId>, MeshError>::failure(
            make_mesh_error(MeshErrorCode::InvalidFace, "cannot traverse an invalid face id"));
    }

    std::vector<HalfedgeId> result;
    const auto face_handle = storage_->faces[face.index()].handle;
    for (auto halfedge_it = storage_->backend.cfh_iter(face_handle); halfedge_it.is_valid(); ++halfedge_it) {
        const auto slot = reverse_slot(storage_->halfedge_reverse.handle_to_slot, *halfedge_it);
        if (slot == kNoSlot || slot >= storage_->halfedges.size()) {
            return quader::foundation::Result<std::vector<HalfedgeId>, MeshError>::failure(
                make_mesh_error(MeshErrorCode::InvalidHalfedge, "face cycle references an invalid halfedge"));
        }
        result.push_back(HalfedgeId{slot, storage_->halfedges[slot].generation});
    }

    if (result.empty()) {
        return quader::foundation::Result<std::vector<HalfedgeId>, MeshError>::failure(
            make_mesh_error(MeshErrorCode::BrokenFaceCycle, "face cycle did not close"));
    }

    return quader::foundation::Result<std::vector<HalfedgeId>, MeshError>::success(std::move(result));
}

quader::foundation::Result<std::array<HalfedgeId, 2>, MeshError> Polyhedron::edge_halfedges(EdgeId edge) const
{
    if (!is_valid(edge)) {
        return quader::foundation::Result<std::array<HalfedgeId, 2>, MeshError>::failure(
            make_mesh_error(MeshErrorCode::InvalidEdge, "cannot traverse an invalid edge id"));
    }

    const auto edge_handle = storage_->edges[edge.index()].handle;
    const auto first_handle = storage_->backend.halfedge_handle(edge_handle, 0);
    const auto second_handle = storage_->backend.halfedge_handle(edge_handle, 1);
    const auto first_slot = reverse_slot(storage_->halfedge_reverse.handle_to_slot, first_handle);
    const auto second_slot = reverse_slot(storage_->halfedge_reverse.handle_to_slot, second_handle);
    if (first_slot == kNoSlot || second_slot == kNoSlot || first_slot >= storage_->halfedges.size()
        || second_slot >= storage_->halfedges.size()) {
        return quader::foundation::Result<std::array<HalfedgeId, 2>, MeshError>::failure(
            make_mesh_error(MeshErrorCode::InvalidHalfedge, "edge references an invalid halfedge"));
    }

    return quader::foundation::Result<std::array<HalfedgeId, 2>, MeshError>::success(
        {HalfedgeId{first_slot, storage_->halfedges[first_slot].generation},
         HalfedgeId{second_slot, storage_->halfedges[second_slot].generation}});
}

quader::foundation::Result<MeshCompactionReport, MeshError> Polyhedron::compact_storage()
{
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

    std::vector<detail::QuaderOpenMesh::VertexHandle*> vertex_handles;
    std::vector<detail::QuaderOpenMesh::HalfedgeHandle*> halfedge_handles;
    std::vector<detail::QuaderOpenMesh::FaceHandle*> face_handles;
    vertex_handles.reserve(vertex_count());
    halfedge_handles.reserve(halfedge_count());
    face_handles.reserve(face_count());

    for (auto& slot : storage_->vertices) {
        if (slot.alive) {
            vertex_handles.push_back(&slot.handle);
        }
    }
    for (auto& slot : storage_->halfedges) {
        if (slot.alive) {
            halfedge_handles.push_back(&slot.handle);
        }
    }
    for (auto& slot : storage_->faces) {
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
        auto& edge_slot = storage_->edges[index];
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

    const auto validation = validate_mesh(*this);
    if (!validation.ok()) {
        return quader::foundation::Result<MeshCompactionReport, MeshError>::failure(
            make_mesh_error(MeshErrorCode::CompactionFailed, "mesh validation failed after storage compaction"));
    }

    return quader::foundation::Result<MeshCompactionReport, MeshError>::success(report);
}

MeshAttributes& Polyhedron::attributes() noexcept
{
    return attributes_;
}

const MeshAttributes& Polyhedron::attributes() const noexcept
{
    return attributes_;
}

void Polyhedron::test_corrupt_vertex_position(VertexId vertex, quader::math::Vec3 position)
{
    if (vertex.index() < storage_->vertices.size() && storage_->vertices[vertex.index()].handle.is_valid()) {
        storage_->backend.set_point(storage_->vertices[vertex.index()].handle, to_openmesh_point(position));
    }
}

void Polyhedron::test_corrupt_vertex_backend_handle(VertexId vertex)
{
    if (vertex.index() < storage_->vertices.size()) {
        clear_reverse(storage_->vertex_reverse.handle_to_slot, storage_->vertices[vertex.index()].handle);
        storage_->vertices[vertex.index()].handle = {};
    }
}

void Polyhedron::test_corrupt_halfedge_origin_mapping(HalfedgeId halfedge)
{
    if (halfedge.index() < storage_->halfedges.size()
        && storage_->halfedges[halfedge.index()].handle.is_valid()) {
        const auto vertex = storage_->backend.from_vertex_handle(storage_->halfedges[halfedge.index()].handle);
        const auto index = handle_index(vertex);
        if (index >= 0) {
            const auto reverse_index = static_cast<std::size_t>(index);
            if (storage_->vertex_reverse.handle_to_slot.size() <= reverse_index) {
                storage_->vertex_reverse.handle_to_slot.resize(reverse_index + 1U, kNoSlot);
            }
            storage_->vertex_reverse.handle_to_slot[reverse_index] =
                static_cast<quader::foundation::IdIndex>(storage_->vertices.size() + 16U);
        }
    }
}

void Polyhedron::test_corrupt_halfedge_reverse_mapping(HalfedgeId halfedge)
{
    if (halfedge.index() < storage_->halfedges.size()) {
        clear_reverse(storage_->halfedge_reverse.handle_to_slot, storage_->halfedges[halfedge.index()].handle);
    }
}

void Polyhedron::test_corrupt_face_backend_handle(FaceId face)
{
    if (face.index() < storage_->faces.size()) {
        clear_reverse(storage_->face_reverse.handle_to_slot, storage_->faces[face.index()].handle);
        storage_->faces[face.index()].handle = {};
    }
}

} // namespace quader::mesh
