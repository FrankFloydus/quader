#pragma once

#include "foundation/id.hpp"
#include "mesh/core/detail/openmesh_traits.hpp"
#include "mesh/core/mesh_ids.hpp"

#include <vector>

namespace quader::mesh::detail {

template <class Handle>
struct OpenMeshIdSlot {
    quader::foundation::IdGeneration generation = 1;
    bool alive = false;
    Handle handle;
};

template <class Handle>
struct OpenMeshReverseMap {
    std::vector<quader::foundation::IdIndex> handle_to_slot;
};

struct OpenMeshEdgeSlot {
    quader::foundation::IdGeneration generation = 1;
    bool alive = false;
    QuaderOpenMesh::EdgeHandle handle;
    HalfedgeId representative_halfedge;
};

} // namespace quader::mesh::detail
