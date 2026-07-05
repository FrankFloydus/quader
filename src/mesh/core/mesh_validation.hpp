#pragma once

#include "foundation/diagnostic.hpp"

#include <span>
#include <string>
#include <vector>

namespace quader::mesh {

class Polyhedron;

enum class MeshValidationCode {
	InvalidId,
	MissingBackendHandle,
	MissingQuaderId,
	ReverseMappingMismatch,
	DeletedElementMapped,
	AttributeDomainMismatch,
	BrokenOppositePair,
	BrokenNextPrevCycle,
	FaceCycleMismatch,
	VertexReferenceMismatch,
	EdgeReferenceMismatch,
	BoundaryPlaceholderMismatch,
	DegenerateFace,
	NonFinitePosition,
	BrokenFaceCycle,
};

struct MeshValidationIssue {
	MeshValidationCode code = MeshValidationCode::InvalidId;
	quader::foundation::Diagnostic diagnostic;
};

class MeshValidationReport final {
public:
	[[nodiscard]] bool ok() const noexcept {
		return issues_.empty();
	}

	[[nodiscard]] std::span<const MeshValidationIssue> issues() const noexcept {
		return issues_;
	}

	void add_issue(MeshValidationCode code, const std::string &message) {
		issues_.push_back(MeshValidationIssue{
				code,
				quader::foundation::Diagnostic{ quader::foundation::Severity::Error, message },
		});
	}

private:
	std::vector<MeshValidationIssue> issues_;
};

[[nodiscard]] MeshValidationReport validate_mesh(const Polyhedron &mesh);

} // namespace quader::mesh
