/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
#include "crimson/gpu/shader_library_runtime.hpp"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <bgfx/bgfx.h>

namespace crimson::gpu {
namespace {

[[nodiscard]] std::string shader_resource_name(
		ShaderProgramId program,
		ShaderStage stage) {
	return std::string(shader_program_id_name(program)) + "/" + std::string(shader_stage_name(stage));
}

} // namespace

ShaderLibrary::ShaderLibrary(std::filesystem::path shader_root) : shader_root_(std::move(shader_root)) {
}

const std::filesystem::path &ShaderLibrary::shader_root() const noexcept {
	return shader_root_;
}

std::filesystem::path ShaderLibrary::shader_path(
		ShaderProgramId program,
		ShaderStage stage,
		ShaderTarget target) const {
	return shader_root_ / shader_target_directory_name(target) / shader_file_name(program, stage);
}

std::optional<ShaderBinaryRef> ShaderLibrary::shader_binary(
		ShaderProgramId program,
		ShaderStage stage,
		ShaderTarget target) const {
	const std::string_view kFileName = shader_file_name(program, stage);
	if (kFileName.empty()) {
		return std::nullopt;
	}

	return ShaderBinaryRef{
		.program = program,
		.stage = stage,
		.target = target,
		.relative_path = std::filesystem::path(shader_target_directory_name(target)) / kFileName,
	};
}

bgfx::ShaderHandle load_shader(
		const ShaderLibrary &library,
		ShaderProgramId program,
		ShaderStage stage,
		ShaderTarget target,
		std::vector<RendererDiagnostic> &diagnostics) {
	const std::optional<ShaderBinaryRef> kBinary = library.shader_binary(program, stage, target);
	if (!kBinary) {
		diagnostics.push_back(RendererDiagnostic{
				.severity = RendererDiagnosticSeverity::Error,
				.code = RendererDiagnosticCode::ShaderManifestInvalid,
				.message = "Crimson shader program is not present in the runtime manifest map.",
				.detail = shader_resource_name(program, stage),
				.subsystem = "shader",
				.resource_name = shader_resource_name(program, stage),
		});
		return BGFX_INVALID_HANDLE;
	}

	const std::filesystem::path kPath = library.shader_root() / kBinary->relative_path;
	if (!std::filesystem::is_directory(kPath.parent_path())) {
		diagnostics.push_back(RendererDiagnostic{
				.severity = RendererDiagnosticSeverity::Error,
				.code = RendererDiagnosticCode::ShaderTargetMissing,
				.message = "Crimson shader target folder is missing.",
				.detail = kPath.parent_path().string(),
				.subsystem = "shader",
				.resource_name = std::string(shader_target_directory_name(target)),
		});
		return BGFX_INVALID_HANDLE;
	}

	std::ifstream file(kPath, std::ios::binary);
	if (!file) {
		diagnostics.push_back(RendererDiagnostic{
				.severity = RendererDiagnosticSeverity::Error,
				.code = RendererDiagnosticCode::ShaderFileMissing,
				.message = "Crimson shader file is missing.",
				.detail = kPath.string(),
				.subsystem = "shader",
				.resource_name = shader_resource_name(program, stage),
		});
		return BGFX_INVALID_HANDLE;
	}

	const std::vector<char> kBytes{
		std::istreambuf_iterator<char>(file),
		std::istreambuf_iterator<char>()
	};
	if (kBytes.empty()) {
		diagnostics.push_back(RendererDiagnostic{
				.severity = RendererDiagnosticSeverity::Error,
				.code = RendererDiagnosticCode::ShaderFileMissing,
				.message = "Crimson shader file is empty.",
				.detail = kPath.string(),
				.subsystem = "shader",
				.resource_name = shader_resource_name(program, stage),
		});
		return BGFX_INVALID_HANDLE;
	}

	const bgfx::Memory *memory = bgfx::copy(kBytes.data(), static_cast<std::uint32_t>(kBytes.size()));
	bgfx::ShaderHandle shader = bgfx::createShader(memory);
	if (!bgfx::isValid(shader)) {
		diagnostics.push_back(RendererDiagnostic{
				.severity = RendererDiagnosticSeverity::Error,
				.code = RendererDiagnosticCode::ShaderProgramCreationFailed,
				.message = "Crimson failed to create a runtime shader handle.",
				.detail = kPath.string(),
				.subsystem = "gpu.shader",
				.resource_name = shader_resource_name(program, stage),
		});
		return BGFX_INVALID_HANDLE;
	}

	const std::string kName = kPath.filename().string();
	bgfx::setName(shader, kName.c_str());
	return shader;
}

std::string_view shader_target_name(ShaderTarget target) noexcept {
	switch (target) {
		case ShaderTarget::Vulkan:
			return "Vulkan";
		case ShaderTarget::Direct3D11:
			return "Direct3D11";
		case ShaderTarget::Direct3D12:
			return "Direct3D12";
		case ShaderTarget::Metal:
			return "Metal";
	}

	return "Unknown";
}

std::string_view shader_target_directory_name(ShaderTarget target) noexcept {
	switch (target) {
		case ShaderTarget::Vulkan:
			return "vulkan";
		case ShaderTarget::Direct3D11:
			return "dx11";
		case ShaderTarget::Direct3D12:
			return "dx12";
		case ShaderTarget::Metal:
			return "metal";
	}

	return "";
}

std::string_view shader_stage_name(ShaderStage stage) noexcept {
	switch (stage) {
		case ShaderStage::Vertex:
			return "Vertex";
		case ShaderStage::Fragment:
			return "Fragment";
	}

	return "Unknown";
}

std::string_view shader_file_name(ShaderProgramId program, ShaderStage stage) noexcept {
	switch (program) {
		case ShaderProgramId::ToneMap:
			return stage == ShaderStage::Vertex ? "fullscreen_triangle.bin" : "tone_map.bin";
		case ShaderProgramId::Present:
			return stage == ShaderStage::Vertex ? "fullscreen_triangle.bin" : "present.bin";
		case ShaderProgramId::OpaquePbr:
			return stage == ShaderStage::Vertex ? "opaque_pbr.vs.bin" : "opaque_pbr.fs.bin";
		case ShaderProgramId::AlphaCutoutPbr:
			return stage == ShaderStage::Vertex ? "alpha_cutout_pbr.vs.bin" : "alpha_cutout_pbr.fs.bin";
		case ShaderProgramId::TransparentPbr:
			return stage == ShaderStage::Vertex ? "transparent_pbr.vs.bin" : "transparent_pbr.fs.bin";
		case ShaderProgramId::UnlitSurface:
			return stage == ShaderStage::Vertex ? "unlit_surface.vs.bin" : "unlit_surface.fs.bin";
		case ShaderProgramId::OverlayUnlit:
			return stage == ShaderStage::Vertex ? "overlay_unlit.vs.bin" : "overlay_unlit.fs.bin";
		case ShaderProgramId::OverlaySolid:
			return stage == ShaderStage::Vertex ? "overlay_solid.vs.bin" : "overlay_solid.fs.bin";
		case ShaderProgramId::OverlayDeviceSolid:
			return stage == ShaderStage::Vertex ? "overlay_device_solid.vs.bin" : "overlay_device_solid.fs.bin";
		case ShaderProgramId::OverlayLine:
			return stage == ShaderStage::Vertex ? "overlay_line.vs.bin" : "overlay_line.fs.bin";
		case ShaderProgramId::OverlayEditLine:
			return stage == ShaderStage::Vertex ? "overlay_edit_line.vs.bin" : "overlay_edit_line.fs.bin";
		case ShaderProgramId::Picking:
			return stage == ShaderStage::Vertex ? "picking.vs.bin" : "picking.fs.bin";
	}

	return "";
}

} // namespace crimson::gpu
