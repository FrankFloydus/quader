#include "crimson/gpu/gpu_material_cache.hpp"

#include "crimson/color/color_space.hpp"
#include "crimson/gpu/gpu_handles.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <utility>
#include <variant>

#include <bgfx/bgfx.h>

namespace crimson::gpu {
namespace {

struct DefaultTexture {
	UniqueTextureHandle texture;
	TextureColorSpace color_space = TextureColorSpace::Linear;
};

[[nodiscard]] float finite_or_default(float value, float fallback) noexcept {
	return std::isfinite(value) ? value : fallback;
}

[[nodiscard]] const MaterialParameterValue *parameter_value(
		const MaterialInstance &material,
		std::string_view name) noexcept {
	for (const MaterialParameter &parameter : material.parameters) {
		if (parameter.name == name) {
			return &parameter.value;
		}
	}
	return nullptr;
}

[[nodiscard]] float float_parameter(
		const MaterialInstance &material,
		std::string_view name,
		float fallback) noexcept {
	const MaterialParameterValue *value = parameter_value(material, name);
	const auto *number = value == nullptr ? nullptr : std::get_if<float>(value);
	return number == nullptr ? fallback : finite_or_default(*number, fallback);
}

[[nodiscard]] bool bool_parameter(
		const MaterialInstance &material,
		std::string_view name,
		bool fallback) noexcept {
	const MaterialParameterValue *value = parameter_value(material, name);
	const auto *flag = value == nullptr ? nullptr : std::get_if<bool>(value);
	return flag == nullptr ? fallback : *flag;
}

[[nodiscard]] MaterialColorSrgb color_parameter(
		const MaterialInstance &material,
		std::string_view name,
		MaterialColorSrgb fallback) noexcept {
	const MaterialParameterValue *value = parameter_value(material, name);
	const auto *color = value == nullptr ? nullptr : std::get_if<MaterialColorSrgb>(value);
	return color == nullptr ? fallback : *color;
}

[[nodiscard]] MaterialVec2 vec2_parameter(
		const MaterialInstance &material,
		std::string_view name,
		MaterialVec2 fallback) noexcept {
	const MaterialParameterValue *value = parameter_value(material, name);
	const auto *vector = value == nullptr ? nullptr : std::get_if<MaterialVec2>(value);
	return vector == nullptr ? fallback : *vector;
}

[[nodiscard]] std::array<float, 4> linear_color_array(MaterialColorSrgb color) noexcept {
	const ColorLinear kLinear = srgb_to_linear(ColorSrgb{ color.r, color.g, color.b, color.a });
	return { kLinear.r, kLinear.g, kLinear.b, kLinear.a };
}

[[nodiscard]] UniqueTextureHandle create_default_texture(std::uint32_t rgba8, TextureColorSpace color_space) {
	const std::uint64_t kFlags = BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP | BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MAG_POINT | (color_space == TextureColorSpace::Srgb ? BGFX_TEXTURE_SRGB : BGFX_TEXTURE_NONE);
	return UniqueTextureHandle(bgfx::createTexture2D(
			1,
			1,
			false,
			1,
			bgfx::TextureFormat::RGBA8,
			kFlags,
			bgfx::copy(&rgba8, sizeof(rgba8))));
}

void push_resource_error(RendererStatus &status, std::string message, std::string resource_name) {
	status.diagnostics.push_back(RendererDiagnostic{
			.severity = RendererDiagnosticSeverity::Error,
			.code = RendererDiagnosticCode::ResourceCreationFailed,
			.message = std::move(message),
			.subsystem = "gpu.material",
			.resource_name = std::move(resource_name),
	});
}

} // namespace

struct GpuMaterialCache::Impl {
	UniqueUniformHandle base_color_uniform;
	UniqueUniformHandle emissive_uniform;
	UniqueUniformHandle factors_uniform;
	UniqueUniformHandle uv_transform_uniform;
	UniqueUniformHandle flags_uniform;
	std::array<DefaultTexture, 5> default_textures;
	FrameUploadStats upload_stats;
	bool initialized = false;
};

GpuPbrMaterialBlock pack_pbr_material_block(
		const MaterialInstance &material,
		const BaseShaderDefinition &definition) noexcept {
	const MaterialColorSrgb kBaseColor = color_parameter(
			material,
			"base_color",
			MaterialColorSrgb{ 1.0F, 1.0F, 1.0F, 1.0F });
	const MaterialColorSrgb kEmissiveColor = color_parameter(
			material,
			"emissive_color",
			MaterialColorSrgb{ 0.0F, 0.0F, 0.0F, 1.0F });
	const float kEmissiveStrength = float_parameter(material, "emissive_strength", 0.0F);
	const float kMetallic = std::clamp(float_parameter(material, "metallic", 0.0F), 0.0F, 1.0F);
	const float kRoughness = std::clamp(float_parameter(material, "roughness", 0.5F), 0.0F, 1.0F);
	const float kNormalStrength = std::max(0.0F, float_parameter(material, "normal_strength", 1.0F));
	const float kOcclusionStrength = std::clamp(float_parameter(material, "occlusion_strength", 1.0F), 0.0F, 1.0F);
	const float kOpacity = std::clamp(float_parameter(material, "opacity", kBaseColor.a), 0.0F, 1.0F);
	const float kAlphaCutoff = std::clamp(float_parameter(material, "alpha_cutoff", 0.5F), 0.0F, 1.0F);
	const MaterialVec2 kUvTiling = vec2_parameter(material, "uv_tiling", MaterialVec2{ 1.0F, 1.0F });
	const MaterialVec2 kUvOffset = vec2_parameter(material, "uv_offset", MaterialVec2{ 0.0F, 0.0F });
	const bool kDoubleSided = bool_parameter(material, "double_sided", material.overrides.double_sided);

	GpuPbrMaterialBlock block;
	block.base_color_factor_linear = linear_color_array(kBaseColor);
	if (definition.id == BaseShaderId::TransparentPbr) {
		block.base_color_factor_linear[3] = kOpacity;
	}
	block.emissive_factor_linear = linear_color_array(kEmissiveColor);
	block.emissive_factor_linear[3] = std::max(0.0F, kEmissiveStrength);
	block.factors = { kMetallic, kRoughness, kNormalStrength, kOcclusionStrength };
	if (definition.id == BaseShaderId::AlphaCutoutPbr) {
		block.factors[3] = kAlphaCutoff;
	}
	if (definition.id == BaseShaderId::TransparentPbr) {
		block.factors[3] = kOpacity;
	}
	block.uv_transform_0 = { kUvTiling.x, kUvTiling.y, kUvOffset.x, kUvOffset.y };
	block.flags = {
		kDoubleSided ? 1.0F : 0.0F,
		definition.id == BaseShaderId::AlphaCutoutPbr ? 1.0F : 0.0F,
		definition.id == BaseShaderId::TransparentPbr ? 1.0F : 0.0F,
		0.0F,
	};
	return block;
}

GpuMaterialCache::GpuMaterialCache() : impl_(std::make_unique<Impl>()) {
}

GpuMaterialCache::~GpuMaterialCache() = default;

bool GpuMaterialCache::initialize(RendererStatus &status) {
	shutdown();

	impl_->base_color_uniform.reset(bgfx::createUniform("u_pbrBaseColor", bgfx::UniformType::Vec4));
	impl_->emissive_uniform.reset(bgfx::createUniform("u_pbrEmissive", bgfx::UniformType::Vec4));
	impl_->factors_uniform.reset(bgfx::createUniform("u_pbrFactors", bgfx::UniformType::Vec4));
	impl_->uv_transform_uniform.reset(bgfx::createUniform("u_pbrUvTransform0", bgfx::UniformType::Vec4));
	impl_->flags_uniform.reset(bgfx::createUniform("u_pbrFlags", bgfx::UniformType::Vec4));
	impl_->default_textures[0] = DefaultTexture{
		create_default_texture(0xffffffffU, TextureColorSpace::Srgb),
		TextureColorSpace::Srgb,
	};
	impl_->default_textures[1] = DefaultTexture{
		create_default_texture(0xff8080ffU, TextureColorSpace::Linear),
		TextureColorSpace::Linear,
	};
	impl_->default_textures[2] = DefaultTexture{
		create_default_texture(0xffff8080U, TextureColorSpace::Data),
		TextureColorSpace::Data,
	};
	impl_->default_textures[3] = DefaultTexture{
		create_default_texture(0xffffffffU, TextureColorSpace::Linear),
		TextureColorSpace::Linear,
	};
	impl_->default_textures[4] = DefaultTexture{
		create_default_texture(0xff000000U, TextureColorSpace::Srgb),
		TextureColorSpace::Srgb,
	};

	impl_->initialized = impl_->base_color_uniform.valid() && impl_->emissive_uniform.valid() && impl_->factors_uniform.valid() && impl_->uv_transform_uniform.valid() && impl_->flags_uniform.valid();
	for (const DefaultTexture &texture : impl_->default_textures) {
		impl_->initialized = impl_->initialized && texture.texture.valid();
	}

	if (!impl_->initialized) {
		push_resource_error(status, "Crimson failed to create default PBR material GPU resources.", "GpuMaterialCache");
		shutdown();
		return false;
	}

	impl_->upload_stats.texture_upload_count += static_cast<std::uint32_t>(impl_->default_textures.size());
	impl_->upload_stats.uploaded_texture_bytes += impl_->default_textures.size() * sizeof(std::uint32_t);
	return true;
}

void GpuMaterialCache::shutdown() noexcept {
	for (DefaultTexture &texture : impl_->default_textures) {
		texture.texture.reset();
		texture.color_space = TextureColorSpace::Linear;
	}
	impl_->base_color_uniform.reset();
	impl_->emissive_uniform.reset();
	impl_->factors_uniform.reset();
	impl_->uv_transform_uniform.reset();
	impl_->flags_uniform.reset();
	impl_->upload_stats = {};
	impl_->initialized = false;
}

GpuPbrMaterialBlock GpuMaterialCache::material_block(
		const MaterialSystem &materials,
		RenderMaterialHandle material,
		const BaseShaderDefinition &definition) const noexcept {
	if (const MaterialInstance *instance = materials.get(material)) {
		return pack_pbr_material_block(*instance, definition);
	}
	return pack_pbr_material_block(make_default_material_instance(definition), definition);
}

void GpuMaterialCache::bind_pbr_material(const GpuPbrMaterialBlock &block) const noexcept {
	if (!impl_->initialized) {
		return;
	}

	bgfx::setUniform(impl_->base_color_uniform.get(), block.base_color_factor_linear.data());
	bgfx::setUniform(impl_->emissive_uniform.get(), block.emissive_factor_linear.data());
	bgfx::setUniform(impl_->factors_uniform.get(), block.factors.data());
	bgfx::setUniform(impl_->uv_transform_uniform.get(), block.uv_transform_0.data());
	bgfx::setUniform(impl_->flags_uniform.get(), block.flags.data());
}

std::size_t GpuMaterialCache::live_texture_count() const noexcept {
	std::size_t count = 0;
	for (const DefaultTexture &texture : impl_->default_textures) {
		if (texture.texture.valid()) {
			++count;
		}
	}
	return count;
}

FrameUploadStats GpuMaterialCache::upload_stats() const noexcept {
	return impl_->upload_stats;
}

bool GpuMaterialCache::initialized() const noexcept {
	return impl_->initialized;
}

} // namespace crimson::gpu
