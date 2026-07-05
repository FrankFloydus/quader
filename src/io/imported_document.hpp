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

#include "document/document.hpp"
#include "document/transform.hpp"
#include "foundation/result.hpp"
#include "io/io_diagnostic.hpp"
#include "io/io_error.hpp"
#include "mesh/core/polyhedron.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace quader::io {

/// Material metadata preserved from an imported source file.
struct ImportedMaterialMetadata {
	/// Source material display name.
	std::string source_name;
	/// Source format identifier.
	std::string source_format;
	/// Source material identifier when provided by the format.
	std::string source_material_id;
	/// Unsupported extension names preserved for future export or diagnostics.
	std::vector<std::string> preserved_extensions;
	/// Material-specific import diagnostics.
	IoDiagnosticList diagnostics;
};

/// Mesh object payload imported before insertion into a `Document`.
struct ImportedMeshObject {
	/// Object name from the source file or importer fallback.
	std::string name;
	/// Mesh data owned by the imported object.
	quader::mesh::Polyhedron mesh;
	/// Object transform in document space.
	quader::document::Transform transform;
	/// Optional index into `DocumentFragment::materials`.
	std::optional<std::size_t> material_index;
};

/// Imported document subtree that can be appended to an existing document.
struct DocumentFragment {
	/// Mesh objects contained in the fragment.
	std::vector<ImportedMeshObject> mesh_objects;
	/// Material metadata referenced by imported objects.
	std::vector<ImportedMaterialMetadata> materials;
	/// Fragment-level import diagnostics.
	IoDiagnosticList diagnostics;
};

/// Full imported document payload.
struct ImportedDocument {
	/// Root fragment that becomes document contents.
	DocumentFragment root_fragment;
	/// Source name or path displayed in diagnostics.
	std::string source_name;
	/// Document-level import diagnostics.
	IoDiagnosticList diagnostics;
};

/**
 * Build a `Document` from imported data.
 *
 * @param imported Imported payload to consume.
 * @return Document on success, or an I/O validation error.
 */
[[nodiscard]] quader::foundation::Result<quader::document::Document, IoError>
build_document_from_import(ImportedDocument imported);

} // namespace quader::io
