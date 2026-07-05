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

namespace quader::document {

/// Error categories reported by document validation and mutation APIs.
enum class DocumentErrorCode {
	InvalidObject,       ///< Object id is unknown, stale, or otherwise unusable.
	ObjectAlreadyExists, ///< A restore operation targets an already-live object slot.
	InvalidMesh,         ///< Mesh topology or geometry failed document validation.
	InvalidTransform,    ///< Transform contains non-finite values.
	InvalidMaterial,     ///< Material values are non-finite or outside accepted ranges.
	InvalidSelection,    ///< Selection payload cannot be represented by the document.
	InvalidVertex,       ///< Vertex component reference is invalid.
	InvalidEdge,         ///< Edge component reference is invalid.
	InvalidFace,         ///< Face component reference is invalid.
};

/// Structured document-layer error with a user-presentable diagnostic.
struct DocumentError {
	/// Machine-readable error category.
	DocumentErrorCode code = DocumentErrorCode::InvalidObject;
	/// Diagnostic message and severity for callers that surface failures.
	quader::foundation::Diagnostic diagnostic;
};

/**
 * Build a document error diagnostic with error severity.
 *
 * @param code Error category.
 * @param message Diagnostic message to move into the error.
 * @return Document error carrying `code` and `message`.
 */
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
