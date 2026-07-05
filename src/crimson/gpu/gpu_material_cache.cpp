/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
#include "crimson/gpu/gpu_material_cache.hpp"

#include "crimson/color/color_space.hpp"
#include "crimson/gpu/gpu_handles.hpp"
#include "crimson/material/default_material.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include <bgfx/bgfx.h>
#include <lodepng.h>

namespace crimson::gpu {
namespace {

struct DefaultTexture {
	UniqueTextureHandle texture;
	TextureColorSpace color_space = TextureColorSpace::Linear;
};

enum class DefaultTextureIndex : std::size_t {
	BaseColorFallback = 0,
	MetallicRoughnessFallback = 1,
	NormalFallback = 2,
	OcclusionFallback = 3,
	EmissiveFallback = 4,
	DefaultAlbedo = 5,
	Count = 6,
};

constexpr std::size_t kDefaultTextureCount = static_cast<std::size_t>(DefaultTextureIndex::Count);
constexpr RenderTextureHandle kDefaultAlbedoTextureHandle{ static_cast<std::uint32_t>(DefaultTextureIndex::DefaultAlbedo) + 1U, 1U };
constexpr std::uint8_t kBaseColorTextureStage = 0;

struct DecodedTextureImage {
	std::vector<unsigned char> rgba8;
	std::uint16_t width = 0;
	std::uint16_t height = 0;
};

constexpr std::size_t kRgba8ChannelCount = 4U;

[[nodiscard]] std::size_t texture_index(DefaultTextureIndex index) noexcept {
	return static_cast<std::size_t>(index);
}

[[nodiscard]] std::size_t rgba8_byte_count(std::uint16_t width, std::uint16_t height) noexcept {
	return static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * kRgba8ChannelCount;
}

[[nodiscard]] float unorm8_to_float(unsigned char value) noexcept {
	return static_cast<float>(value) / 255.0F;
}

[[nodiscard]] unsigned char float_to_unorm8(float value) noexcept {
	const float kClamped = std::clamp(std::isfinite(value) ? value : 0.0F, 0.0F, 1.0F);
	return static_cast<unsigned char>(std::round(kClamped * 255.0F));
}

[[nodiscard]] float texel_channel_to_linear(
		std::span<const unsigned char> rgba8,
		std::size_t pixel_index,
		std::uint8_t channel,
		TextureColorSpace color_space) noexcept {
	const float kValue = unorm8_to_float(rgba8[pixel_index * kRgba8ChannelCount + channel]);
	if (channel == 3U || color_space != TextureColorSpace::Srgb) {
		return kValue;
	}

	ColorLinear linear;
	if (channel == 0U) {
		linear = srgb_to_linear(ColorSrgb{ kValue, 0.0F, 0.0F, 1.0F });
		return linear.r;
	}
	if (channel == 1U) {
		linear = srgb_to_linear(ColorSrgb{ 0.0F, kValue, 0.0F, 1.0F });
		return linear.g;
	}

	linear = srgb_to_linear(ColorSrgb{ 0.0F, 0.0F, kValue, 1.0F });
	return linear.b;
}

[[nodiscard]] unsigned char linear_channel_to_texel(
		float value,
		std::uint8_t channel,
		TextureColorSpace color_space) noexcept {
	if (channel == 3U || color_space != TextureColorSpace::Srgb) {
		return float_to_unorm8(value);
	}

	ColorSrgb srgb;
	if (channel == 0U) {
		srgb = linear_to_srgb(ColorLinear{ value, 0.0F, 0.0F, 1.0F });
		return float_to_unorm8(srgb.r);
	}
	if (channel == 1U) {
		srgb = linear_to_srgb(ColorLinear{ 0.0F, value, 0.0F, 1.0F });
		return float_to_unorm8(srgb.g);
	}

	srgb = linear_to_srgb(ColorLinear{ 0.0F, 0.0F, value, 1.0F });
	return float_to_unorm8(srgb.b);
}

[[nodiscard]] Rgba8MipLevel downsample_rgba8_mip(
		const Rgba8MipLevel &source,
		TextureColorSpace color_space) {
	const std::uint16_t kWidth = std::max<std::uint16_t>(static_cast<std::uint16_t>(1U),
			static_cast<std::uint16_t>(source.width / 2U));
	const std::uint16_t kHeight = std::max<std::uint16_t>(static_cast<std::uint16_t>(1U),
			static_cast<std::uint16_t>(source.height / 2U));
	Rgba8MipLevel result{
		.rgba8 = std::vector<unsigned char>(rgba8_byte_count(kWidth, kHeight)),
		.width = kWidth,
		.height = kHeight,
	};

	for (std::uint16_t y = 0; y < kHeight; ++y) {
		for (std::uint16_t x = 0; x < kWidth; ++x) {
			const std::uint32_t kSourceX = static_cast<std::uint32_t>(x) * 2U;
			const std::uint32_t kSourceY = static_cast<std::uint32_t>(y) * 2U;
			float channels[4] = {};
			std::uint32_t sample_count = 0;
			for (std::uint32_t offset_y = 0; offset_y < 2U && kSourceY + offset_y < source.height; ++offset_y) {
				for (std::uint32_t offset_x = 0; offset_x < 2U && kSourceX + offset_x < source.width; ++offset_x) {
					const std::size_t kPixelIndex =
							(static_cast<std::size_t>(kSourceY + offset_y) * source.width) + (kSourceX + offset_x);
					for (std::uint8_t channel = 0; channel < kRgba8ChannelCount; ++channel) {
						channels[channel] += texel_channel_to_linear(source.rgba8, kPixelIndex, channel, color_space);
					}
					++sample_count;
				}
			}

			const float kInvSampleCount = sample_count == 0U ? 0.0F : 1.0F / static_cast<float>(sample_count);
			const std::size_t kTargetIndex = (static_cast<std::size_t>(y) * kWidth + x) * kRgba8ChannelCount;
			for (std::uint8_t channel = 0; channel < kRgba8ChannelCount; ++channel) {
				result.rgba8[kTargetIndex + channel] =
						linear_channel_to_texel(channels[channel] * kInvSampleCount, channel, color_space);
			}
		}
	}

	return result;
}

[[nodiscard]] std::vector<unsigned char> flatten_mip_chain(std::span<const Rgba8MipLevel> mips) {
	std::size_t byte_count = 0;
	for (const Rgba8MipLevel &mip : mips) {
		byte_count += mip.rgba8.size();
	}

	std::vector<unsigned char> flattened;
	flattened.reserve(byte_count);
	for (const Rgba8MipLevel &mip : mips) {
		flattened.insert(flattened.end(), mip.rgba8.begin(), mip.rgba8.end());
	}
	return flattened;
}

[[nodiscard]] std::uint64_t rgba8_mip_chain_byte_count(std::uint16_t width, std::uint16_t height) noexcept {
	std::uint64_t byte_count = 0;
	while (width > 0U && height > 0U) {
		byte_count += rgba8_byte_count(width, height);
		if (width == 1U && height == 1U) {
			break;
		}
		width = std::max<std::uint16_t>(static_cast<std::uint16_t>(1U),
				static_cast<std::uint16_t>(width / 2U));
		height = std::max<std::uint16_t>(static_cast<std::uint16_t>(1U),
				static_cast<std::uint16_t>(height / 2U));
	}
	return byte_count;
}

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

[[nodiscard]] constexpr std::uint64_t texture_color_space_flag(TextureColorSpace color_space) noexcept {
	return color_space == TextureColorSpace::Srgb ? BGFX_TEXTURE_SRGB : BGFX_TEXTURE_NONE;
}

[[nodiscard]] UniqueTextureHandle create_default_texture(std::uint32_t rgba8, TextureColorSpace color_space) {
	const std::uint64_t kFlags = BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP | BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MAG_POINT | texture_color_space_flag(color_space);
	return UniqueTextureHandle(bgfx::createTexture2D(
			1,
			1,
			false,
			1,
			bgfx::TextureFormat::RGBA8,
			kFlags,
			bgfx::copy(&rgba8, sizeof(rgba8))));
}

[[nodiscard]] UniqueTextureHandle create_rgba_texture(
		const DecodedTextureImage &image,
		TextureColorSpace color_space) {
	const std::uint64_t kFlags = material_texture_sampler_flags() | texture_color_space_flag(color_space);
	const std::vector<Rgba8MipLevel> kMips = make_rgba8_mip_chain(
			image.rgba8,
			image.width,
			image.height,
			color_space);
	if (kMips.empty()) {
		return UniqueTextureHandle{};
	}

	const std::vector<unsigned char> kFlattened = flatten_mip_chain(kMips);
	return UniqueTextureHandle(bgfx::createTexture2D(
			image.width,
			image.height,
			kMips.size() > 1U,
			1,
			bgfx::TextureFormat::RGBA8,
			kFlags,
			bgfx::copy(kFlattened.data(), static_cast<std::uint32_t>(kFlattened.size()))));
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

void push_texture_warning(RendererStatus &status, std::string message, std::string detail, std::string resource_name) {
	status.diagnostics.push_back(RendererDiagnostic{
			.severity = RendererDiagnosticSeverity::Warning,
			.code = RendererDiagnosticCode::ResourceCreationFailed,
			.message = std::move(message),
			.detail = std::move(detail),
			.subsystem = "gpu.material",
			.resource_name = std::move(resource_name),
	});
}

[[nodiscard]] std::optional<DecodedTextureImage> decode_png_texture(
		const std::filesystem::path &path,
		RendererStatus &status) {
	if (!std::filesystem::exists(path)) {
		push_texture_warning(
				status,
				"Crimson could not find the default material albedo texture.",
				"Falling back to the solid base-color texture.",
				path.string());
		return std::nullopt;
	}

	std::vector<unsigned char> pixels;
	unsigned width = 0;
	unsigned height = 0;
	const unsigned kError = lodepng::decode(pixels, width, height, path.string());
	if (kError != 0) {
		push_texture_warning(
				status,
				"Crimson could not decode the default material albedo texture.",
				std::string(lodepng_error_text(kError)),
				path.string());
		return std::nullopt;
	}
	if (width == 0 || height == 0 || width > std::numeric_limits<std::uint16_t>::max() || height > std::numeric_limits<std::uint16_t>::max()) {
		push_texture_warning(
				status,
				"Crimson rejected the default material albedo texture dimensions.",
				"Decoded PNG dimensions must fit a BGFX 2D texture.",
				path.string());
		return std::nullopt;
	}

	return DecodedTextureImage{
		.rgba8 = std::move(pixels),
		.width = static_cast<std::uint16_t>(width),
		.height = static_cast<std::uint16_t>(height),
	};
}

[[nodiscard]] UniqueTextureHandle load_default_albedo_texture(
		const RendererConfig &config,
		RendererStatus &status,
		FrameUploadStats &upload_stats) {
	if (config.asset_root.empty()) {
		return UniqueTextureHandle{};
	}

	const std::filesystem::path kPath = config.asset_root / std::filesystem::path(std::string(kDefaultQuaderAlbedoTexturePath));
	std::optional<DecodedTextureImage> decoded = decode_png_texture(kPath, status);
	if (!decoded) {
		return UniqueTextureHandle{};
	}

	const std::uint64_t kTextureBytes = rgba8_mip_chain_byte_count(decoded->width, decoded->height);
	UniqueTextureHandle texture = create_rgba_texture(*decoded, TextureColorSpace::Srgb);
	if (!texture.valid()) {
		push_texture_warning(
				status,
				"Crimson failed to create the default material albedo GPU texture.",
				"Falling back to the solid base-color texture.",
				kPath.string());
		return UniqueTextureHandle{};
	}

	++upload_stats.texture_upload_count;
	upload_stats.uploaded_texture_bytes += kTextureBytes;
	return texture;
}

[[nodiscard]] RenderTextureHandle texture_binding(
		const MaterialInstance &material,
		std::string_view name) noexcept {
	for (const MaterialTextureBinding &binding : material.textures) {
		if (binding.name == name) {
			return binding.texture;
		}
	}
	return {};
}

} // namespace

std::uint8_t rgba8_mip_level_count(std::uint16_t width, std::uint16_t height) noexcept {
	if (width == 0U || height == 0U) {
		return 0U;
	}

	std::uint16_t dimension = std::max(width, height);
	std::uint8_t levels = 1U;
	while (dimension > 1U) {
		dimension = static_cast<std::uint16_t>(dimension / 2U);
		++levels;
	}
	return levels;
}

std::vector<Rgba8MipLevel> make_rgba8_mip_chain(
		std::span<const unsigned char> base_level,
		std::uint16_t width,
		std::uint16_t height,
		TextureColorSpace color_space) {
	if (width == 0U || height == 0U || base_level.size() != rgba8_byte_count(width, height)) {
		return {};
	}

	std::vector<Rgba8MipLevel> mips;
	mips.reserve(rgba8_mip_level_count(width, height));
	mips.push_back(Rgba8MipLevel{
			.rgba8 = std::vector<unsigned char>(base_level.begin(), base_level.end()),
			.width = width,
			.height = height,
	});

	while (mips.back().width > 1U || mips.back().height > 1U) {
		mips.push_back(downsample_rgba8_mip(mips.back(), color_space));
	}

	return mips;
}

std::uint64_t material_texture_sampler_flags() noexcept {
	return BGFX_SAMPLER_MIN_ANISOTROPIC | BGFX_SAMPLER_MAG_ANISOTROPIC;
}

bool material_texture_sampler_uses_anisotropic_repeat_filtering(std::uint64_t flags) noexcept {
	constexpr std::uint64_t kDisallowed =
			BGFX_SAMPLER_U_CLAMP |
			BGFX_SAMPLER_V_CLAMP |
			BGFX_SAMPLER_MIN_POINT |
			BGFX_SAMPLER_MAG_POINT |
			BGFX_SAMPLER_MIP_POINT;
	return (flags & BGFX_SAMPLER_MIN_ANISOTROPIC) != 0U &&
			(flags & BGFX_SAMPLER_MAG_ANISOTROPIC) != 0U &&
			(flags & kDisallowed) == 0U;
}

struct GpuMaterialCache::Impl {
	UniqueUniformHandle base_color_uniform;
	UniqueUniformHandle emissive_uniform;
	UniqueUniformHandle factors_uniform;
	UniqueUniformHandle uv_transform_uniform;
	UniqueUniformHandle flags_uniform;
	UniqueUniformHandle base_color_texture_uniform;
	std::array<DefaultTexture, kDefaultTextureCount> default_textures;
	FrameUploadStats upload_stats;
	bool initialized = false;

