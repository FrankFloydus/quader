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

#include "io/imported_document.hpp"
#include "io/io_diagnostic.hpp"

namespace quader::io {

/// Validation issue categories for imported document fragments.
enum class FragmentValidationCode {
	/// Fragment contains no importable content.
	EmptyFragment,
	/// Fragment contains invalid mesh data.
	InvalidMesh,
	/// Fragment contains an invalid object transform.
	InvalidTransform,
	/// Fragment references a material index that is not present.
	InvalidMaterialReference,
};

/// Result of validating an imported document fragment.
struct FragmentValidationResult {
	/// Diagnostics accumulated during validation.
	IoDiagnosticList diagnostics;

	/**
	 * Check whether validation succeeded.
	 *
	 * @return True when no error diagnostics are present.
	 */
	[[nodiscard]] bool ok() const noexcept;
};

/**
 * Validate imported fragment data before document insertion.
 *
 * @param fragment Fragment to inspect.
 * @return Validation diagnostics and success state.
 */
[[nodiscard]] FragmentValidationResult validate_document_fragment(const DocumentFragment &fragment);

} // namespace quader::io
