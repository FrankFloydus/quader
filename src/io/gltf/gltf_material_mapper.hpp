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

#include "crimson/material/base_shader_registry.hpp"
#include "crimson/material/material_instance.hpp"
#include "crimson/material/material_texture.hpp"
#include "foundation/result.hpp"
#include "io/gltf/gltf_material_source.hpp"
#include "io/imported_document.hpp"
#include "io/io_diagnostic.hpp"
#include "io/io_error.hpp"

#include <string>
#include <vector>

namespace quader::io::gltf {

/// Mapping from a glTF texture reference to a Crimson material texture slot.
struct TextureSlotMapping {
	/// Crimson material texture slot name.
	std::string slot_name;
	/// Source glTF texture reference.
	GltfTextureRef source;
	/// Color-space expectation for the texture data.
	crimson::TextureColorSpace expected_color_space = crimson::TextureColorSpace::Linear;
};

/// Result payload for mapping one glTF material to Crimson.
struct GltfMaterialMapping {
	/// Crimson material instance configured from source material fields.
	crimson::MaterialInstance material;
	/// Texture slot mappings extracted from the source material.
	std::vector<TextureSlotMapping> texture_slots;
	/// Source metadata preserved for the imported document.
	ImportedMaterialMetadata metadata;
	/// Non-fatal diagnostics emitted while mapping.
	IoDiagnosticList diagnostics;
};

/**
 * Map a glTF material source into a Crimson material instance.
 *
 * @param source Source glTF material fields.
 * @param registry Base shader registry used to choose material schema.
 * @return Mapped material payload, or an I/O material-mapping error.
 */
[[nodiscard]] quader::foundation::Result<GltfMaterialMapping, IoError> map_gltf_material_to_crimson(
		const GltfMaterialSource &source,
		const crimson::BaseShaderRegistry &registry);

} // namespace quader::io::gltf
