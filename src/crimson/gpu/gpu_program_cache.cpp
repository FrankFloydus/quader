/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
#include "crimson/gpu/gpu_program_cache.hpp"

#include "crimson/gpu/shader_library_runtime.hpp"

#include <string>

namespace crimson::gpu {
namespace {

void destroy_shader(bgfx::ShaderHandle &handle) noexcept {
	if (bgfx::isValid(handle)) {
		bgfx::destroy(handle);
		handle = BGFX_INVALID_HANDLE;
	}
}

void push_program_creation_error(
		RendererStatus &status,
		ShaderProgramId program) {
	status.diagnostics.push_back(RendererDiagnostic{
			.severity = RendererDiagnosticSeverity::Error,
			.code = RendererDiagnosticCode::ShaderProgramCreationFailed,
			.message = "Crimson failed to create a runtime shader program.",
			.detail = std::string(shader_program_id_name(program)),
			.subsystem = "gpu.program",
			.resource_name = std::string(shader_program_id_name(program)),
	});
}

} // namespace

RenderProgramHandle GpuProgramCache::load_program(
		const ShaderLibrary &library,
		ShaderProgramId program,
		ShaderTarget target,
		RendererStatus &status) {
	if (const auto kCached = handles_by_program_.find(program); kCached != handles_by_program_.end()) {
		return kCached->second;
	}

	bgfx::ShaderHandle vertex = load_shader(library, program, ShaderStage::Vertex, target, status.diagnostics);
	bgfx::ShaderHandle fragment = load_shader(library, program, ShaderStage::Fragment, target, status.diagnostics);
	if (!bgfx::isValid(vertex) || !bgfx::isValid(fragment)) {
		destroy_shader(vertex);
		destroy_shader(fragment);
		return {};
	}

	bgfx::ProgramHandle bgfx_program = bgfx::createProgram(vertex, fragment, true);
	if (!bgfx::isValid(bgfx_program)) {
		push_program_creation_error(status, program);
		return {};
	}

	const RenderProgramHandle kHandle = programs_.create(GpuProgramResource{
			.program = UniqueProgramHandle(bgfx_program),
	});
	handles_by_program_.emplace(program, kHandle);
	return kHandle;
}

bgfx::ProgramHandle GpuProgramCache::program(RenderProgramHandle handle) const noexcept {
	if (const GpuProgramResource *resource = programs_.get(handle)) {
		return resource->program.get();
	}
	return BGFX_INVALID_HANDLE;
}

RenderProgramHandle GpuProgramCache::cached_handle(ShaderProgramId program) const noexcept {
	const auto kFound = handles_by_program_.find(program);
	return kFound == handles_by_program_.end() ? RenderProgramHandle{} : kFound->second;
}

std::size_t GpuProgramCache::live_program_count() const noexcept {
	return programs_.live_count();
}

void GpuProgramCache::clear() noexcept {
	handles_by_program_.clear();
	programs_.clear();
}

} // namespace crimson::gpu
