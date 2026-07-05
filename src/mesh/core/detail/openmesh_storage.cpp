#include "mesh/core/detail/openmesh_storage.hpp"

namespace quader::mesh::detail {

OpenMeshStorage::OpenMeshStorage() {
	backend.request_vertex_status();
	backend.request_halfedge_status();
	backend.request_edge_status();
	backend.request_face_status();
}

} // namespace quader::mesh::detail
