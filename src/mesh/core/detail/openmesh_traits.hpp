#pragma once

#include <OpenMesh/Core/Mesh/PolyMesh_ArrayKernelT.hh>
#include <OpenMesh/Core/Mesh/Traits.hh>

namespace quader::mesh::detail {

struct QuaderOpenMeshTraits : OpenMesh::DefaultTraits {
    VertexAttributes(OpenMesh::Attributes::Status);
    HalfedgeAttributes(OpenMesh::Attributes::PrevHalfedge | OpenMesh::Attributes::Status);
    EdgeAttributes(OpenMesh::Attributes::Status);
    FaceAttributes(OpenMesh::Attributes::Status);
};

using QuaderOpenMesh = OpenMesh::PolyMesh_ArrayKernelT<QuaderOpenMeshTraits>;

} // namespace quader::mesh::detail
