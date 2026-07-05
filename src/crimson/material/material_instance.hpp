#pragma once

#include "crimson/material/material_parameter.hpp"
#include "crimson/material/material_texture.hpp"
#include "crimson/renderer_types.hpp"

#include <string>
#include <vector>

namespace crimson {

struct MaterialRenderOverrides {
	bool double_sided = false;

	friend bool operator==(const MaterialRenderOverrides &, const MaterialRenderOverrides &) = default;
};

struct MaterialInstance {
	std::string debug_name;
	BaseShaderId base_shader_id = BaseShaderId::OpaquePbr;
	std::vector<MaterialParameter> parameters;
	std::vector<MaterialTextureBinding> textures;
	MaterialRenderOverrides overrides;
};

} // namespace crimson
