/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
#include "mesh/core/detail/openmesh_storage.hpp"

namespace quader::mesh::detail {

OpenMeshStorage::OpenMeshStorage() {
	backend.request_vertex_status();
	backend.request_halfedge_status();
	backend.request_edge_status();
	backend.request_face_status();
}

} // namespace quader::mesh::detail
