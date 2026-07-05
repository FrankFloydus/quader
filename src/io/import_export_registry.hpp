#pragma once

#include "foundation/result.hpp"
#include "io/exporter.hpp"
#include "io/importer.hpp"
#include "io/io_error.hpp"

#include <filesystem>
#include <memory>
#include <vector>

namespace quader::io {

class ImportExportRegistry final {
public:
	[[nodiscard]] quader::foundation::Result<void, IoError> register_importer(
			std::unique_ptr<IImporter> importer);
	[[nodiscard]] quader::foundation::Result<void, IoError> register_exporter(
			std::unique_ptr<IExporter> exporter);

	[[nodiscard]] const IImporter *importer_for_path(const std::filesystem::path &path) const;
	[[nodiscard]] const IExporter *exporter_for_path(const std::filesystem::path &path) const;
	[[nodiscard]] std::vector<FileFormatDescriptor> import_formats() const;
	[[nodiscard]] std::vector<FileFormatDescriptor> export_formats() const;

private:
	std::vector<std::unique_ptr<IImporter>> importers_;
	std::vector<std::unique_ptr<IExporter>> exporters_;
};

} // namespace quader::io
