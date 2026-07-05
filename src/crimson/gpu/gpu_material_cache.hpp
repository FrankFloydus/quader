#pragma once

#include "crimson/frame/frame_stats.hpp"
#include "crimson/material/material_system.hpp"
#include "crimson/renderer_diagnostics.hpp"

#include <array>
#include <cstddef>
#include <memory>

namespace crimson::gpu {

struct GpuPbrMaterialBlock {
	std::array<float, 4> base_color_factor_linear{ 1.0F, 1.0F, 1.0F, 1.0F };
	std::array<float, 4> emissive_factor_linear{ 0.0F, 0.0F, 0.0F, 0.0F };
	std::array<float, 4> factors{ 0.0F, 0.5F, 1.0F, 1.0F };
	std::array<float, 4> uv_transform_0{ 1.0F, 1.0F, 0.0F, 0.0F };
	std::array<float, 4> flags{};
};

[[nodiscard]] GpuPbrMaterialBlock pack_pbr_material_block(
		const MaterialInstance &material,
		const BaseShaderDefinition &definition) noexcept;

class GpuMaterialCache final {
public:
	GpuMaterialCache();
	~GpuMaterialCache();

	GpuMaterialCache(const GpuMaterialCache &) = delete;
	GpuMaterialCache &operator=(const GpuMaterialCache &) = delete;

	[[nodiscard]] bool initialize(RendererStatus &status);
	void shutdown() noexcept;

	[[nodiscard]] GpuPbrMaterialBlock material_block(
			const MaterialSystem &materials,
			RenderMaterialHandle material,
			const BaseShaderDefinition &definition) const noexcept;
	void bind_pbr_material(const GpuPbrMaterialBlock &block) const noexcept;

	[[nodiscard]] std::size_t live_texture_count() const noexcept;
	[[nodiscard]] FrameUploadStats upload_stats() const noexcept;
	[[nodiscard]] bool initialized() const noexcept;

private:
	struct Impl;

	std::unique_ptr<Impl> impl_;
};

} // namespace crimson::gpu
