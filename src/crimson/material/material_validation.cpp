/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
#include "crimson/material/material_validation.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace crimson {
namespace {

[[nodiscard]] RendererDiagnostic material_diagnostic(
		RendererDiagnosticCode code,
		std::string message,
		std::string detail,
		std::string resource_name) {
	return RendererDiagnostic{
		.severity = RendererDiagnosticSeverity::Error,
		.code = code,
		.message = std::move(message),
		.detail = std::move(detail),
		.subsystem = "material",
		.resource_name = std::move(resource_name),
	};
}

[[nodiscard]] bool contains_name(const std::vector<std::string> &names, std::string_view name) noexcept {
	for (const std::string &existing : names) {
		if (existing == name) {
			return true;
		}
	}

	return false;
}

} // namespace

quader::foundation::Result<void, RendererDiagnostic> validate_material_instance(
		const BaseShaderRegistry &registry,
		const MaterialInstance &instance) {
	const BaseShaderDefinition *definition = registry.find(instance.base_shader_id);
	if (definition == nullptr) {
		return quader::foundation::Result<void, RendererDiagnostic>::failure(material_diagnostic(
				RendererDiagnosticCode::MaterialSchemaInvalid,
				"Crimson material references an unknown base shader.",
				std::string(base_shader_id_name(instance.base_shader_id)),
				instance.debug_name));
	}

	std::vector<std::string> seen_parameters;
	seen_parameters.reserve(instance.parameters.size());
	for (const MaterialParameter &parameter : instance.parameters) {
		if (contains_name(seen_parameters, parameter.name)) {
			return quader::foundation::Result<void, RendererDiagnostic>::failure(material_diagnostic(
					RendererDiagnosticCode::MaterialValidationFailed,
					"Crimson material contains a duplicate parameter.",
					parameter.name,
					definition->name));
		}
		seen_parameters.push_back(parameter.name);

		const MaterialParameterDesc *desc = find_parameter_desc(*definition, parameter.name);
		if (desc == nullptr) {
			return quader::foundation::Result<void, RendererDiagnostic>::failure(material_diagnostic(
					RendererDiagnosticCode::MaterialValidationFailed,
					"Crimson material contains a parameter that is not declared by its base shader.",
					parameter.name,
					definition->name));
		}

		const MaterialParameterKind kValueKind = material_parameter_value_kind(parameter.value);
		if (kValueKind != desc->kind) {
			return quader::foundation::Result<void, RendererDiagnostic>::failure(material_diagnostic(
					RendererDiagnosticCode::MaterialValidationFailed,
					"Crimson material parameter has the wrong value kind.",
					parameter.name + " expected " + material_parameter_kind_name(desc->kind) + " got " + material_parameter_kind_name(kValueKind),
					definition->name));
		}
	}

	std::vector<std::string> seen_textures;
	seen_textures.reserve(instance.textures.size());
	for (const MaterialTextureBinding &texture : instance.textures) {
		if (contains_name(seen_textures, texture.name)) {
			return quader::foundation::Result<void, RendererDiagnostic>::failure(material_diagnostic(
					RendererDiagnosticCode::MaterialValidationFailed,
					"Crimson material contains a duplicate texture slot binding.",
					texture.name,
					definition->name));
		}
		seen_textures.push_back(texture.name);

		if (find_texture_slot_desc(*definition, texture.name) == nullptr) {
			return quader::foundation::Result<void, RendererDiagnostic>::failure(material_diagnostic(
					RendererDiagnosticCode::MaterialValidationFailed,
					"Crimson material contains a texture slot that is not declared by its base shader.",
					texture.name,
					definition->name));
		}
	}

	return quader::foundation::Result<void, RendererDiagnostic>::success();
}

} // namespace crimson
