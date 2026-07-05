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

struct TextureSlotMapping {
    std::string slot_name;
    GltfTextureRef source;
    crimson::TextureColorSpace expected_color_space = crimson::TextureColorSpace::Linear;
};

struct GltfMaterialMapping {
    crimson::MaterialInstance material;
    std::vector<TextureSlotMapping> texture_slots;
    ImportedMaterialMetadata metadata;
    IoDiagnosticList diagnostics;
};

[[nodiscard]] quader::foundation::Result<GltfMaterialMapping, IoError> map_gltf_material_to_crimson(
    const GltfMaterialSource& source,
    const crimson::BaseShaderRegistry& registry);

} // namespace quader::io::gltf
