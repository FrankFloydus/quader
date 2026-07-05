#pragma once

#include "foundation/result.hpp"
#include "io/exporter.hpp"
#include "io/import_export_registry.hpp"
#include "io/io_error.hpp"

namespace quader::io {

class ExportService final {
public:
	explicit ExportService(const ImportExportRegistry &registry);

	[[nodiscard]] quader::foundation::Result<ExportResult, IoError> export_file(
			const ExportRequest &request) const;

private:
	const ImportExportRegistry &registry_;
};

} // namespace quader::io
