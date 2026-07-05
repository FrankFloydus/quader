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

#include "foundation/diagnostic.hpp"

#include <string>
#include <utility>

namespace quader::mesh {

/// Mesh-core error categories returned by expected-failure APIs.
enum class MeshErrorCode {
	/// Vertex identifier is invalid, stale, or not present.
	InvalidVertex,
	/// Halfedge identifier is invalid, stale, or not present.
	InvalidHalfedge,
	/// Edge identifier is invalid, stale, or not present.
	InvalidEdge,
	/// Face identifier is invalid, stale, or not present.
	InvalidFace,
	/// Attribute identifier is invalid or not present.
	InvalidAttribute,
	/// Attribute name already exists in the requested domain.
	AttributeNameAlreadyExists,
	/// Attribute storage type does not match the requested value type.
	AttributeTypeMismatch,
	/// Attribute slot index is outside the attribute array.
	AttributeIndexOutOfRange,
	/// Face creation received fewer than three vertices.
	FaceNeedsAtLeastThreeVertices,
	/// Geometry contains a NaN or infinity coordinate.
	NonFinitePosition,
	/// Face geometry has degenerate area.
	DegenerateFace,
	/// Vertex cannot be deleted because topology still references it.
	VertexStillReferenced,
	/// Face traversal detected an invalid cycle.
	BrokenFaceCycle,
	/// Topology would become or already is non-manifold.
	NonManifoldTopology,
	/// Backend mesh storage rejected a face operation.
	BackendRejectedFace,
	/// Quader ID to backend-handle mapping is inconsistent.
	BackendMappingCorrupt,
	/// Storage compaction failed to preserve mesh mappings.
	CompactionFailed,
	/// Input geometry failed validation.
	InvalidGeometry,
};

/// Structured mesh error with a code and user/developer diagnostic.
struct MeshError {
	/// Machine-readable mesh error category.
	MeshErrorCode code = MeshErrorCode::InvalidVertex;
	/// Human-readable diagnostic associated with the error.
	quader::foundation::Diagnostic diagnostic;
};

/**
 * Create a mesh error from a code and message.
 *
 * @param code Mesh error category.
 * @param message Diagnostic message to store.
 * @return Error with `Severity::Error`.
 */
[[nodiscard]] inline MeshError make_mesh_error(MeshErrorCode code, std::string message) {
	return MeshError{
		code,
		quader::foundation::Diagnostic{
				quader::foundation::Severity::Error,
				std::move(message),
		},
	};
}

} // namespace quader::mesh
