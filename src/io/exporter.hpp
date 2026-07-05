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

#include "document/document.hpp"
#include "foundation/result.hpp"
#include "io/export_options.hpp"
#include "io/file_format.hpp"
#include "io/io_diagnostic.hpp"
#include "io/io_error.hpp"

#include <filesystem>

namespace quader::io {

/// Request passed to an exporter.
struct ExportRequest {
	/// Destination path selected by the caller.
	std::filesystem::path path;
	/// Document to serialize; borrowed for the duration of the call.
	const quader::document::Document &document;
	/// Export options chosen by the caller.
	ExportOptions options;
};

/// Result returned by a successful export operation.
struct ExportResult {
	/// Non-fatal diagnostics produced while exporting.
	IoDiagnosticList diagnostics;
};

/// Interface implemented by project-owned document exporters.
class IExporter {
public:
	/// Destroy an exporter through the interface.
	virtual ~IExporter() = default;

	/**
	 * Return the file format handled by this exporter.
	 *
	 * @return Format descriptor by value.
	 */
	[[nodiscard]] virtual FileFormatDescriptor descriptor() const = 0;
	/**
	 * Check whether this exporter accepts a path.
	 *
	 * @param path Destination path to test.
	 * @return True when the exporter can handle the path extension.
	 */
	[[nodiscard]] virtual bool can_export(const std::filesystem::path &path) const = 0;
	/**
	 * Export a document to disk.
	 *
	 * @param request Export request and borrowed document.
	 * @return Export result, or an I/O error.
	 */
	[[nodiscard]] virtual quader::foundation::Result<ExportResult, IoError> export_file(
			const ExportRequest &request) const = 0;
};

} // namespace quader::io
