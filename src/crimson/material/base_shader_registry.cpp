#include "crimson/material/base_shader_registry.hpp"

#include <utility>

namespace crimson {
namespace {

constexpr VertexAttributeMask kPbrVertexAttributes =
		VertexAttributePosition | VertexAttributeNormal | VertexAttributeTangent | VertexAttributeUv0;

constexpr VertexAttributeMask kOverlayVertexAttributes =
		VertexAttributePosition | VertexAttributeColor0;

[[nodiscard]] MaterialParameterDesc float_param(std::string name, float default_value, float min_value, float max_value) {
	return MaterialParameterDesc{
		.name = std::move(name),
		.kind = MaterialParameterKind::Float,
		.default_value = default_value,
		.min_value = min_value,
		.max_value = max_value,
	};
}

[[nodiscard]] MaterialParameterDesc vec2_param(
		std::string name,
		MaterialVec2 default_value,
		MaterialVec2 min_value,
		MaterialVec2 max_value) {
	return MaterialParameterDesc{
		.name = std::move(name),
		.kind = MaterialParameterKind::Vec2,
		.default_value = default_value,
		.min_value = min_value,
		.max_value = max_value,
	};
}

[[nodiscard]] MaterialParameterDesc color_param(std::string name, MaterialColorSrgb default_value) {
	return MaterialParameterDesc{
		.name = std::move(name),
		.kind = MaterialParameterKind::ColorSrgb,
		.default_value = default_value,
		.min_value = MaterialColorSrgb{ 0.0F, 0.0F, 0.0F, 0.0F },
		.max_value = MaterialColorSrgb{ 1.0F, 1.0F, 1.0F, 1.0F },
	};
}

[[nodiscard]] MaterialParameterDesc bool_param(std::string name, bool default_value) {
	return MaterialParameterDesc{
		.name = std::move(name),
		.kind = MaterialParameterKind::Bool,
		.default_value = default_value,
		.min_value = false,
		.max_value = true,
	};
}

[[nodiscard]] MaterialParameterDesc enum_param(
		std::string name,
		std::int32_t default_value,
		std::int32_t min_value,
		std::int32_t max_value) {
	return MaterialParameterDesc{
		.name = std::move(name),
		.kind = MaterialParameterKind::Enum,
		.default_value = MaterialEnumValue{ default_value },
		.min_value = MaterialEnumValue{ min_value },
		.max_value = MaterialEnumValue{ max_value },
	};
}

[[nodiscard]] MaterialTextureSlotDesc texture_slot(
		std::string name,
		TextureColorSpace color_space,
		RenderTextureHandle default_texture = {}) {
	return MaterialTextureSlotDesc{
		.name = std::move(name),
		.color_space = color_space,
		.default_texture = default_texture,
		.required = false,
	};
}

[[nodiscard]] MaterialUiFieldDesc parameter_field(
		std::string label,
		MaterialUiControl control,
		std::string binding_name) {
	return MaterialUiFieldDesc{
		.label = std::move(label),
		.kind = MaterialUiFieldKind::Parameter,
		.control = control,
		.binding_name = std::move(binding_name),
	};
}

[[nodiscard]] MaterialUiFieldDesc texture_field(std::string label, std::string binding_name) {
	return MaterialUiFieldDesc{
		.label = std::move(label),
		.kind = MaterialUiFieldKind::TextureSlot,
		.control = MaterialUiControl::Texture,
		.binding_name = std::move(binding_name),
	};
}

[[nodiscard]] std::vector<MaterialParameterDesc> opaque_pbr_parameters() {
	return {
		color_param("base_color", MaterialColorSrgb{ 1.0F, 1.0F, 1.0F, 1.0F }),
		float_param("metallic", 0.0F, 0.0F, 1.0F),
		float_param("roughness", 0.5F, 0.0F, 1.0F),
		float_param("normal_strength", 1.0F, 0.0F, 4.0F),
		float_param("occlusion_strength", 1.0F, 0.0F, 1.0F),
		color_param("emissive_color", MaterialColorSrgb{ 0.0F, 0.0F, 0.0F, 1.0F }),
		float_param("emissive_strength", 0.0F, 0.0F, 100000.0F),
		bool_param("double_sided", false),
		enum_param("uv_set", 0, 0, 1),
		vec2_param("uv_tiling", MaterialVec2{ 1.0F, 1.0F }, MaterialVec2{ 0.0F, 0.0F }, MaterialVec2{ 10000.0F, 10000.0F }),
		vec2_param("uv_offset", MaterialVec2{ 0.0F, 0.0F }, MaterialVec2{ -10000.0F, -10000.0F }, MaterialVec2{ 10000.0F, 10000.0F }),
	};
}

[[nodiscard]] std::vector<MaterialTextureSlotDesc> opaque_pbr_texture_slots() {
	return {
		texture_slot("base_color", TextureColorSpace::Srgb),
		texture_slot("metallic_roughness", TextureColorSpace::Linear),
		texture_slot("normal", TextureColorSpace::Data),
		texture_slot("occlusion", TextureColorSpace::Linear),
		texture_slot("emissive", TextureColorSpace::Srgb),
	};
}

[[nodiscard]] std::vector<MaterialUiFieldDesc> opaque_pbr_ui_schema() {
	return {
		parameter_field("Base Color", MaterialUiControl::Color, "base_color"),
		texture_field("Base Color Texture", "base_color"),
		parameter_field("Metallic", MaterialUiControl::Float, "metallic"),
		parameter_field("Roughness", MaterialUiControl::Float, "roughness"),
		texture_field("Metallic/Roughness Texture", "metallic_roughness"),
		texture_field("Normal Texture", "normal"),
		parameter_field("Normal Strength", MaterialUiControl::Float, "normal_strength"),
		texture_field("Ambient Occlusion Texture", "occlusion"),
		parameter_field("Ambient Occlusion Strength", MaterialUiControl::Float, "occlusion_strength"),
		parameter_field("Emissive Color", MaterialUiControl::Color, "emissive_color"),
		texture_field("Emissive Texture", "emissive"),
		parameter_field("Emissive Strength", MaterialUiControl::Float, "emissive_strength"),
		parameter_field("Double Sided", MaterialUiControl::Checkbox, "double_sided"),
		parameter_field("UV Set", MaterialUiControl::Combo, "uv_set"),
		parameter_field("UV Tiling", MaterialUiControl::Vec2, "uv_tiling"),
		parameter_field("UV Offset", MaterialUiControl::Vec2, "uv_offset"),
	};
}

[[nodiscard]] BaseShaderDefinition make_opaque_pbr() {
	return BaseShaderDefinition{
		.id = BaseShaderId::OpaquePbr,
		.name = "OpaquePbr",
		.domain = RenderDomain::LitSurface,
		.default_queue = RenderQueue::Opaque,
		.depth_mode = DepthMode::TestWrite,
		.blend_mode = BlendMode::Off,
		.cull_mode = CullMode::Back,
		.shadow_mode = ShadowMode::CastAndReceive,
		.required_attributes = kPbrVertexAttributes,
		.parameters = opaque_pbr_parameters(),
		.texture_slots = opaque_pbr_texture_slots(),
		.ui_schema = opaque_pbr_ui_schema(),
		.program = ShaderProgramId::OpaquePbr,
	};
}

[[nodiscard]] BaseShaderDefinition make_alpha_cutout_pbr() {
	std::vector<MaterialParameterDesc> parameters = opaque_pbr_parameters();
	parameters.push_back(float_param("alpha_cutoff", 0.5F, 0.0F, 1.0F));
	parameters.push_back(enum_param("alpha_source_channel", 3, 0, 3));

	std::vector<MaterialUiFieldDesc> ui_schema = opaque_pbr_ui_schema();
	ui_schema.push_back(parameter_field("Alpha Cutoff", MaterialUiControl::Float, "alpha_cutoff"));
	ui_schema.push_back(parameter_field("Alpha Source Channel", MaterialUiControl::Combo, "alpha_source_channel"));

	return BaseShaderDefinition{
		.id = BaseShaderId::AlphaCutoutPbr,
		.name = "AlphaCutoutPbr",
		.domain = RenderDomain::LitSurface,
		.default_queue = RenderQueue::AlphaCutout,
		.depth_mode = DepthMode::TestWrite,
		.blend_mode = BlendMode::Off,
		.cull_mode = CullMode::Back,
		.shadow_mode = ShadowMode::AlphaTestedCastAndReceive,
		.required_attributes = kPbrVertexAttributes,
		.parameters = std::move(parameters),
		.texture_slots = opaque_pbr_texture_slots(),
		.ui_schema = std::move(ui_schema),
		.program = ShaderProgramId::AlphaCutoutPbr,
	};
}

[[nodiscard]] BaseShaderDefinition make_transparent_pbr() {
	return BaseShaderDefinition{
		.id = BaseShaderId::TransparentPbr,
		.name = "TransparentPbr",
		.domain = RenderDomain::TransparentSurface,
		.default_queue = RenderQueue::Transparent,
		.depth_mode = DepthMode::TestReadOnly,
		.blend_mode = BlendMode::AlphaBlend,
		.cull_mode = CullMode::Back,
		.shadow_mode = ShadowMode::None,
		.required_attributes = kPbrVertexAttributes,
		.parameters = {
				color_param("base_color", MaterialColorSrgb{ 1.0F, 1.0F, 1.0F, 1.0F }),
				float_param("opacity", 0.5F, 0.0F, 1.0F),
				float_param("metallic", 0.0F, 0.0F, 1.0F),
				float_param("roughness", 0.5F, 0.0F, 1.0F),
				float_param("normal_strength", 1.0F, 0.0F, 4.0F),
				color_param("emissive_color", MaterialColorSrgb{ 0.0F, 0.0F, 0.0F, 1.0F }),
				float_param("emissive_strength", 0.0F, 0.0F, 100000.0F),
				bool_param("double_sided", false),
		},
		.texture_slots = {
				texture_slot("base_color", TextureColorSpace::Srgb),
				texture_slot("metallic_roughness", TextureColorSpace::Linear),
				texture_slot("normal", TextureColorSpace::Data),
				texture_slot("emissive", TextureColorSpace::Srgb),
		},
		.ui_schema = {
				parameter_field("Base Color", MaterialUiControl::Color, "base_color"),
				texture_field("Base Color Texture", "base_color"),
				parameter_field("Opacity", MaterialUiControl::Float, "opacity"),
				parameter_field("Metallic", MaterialUiControl::Float, "metallic"),
				parameter_field("Roughness", MaterialUiControl::Float, "roughness"),
				texture_field("Metallic/Roughness Texture", "metallic_roughness"),
				texture_field("Normal Texture", "normal"),
				parameter_field("Normal Strength", MaterialUiControl::Float, "normal_strength"),
				parameter_field("Emissive Color", MaterialUiControl::Color, "emissive_color"),
				texture_field("Emissive Texture", "emissive"),
				parameter_field("Emissive Strength", MaterialUiControl::Float, "emissive_strength"),
				parameter_field("Double Sided", MaterialUiControl::Checkbox, "double_sided"),
		},
		.program = ShaderProgramId::TransparentPbr,
	};
}

[[nodiscard]] BaseShaderDefinition make_overlay_unlit() {
	return BaseShaderDefinition{
		.id = BaseShaderId::OverlayUnlit,
		.name = "OverlayUnlit",
		.domain = RenderDomain::Overlay,
		.default_queue = RenderQueue::OverlayDepthTested,
		.depth_mode = DepthMode::OverlayCommand,
		.blend_mode = BlendMode::AlphaBlend,
		.cull_mode = CullMode::None,
		.shadow_mode = ShadowMode::None,
		.required_attributes = kOverlayVertexAttributes,
		.parameters = {
				color_param("color", MaterialColorSrgb{ 1.0F, 1.0F, 1.0F, 1.0F }),
				float_param("opacity", 1.0F, 0.0F, 1.0F),
				enum_param("line_width_mode", 0, 0, 2),
				enum_param("depth_mode", 0, 0, 2),
				enum_param("pattern", 0, 0, 8),
		},
		.texture_slots = {},
		.ui_schema = {
				parameter_field("Color", MaterialUiControl::Color, "color"),
				parameter_field("Opacity", MaterialUiControl::Float, "opacity"),
				parameter_field("Line Width Mode", MaterialUiControl::Combo, "line_width_mode"),
				parameter_field("Depth Mode", MaterialUiControl::Combo, "depth_mode"),
				parameter_field("Pattern", MaterialUiControl::Combo, "pattern"),
		},
		.program = ShaderProgramId::OverlayUnlit,
	};
}

} // namespace

BaseShaderRegistry::BaseShaderRegistry(std::vector<BaseShaderDefinition> definitions) : definitions_(std::move(definitions)) {
}

const BaseShaderDefinition *BaseShaderRegistry::find(BaseShaderId id) const noexcept {
	for (const BaseShaderDefinition &definition : definitions_) {
		if (definition.id == id) {
			return &definition;
		}
	}

	return nullptr;
}

std::span<const BaseShaderDefinition> BaseShaderRegistry::definitions() const noexcept {
	return definitions_;
}

bool BaseShaderRegistry::empty() const noexcept {
	return definitions_.empty();
}

BaseShaderRegistry make_v1_base_shader_registry() {
	std::vector<BaseShaderDefinition> definitions;
	definitions.reserve(4);
	definitions.push_back(make_opaque_pbr());
	definitions.push_back(make_alpha_cutout_pbr());
	definitions.push_back(make_transparent_pbr());
	definitions.push_back(make_overlay_unlit());
	return BaseShaderRegistry(std::move(definitions));
}

} // namespace crimson
