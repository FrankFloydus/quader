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

#include <span>
#include <string>
#include <vector>

namespace quader::mesh {

class Polyhedron;

/// Mesh validation issue categories.
enum class MeshValidationCode {
	/// A Quader identifier is invalid or stale.
	InvalidId,
	/// A live Quader element has no backend handle.
	MissingBackendHandle,
	/// A live backend element has no Quader ID.
	MissingQuaderId,
	/// Forward and reverse backend mappings disagree.
	ReverseMappingMismatch,
	/// A deleted element is still present in an active mapping.
	DeletedElementMapped,
	/// Attribute array size does not match its element domain.
	AttributeDomainMismatch,
	/// Opposite halfedge links are not reciprocal.
	BrokenOppositePair,
	/// Next/previous halfedge links are not reciprocal.
	BrokenNextPrevCycle,
	/// Halfedge face membership does not match the face cycle.
	FaceCycleMismatch,
	/// Vertex reference is missing or inconsistent.
	VertexReferenceMismatch,
	/// Edge reference is missing or inconsistent.
	EdgeReferenceMismatch,
	/// Boundary placeholder representation is inconsistent.
	BoundaryPlaceholderMismatch,
	/// Face has degenerate geometric area.
	DegenerateFace,
	/// Vertex position contains NaN or infinity.
	NonFinitePosition,
	/// Face cycle traversal failed.
	BrokenFaceCycle,
};

/// One mesh validation issue.
struct MeshValidationIssue {
	/// Machine-readable validation category.
	MeshValidationCode code = MeshValidationCode::InvalidId;
	/// Human-readable validation diagnostic.
	quader::foundation::Diagnostic diagnostic;
};

/// Collection of issues produced by mesh validation.
class MeshValidationReport final {
public:
	/**
	 * Check whether validation found no issues.
	 *
	 * @return True when the issue list is empty.
	 */
	[[nodiscard]] bool ok() const noexcept {
		return issues_.empty();
	}

	/**
	 * Return validation issues.
	 *
	 * @return Borrowed span valid until the report is mutated or destroyed.
	 */
	[[nodiscard]] std::span<const MeshValidationIssue> issues() const noexcept {
		return issues_;
	}

	/**
	 * Append a validation issue.
	 *
	 * @param code Issue category.
	 * @param message Diagnostic message to store.
	 */
	void add_issue(MeshValidationCode code, const std::string &message) {
		issues_.push_back(MeshValidationIssue{
				code,
				quader::foundation::Diagnostic{ quader::foundation::Severity::Error, message },
		});
	}

private:
	std::vector<MeshValidationIssue> issues_;
};

/**
 * Validate mesh topology, mappings, attributes, and geometry invariants.
 *
 * @param mesh Mesh to inspect.
 * @return Validation report containing every issue detected by the pass.
 */
[[nodiscard]] MeshValidationReport validate_mesh(const Polyhedron &mesh);

} // namespace quader::mesh
