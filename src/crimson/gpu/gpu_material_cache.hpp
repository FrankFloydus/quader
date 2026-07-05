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
#include "crimson/renderer_diagnostics.hpp"

#include <array>
#include <cstddef>
#include <memory>

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
};

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
	 * @param status Status receiving diagnostics.
	 * @return True when initialization succeeds.
	 */
	[[nodiscard]] bool initialize(RendererStatus &status);
	/// Release all owned GPU resources.
	void shutdown() noexcept;

	/**
	 * Return a packed material block.
	 *
	 * @param materials Material system used to resolve `material`.
	 * @param material Material handle to resolve.
	 * @param definition Base shader definition used for packing.
	 * @return Packed material block, using defaults when resolution fails.
	 */
	[[nodiscard]] GpuPbrMaterialBlock material_block(
			const MaterialSystem &materials,
			RenderMaterialHandle material,
			const BaseShaderDefinition &definition) const noexcept;
	/**
	 * Bind packed PBR material constants for subsequent draws.
	 *
	 * @param block Packed material constants.
	 */
	void bind_pbr_material(const GpuPbrMaterialBlock &block) const noexcept;

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
