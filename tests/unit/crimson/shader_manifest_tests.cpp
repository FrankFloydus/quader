#include "crimson/gpu/shader_library.hpp"

#include <gtest/gtest.h>

#include <iostream>
#include <string_view>

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

TEST(ShaderManifest, RuntimeLookupPointsToCompiledTargetFolders) {
	crimson::gpu::ShaderLibrary library("runtime/shaders");

	expect_shader_path(library,
			crimson::ShaderProgramId::PrototypeLitCube,
			crimson::gpu::ShaderStage::Vertex,
			crimson::gpu::ShaderTarget::Vulkan,
			"vulkan/vs_cube.bin",
			"Vulkan shader path uses the Vulkan compiled target folder");

	expect_shader_path(library,
			crimson::ShaderProgramId::PrototypeLitCube,
			crimson::gpu::ShaderStage::Fragment,
			crimson::gpu::ShaderTarget::Direct3D11,
			"dx11/fs_cube.bin",
			"Direct3D11 shader path uses the dx11 compiled target folder");

	expect_shader_path(library,
			crimson::ShaderProgramId::PrototypeGridOverlay,
			crimson::gpu::ShaderStage::Vertex,
			crimson::gpu::ShaderTarget::Direct3D12,
			"dx12/vs_grid.bin",
			"Direct3D12 shader path uses the dx12 compiled target folder");

	expect_shader_path(library,
			crimson::ShaderProgramId::PrototypeGridOverlay,
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
			crimson::ShaderProgramId::Picking,
			crimson::gpu::ShaderStage::Fragment,
			crimson::gpu::ShaderTarget::Direct3D11,
			"dx11/picking.fs.bin",
			"Picking shader path uses the Direct3D11 compiled target folder");
}

TEST(ShaderManifest, RuntimeLookupNeverPointsToShaderSources) {
	crimson::gpu::ShaderLibrary library("runtime/shaders");
	const auto kBinary = library.shader_binary(
			crimson::ShaderProgramId::PrototypeLitCube,
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

} // namespace
