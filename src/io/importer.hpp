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

#include "foundation/result.hpp"
#include "io/file_format.hpp"
#include "io/import_options.hpp"
#include "io/imported_document.hpp"
#include "io/io_diagnostic.hpp"
#include "io/io_error.hpp"

#include <filesystem>
#include <variant>

namespace quader::io {

/// Request passed to an importer.
struct ImportRequest {
	/// Source path selected by the caller.
	std::filesystem::path path;
	/// Import options chosen by the caller.
	ImportOptions options;
};

/// Result returned by a successful import operation.
struct ImportResult {
	/// Imported payload, either a full document or appendable fragment.
	std::variant<ImportedDocument, DocumentFragment> payload;
	/// Non-fatal diagnostics produced while importing.
	IoDiagnosticList diagnostics;
};

/// Interface implemented by project-owned document importers.
class IImporter {
public:
	/// Destroy an importer through the interface.
	virtual ~IImporter() = default;

	/**
	 * Return the file format handled by this importer.
	 *
	 * @return Format descriptor by value.
	 */
	[[nodiscard]] virtual FileFormatDescriptor descriptor() const = 0;
	/**
	 * Check whether this importer accepts a path.
	 *
	 * @param path Source path to test.
	 * @return True when the importer can handle the path extension.
	 */
	[[nodiscard]] virtual bool can_import(const std::filesystem::path &path) const = 0;
	/**
	 * Import a document or fragment from disk.
	 *
	 * @param request Import request.
	 * @return Import payload, or an I/O error.
	 */
	[[nodiscard]] virtual quader::foundation::Result<ImportResult, IoError> import_file(
			const ImportRequest &request) const = 0;
};

} // namespace quader::io
