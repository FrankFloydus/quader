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

#include "io/io_diagnostic.hpp"

#include <filesystem>
#include <string>
#include <utility>

namespace quader::io {

/// Expected failure categories for import/export services.
enum class IoErrorCode {
	/// No registered importer/exporter supports the requested format.
	UnsupportedFormat,
	/// Filesystem access failed.
	FileAccessFailed,
	/// Source data could not be parsed.
	ParseFailed,
	/// Imported or exported data failed validation.
	ValidationFailed,
	/// Export operation failed.
	ExportFailed,
};

/// Structured I/O error with primary and related diagnostics.
struct IoError {
	/// Machine-readable error category.
	IoErrorCode code = IoErrorCode::UnsupportedFormat;
	/// Primary diagnostic for the error.
	IoDiagnostic diagnostic;
	/// Related diagnostics, including the primary diagnostic.
	IoDiagnosticList related;
};

/**
 * Create an I/O error from an existing diagnostic.
 *
 * @param code Error category.
 * @param diagnostic Primary diagnostic.
 * @return Error whose related list includes the primary diagnostic.
 */
[[nodiscard]] inline IoError make_io_error(IoErrorCode code, IoDiagnostic diagnostic) {
	IoError error;
	error.code = code;
	error.diagnostic = std::move(diagnostic);
	error.related.add(error.diagnostic);
	return error;
}

/**
 * Create an I/O error from diagnostic fields.
 *
 * @param error_code Error category.
 * @param diagnostic_code Diagnostic category.
 * @param message Diagnostic message.
 * @param source_path Optional path associated with the error.
 * @param subject Optional subject associated with the error.
 * @return Error whose related list includes the primary diagnostic.
 */
[[nodiscard]] inline IoError make_io_error(IoErrorCode error_code,
		IoDiagnosticCode diagnostic_code,
		std::string message,
		std::filesystem::path source_path = {},
		std::string subject = {}) {
	return make_io_error(
			error_code,
			IoDiagnostic{
					IoSeverity::Error,
					diagnostic_code,
					std::move(message),
					std::move(source_path),
					{},
					std::move(subject),
			});
}

} // namespace quader::io
