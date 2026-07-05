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

#include "crimson/material/base_shader.hpp"
#include "crimson/material/material_instance.hpp"

#include <string_view>

namespace crimson {

/// Display/debug name used for Quader's renderer default material.
inline constexpr std::string_view kDefaultQuaderMaterialName = "Default Quader Material";
/// Runtime-asset-relative path to the reference default albedo texture.
inline constexpr std::string_view kDefaultQuaderAlbedoTexturePath = "materials/default/default_albedo.png";
/// Runtime-asset-relative path to the reference default roughness texture.
inline constexpr std::string_view kDefaultQuaderRoughnessTexturePath = "materials/default/default_roughness.png";

/**
 * Build Quader's default material instance for a compatible surface shader schema.
 *
 * @param definition Base shader definition used for schema defaults.
 * @param albedo_texture Texture handle bound to the `base_color` texture slot when declared.
 * @return Default material instance using Quader defaults for fields declared by the shader.
 */
[[nodiscard]] MaterialInstance make_default_quader_material_instance(
		const BaseShaderDefinition &definition,
		RenderTextureHandle albedo_texture);

} // namespace crimson
