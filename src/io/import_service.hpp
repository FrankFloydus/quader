#pragma once

#include "foundation/result.hpp"
#include "io/import_export_registry.hpp"
#include "io/importer.hpp"
#include "io/io_error.hpp"

namespace quader::io {

class ImportService final {
public:
    explicit ImportService(const ImportExportRegistry& registry);

    [[nodiscard]] quader::foundation::Result<ImportResult, IoError> import_file(
        const ImportRequest& request) const;

private:
    const ImportExportRegistry& registry_;
};

} // namespace quader::io
