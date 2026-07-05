#pragma once

#include "io/io_diagnostic.hpp"

#include <filesystem>
#include <string>
#include <utility>

namespace quader::io {

enum class IoErrorCode {
    UnsupportedFormat,
    FileAccessFailed,
    ParseFailed,
    ValidationFailed,
    ExportFailed,
};

struct IoError {
    IoErrorCode code = IoErrorCode::UnsupportedFormat;
    IoDiagnostic diagnostic;
    IoDiagnosticList related;
};

[[nodiscard]] inline IoError make_io_error(IoErrorCode code, IoDiagnostic diagnostic)
{
    IoError error;
    error.code = code;
    error.diagnostic = std::move(diagnostic);
    error.related.add(error.diagnostic);
    return error;
}

[[nodiscard]] inline IoError make_io_error(IoErrorCode error_code,
                                           IoDiagnosticCode diagnostic_code,
                                           std::string message,
                                           std::filesystem::path source_path = {},
                                           std::string subject = {})
{
    return make_io_error(
        error_code,
        IoDiagnostic{
            IoSeverity::Error,
            diagnostic_code,
            std::move(message),
            std::move(source_path),
            {},
            std::move(subject),
        });
}

} // namespace quader::io
