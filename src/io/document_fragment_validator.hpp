#pragma once

#include "io/imported_document.hpp"
#include "io/io_diagnostic.hpp"

namespace quader::io {

enum class FragmentValidationCode {
    EmptyFragment,
    InvalidMesh,
    InvalidTransform,
    InvalidMaterialReference,
};

struct FragmentValidationResult {
    IoDiagnosticList diagnostics;

    [[nodiscard]] bool ok() const noexcept;
};

[[nodiscard]] FragmentValidationResult validate_document_fragment(const DocumentFragment& fragment);

} // namespace quader::io
