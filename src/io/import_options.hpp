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

namespace quader::io {

/// Import mode requested by the caller.
enum class DocumentImportMode {
	/// Replace the current document with the imported document.
	ReplaceDocument,
	/// Append an imported fragment to the current document.
	AppendFragment,
};

/// Options that control import payload shape and validation.
struct ImportOptions {
	/// Whether the importer should produce replacement or append payloads.
	DocumentImportMode mode = DocumentImportMode::AppendFragment;
	/// Preserve unsupported material metadata for diagnostics or later export.
	bool preserve_unknown_material_metadata = true;
	/// Validate imported meshes before reporting success.
	bool validate_meshes_before_success = true;
};

} // namespace quader::io
