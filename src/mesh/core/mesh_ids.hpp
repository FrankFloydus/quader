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

#include "foundation/id.hpp"

namespace quader::mesh {

/// Tag for stable mesh vertex identifiers.
struct VertexTag;
/// Tag for stable mesh halfedge identifiers.
struct HalfedgeTag;
/// Tag for stable mesh edge identifiers.
struct EdgeTag;
/// Tag for stable mesh face identifiers.
struct FaceTag;
/// Tag for mesh attribute identifiers.
struct AttributeTag;

/// Generational identifier for a vertex record.
using VertexId = quader::foundation::GenerationalId<VertexTag>;
/// Generational identifier for a halfedge record.
using HalfedgeId = quader::foundation::GenerationalId<HalfedgeTag>;
/// Generational identifier for an edge record.
using EdgeId = quader::foundation::GenerationalId<EdgeTag>;
/// Generational identifier for a face record.
using FaceId = quader::foundation::GenerationalId<FaceTag>;
/// Index-only identifier for an attribute descriptor.
using AttributeId = quader::foundation::Id<AttributeTag>;

} // namespace quader::mesh
