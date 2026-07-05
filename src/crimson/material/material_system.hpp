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

#include "crimson/material/base_shader_registry.hpp"
#include "crimson/material/material_instance.hpp"
#include "crimson/renderer_diagnostics.hpp"
#include "foundation/result.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace crimson {

/**
 * Build a material instance populated with a base shader's default values.
 *
 * @param definition Base shader definition providing defaults.
 * @return Normalized default material instance.
 */
[[nodiscard]] MaterialInstance make_default_material_instance(const BaseShaderDefinition &definition);

/// Owns renderer material instances and validates them against base shader schemas.
class MaterialSystem final {
public:
	/// Construct with the built-in V1 base shader registry.
	MaterialSystem();
	/// Construct with an explicit base shader registry.
	explicit MaterialSystem(BaseShaderRegistry registry);

	/// Return the base shader registry used for validation.
	[[nodiscard]] const BaseShaderRegistry &registry() const noexcept;
	/**
	 * Create a material using default values for a base shader.
	 *
	 * @param base_shader_id Base shader id to instantiate.
	 * @return Stable material handle, or a renderer diagnostic.
	 */
	[[nodiscard]] quader::foundation::Result<RenderMaterialHandle, RendererDiagnostic> create_default_material(
			BaseShaderId base_shader_id);
	/**
	 * Create a validated material instance.
	 *
	 * @param instance Material instance to normalize and store.
	 * @return Stable material handle, or a renderer diagnostic.
	 */
	[[nodiscard]] quader::foundation::Result<RenderMaterialHandle, RendererDiagnostic> create_material(
			const MaterialInstance &instance);
	/// Return a const material pointer, or `nullptr` for invalid/stale handles.
	[[nodiscard]] const MaterialInstance *get(RenderMaterialHandle handle) const noexcept;
	/// Return a mutable material pointer, or `nullptr` for invalid/stale handles.
	[[nodiscard]] MaterialInstance *get(RenderMaterialHandle handle) noexcept;
	/// Destroy a material handle, returning false for invalid/stale handles.
	[[nodiscard]] bool destroy(RenderMaterialHandle handle) noexcept;
	/// Return the number of live material instances.
	[[nodiscard]] std::size_t live_material_count() const noexcept;

	/// Set one named material parameter after schema validation.
	[[nodiscard]] quader::foundation::Result<void, RendererDiagnostic> set_parameter(
			RenderMaterialHandle handle,
			std::string name,
			MaterialParameterValue value);
	/// Bind a texture handle to one named texture slot after schema validation.
	[[nodiscard]] quader::foundation::Result<void, RendererDiagnostic> bind_texture(
			RenderMaterialHandle handle,
			std::string name,
			RenderTextureHandle texture);

private:
	struct Slot {
		MaterialInstance material;
		std::uint32_t generation = 1;
		bool occupied = false;
	};

	[[nodiscard]] Slot *slot_for(RenderMaterialHandle handle) noexcept;
	[[nodiscard]] const Slot *slot_for(RenderMaterialHandle handle) const noexcept;
	[[nodiscard]] quader::foundation::Result<MaterialInstance, RendererDiagnostic> normalize(
			const MaterialInstance &instance) const;
	[[nodiscard]] RendererDiagnostic invalid_handle_diagnostic(RenderMaterialHandle handle) const;

	BaseShaderRegistry registry_;
	std::vector<Slot> slots_;
	std::vector<std::uint32_t> free_indices_;
	std::size_t live_material_count_ = 0;
};

} // namespace crimson
