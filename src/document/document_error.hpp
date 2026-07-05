#pragma once

#include "foundation/diagnostic.hpp"

#include <string>
#include <utility>

namespace quader::document {

enum class DocumentErrorCode {
	InvalidObject,
	ObjectAlreadyExists,
	InvalidMesh,
	InvalidTransform,
	InvalidSelection,
	InvalidVertex,
	InvalidEdge,
	InvalidFace,
};

struct DocumentError {
	DocumentErrorCode code = DocumentErrorCode::InvalidObject;
	quader::foundation::Diagnostic diagnostic;
};

[[nodiscard]] inline DocumentError make_document_error(DocumentErrorCode code, std::string message) {
	return DocumentError{
		code,
		quader::foundation::Diagnostic{
				quader::foundation::Severity::Error,
				std::move(message),
		},
	};
}

} // namespace quader::document
