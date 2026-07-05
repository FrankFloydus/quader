#pragma once

#include "foundation/result.hpp"
#include "io/file_format.hpp"
#include "io/imported_document.hpp"
#include "io/import_options.hpp"
#include "io/io_diagnostic.hpp"
#include "io/io_error.hpp"

#include <filesystem>
#include <variant>

namespace quader::io {

struct ImportRequest {
    std::filesystem::path path;
    ImportOptions options;
};

struct ImportResult {
    std::variant<ImportedDocument, DocumentFragment> payload;
    IoDiagnosticList diagnostics;
};

class IImporter {
public:
    virtual ~IImporter() = default;

    [[nodiscard]] virtual FileFormatDescriptor descriptor() const = 0;
    [[nodiscard]] virtual bool can_import(const std::filesystem::path& path) const = 0;
    [[nodiscard]] virtual quader::foundation::Result<ImportResult, IoError> import_file(
        const ImportRequest& request) const = 0;
};

} // namespace quader::io
