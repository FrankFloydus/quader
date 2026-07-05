#include "io/gltf/gltf_material_mapper.hpp"

#include "crimson/material/material_system.hpp"
#include "crimson/material/material_validation.hpp"

#include <algorithm>
#include <string_view>
#include <utility>

namespace quader::io::gltf {
namespace {

constexpr std::string_view kGltfSourceFormat = "glTF 2.0";

[[nodiscard]] crimson::BaseShaderId base_shader_for_alpha_mode(GltfAlphaMode mode) noexcept {
	switch (mode) {
		case GltfAlphaMode::Opaque:
			return crimson::BaseShaderId::OpaquePbr;
		case GltfAlphaMode::Mask:
			return crimson::BaseShaderId::AlphaCutoutPbr;
		case GltfAlphaMode::Blend:
			return crimson::BaseShaderId::TransparentPbr;
	}

	return crimson::BaseShaderId::OpaquePbr;
}

[[nodiscard]] IoDiagnostic make_mapping_diagnostic(
		IoSeverity severity,
		IoDiagnosticCode code,
		std::string message,
		std::string subject) {
	return IoDiagnostic{
		severity,
		code,
		std::move(message),
		{},
		{},
		std::move(subject),
	};
}

[[nodiscard]] IoError make_mapping_error(std::string message, const std::string &detail, std::string subject) {
	if (!detail.empty()) {
		message += " ";
		message += detail;
	}

	return make_io_error(
			IoErrorCode::ValidationFailed,
			make_mapping_diagnostic(
					IoSeverity::Error,
					IoDiagnosticCode::MaterialMappingFailed,
					std::move(message),
					std::move(subject)));
}

void add_nonfatal_warning(
		GltfMaterialMapping &mapping,
		std::string message,
		std::string subject) {
	IoDiagnostic diagnostic = make_mapping_diagnostic(
			IoSeverity::Warning,
			IoDiagnosticCode::UnsupportedFeaturePreserved,
			std::move(message),
			std::move(subject));
	mapping.diagnostics.add(diagnostic);
	mapping.metadata.diagnostics.add(std::move(diagnostic));
}

void set_parameter_if_declared(
		crimson::MaterialInstance &material,
		const crimson::BaseShaderDefinition &definition,
		std::string_view name,
		crimson::MaterialParameterValue value) {
	if (crimson::find_parameter_desc(definition, name) == nullptr) {
		return;
	}

	for (crimson::MaterialParameter &parameter : material.parameters) {
		if (parameter.name == name) {
			parameter.value = value;
			return;
		}
	}

	material.parameters.push_back(crimson::MaterialParameter{
			.name = std::string(name),
			.value = value,
	});
}

[[nodiscard]] const std::optional<GltfTextureRef> *texture_source_for_slot(
		const GltfMaterialSource &source,
		std::string_view slot_name) noexcept {
	if (slot_name == "base_color") {
		return &source.pbr.base_color_texture;
	}
	if (slot_name == "metallic_roughness") {
		return &source.pbr.metallic_roughness_texture;
	}
	if (slot_name == "normal") {
		return &source.normal_texture;
	}
	if (slot_name == "occlusion") {
		return &source.occlusion_texture;
	}
	if (slot_name == "emissive") {
		return &source.emissive_texture;
	}

	return nullptr;
}

void append_texture_slot_mappings(
		GltfMaterialMapping &mapping,
		const GltfMaterialSource &source,
		const crimson::BaseShaderDefinition &definition) {
	for (const crimson::MaterialTextureSlotDesc &slot : definition.texture_slots) {
		const std::optional<GltfTextureRef> *texture = texture_source_for_slot(source, slot.name);
		if (texture == nullptr || !texture->has_value()) {
			continue;
		}

		mapping.texture_slots.push_back(TextureSlotMapping{
				.slot_name = slot.name,
				.source = texture->value(),
				.expected_color_space = slot.color_space,
		});
	}
}

void preserve_unsupported_extensions(GltfMaterialMapping &mapping, const GltfMaterialSource &source) {
	mapping.metadata.preserved_extensions = source.unsupported_extensions;
	std::sort(mapping.metadata.preserved_extensions.begin(), mapping.metadata.preserved_extensions.end());
	mapping.metadata.preserved_extensions.erase(
			std::unique(mapping.metadata.preserved_extensions.begin(), mapping.metadata.preserved_extensions.end()),
			mapping.metadata.preserved_extensions.end());

	for (const std::string &extension : mapping.metadata.preserved_extensions) {
		add_nonfatal_warning(
				mapping,
				"Preserved unsupported glTF material extension '" + extension + "'.",
				extension);
	}
}

void preserve_unsupported_occlusion(GltfMaterialMapping &mapping, const GltfMaterialSource &source) {
	const bool kHasUnsupportedOcclusion = source.occlusion_texture.has_value() || source.occlusion_strength != 1.0F;
	if (!kHasUnsupportedOcclusion) {
		return;
	}

	add_nonfatal_warning(
			mapping,
			"Preserved glTF occlusion data because the selected Crimson base shader does not declare an occlusion slot.",
			"occlusion");
}

} // namespace

quader::foundation::Result<GltfMaterialMapping, IoError> map_gltf_material_to_crimson(
		const GltfMaterialSource &source,
		const crimson::BaseShaderRegistry &registry) {
	const crimson::BaseShaderId kBaseShaderId = base_shader_for_alpha_mode(source.alpha_mode);
	const crimson::BaseShaderDefinition *definition = registry.find(kBaseShaderId);
	if (definition == nullptr) {
		return quader::foundation::Result<GltfMaterialMapping, IoError>::failure(make_mapping_error(
				"Crimson base shader schema is unavailable for glTF material mapping.",
				std::string(crimson::base_shader_id_name(kBaseShaderId)),
				source.name));
	}

	GltfMaterialMapping mapping;
	mapping.material = crimson::make_default_material_instance(*definition);
	mapping.material.debug_name = source.name;
	mapping.material.overrides.double_sided = source.double_sided;
	mapping.metadata.source_name = source.name;
	mapping.metadata.source_format = std::string(kGltfSourceFormat);
	mapping.metadata.source_material_id = source.name;

	set_parameter_if_declared(mapping.material, *definition, "base_color", source.pbr.base_color_factor);
	set_parameter_if_declared(mapping.material, *definition, "metallic", source.pbr.metallic_factor);
	set_parameter_if_declared(mapping.material, *definition, "roughness", source.pbr.roughness_factor);
	set_parameter_if_declared(mapping.material, *definition, "normal_strength", source.normal_scale);
	set_parameter_if_declared(mapping.material, *definition, "emissive_color", source.emissive_factor);
	set_parameter_if_declared(mapping.material, *definition, "emissive_strength", 1.0F);
	set_parameter_if_declared(mapping.material, *definition, "double_sided", source.double_sided);

	if (crimson::find_parameter_desc(*definition, "occlusion_strength") != nullptr) {
		set_parameter_if_declared(mapping.material, *definition, "occlusion_strength", source.occlusion_strength);
	} else {
		preserve_unsupported_occlusion(mapping, source);
	}

	if (source.alpha_mode == GltfAlphaMode::Mask) {
		set_parameter_if_declared(mapping.material, *definition, "alpha_cutoff", source.alpha_cutoff);
	}
	if (source.alpha_mode == GltfAlphaMode::Blend) {
		set_parameter_if_declared(mapping.material, *definition, "opacity", source.pbr.base_color_factor.a);
	}

	append_texture_slot_mappings(mapping, source, *definition);
	preserve_unsupported_extensions(mapping, source);

	const auto kValidation = crimson::validate_material_instance(registry, mapping.material);
	if (!kValidation) {
		IoError error = make_mapping_error(
				"Crimson rejected mapped glTF material.",
				kValidation.error().message + " " + kValidation.error().detail,
				source.name);
		error.related.append(mapping.diagnostics);
		return quader::foundation::Result<GltfMaterialMapping, IoError>::failure(std::move(error));
	}

	return quader::foundation::Result<GltfMaterialMapping, IoError>::success(std::move(mapping));
}

} // namespace quader::io::gltf
