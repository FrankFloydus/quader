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
#include "io/exporter.hpp"
#include "io/import_export_registry.hpp"
#include "io/io_error.hpp"

namespace quader::io {

/// Service that selects an exporter from the registry and runs it.
class ExportService final {
public:
	/**
	 * Create an export service over a registry.
	 *
	 * @param registry Registry borrowed for the service lifetime.
	 */
	explicit ExportService(const ImportExportRegistry &registry);

	/**
	 * Export a document using the exporter matching the request path.
	 *
	 * @param request Export request and borrowed document.
	 * @return Export result, or an I/O error when no exporter can handle it.
	 */
	[[nodiscard]] quader::foundation::Result<ExportResult, IoError> export_file(
			const ExportRequest &request) const;

private:
	const ImportExportRegistry &registry_;
};

} // namespace quader::io
