#pragma once

#include "crimson/material/base_shader_registry.hpp"
#include "crimson/material/material_instance.hpp"
#include "crimson/renderer_diagnostics.hpp"
#include "foundation/result.hpp"

namespace crimson {

[[nodiscard]] quader::foundation::Result<void, RendererDiagnostic> validate_material_instance(
    const BaseShaderRegistry& registry,
    const MaterialInstance& instance);

} // namespace crimson
