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

struct ImportedMaterialMetadata {
	std::string source_name;
	std::string source_format;
	std::string source_material_id;
	std::vector<std::string> preserved_extensions;
	IoDiagnosticList diagnostics;
};

struct ImportedMeshObject {
	std::string name;
	quader::mesh::Polyhedron mesh;
	quader::document::Transform transform;
	std::optional<std::size_t> material_index;
};

struct DocumentFragment {
	std::vector<ImportedMeshObject> mesh_objects;
	std::vector<ImportedMaterialMetadata> materials;
	IoDiagnosticList diagnostics;
};

struct ImportedDocument {
	DocumentFragment root_fragment;
	std::string source_name;
	IoDiagnosticList diagnostics;
};

[[nodiscard]] quader::foundation::Result<quader::document::Document, IoError>
build_document_from_import(ImportedDocument imported);

} // namespace quader::io
