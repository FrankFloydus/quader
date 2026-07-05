#pragma once

#include "crimson/material/base_shader_registry.hpp"
#include "crimson/material/material_instance.hpp"
#include "crimson/renderer_diagnostics.hpp"
#include "foundation/result.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace crimson {

[[nodiscard]] MaterialInstance make_default_material_instance(const BaseShaderDefinition &definition);

class MaterialSystem final {
public:
	MaterialSystem();
	explicit MaterialSystem(BaseShaderRegistry registry);

	[[nodiscard]] const BaseShaderRegistry &registry() const noexcept;
	[[nodiscard]] quader::foundation::Result<RenderMaterialHandle, RendererDiagnostic> create_default_material(
			BaseShaderId base_shader_id);
	[[nodiscard]] quader::foundation::Result<RenderMaterialHandle, RendererDiagnostic> create_material(
			const MaterialInstance &instance);
	[[nodiscard]] const MaterialInstance *get(RenderMaterialHandle handle) const noexcept;
	[[nodiscard]] MaterialInstance *get(RenderMaterialHandle handle) noexcept;
	[[nodiscard]] bool destroy(RenderMaterialHandle handle) noexcept;
	[[nodiscard]] std::size_t live_material_count() const noexcept;

	[[nodiscard]] quader::foundation::Result<void, RendererDiagnostic> set_parameter(
			RenderMaterialHandle handle,
			std::string name,
			MaterialParameterValue value);
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
