#pragma once

#include "foundation/diagnostic.hpp"

#include <string>
#include <utility>

namespace quader::mesh {

enum class MeshErrorCode {
    InvalidVertex,
    InvalidHalfedge,
    InvalidEdge,
    InvalidFace,
    InvalidAttribute,
    AttributeNameAlreadyExists,
    AttributeTypeMismatch,
    AttributeIndexOutOfRange,
    FaceNeedsAtLeastThreeVertices,
    NonFinitePosition,
    DegenerateFace,
    VertexStillReferenced,
    BrokenFaceCycle,
    NonManifoldTopology,
    BackendRejectedFace,
    BackendMappingCorrupt,
    CompactionFailed,
    InvalidGeometry,
};

struct MeshError {
    MeshErrorCode code = MeshErrorCode::InvalidVertex;
    quader::foundation::Diagnostic diagnostic;
};

[[nodiscard]] inline MeshError make_mesh_error(MeshErrorCode code, std::string message)
{
    return MeshError{
        code,
        quader::foundation::Diagnostic{
            quader::foundation::Severity::error,
            std::move(message),
        },
    };
}

} // namespace quader::mesh
