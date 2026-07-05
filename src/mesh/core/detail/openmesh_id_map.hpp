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
#include "mesh/core/detail/openmesh_traits.hpp"
#include "mesh/core/mesh_ids.hpp"

#include <vector>

namespace quader::mesh::detail {

template <class Handle>
struct OpenMeshIdSlot {
	quader::foundation::IdGeneration generation = 1;
	bool alive = false;
	Handle handle{ -1 };
};

template <class Handle>
struct OpenMeshReverseMap {
	std::vector<quader::foundation::IdIndex> handle_to_slot;
};

struct OpenMeshEdgeSlot {
	quader::foundation::IdGeneration generation = 1;
	bool alive = false;
	QuaderOpenMesh::EdgeHandle handle{ -1 };
	HalfedgeId representative_halfedge;
};

} // namespace quader::mesh::detail
