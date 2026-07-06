/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
#include "crimson/gpu/gpu_device.hpp"
#include "crimson/gpu/gpu_resources.hpp"
#include "crimson/gpu/shader_library.hpp"

#include <gtest/gtest.h>

#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

void expect_true(bool condition, std::string_view message) {
	EXPECT_TRUE(condition) << message;
}

struct TestResource {
	int value = 0;
};

TEST(GpuShell, ResourceTableRejectsStaleHandlesAfterDestroyAndReuse) {
	crimson::gpu::GpuResourceTable<TestResource, crimson::RenderMeshHandle> table;

	const crimson::RenderMeshHandle kFirst = table.create(TestResource{ 42 });
	expect_true(crimson::is_valid_handle(kFirst), "created resource handle is valid");
	expect_true(table.live_count() == 1, "created resource increments live count");
	expect_true(table.get(kFirst) != nullptr && table.get(kFirst)->value == 42, "created resource is retrievable");

	expect_true(table.destroy(kFirst), "destroying a live resource succeeds");
	expect_true(table.live_count() == 0, "destroyed resource decrements live count");
	expect_true(table.get(kFirst) == nullptr, "destroyed handle no longer resolves");
	expect_true(!table.destroy(kFirst), "destroying a stale handle fails");

	const crimson::RenderMeshHandle kSecond = table.create(TestResource{ 84 });
	expect_true(kSecond.index == kFirst.index, "resource table reuses freed slots");
	expect_true(kSecond.generation != kFirst.generation, "reused slot advances generation");
	expect_true(table.get(kFirst) == nullptr, "old generation does not resolve after reuse");
	expect_true(table.get(kSecond) != nullptr && table.get(kSecond)->value == 84, "new generation resolves after reuse");
}

TEST(GpuShell, BackendSelectionUsesPlatformPriorityAndPreferences) {
	using crimson::GraphicsBackendPreference;
	using crimson::NativeSurfacePlatform;
	using crimson::gpu::GraphicsBackend;

	const std::vector<GraphicsBackend> kAllWindows = {
		GraphicsBackend::Direct3D11,
		GraphicsBackend::Vulkan,
		GraphicsBackend::Direct3D12,
	};

	expect_true(
			crimson::gpu::choose_backend(NativeSurfacePlatform::Windows, GraphicsBackendPreference::Auto, kAllWindows) == GraphicsBackend::Vulkan,
			"Windows auto preference chooses Vulkan first");
	expect_true(
			crimson::gpu::choose_backend(NativeSurfacePlatform::Windows, GraphicsBackendPreference::Direct3D11, kAllWindows) == GraphicsBackend::Direct3D11,
			"explicit Direct3D11 preference is honored when available");

	const std::vector<GraphicsBackend> kWindowsWithoutVulkan = {
		GraphicsBackend::Direct3D11,
		GraphicsBackend::Direct3D12,
	};
	expect_true(
			crimson::gpu::choose_backend(
					NativeSurfacePlatform::Windows,
					GraphicsBackendPreference::Auto,
					kWindowsWithoutVulkan) == GraphicsBackend::Direct3D12,
			"Windows auto preference falls back to Direct3D12");

	const std::vector<GraphicsBackend> kMetalOnly = { GraphicsBackend::Metal };
	expect_true(
			crimson::gpu::choose_backend(NativeSurfacePlatform::MacOS, GraphicsBackendPreference::Auto, kMetalOnly) == GraphicsBackend::Metal,
			"macOS auto preference chooses Metal");

	const std::vector<GraphicsBackend> kVulkanOnly = { GraphicsBackend::Vulkan };
	expect_true(
			crimson::gpu::choose_backend(NativeSurfacePlatform::LinuxX11, GraphicsBackendPreference::Auto, kVulkanOnly) == GraphicsBackend::Vulkan,
			"Linux auto preference chooses Vulkan");

	expect_true(
			!crimson::gpu::choose_backend(NativeSurfacePlatform::MacOS, GraphicsBackendPreference::Vulkan, kMetalOnly),
			"unsupported explicit preference returns no backend");
	expect_true(
			!crimson::gpu::choose_backend(NativeSurfacePlatform::Unknown, GraphicsBackendPreference::Auto, kAllWindows),
			"unknown platform returns no backend");
}