	[[nodiscard]] bgfx::TextureHandle texture_for(
			RenderTextureHandle handle,
			DefaultTextureIndex fallback) const noexcept {
		if (is_valid_handle(handle) && handle.generation == 1U) {
			const std::size_t kIndex = static_cast<std::size_t>(handle.index - 1U);
			if (kIndex < default_textures.size() && default_textures[kIndex].texture.valid()) {
				return default_textures[kIndex].texture.get();
			}
		}

		return default_textures[texture_index(fallback)].texture.get();
	}
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

bool GpuMaterialCache::initialize(const RendererConfig &config, RendererStatus &status) {
	shutdown();

	impl_->base_color_uniform.reset(bgfx::createUniform("u_pbrBaseColor", bgfx::UniformType::Vec4));
	impl_->emissive_uniform.reset(bgfx::createUniform("u_pbrEmissive", bgfx::UniformType::Vec4));
	impl_->factors_uniform.reset(bgfx::createUniform("u_pbrFactors", bgfx::UniformType::Vec4));
	impl_->uv_transform_uniform.reset(bgfx::createUniform("u_pbrUvTransform0", bgfx::UniformType::Vec4));
	impl_->flags_uniform.reset(bgfx::createUniform("u_pbrFlags", bgfx::UniformType::Vec4));
	impl_->base_color_texture_uniform.reset(bgfx::createUniform("s_pbrBaseColorTexture", bgfx::UniformType::Sampler));
	impl_->default_textures[texture_index(DefaultTextureIndex::BaseColorFallback)] = DefaultTexture{
		create_default_texture(0xffffffffU, TextureColorSpace::Srgb),
		TextureColorSpace::Srgb,
	};
	impl_->default_textures[texture_index(DefaultTextureIndex::MetallicRoughnessFallback)] = DefaultTexture{
		create_default_texture(0xff8080ffU, TextureColorSpace::Linear),
		TextureColorSpace::Linear,
	};
	impl_->default_textures[texture_index(DefaultTextureIndex::NormalFallback)] = DefaultTexture{
		create_default_texture(0xffff8080U, TextureColorSpace::Data),
		TextureColorSpace::Data,
	};
	impl_->default_textures[texture_index(DefaultTextureIndex::OcclusionFallback)] = DefaultTexture{
		create_default_texture(0xffffffffU, TextureColorSpace::Linear),
		TextureColorSpace::Linear,
	};
	impl_->default_textures[texture_index(DefaultTextureIndex::EmissiveFallback)] = DefaultTexture{
		create_default_texture(0xff000000U, TextureColorSpace::Srgb),
		TextureColorSpace::Srgb,
	};
	impl_->default_textures[texture_index(DefaultTextureIndex::DefaultAlbedo)] = DefaultTexture{
		load_default_albedo_texture(config, status, impl_->upload_stats),
		TextureColorSpace::Srgb,
	};

	impl_->initialized = impl_->base_color_uniform.valid() && impl_->emissive_uniform.valid() && impl_->factors_uniform.valid() && impl_->uv_transform_uniform.valid() && impl_->flags_uniform.valid() && impl_->base_color_texture_uniform.valid();
	for (std::size_t index = 0; index < texture_index(DefaultTextureIndex::DefaultAlbedo); ++index) {
		impl_->initialized = impl_->initialized && impl_->default_textures[index].texture.valid();
	}

	if (!impl_->initialized) {
		push_resource_error(status, "Crimson failed to create default PBR material GPU resources.", "GpuMaterialCache");
		shutdown();
		return false;
	}

	impl_->upload_stats.texture_upload_count += static_cast<std::uint32_t>(texture_index(DefaultTextureIndex::DefaultAlbedo));
	impl_->upload_stats.uploaded_texture_bytes += texture_index(DefaultTextureIndex::DefaultAlbedo) * sizeof(std::uint32_t);
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
	impl_->base_color_texture_uniform.reset();
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

void GpuMaterialCache::bind_pbr_textures(
		const MaterialSystem &materials,
		RenderMaterialHandle material,
		const BaseShaderDefinition &definition) const noexcept {
	if (!impl_->initialized || find_texture_slot_desc(definition, "base_color") == nullptr) {
		return;
	}

	RenderTextureHandle base_color_texture;
	if (const MaterialInstance *instance = materials.get(material)) {
		base_color_texture = texture_binding(*instance, "base_color");
	}

	bgfx::setTexture(
			kBaseColorTextureStage,
			impl_->base_color_texture_uniform.get(),
			impl_->texture_for(base_color_texture, DefaultTextureIndex::BaseColorFallback),
			static_cast<std::uint32_t>(material_texture_sampler_flags()));
}

RenderTextureHandle GpuMaterialCache::default_albedo_texture_handle() const noexcept {
	return kDefaultAlbedoTextureHandle;
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
