#pragma once

#include "foundation/result.hpp"
#include "document/document.hpp"
#include "io/export_options.hpp"
#include "io/file_format.hpp"
#include "io/io_diagnostic.hpp"
#include "io/io_error.hpp"

#include <filesystem>

namespace quader::io {

struct ExportRequest {
    std::filesystem::path path;
    const quader::document::Document& document;
    ExportOptions options;
};

struct ExportResult {
    IoDiagnosticList diagnostics;
};

class IExporter {
public:
    virtual ~IExporter() = default;

    [[nodiscard]] virtual FileFormatDescriptor descriptor() const = 0;
    [[nodiscard]] virtual bool can_export(const std::filesystem::path& path) const = 0;
    [[nodiscard]] virtual quader::foundation::Result<ExportResult, IoError> export_file(
        const ExportRequest& request) const = 0;
};

} // namespace quader::io
