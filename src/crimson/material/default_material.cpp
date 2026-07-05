/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
#include "crimson/material/default_material.hpp"

#include "crimson/material/material_system.hpp"

#include <string>
#include <utility>

namespace crimson {
namespace {

void set_parameter(MaterialInstance &instance, std::string_view name, MaterialParameterValue value) {
	for (MaterialParameter &parameter : instance.parameters) {
		if (parameter.name == name) {
			parameter.value = value;
			return;
		}
	}
	instance.parameters.push_back(MaterialParameter{
			.name = std::string(name),
			.value = std::move(value),
	});
}

void set_texture(MaterialInstance &instance, std::string_view name, RenderTextureHandle texture) {
	for (MaterialTextureBinding &binding : instance.textures) {
		if (binding.name == name) {
			binding.texture = texture;
			return;
		}
	}
	instance.textures.push_back(MaterialTextureBinding{
			.name = std::string(name),
			.texture = texture,
	});
}

} // namespace

MaterialInstance make_default_quader_material_instance(
		const BaseShaderDefinition &definition,
		RenderTextureHandle albedo_texture) {
	MaterialInstance instance = make_default_material_instance(definition);
	instance.debug_name = std::string(kDefaultQuaderMaterialName);
	if (find_parameter_desc(definition, "base_color") != nullptr) {
		set_parameter(instance, "base_color", MaterialColorSrgb{ 1.0F, 1.0F, 1.0F, 1.0F });
	}
	if (find_parameter_desc(definition, "roughness") != nullptr) {
		set_parameter(instance, "roughness", 1.0F);
	}
	if (find_parameter_desc(definition, "metallic") != nullptr) {
		set_parameter(instance, "metallic", 0.0F);
	}
	if (find_texture_slot_desc(definition, "base_color") != nullptr) {
		set_texture(instance, "base_color", albedo_texture);
	}
	return instance;
}

} // namespace crimson
