#pragma once

#include "document/object_id.hpp"
#include "document/transform.hpp"
#include "mesh/core/polyhedron.hpp"

#include <string>

namespace quader::document {

enum class SceneObjectKind {
	Mesh,
};

struct MeshObject {
	ObjectId id;
	std::string name;
	Transform transform;
	quader::mesh::Polyhedron mesh;
};

} // namespace quader::document
