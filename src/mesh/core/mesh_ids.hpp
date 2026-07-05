#pragma once

#include "foundation/id.hpp"

namespace quader::mesh {

struct VertexTag;
struct HalfedgeTag;
struct EdgeTag;
struct FaceTag;
struct AttributeTag;

using VertexId = quader::foundation::GenerationalId<VertexTag>;
using HalfedgeId = quader::foundation::GenerationalId<HalfedgeTag>;
using EdgeId = quader::foundation::GenerationalId<EdgeTag>;
using FaceId = quader::foundation::GenerationalId<FaceTag>;
using AttributeId = quader::foundation::Id<AttributeTag>;

} // namespace quader::mesh
