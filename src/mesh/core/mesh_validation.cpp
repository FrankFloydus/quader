#include "mesh/core/mesh_validation.hpp"

#include "geometry/normals.hpp"
#include "geometry/predicates.hpp"
#include "mesh/core/detail/openmesh_storage.hpp"
#include "mesh/core/polyhedron.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace quader::mesh {
namespace {

constexpr quader::foundation::IdIndex kNoSlot = quader::foundation::kInvalidIdIndex;

[[nodiscard]] std::string slot_message(const char* type, std::size_t index, const char* message)
{
    return std::string(type) + " slot " + std::to_string(index) + ": " + message;
}

template <class Handle>
[[nodiscard]] int handle_index(Handle handle) noexcept
{
    return handle.is_valid() ? handle.idx() : -1;
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

template <class Handle>
[[nodiscard]] bool live_backend_handle(const detail::QuaderOpenMesh& backend, Handle handle) noexcept
{
    return handle.is_valid() && backend.is_valid_handle(handle) && !backend.status(handle).deleted();
}

[[nodiscard]] quader::math::Vec3 from_openmesh_point(detail::QuaderOpenMesh::Point point) noexcept
{
    return quader::math::Vec3{point[0], point[1], point[2]};
}

template <class Slot>
void validate_reverse_map(MeshValidationReport& report,
                          const char* type,
                          const std::vector<Slot>& slots,
                          const std::vector<quader::foundation::IdIndex>& reverse)
{
    for (quader::foundation::IdIndex handle_index_value = 0; handle_index_value < reverse.size();
         ++handle_index_value) {
        const auto slot_index = reverse[handle_index_value];
        if (slot_index == kNoSlot) {
            continue;
        }

        if (slot_index >= slots.size() || !slots[slot_index].alive) {
            report.add_issue(MeshValidationCode::MissingQuaderId,
                             std::string(type) + " reverse map points at a missing Quader id");
            continue;
        }

        if (handle_index(slots[slot_index].handle) != static_cast<int>(handle_index_value)) {
            report.add_issue(MeshValidationCode::ReverseMappingMismatch,
                             std::string(type) + " reverse map points at a mismatched backend handle");
        }
    }
}

template <class Slot>
void validate_backend_slot(MeshValidationReport& report,
                           const detail::QuaderOpenMesh& backend,
                           const char* type,
                           quader::foundation::IdIndex slot_index,
                           const Slot& slot,
                           const std::vector<quader::foundation::IdIndex>& reverse)
{
    if (!slot.alive) {
        return;
    }

    if (!slot.handle.is_valid() || !backend.is_valid_handle(slot.handle)) {
        report.add_issue(MeshValidationCode::MissingBackendHandle,
                         slot_message(type, slot_index, "missing backend handle"));
        return;
    }

    if (backend.status(slot.handle).deleted()) {
        report.add_issue(MeshValidationCode::DeletedElementMapped,
                         slot_message(type, slot_index, "maps to a deleted backend element"));
        return;
    }

    if (reverse_slot(reverse, slot.handle) != slot_index) {
        report.add_issue(MeshValidationCode::ReverseMappingMismatch,
                         slot_message(type, slot_index, "reverse map does not point back to this Quader id"));
    }
}

} // namespace

MeshValidationReport validate_mesh(const Polyhedron& mesh)
{
    MeshValidationReport report;
    const auto& storage = *mesh.storage_;
    const auto& backend = storage.backend;

    validate_reverse_map(report, "vertex", storage.vertices, storage.vertex_reverse.handle_to_slot);
    validate_reverse_map(report, "halfedge", storage.halfedges, storage.halfedge_reverse.handle_to_slot);
    validate_reverse_map(report, "edge", storage.edges, storage.edge_reverse.handle_to_slot);
    validate_reverse_map(report, "face", storage.faces, storage.face_reverse.handle_to_slot);

    for (quader::foundation::IdIndex index = 0; index < storage.vertices.size(); ++index) {
        const auto& vertex = storage.vertices[index];
        validate_backend_slot(report, backend, "vertex", index, vertex, storage.vertex_reverse.handle_to_slot);
        if (!vertex.alive || !live_backend_handle(backend, vertex.handle)) {
            continue;
        }

        if (!quader::math::is_finite(from_openmesh_point(backend.point(vertex.handle)))) {
            report.add_issue(MeshValidationCode::NonFinitePosition,
                             slot_message("vertex", index, "position contains non-finite values"));
        }

        const auto outgoing = backend.halfedge_handle(vertex.handle);
        if (outgoing.is_valid()) {
            const auto outgoing_slot = reverse_slot(storage.halfedge_reverse.handle_to_slot, outgoing);
            if (outgoing_slot == kNoSlot || outgoing_slot >= storage.halfedges.size()
                || !storage.halfedges[outgoing_slot].alive) {
                report.add_issue(MeshValidationCode::InvalidId,
                                 slot_message("vertex", index, "outgoing halfedge has no Quader id"));
            } else if (backend.from_vertex_handle(outgoing) != vertex.handle) {
                report.add_issue(MeshValidationCode::VertexReferenceMismatch,
                                 slot_message("vertex", index, "outgoing halfedge does not originate here"));
            }
        }
    }

    for (quader::foundation::IdIndex index = 0; index < storage.edges.size(); ++index) {
        const auto& edge = storage.edges[index];
        validate_backend_slot(report, backend, "edge", index, edge, storage.edge_reverse.handle_to_slot);
        if (!edge.alive || !live_backend_handle(backend, edge.handle)) {
            continue;
        }

        if (!mesh.is_valid(edge.representative_halfedge)) {
            report.add_issue(MeshValidationCode::InvalidId,
                             slot_message("edge", index, "representative halfedge id is invalid"));
        } else {
            const auto representative = storage.halfedges[edge.representative_halfedge.index()].handle;
            if (backend.edge_handle(representative) != edge.handle) {
                report.add_issue(MeshValidationCode::EdgeReferenceMismatch,
                                 slot_message("edge", index, "representative halfedge points at another edge"));
            }
        }
    }

    for (quader::foundation::IdIndex index = 0; index < storage.halfedges.size(); ++index) {
        const auto& halfedge = storage.halfedges[index];
        validate_backend_slot(report, backend, "halfedge", index, halfedge, storage.halfedge_reverse.handle_to_slot);
        if (!halfedge.alive || !live_backend_handle(backend, halfedge.handle)) {
            continue;
        }

        const HalfedgeId halfedge_id{index, halfedge.generation};
        const auto origin_handle = backend.from_vertex_handle(halfedge.handle);
        const auto origin_slot = reverse_slot(storage.vertex_reverse.handle_to_slot, origin_handle);
        if (origin_slot == kNoSlot || origin_slot >= storage.vertices.size() || !storage.vertices[origin_slot].alive) {
            report.add_issue(MeshValidationCode::InvalidId,
                             slot_message("halfedge", index, "origin vertex id is invalid"));
        }

        const auto edge_handle = backend.edge_handle(halfedge.handle);
        const auto edge_slot = reverse_slot(storage.edge_reverse.handle_to_slot, edge_handle);
        if (edge_slot == kNoSlot || edge_slot >= storage.edges.size() || !storage.edges[edge_slot].alive) {
            report.add_issue(MeshValidationCode::InvalidId,
                             slot_message("halfedge", index, "edge id is invalid"));
        }

        const auto opposite_handle = backend.opposite_halfedge_handle(halfedge.handle);
        const auto opposite_slot = reverse_slot(storage.halfedge_reverse.handle_to_slot, opposite_handle);
        if (opposite_slot == kNoSlot || opposite_slot >= storage.halfedges.size()
            || !storage.halfedges[opposite_slot].alive) {
            report.add_issue(MeshValidationCode::BrokenOppositePair,
                             slot_message("halfedge", index, "opposite halfedge id is invalid"));
        } else {
            const auto opposite = storage.halfedges[opposite_slot].handle;
            if (backend.opposite_halfedge_handle(opposite) != halfedge.handle
                || backend.edge_handle(opposite) != edge_handle) {
                report.add_issue(MeshValidationCode::BrokenOppositePair,
                                 slot_message("halfedge", index, "opposite halfedge does not point back"));
            }
        }

        if (backend.is_boundary(halfedge.handle)) {
            if (backend.face_handle(halfedge.handle).is_valid()) {
                report.add_issue(MeshValidationCode::BoundaryPlaceholderMismatch,
                                 slot_message("halfedge", index, "boundary halfedge has a face"));
            }
            continue;
        }

        const auto face_handle = backend.face_handle(halfedge.handle);
        const auto face_slot = reverse_slot(storage.face_reverse.handle_to_slot, face_handle);
        if (face_slot == kNoSlot || face_slot >= storage.faces.size() || !storage.faces[face_slot].alive) {
            report.add_issue(MeshValidationCode::InvalidId,
                             slot_message("halfedge", index, "face id is invalid"));
        }

        const auto next_handle = backend.next_halfedge_handle(halfedge.handle);
        const auto prev_handle = backend.prev_halfedge_handle(halfedge.handle);
        const auto next_slot = reverse_slot(storage.halfedge_reverse.handle_to_slot, next_handle);
        const auto prev_slot = reverse_slot(storage.halfedge_reverse.handle_to_slot, prev_handle);
        if (next_slot == kNoSlot || prev_slot == kNoSlot || next_slot >= storage.halfedges.size()
            || prev_slot >= storage.halfedges.size() || !storage.halfedges[next_slot].alive
            || !storage.halfedges[prev_slot].alive) {
            report.add_issue(MeshValidationCode::BrokenNextPrevCycle,
                             slot_message("halfedge", index, "next or prev id is invalid"));
            continue;
        }

        const auto next = storage.halfedges[next_slot].handle;
        const auto prev = storage.halfedges[prev_slot].handle;
        if (backend.prev_halfedge_handle(next) != halfedge.handle
            || backend.next_halfedge_handle(prev) != halfedge.handle) {
            report.add_issue(MeshValidationCode::BrokenNextPrevCycle,
                             slot_message("halfedge", index, "next/prev links are not reciprocal"));
        }
        if (backend.face_handle(next) != face_handle || backend.face_handle(prev) != face_handle) {
            report.add_issue(MeshValidationCode::FaceCycleMismatch,
                             slot_message("halfedge", index, "next/prev halfedges belong to another face"));
        }

        (void)halfedge_id;
    }

    for (quader::foundation::IdIndex index = 0; index < storage.faces.size(); ++index) {
        const auto& face = storage.faces[index];
        validate_backend_slot(report, backend, "face", index, face, storage.face_reverse.handle_to_slot);
        if (!face.alive || !live_backend_handle(backend, face.handle)) {
            continue;
        }

        std::vector<quader::math::Vec3> positions;
        for (auto halfedge_it = backend.cfh_iter(face.handle); halfedge_it.is_valid(); ++halfedge_it) {
            const auto halfedge_handle = *halfedge_it;
            const auto halfedge_slot = reverse_slot(storage.halfedge_reverse.handle_to_slot, halfedge_handle);
            if (halfedge_slot == kNoSlot || halfedge_slot >= storage.halfedges.size()
                || !storage.halfedges[halfedge_slot].alive) {
                report.add_issue(MeshValidationCode::InvalidId,
                                 slot_message("face", index, "cycle references an invalid halfedge"));
                break;
            }

            if (backend.is_boundary(halfedge_handle) || backend.face_handle(halfedge_handle) != face.handle) {
                report.add_issue(MeshValidationCode::FaceCycleMismatch,
                                 slot_message("face", index, "cycle contains a boundary or mismatched halfedge"));
                break;
            }

            const auto vertex_handle = backend.from_vertex_handle(halfedge_handle);
            const auto vertex_slot = reverse_slot(storage.vertex_reverse.handle_to_slot, vertex_handle);
            if (vertex_slot == kNoSlot || vertex_slot >= storage.vertices.size() || !storage.vertices[vertex_slot].alive) {
                report.add_issue(MeshValidationCode::InvalidId,
                                 slot_message("face", index, "cycle contains an invalid vertex"));
                break;
            }

            positions.push_back(from_openmesh_point(backend.point(vertex_handle)));
        }

        if (positions.size() < 3U || !quader::geometry::all_points_finite(positions)) {
            report.add_issue(MeshValidationCode::DegenerateFace,
                             slot_message("face", index, "cycle does not contain a finite polygon"));
            continue;
        }

        if (quader::geometry::has_degenerate_area(positions)) {
            report.add_issue(MeshValidationCode::DegenerateFace,
                             slot_message("face", index, "cycle polygon area is degenerate"));
        }
    }

    return report;
}

} // namespace quader::mesh
