#pragma once

#include "crimson/material/material_parameter.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace quader::io::gltf {

enum class GltfAlphaMode {
    Opaque,
    Mask,
    Blend,
};

struct GltfTextureRef {
    std::string source_uri;
    std::int32_t texcoord = 0;

    friend bool operator==(const GltfTextureRef&, const GltfTextureRef&) = default;
};

struct GltfPbrMetallicRoughness {
    crimson::MaterialColorSrgb base_color_factor{1.0F, 1.0F, 1.0F, 1.0F};
    std::optional<GltfTextureRef> base_color_texture;
    float metallic_factor = 1.0F;
    float roughness_factor = 1.0F;
    std::optional<GltfTextureRef> metallic_roughness_texture;
};

struct GltfMaterialSource {
    std::string name;
    GltfPbrMetallicRoughness pbr;
    std::optional<GltfTextureRef> normal_texture;
    float normal_scale = 1.0F;
    std::optional<GltfTextureRef> occlusion_texture;
    float occlusion_strength = 1.0F;
    crimson::MaterialColorSrgb emissive_factor{0.0F, 0.0F, 0.0F, 1.0F};
    std::optional<GltfTextureRef> emissive_texture;
    GltfAlphaMode alpha_mode = GltfAlphaMode::Opaque;
    float alpha_cutoff = 0.5F;
    bool double_sided = false;
    std::vector<std::string> unsupported_extensions;
};

} // namespace quader::io::gltf
