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
#include "crimson/material/material_texture.hpp"
#include "crimson/renderer_types.hpp"

#include <string>
#include <vector>

namespace crimson {

/// Per-material render-state overrides outside the base shader schema.
struct MaterialRenderOverrides {
	/// True when back-face culling should be disabled.
	bool double_sided = false;

	friend bool operator==(const MaterialRenderOverrides &, const MaterialRenderOverrides &) = default;
};

/// Material instance bound to one base shader and its declared parameters.
struct MaterialInstance {
	/// Developer-facing material name.
	std::string debug_name;
	/// Base shader schema used to validate parameters and textures.
	BaseShaderId base_shader_id = BaseShaderId::OpaquePbr;
	/// Material parameter values.
	std::vector<MaterialParameter> parameters;
	/// Texture bindings keyed by slot name.
	std::vector<MaterialTextureBinding> textures;
	/// Render-state overrides.
	MaterialRenderOverrides overrides;
};

} // namespace crimson
