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

#include "mesh/core/detail/openmesh_id_map.hpp"
#include "mesh/core/detail/openmesh_traits.hpp"

#include <vector>

namespace quader::mesh::detail {

class OpenMeshStorage final {
public:
	OpenMeshStorage();

	QuaderOpenMesh backend;

	std::vector<OpenMeshIdSlot<QuaderOpenMesh::VertexHandle>> vertices;
	std::vector<OpenMeshIdSlot<QuaderOpenMesh::HalfedgeHandle>> halfedges;
	std::vector<OpenMeshEdgeSlot> edges;
	std::vector<OpenMeshIdSlot<QuaderOpenMesh::FaceHandle>> faces;

	std::vector<quader::foundation::IdIndex> free_vertices;
	std::vector<quader::foundation::IdIndex> free_halfedges;
	std::vector<quader::foundation::IdIndex> free_edges;
	std::vector<quader::foundation::IdIndex> free_faces;

	OpenMeshReverseMap<QuaderOpenMesh::VertexHandle> vertex_reverse;
	OpenMeshReverseMap<QuaderOpenMesh::HalfedgeHandle> halfedge_reverse;
	OpenMeshReverseMap<QuaderOpenMesh::EdgeHandle> edge_reverse;
	OpenMeshReverseMap<QuaderOpenMesh::FaceHandle> face_reverse;
};

} // namespace quader::mesh::detail
