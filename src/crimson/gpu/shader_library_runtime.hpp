#pragma once

#include "crimson/gpu/shader_library.hpp"
#include "crimson/renderer_diagnostics.hpp"

#include <vector>

#include <bgfx/bgfx.h>

namespace crimson::gpu {

[[nodiscard]] bgfx::ShaderHandle load_shader(
		const ShaderLibrary &library,
		ShaderProgramId program,
		ShaderStage stage,
		ShaderTarget target,
		std::vector<RendererDiagnostic> &diagnostics);

} // namespace crimson::gpu
