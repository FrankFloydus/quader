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
			crimson::ShaderProgramId::OverlayUnlit,
			crimson::gpu::ShaderStage::Vertex,
			crimson::gpu::ShaderTarget::Direct3D12,
			"dx12/overlay_unlit.vs.bin",
			"Direct3D12 overlay shader path uses the dx12 compiled target folder");

	expect_shader_path(library,
			crimson::ShaderProgramId::OverlayUnlit,
			crimson::gpu::ShaderStage::Fragment,
			crimson::gpu::ShaderTarget::Metal,
			"metal/overlay_unlit.fs.bin",
			"Metal overlay shader path uses the Metal compiled target folder");

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
			crimson::ShaderProgramId::OverlayEditLine,
			crimson::gpu::ShaderStage::Vertex,
			crimson::gpu::ShaderTarget::Direct3D12,
			"dx12/overlay_edit_line.vs.bin",
			"OverlayEditLine vertex shader path uses the Direct3D12 compiled target folder");

	expect_shader_path(library,
			crimson::ShaderProgramId::OverlayEditLine,
			crimson::gpu::ShaderStage::Fragment,
			crimson::gpu::ShaderTarget::Direct3D12,
			"dx12/overlay_edit_line.fs.bin",
			"OverlayEditLine fragment shader path uses the Direct3D12 compiled target folder");

	expect_shader_path(library,
			crimson::ShaderProgramId::OverlaySolid,
			crimson::gpu::ShaderStage::Fragment,
			crimson::gpu::ShaderTarget::Direct3D11,
			"dx11/overlay_solid.fs.bin",
			"OverlaySolid shader path uses the Direct3D11 compiled target folder");

	expect_shader_path(library,
			crimson::ShaderProgramId::OverlayDeviceSolid,
			crimson::gpu::ShaderStage::Vertex,
			crimson::gpu::ShaderTarget::Direct3D12,
			"dx12/overlay_device_solid.vs.bin",
			"OverlayDeviceSolid shader path uses the Direct3D12 compiled target folder");

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
			crimson::ShaderProgramId::OpaquePbr,
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

TEST(ShaderManifest, MeshSurfaceGridUsesWorldProjectionAndMaterialUniforms) {
	const std::vector<std::filesystem::path> kShaderSources = {
		"shaders/pbr/opaque_pbr.fs.sc",
		"shaders/pbr/alpha_cutout_pbr.fs.sc",
		"shaders/pbr/transparent_pbr.fs.sc",
		"shaders/pbr/unlit_surface.fs.sc",
	};

	for (const std::filesystem::path &relative_path : kShaderSources) {
		const std::string source = read_text_file(find_repo_file(relative_path));
		SCOPED_TRACE(relative_path.generic_string());
		expect_true(source.find("surfaceGridMinorColor") != std::string::npos,
				"mesh shader declares minor surface grid color data");
		expect_true(source.find("surfaceGridMajorColor") != std::string::npos,
				"mesh shader declares major surface grid color data");
		expect_true(source.find("surfaceGridSize") != std::string::npos,
				"mesh shader declares minor surface grid spacing data");
		expect_true(source.find("surfaceGridMajorSize") != std::string::npos,
				"mesh shader declares major surface grid spacing data");
		expect_true(source.find("surfaceGridMinorLineThickness") != std::string::npos,
				"mesh shader declares minor surface grid thickness data");
		expect_true(source.find("surfaceGridMajorLineThickness") != std::string::npos,
				"mesh shader declares major surface grid thickness data");
		expect_true(source.find("surfaceGridEnabled") != std::string::npos,
				"mesh shader declares surface grid enable data");
		expect_true(source.find("surfaceGridCoordinates(worldPosition") != std::string::npos,
				"mesh shader projects the surface grid from world position");
		expect_true(source.find("v_texcoord0.xz") == std::string::npos &&
						source.find("v_texcoord0.xy") == std::string::npos &&
						source.find("v_texcoord0.yz") == std::string::npos,
				"mesh surface grid projection does not use texture UV coordinates");
		expect_true(source.find("return coverage * min(thickness, 1.0)") == std::string::npos,
				"mesh surface grid coverage is not multiplied by min(thickness, 1.0)");
		expect_true(source.find("baseColor.rgb = applySurfaceGrid") != std::string::npos,
				"mesh surface grid composes over base color without replacing alpha");
	}
}