TEST(GpuShell, BackendFailureDiagnosticIsStructured) {
	const crimson::RendererDiagnostic kDiagnostic = crimson::gpu::make_backend_unsupported_diagnostic(
			crimson::NativeSurfacePlatform::Windows,
			crimson::GraphicsBackendPreference::Vulkan);

	expect_true(
			kDiagnostic.severity == crimson::RendererDiagnosticSeverity::Fatal,
			"backend failure diagnostic is fatal");
	expect_true(
			kDiagnostic.code == crimson::RendererDiagnosticCode::BackendUnsupported,
			"backend failure diagnostic uses BackendUnsupported code");
	expect_true(
			kDiagnostic.detail.find("Windows") != std::string::npos && kDiagnostic.detail.find("Vulkan") != std::string::npos,
			"backend failure diagnostic includes platform and request");
}

TEST(GpuShell, ShaderLibraryResolvesManifestTargetShaderPaths) {
	crimson::gpu::ShaderLibrary library("shaders");

	expect_true(
			crimson::gpu::shader_file_name(
					crimson::ShaderProgramId::OpaquePbr,
					crimson::gpu::ShaderStage::Fragment) == "opaque_pbr.fs.bin",
			"OpaquePbr fragment shader name is stable");
	expect_true(
			crimson::gpu::shader_file_name(
					crimson::ShaderProgramId::OverlayUnlit,
					crimson::gpu::ShaderStage::Vertex) == "overlay_unlit.vs.bin",
			"OverlayUnlit vertex shader name is stable");
	expect_true(
			crimson::gpu::shader_file_name(
					crimson::ShaderProgramId::OverlayLine,
					crimson::gpu::ShaderStage::Fragment) == "overlay_line.fs.bin",
			"OverlayLine fragment shader name is stable");
	expect_true(
			crimson::gpu::shader_file_name(
					crimson::ShaderProgramId::OverlayEditLine,
					crimson::gpu::ShaderStage::Fragment) == "overlay_edit_line.fs.bin",
			"OverlayEditLine fragment shader name is stable");
	expect_true(
			crimson::gpu::shader_file_name(
					crimson::ShaderProgramId::OverlaySolid,
					crimson::gpu::ShaderStage::Vertex) == "overlay_solid.vs.bin",
			"OverlaySolid vertex shader name is stable");
	expect_true(
			crimson::gpu::shader_file_name(
					crimson::ShaderProgramId::OverlayDeviceSolid,
					crimson::gpu::ShaderStage::Vertex) == "overlay_device_solid.vs.bin",
			"OverlayDeviceSolid vertex shader name is stable");
	expect_true(
			crimson::gpu::shader_file_name(
					crimson::ShaderProgramId::Picking,
					crimson::gpu::ShaderStage::Fragment) == "picking.fs.bin",
			"Picking fragment shader name is stable");
	expect_true(
			library.shader_path(
						   crimson::ShaderProgramId::OverlayUnlit,
						   crimson::gpu::ShaderStage::Fragment,
						   crimson::gpu::ShaderTarget::Vulkan)
							.generic_string() == "shaders/vulkan/overlay_unlit.fs.bin",
			"shader library resolves Vulkan overlay paths relative to shader root");
	expect_true(
			library.shader_path(
						   crimson::ShaderProgramId::OverlayUnlit,
						   crimson::gpu::ShaderStage::Vertex,
						   crimson::gpu::ShaderTarget::Direct3D12)
							.generic_string() == "shaders/dx12/overlay_unlit.vs.bin",
			"shader library resolves Direct3D12 overlay target paths");
	expect_true(
			library.shader_path(
						   crimson::ShaderProgramId::OverlaySolid,
						   crimson::gpu::ShaderStage::Fragment,
						   crimson::gpu::ShaderTarget::Direct3D11)
							.generic_string() == "shaders/dx11/overlay_solid.fs.bin",
			"shader library resolves Direct3D11 solid overlay target paths");
	expect_true(
			library.shader_path(
						   crimson::ShaderProgramId::OverlayDeviceSolid,
						   crimson::gpu::ShaderStage::Fragment,
						   crimson::gpu::ShaderTarget::Direct3D11)
							.generic_string() == "shaders/dx11/overlay_device_solid.fs.bin",
			"shader library resolves Direct3D11 device solid overlay target paths");
	expect_true(
			crimson::gpu::shader_target_directory_name(crimson::gpu::ShaderTarget::Metal) == "metal",
			"Metal target directory name is stable");
}

} // namespace
