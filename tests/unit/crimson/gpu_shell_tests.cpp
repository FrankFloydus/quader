#include "crimson/gpu/gpu_device.hpp"
#include "crimson/gpu/gpu_resources.hpp"
#include "crimson/gpu/shader_library.hpp"

#include <gtest/gtest.h>

#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

void expect_true(bool condition, std::string_view message)
{
    EXPECT_TRUE(condition) << message;
}

struct TestResource {
    int value = 0;
};

TEST(GpuShell, ResourceTableRejectsStaleHandlesAfterDestroyAndReuse)
{
    crimson::gpu::GpuResourceTable<TestResource, crimson::RenderMeshHandle> table;

    const crimson::RenderMeshHandle first = table.create(TestResource{42});
    expect_true(crimson::is_valid_handle(first), "created resource handle is valid");
    expect_true(table.live_count() == 1, "created resource increments live count");
    expect_true(table.get(first) != nullptr && table.get(first)->value == 42, "created resource is retrievable");

    expect_true(table.destroy(first), "destroying a live resource succeeds");
    expect_true(table.live_count() == 0, "destroyed resource decrements live count");
    expect_true(table.get(first) == nullptr, "destroyed handle no longer resolves");
    expect_true(!table.destroy(first), "destroying a stale handle fails");

    const crimson::RenderMeshHandle second = table.create(TestResource{84});
    expect_true(second.index == first.index, "resource table reuses freed slots");
    expect_true(second.generation != first.generation, "reused slot advances generation");
    expect_true(table.get(first) == nullptr, "old generation does not resolve after reuse");
    expect_true(table.get(second) != nullptr && table.get(second)->value == 84, "new generation resolves after reuse");
}

TEST(GpuShell, BackendSelectionUsesPlatformPriorityAndPreferences)
{
    using crimson::GraphicsBackendPreference;
    using crimson::NativeSurfacePlatform;
    using crimson::gpu::GraphicsBackend;

    const std::vector<GraphicsBackend> all_windows = {
        GraphicsBackend::Direct3D11,
        GraphicsBackend::Vulkan,
        GraphicsBackend::Direct3D12,
    };

    expect_true(
        crimson::gpu::choose_backend(NativeSurfacePlatform::Windows, GraphicsBackendPreference::Auto, all_windows)
            == GraphicsBackend::Vulkan,
        "Windows auto preference chooses Vulkan first");
    expect_true(
        crimson::gpu::choose_backend(NativeSurfacePlatform::Windows, GraphicsBackendPreference::Direct3D11, all_windows)
            == GraphicsBackend::Direct3D11,
        "explicit Direct3D11 preference is honored when available");

    const std::vector<GraphicsBackend> windows_without_vulkan = {
        GraphicsBackend::Direct3D11,
        GraphicsBackend::Direct3D12,
    };
    expect_true(
        crimson::gpu::choose_backend(
            NativeSurfacePlatform::Windows,
            GraphicsBackendPreference::Auto,
            windows_without_vulkan)
            == GraphicsBackend::Direct3D12,
        "Windows auto preference falls back to Direct3D12");

    const std::vector<GraphicsBackend> metal_only = {GraphicsBackend::Metal};
    expect_true(
        crimson::gpu::choose_backend(NativeSurfacePlatform::MacOS, GraphicsBackendPreference::Auto, metal_only)
            == GraphicsBackend::Metal,
        "macOS auto preference chooses Metal");

    const std::vector<GraphicsBackend> vulkan_only = {GraphicsBackend::Vulkan};
    expect_true(
        crimson::gpu::choose_backend(NativeSurfacePlatform::LinuxX11, GraphicsBackendPreference::Auto, vulkan_only)
            == GraphicsBackend::Vulkan,
        "Linux auto preference chooses Vulkan");

    expect_true(
        !crimson::gpu::choose_backend(NativeSurfacePlatform::MacOS, GraphicsBackendPreference::Vulkan, metal_only),
        "unsupported explicit preference returns no backend");
    expect_true(
        !crimson::gpu::choose_backend(NativeSurfacePlatform::Unknown, GraphicsBackendPreference::Auto, all_windows),
        "unknown platform returns no backend");
}

TEST(GpuShell, BackendFailureDiagnosticIsStructured)
{
    const crimson::RendererDiagnostic diagnostic = crimson::gpu::make_backend_unsupported_diagnostic(
        crimson::NativeSurfacePlatform::Windows,
        crimson::GraphicsBackendPreference::Vulkan);

    expect_true(
        diagnostic.severity == crimson::RendererDiagnosticSeverity::Fatal,
        "backend failure diagnostic is fatal");
    expect_true(
        diagnostic.code == crimson::RendererDiagnosticCode::BackendUnsupported,
        "backend failure diagnostic uses BackendUnsupported code");
    expect_true(
        diagnostic.detail.find("Windows") != std::string::npos
            && diagnostic.detail.find("Vulkan") != std::string::npos,
        "backend failure diagnostic includes platform and request");
}

TEST(GpuShell, ShaderLibraryResolvesManifestTargetShaderPaths)
{
    crimson::gpu::ShaderLibrary library("shaders");

    expect_true(
        crimson::gpu::shader_file_name(
            crimson::ShaderProgramId::PrototypeLitCube,
            crimson::gpu::ShaderStage::Vertex)
            == "vs_cube.bin",
        "prototype cube vertex shader name is stable");
    expect_true(
        crimson::gpu::shader_file_name(
            crimson::ShaderProgramId::PrototypeGridOverlay,
            crimson::gpu::ShaderStage::Fragment)
            == "fs_grid.bin",
        "prototype grid fragment shader name is stable");
    expect_true(
        crimson::gpu::shader_file_name(
            crimson::ShaderProgramId::OpaquePbr,
            crimson::gpu::ShaderStage::Fragment)
            == "opaque_pbr.fs.bin",
        "OpaquePbr fragment shader name is stable");
    expect_true(
        crimson::gpu::shader_file_name(
            crimson::ShaderProgramId::OverlayUnlit,
            crimson::gpu::ShaderStage::Vertex)
            == "overlay_unlit.vs.bin",
        "OverlayUnlit vertex shader name is stable");
    expect_true(
        crimson::gpu::shader_file_name(
            crimson::ShaderProgramId::Picking,
            crimson::gpu::ShaderStage::Fragment)
            == "picking.fs.bin",
        "Picking fragment shader name is stable");
    expect_true(
        library.shader_path(
            crimson::ShaderProgramId::PrototypeLitCube,
            crimson::gpu::ShaderStage::Fragment,
            crimson::gpu::ShaderTarget::Vulkan)
            .generic_string()
            == "shaders/vulkan/fs_cube.bin",
        "shader library resolves Vulkan paths relative to shader root");
    expect_true(
        library.shader_path(
            crimson::ShaderProgramId::PrototypeLitCube,
            crimson::gpu::ShaderStage::Fragment,
            crimson::gpu::ShaderTarget::Direct3D12)
            .generic_string()
            == "shaders/dx12/fs_cube.bin",
        "shader library resolves Direct3D12 target paths");
    expect_true(
        crimson::gpu::shader_target_directory_name(crimson::gpu::ShaderTarget::Metal) == "metal",
        "Metal target directory name is stable");
}

} // namespace