TEST(ShaderManifest, OverlayLineUsesReferenceDeviceSpaceFeatherPath) {
	const std::string vertex_source = read_text_file(find_repo_file("shaders/overlay_line.vs.sc"));
	const std::string fragment_source = read_text_file(find_repo_file("shaders/overlay_line.fs.sc"));
	expect_true(vertex_source.find("u_modelViewProj") == std::string::npos,
			"generic overlay line shader does not reproject device-space expanded quads");
	expect_true(vertex_source.find("gl_Position = vec4(a_position, 1.0)") != std::string::npos,
			"generic overlay line shader consumes direct device-space positions");
	expect_true(vertex_source.find("v_lineDistancePixels") != std::string::npos,
			"generic overlay line shader carries signed distance for feathering");
	expect_true(fragment_source.find("u_lineParams") != std::string::npos,
			"generic overlay line fragment shader uses line width and feather parameters");
	expect_true(fragment_source.find("smoothstep") != std::string::npos,
			"generic overlay line fragment shader feathers line edges");
	expect_true(fragment_source.find("u_lineColor.rgb * alpha") != std::string::npos,
			"generic overlay line fragment shader outputs premultiplied alpha");
}

TEST(ShaderManifest, OverlayEditLineUsesDistanceFeatherAndHardwareDepth) {
	const std::string vertex_source = read_text_file(find_repo_file("shaders/overlay_edit_line.vs.sc"));
	const std::string fragment_source = read_text_file(find_repo_file("shaders/overlay_edit_line.fs.sc"));
	expect_true(vertex_source.find("v_lineDistancePixels") != std::string::npos,
			"edit-line vertex shader outputs signed line distance");
	expect_true(vertex_source.find("gl_Position = vec4(a_position, 1.0)") != std::string::npos,
			"edit-line shader consumes device-space expanded quads directly");
	expect_true(vertex_source.find("v_lineCenterDevice") == std::string::npos,
			"edit-line vertex shader does not emit center-depth sampling coordinates");
	expect_true(fragment_source.find("s_sceneDepth") == std::string::npos,
			"edit-line fragment shader leaves occlusion to fixed-function depth testing");
	expect_true(fragment_source.find("u_editLineDepthParams") == std::string::npos,
			"edit-line fragment shader does not reconstruct scene depth");
	expect_true(fragment_source.find("discard") == std::string::npos,
			"edit-line fragment shader has no camera-angle-dependent depth discard");
	expect_true(fragment_source.find("smoothstep") != std::string::npos,
			"edit-line fragment shader keeps distance-based edge feathering");
}

TEST(ShaderManifest, OverlayDeviceSolidConsumesDeviceSpacePositions) {
	const std::string vertex_source = read_text_file(find_repo_file("shaders/overlay_device_solid.vs.sc"));
	expect_true(vertex_source.find("gl_Position = vec4(a_position, 1.0)") != std::string::npos,
			"device solid overlay shader consumes device-space positions directly");
	expect_true(vertex_source.find("u_modelViewProj") == std::string::npos,
			"device solid overlay shader does not reproject point quads");
}

TEST(ShaderManifest, GroundGridUsesPerspectiveFarClipFade) {
	const std::string source = read_text_file(find_repo_file("shaders/overlay_unlit.fs.sc"));
	expect_true(source.find("u_gridParams3") != std::string::npos,
			"ground grid shader has camera far-plane data");
	expect_true(source.find("farClipFade") != std::string::npos,
			"ground grid shader computes far clip fade");
	expect_true(source.find("worldCameraDistance") != std::string::npos,
			"ground grid far fade uses world camera distance");
	expect_true(source.find("mix(1.0, farClipFade") != std::string::npos,
			"ground grid shader can disable far fade for orthographic views");
}

} // namespace
