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

#include "crimson/gpu/gpu_runtime_resources.hpp"
#include "crimson/gpu/shader_library.hpp"
#include "crimson/renderer_diagnostics.hpp"

#include <unordered_map>

#include <bgfx/bgfx.h>

namespace crimson::gpu {

/// Loads and owns GPU shader programs keyed by Crimson program ids.
class GpuProgramCache final {
public:
	/**
	 * Load or return a cached program.
	 *
	 * @param library Shader library used to resolve compiled shader binaries.
	 * @param program Program identifier.
	 * @param target Compiled shader target.
	 * @param status Status receiving diagnostics.
	 * @return Public program handle, or invalid handle on failure.
	 */
	[[nodiscard]] RenderProgramHandle load_program(
			const ShaderLibrary &library,
			ShaderProgramId program,
			ShaderTarget target,
			RendererStatus &status);
	/**
	 * Resolve a bgfx program handle.
	 *
	 * @param handle Public program handle.
	 * @return bgfx program handle, or invalid handle for missing/stale entries.
	 */
	[[nodiscard]] bgfx::ProgramHandle program(RenderProgramHandle handle) const noexcept;
	/**
	 * Return the cached handle for a program id.
	 *
	 * @param program Program id to resolve.
	 * @return Cached public handle, or invalid handle when not loaded.
	 */
	[[nodiscard]] RenderProgramHandle cached_handle(ShaderProgramId program) const noexcept;
	/// Return the number of live GPU programs.
	[[nodiscard]] std::size_t live_program_count() const noexcept;
	/// Destroy all loaded GPU programs.
	void clear() noexcept;

private:
	GpuProgramTable programs_;
	std::unordered_map<ShaderProgramId, RenderProgramHandle> handles_by_program_;
};

} // namespace crimson::gpu
