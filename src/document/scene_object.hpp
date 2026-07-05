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

#include "document/material.hpp"
#include "document/object_id.hpp"
#include "document/transform.hpp"
#include "mesh/core/polyhedron.hpp"

#include <string>

namespace quader::document {

/// Scene object categories currently supported by the document.
enum class SceneObjectKind {
	Mesh, ///< Editable mesh object.
};

/// Editable mesh object stored by the document.
struct MeshObject {
	/// Stable object id assigned by the owning document.
	ObjectId id;
	/// User-visible object name.
	std::string name;
	/// Object-to-world transform.
	Transform transform;
	/// Object material values consumed by render adapters.
	PbrMaterial material;
	/// Editable polygon mesh owned by this object.
	quader::mesh::Polyhedron mesh;
};

} // namespace quader::document
