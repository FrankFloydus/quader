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

#include "crimson/material/material_parameter.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace quader::io::gltf {

/// glTF alpha mode values used for material base-shader selection.
enum class GltfAlphaMode {
	/// Fully opaque material.
	Opaque,
	/// Alpha-tested material using `alpha_cutoff`.
	Mask,
	/// Alpha-blended transparent material.
	Blend,
};

/// Reference to a texture in a glTF source material.
struct GltfTextureRef {
	/// Texture URI from the source file.
	std::string source_uri;
	/// Texture coordinate set index.
	std::int32_t texcoord = 0;

	/// Compare texture references by URI and texture coordinate set.
	friend bool operator==(const GltfTextureRef &, const GltfTextureRef &) = default;
};

/// glTF metallic/roughness material fields.
struct GltfPbrMetallicRoughness {
	/// Base color factor in sRGB space.
	crimson::MaterialColorSrgb base_color_factor{ 1.0F, 1.0F, 1.0F, 1.0F };
	/// Optional base color texture.
	std::optional<GltfTextureRef> base_color_texture;
	/// Metallic factor in the glTF material model.
	float metallic_factor = 1.0F;
	/// Roughness factor in the glTF material model.
	float roughness_factor = 1.0F;
	/// Optional packed metallic/roughness texture.
	std::optional<GltfTextureRef> metallic_roughness_texture;
};

/// Project-owned glTF material DTO used before Crimson mapping.
struct GltfMaterialSource {
	/// Source material name.
	std::string name;
	/// Metallic/roughness PBR fields.
	GltfPbrMetallicRoughness pbr;
	/// Optional normal map texture.
	std::optional<GltfTextureRef> normal_texture;
	/// Normal scale factor.
	float normal_scale = 1.0F;
	/// Optional occlusion map texture.
	std::optional<GltfTextureRef> occlusion_texture;
	/// Occlusion strength factor.
	float occlusion_strength = 1.0F;
	/// Emissive factor in sRGB space.
	crimson::MaterialColorSrgb emissive_factor{ 0.0F, 0.0F, 0.0F, 1.0F };
	/// Optional emissive texture.
	std::optional<GltfTextureRef> emissive_texture;
	/// Alpha mode from the source material.
	GltfAlphaMode alpha_mode = GltfAlphaMode::Opaque;
	/// Alpha cutoff used when `alpha_mode` is `GltfAlphaMode::Mask`.
	float alpha_cutoff = 0.5F;
	/// Whether the material should render as double-sided.
	bool double_sided = false;
	/// Unsupported extension names preserved for diagnostics.
	std::vector<std::string> unsupported_extensions;
};

} // namespace quader::io::gltf
