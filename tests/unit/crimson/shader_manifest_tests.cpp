/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
#include "crimson/gpu/shader_library.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <string_view>
#include <vector>

namespace {

void expect_true(bool condition, std::string_view message) {
	EXPECT_TRUE(condition) << message;
}

void expect_shader_path(
		const crimson::gpu::ShaderLibrary &library,
		crimson::ShaderProgramId program,
		crimson::gpu::ShaderStage stage,
		crimson::gpu::ShaderTarget target,
		std::string_view expected_path,
		std::string_view message) {
	const auto kBinary = library.shader_binary(program, stage, target);
	if (!kBinary.has_value()) {
		ADD_FAILURE() << message;
		return;
	}
	EXPECT_EQ(kBinary.value().relative_path.generic_string(), expected_path);
}

[[nodiscard]] std::filesystem::path find_repo_file(std::filesystem::path relative_path) {
	std::filesystem::path current = std::filesystem::current_path();
	for (int depth = 0; depth < 8; ++depth) {
		const std::filesystem::path candidate = current / relative_path;
		if (std::filesystem::exists(candidate)) {
			return candidate;
		}
		if (!current.has_parent_path() || current == current.parent_path()) {
			break;
		}
		current = current.parent_path();
	}
	return relative_path;
}

[[nodiscard]] std::string read_text_file(const std::filesystem::path &path) {
	std::ifstream file(path, std::ios::binary);
	if (!file) {
		ADD_FAILURE() << "failed to read " << path.string();
		return {};
	}
	return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
}

TEST(ShaderManifest, RuntimeLookupPointsToCompiledTargetFolders) {
	crimson::gpu::ShaderLibrary library("runtime/shaders");

	expect_shader_path(library,
			crimson::ShaderProgramId::ViewportLitCube,
			crimson::gpu::ShaderStage::Vertex,
			crimson::gpu::ShaderTarget::Vulkan,
			"vulkan/vs_cube.bin",
			"Vulkan shader path uses the Vulkan compiled target folder");

	expect_shader_path(library,
			crimson::ShaderProgramId::ViewportLitCube,
			crimson::gpu::ShaderStage::Fragment,
			crimson::gpu::ShaderTarget::Direct3D11,
			"dx11/fs_cube.bin",
			"Direct3D11 shader path uses the dx11 compiled target folder");

	expect_shader_path(library,
			crimson::ShaderProgramId::ViewportGridOverlay,
			crimson::gpu::ShaderStage::Vertex,
			crimson::gpu::ShaderTarget::Direct3D12,
			"dx12/vs_grid.bin",
			"Direct3D12 shader path uses the dx12 compiled target folder");

	expect_shader_path(library,
			crimson::ShaderProgramId::ViewportGridOverlay,
			crimson::gpu::ShaderStage::Fragment,
			crimson::gpu::ShaderTarget::Metal,
			"metal/fs_grid.bin",
			"Metal shader path uses the Metal compiled target folder");

	expect_shader_path(library,
			crimson::ShaderProgramId::ToneMap,
			crimson::gpu::ShaderStage::Fragment,
			crimson::gpu::ShaderTarget::Vulkan,
			"vulkan/tone_map.bin",
			"tone-map shader path uses the Vulkan compiled target folder");

	expect_shader_path(library,
			crimson::ShaderProgramId::Present,
			crimson::gpu::ShaderStage::Fragment,
			crimson::gpu::ShaderTarget::Direct3D12,
			"dx12/present.bin",
			"present shader path uses the Direct3D12 compiled target folder");

	expect_shader_path(library,
			crimson::ShaderProgramId::OpaquePbr,
			crimson::gpu::ShaderStage::Fragment,
			crimson::gpu::ShaderTarget::Vulkan,
			"vulkan/opaque_pbr.fs.bin",
			"OpaquePbr shader path uses the Vulkan compiled target folder");

	expect_shader_path(library,
			crimson::ShaderProgramId::AlphaCutoutPbr,
			crimson::gpu::ShaderStage::Vertex,
			crimson::gpu::ShaderTarget::Direct3D11,
			"dx11/alpha_cutout_pbr.vs.bin",
			"AlphaCutoutPbr shader path uses the Direct3D11 compiled target folder");

	expect_shader_path(library,
			crimson::ShaderProgramId::TransparentPbr,
			crimson::gpu::ShaderStage::Fragment,
			crimson::gpu::ShaderTarget::Metal,
			"metal/transparent_pbr.fs.bin",
			"TransparentPbr shader path uses the Metal compiled target folder");

	expect_shader_path(library,
			crimson::ShaderProgramId::OverlayUnlit,
			crimson::gpu::ShaderStage::Fragment,
			crimson::gpu::ShaderTarget::Vulkan,
			"vulkan/overlay_unlit.fs.bin",
			"OverlayUnlit shader path uses the Vulkan compiled target folder");

	expect_shader_path(library,
			crimson::ShaderProgramId::OverlayLine,
			crimson::gpu::ShaderStage::Vertex,
			crimson::gpu::ShaderTarget::Direct3D12,
			"dx12/overlay_line.vs.bin",
			"OverlayLine shader path uses the Direct3D12 compiled target folder");

	expect_shader_path(library,
			crimson::ShaderProgramId::Picking,
			crimson::gpu::ShaderStage::Fragment,
			crimson::gpu::ShaderTarget::Direct3D11,
			"dx11/picking.fs.bin",
			"Picking shader path uses the Direct3D11 compiled target folder");
}

TEST(ShaderManifest, RuntimeLookupNeverPointsToShaderSources) {
	crimson::gpu::ShaderLibrary library("runtime/shaders");
	const auto kBinary = library.shader_binary(
			crimson::ShaderProgramId::ViewportLitCube,
			crimson::gpu::ShaderStage::Vertex,
			crimson::gpu::ShaderTarget::Vulkan);

	expect_true(kBinary.has_value(), "shader binary ref exists");
	if (!kBinary.has_value()) {
		return;
	}
	const auto &binary_ref = kBinary.value();
	expect_true(binary_ref.relative_path.extension() == ".bin", "shader lookup returns compiled binaries only");
	expect_true(binary_ref.relative_path.extension() != ".sc", "shader lookup never returns shader source files");
}

TEST(ShaderManifest, PbrTextureSamplingPreservesCrimsonPngUvOrientation) {
	const std::vector<std::filesystem::path> kShaderSources = {
		"shaders/pbr/opaque_pbr.fs.sc",
		"shaders/pbr/alpha_cutout_pbr.fs.sc",
		"shaders/pbr/transparent_pbr.fs.sc",
		"shaders/pbr/unlit_surface.fs.sc",
	};

	for (const std::filesystem::path &relative_path : kShaderSources) {
		const std::string source = read_text_file(find_repo_file(relative_path));
		SCOPED_TRACE(relative_path.generic_string());
		expect_true(source.find("1.0 - v_texcoord0.y") == std::string::npos,
				"Crimson samples PNG-backed base-color textures without an extra shader-side vertical flip");
		expect_true(source.find("vec2 uv0 = v_texcoord0 * u_pbrUvTransform0.xy + u_pbrUvTransform0.zw;") != std::string::npos,
				"PBR shader samples the generated UV0 orientation directly");
	}
}

} // namespace
