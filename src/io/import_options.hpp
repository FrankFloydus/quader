#pragma once

namespace quader::io {

enum class DocumentImportMode {
	ReplaceDocument,
	AppendFragment,
};

struct ImportOptions {
	DocumentImportMode mode = DocumentImportMode::AppendFragment;
	bool preserve_unknown_material_metadata = true;
	bool validate_meshes_before_success = true;
};

} // namespace quader::io
