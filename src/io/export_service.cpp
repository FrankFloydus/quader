#include "io/export_service.hpp"

#include <string>

namespace quader::io {

ExportService::ExportService(const ImportExportRegistry& registry)
    : registry_(registry)
{
}

quader::foundation::Result<ExportResult, IoError> ExportService::export_file(
    const ExportRequest& request) const
{
    const IExporter* exporter = registry_.exporter_for_path(request.path);
    if (exporter == nullptr) {
        return quader::foundation::Result<ExportResult, IoError>::failure(make_io_error(
            IoErrorCode::UnsupportedFormat,
            IoDiagnosticCode::ExportNotSupported,
            "no exporter is registered for '" + path_extension_without_dot(request.path) + "' files",
            request.path));
    }

    return exporter->export_file(request);
}

} // namespace quader::io
