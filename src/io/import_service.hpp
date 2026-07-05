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
#include "io/import_export_registry.hpp"
#include "io/importer.hpp"
#include "io/io_error.hpp"

namespace quader::io {

/// Service that selects an importer from the registry and runs it.
class ImportService final {
public:
	/**
	 * Create an import service over a registry.
	 *
	 * @param registry Registry borrowed for the service lifetime.
	 */
	explicit ImportService(const ImportExportRegistry &registry);

	/**
	 * Import a path using the importer matching the request path.
	 *
	 * @param request Import request.
	 * @return Import result, or an I/O error when no importer can handle it.
	 */
	[[nodiscard]] quader::foundation::Result<ImportResult, IoError> import_file(
			const ImportRequest &request) const;

private:
	const ImportExportRegistry &registry_;
};

} // namespace quader::io
