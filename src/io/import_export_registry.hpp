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
#include "io/importer.hpp"
#include "io/io_error.hpp"

#include <filesystem>
#include <memory>
#include <vector>

namespace quader::io {

/// Owns registered importers and exporters and resolves them by path.
class ImportExportRegistry final {
public:
	/**
	 * Register an importer.
	 *
	 * @param importer Importer to take ownership of.
	 * @return Success, or an I/O error for invalid registration.
	 */
	[[nodiscard]] quader::foundation::Result<void, IoError> register_importer(
			std::unique_ptr<IImporter> importer);
	/**
	 * Register an exporter.
	 *
	 * @param exporter Exporter to take ownership of.
	 * @return Success, or an I/O error for invalid registration.
	 */
	[[nodiscard]] quader::foundation::Result<void, IoError> register_exporter(
			std::unique_ptr<IExporter> exporter);

	/**
	 * Resolve an importer by path extension.
	 *
	 * @param path Source path to match.
	 * @return Borrowed importer, or `nullptr` when none matches.
	 */
	[[nodiscard]] const IImporter *importer_for_path(const std::filesystem::path &path) const;
	/**
	 * Resolve an exporter by path extension.
	 *
	 * @param path Destination path to match.
	 * @return Borrowed exporter, or `nullptr` when none matches.
	 */
	[[nodiscard]] const IExporter *exporter_for_path(const std::filesystem::path &path) const;
	/**
	 * Return registered import format descriptors.
	 *
	 * @return Descriptors copied from registered importers.
	 */
	[[nodiscard]] std::vector<FileFormatDescriptor> import_formats() const;
	/**
	 * Return registered export format descriptors.
	 *
	 * @return Descriptors copied from registered exporters.
	 */
	[[nodiscard]] std::vector<FileFormatDescriptor> export_formats() const;

private:
	std::vector<std::unique_ptr<IImporter>> importers_;
	std::vector<std::unique_ptr<IExporter>> exporters_;
};

} // namespace quader::io
