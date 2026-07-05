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

#include "crimson/gpu/shader_library.hpp"
#include "crimson/renderer_diagnostics.hpp"

#include <vector>

#include <bgfx/bgfx.h>

namespace crimson::gpu {

/**
 * Load one compiled shader binary into bgfx.
 *
 * @param library Shader path resolver.
 * @param program Shader program identifier.
 * @param stage Shader stage.
 * @param target Compiled target platform.
 * @param[out] diagnostics Diagnostics appended for load failures.
 * @return bgfx shader handle, or an invalid handle on failure.
 */
[[nodiscard]] bgfx::ShaderHandle load_shader(
		const ShaderLibrary &library,
		ShaderProgramId program,
		ShaderStage stage,
		ShaderTarget target,
		std::vector<RendererDiagnostic> &diagnostics);

} // namespace crimson::gpu
