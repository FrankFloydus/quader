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

#include "crimson/frame/frame_stats.hpp"
#include "crimson/material/material_system.hpp"
#include "crimson/renderer_config.hpp"
#include "crimson/renderer_diagnostics.hpp"
#include "crimson/scene/viewport_settings.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

namespace crimson::gpu {

/// Packed material constants uploaded or bound for PBR shaders.
struct GpuPbrMaterialBlock {
	/// Linear base color factor.
	std::array<float, 4> base_color_factor_linear{ 1.0F, 1.0F, 1.0F, 1.0F };
	/// Linear emissive color and strength.
	std::array<float, 4> emissive_factor_linear{ 0.0F, 0.0F, 0.0F, 0.0F };
	/// Metallic, roughness, normal scale, and occlusion strength factors.
	std::array<float, 4> factors{ 0.0F, 0.5F, 1.0F, 1.0F };
	/// UV scale and offset for the primary UV set.
	std::array<float, 4> uv_transform_0{ 1.0F, 1.0F, 0.0F, 0.0F };
	/// Packed material flags.
	std::array<float, 4> flags{};
	/// Mesh surface grid minor color in linear SDR.
	std::array<float, 4> surface_grid_minor_color_linear{ 0.0F, 0.0F, 0.0F, 0.0F };
	/// Mesh surface grid major color in linear SDR.
	std::array<float, 4> surface_grid_major_color_linear{ 0.0F, 0.0F, 0.0F, 0.0F };
	/// Mesh surface grid spacing and thickness parameters.
	std::array<float, 4> surface_grid_params{ 1.0F, 4.0F, 0.325F, 0.250F };
	/// Mesh surface grid feature flags.
	std::array<float, 4> surface_grid_flags{};
};

/// One RGBA8 mip level prepared for texture upload.
struct Rgba8MipLevel {
	/// Mip texels in tightly packed RGBA8 order.
	std::vector<unsigned char> rgba8;
	/// Mip width in pixels.
	std::uint16_t width = 0;
	/// Mip height in pixels.
	std::uint16_t height = 0;
};

/**
 * Return the full mip-level count for a 2D texture.
 *
 * @param width Base texture width.
 * @param height Base texture height.
 * @return Number of mip levels down to 1x1, or 0 for invalid dimensions.
 */
[[nodiscard]] std::uint8_t rgba8_mip_level_count(std::uint16_t width, std::uint16_t height) noexcept;
/**
 * Build a tightly packed RGBA8 mip chain.
 *
 * @param base_level Base level texels in RGBA8 order.
 * @param width Base texture width.
 * @param height Base texture height.
 * @param color_space Texture color space used for RGB downsampling.
 * @return Generated mip levels, empty when input dimensions or byte count are invalid.
 */
[[nodiscard]] std::vector<Rgba8MipLevel> make_rgba8_mip_chain(
		std::span<const unsigned char> base_level,
		std::uint16_t width,
		std::uint16_t height,
		TextureColorSpace color_space);

/**
 * Return BGFX sampler flags used for repeatable material textures.
 *
 * @return Sampler flags matching Quader's reference mesh material filtering.
 */
[[nodiscard]] std::uint64_t material_texture_sampler_flags() noexcept;

/**
 * Return true when sampler flags represent Quader's anisotropic repeat filtering.
 *
 * @param flags BGFX sampler flags to inspect.
 * @return True when anisotropic min/mag filtering is enabled without clamp or point filtering.
 */
[[nodiscard]] bool material_texture_sampler_uses_anisotropic_repeat_filtering(
		std::uint64_t flags) noexcept;

/**
 * Pack a material instance into the GPU PBR constant layout.
 *
 * @param material Material instance to pack.
 * @param definition Base shader definition used to interpret parameters.
 * @return Packed PBR material block.
 */
[[nodiscard]] GpuPbrMaterialBlock pack_pbr_material_block(
		const MaterialInstance &material,
		const BaseShaderDefinition &definition) noexcept;

/// Owns default GPU material resources and binds PBR material constants.
class GpuMaterialCache final {
public:
	/// Create an uninitialized material cache.
	GpuMaterialCache();
	/// Shut down and release material GPU resources.
	~GpuMaterialCache();

	/// Material caches cannot be copied because they own GPU resources.
	GpuMaterialCache(const GpuMaterialCache &) = delete;
	/// Material caches cannot be copied because they own GPU resources.
	GpuMaterialCache &operator=(const GpuMaterialCache &) = delete;

	/**
	 * Initialize default textures and uniforms.
	 *
	 * @param config Renderer startup configuration.
	 * @param status Status receiving diagnostics.
	 * @return True when initialization succeeds.
	 */
	[[nodiscard]] bool initialize(const RendererConfig &config, RendererStatus &status);
	/// Release all owned GPU resources.
	void shutdown() noexcept;

	/**
	 * Return a packed material block.
	 *
	 * @param materials Material system used to resolve `material`.
	 * @param material Material handle to resolve.
	 * @param definition Base shader definition used for packing.
	 * @param viewport_settings Active viewport render settings.
	 * @return Packed material block, using defaults when resolution fails.
	 */
	[[nodiscard]] GpuPbrMaterialBlock material_block(
			const MaterialSystem &materials,
			RenderMaterialHandle material,
			const BaseShaderDefinition &definition,
			const ViewportSettings &viewport_settings) const noexcept;
	/**
	 * Bind packed PBR material constants for subsequent draws.
	 *
	 * @param block Packed material constants.
	 */
	void bind_pbr_material(const GpuPbrMaterialBlock &block) const noexcept;
	/**
	 * Bind PBR material textures for subsequent draws.
	 *
	 * @param materials Material system used to resolve `material`.
	 * @param material Material handle to resolve.
	 * @param definition Base shader definition used to interpret texture slots.
	 */
	void bind_pbr_textures(
			const MaterialSystem &materials,
			RenderMaterialHandle material,
			const BaseShaderDefinition &definition) const noexcept;

	/// Return the handle for Quader's loaded default albedo texture.
	[[nodiscard]] RenderTextureHandle default_albedo_texture_handle() const noexcept;
	/// Return the number of live default texture resources.
	[[nodiscard]] std::size_t live_texture_count() const noexcept;
	/// Return upload counters accumulated by the material cache.
	[[nodiscard]] FrameUploadStats upload_stats() const noexcept;
	/// Return true when the cache has initialized GPU resources.
	[[nodiscard]] bool initialized() const noexcept;

private:
	struct Impl;

	std::unique_ptr<Impl> impl_;
};

} // namespace crimson::gpu
