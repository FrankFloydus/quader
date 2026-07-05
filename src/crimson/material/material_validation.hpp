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

namespace crimson {

/**
 * Validate a material instance against a base shader registry.
 *
 * @param registry Registry that owns base shader schemas.
 * @param instance Material instance to validate.
 * @return Success, or a renderer diagnostic naming the invalid schema/binding.
 */
[[nodiscard]] quader::foundation::Result<void, RendererDiagnostic> validate_material_instance(
		const BaseShaderRegistry &registry,
		const MaterialInstance &instance);

} // namespace crimson
