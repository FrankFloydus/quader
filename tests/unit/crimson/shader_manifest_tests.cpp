#include "crimson/gpu/shader_library.hpp"

#include <gtest/gtest.h>

#include <iostream>
#include <string_view>

namespace {

void expect_true(bool condition, std::string_view message)
{
    EXPECT_TRUE(condition) << message;
}

TEST(ShaderManifest, RuntimeLookupPointsToCompiledTargetFolders)
{
    crimson::gpu::ShaderLibrary library("runtime/shaders");

    const auto vulkan = library.shader_binary(
        crimson::ShaderProgramId::PrototypeLitCube,
        crimson::gpu::ShaderStage::Vertex,
        crimson::gpu::ShaderTarget::Vulkan);
    expect_true(vulkan.has_value(), "prototype cube vertex shader has a manifest entry");
    expect_true(
        vulkan->relative_path.generic_string() == "vulkan/vs_cube.bin",
        "Vulkan shader path uses the Vulkan compiled target folder");

    const auto dx11 = library.shader_binary(
        crimson::ShaderProgramId::PrototypeLitCube,
        crimson::gpu::ShaderStage::Fragment,
        crimson::gpu::ShaderTarget::Direct3D11);
    expect_true(dx11.has_value(), "prototype cube fragment shader has a manifest entry");
    expect_true(
        dx11->relative_path.generic_string() == "dx11/fs_cube.bin",
        "Direct3D11 shader path uses the dx11 compiled target folder");

    const auto dx12 = library.shader_binary(
        crimson::ShaderProgramId::PrototypeGridOverlay,
        crimson::gpu::ShaderStage::Vertex,
        crimson::gpu::ShaderTarget::Direct3D12);
    expect_true(dx12.has_value(), "prototype grid vertex shader has a manifest entry");
    expect_true(
        dx12->relative_path.generic_string() == "dx12/vs_grid.bin",
        "Direct3D12 shader path uses the dx12 compiled target folder");

    const auto metal = library.shader_binary(
        crimson::ShaderProgramId::PrototypeGridOverlay,
        crimson::gpu::ShaderStage::Fragment,
        crimson::gpu::ShaderTarget::Metal);
    expect_true(metal.has_value(), "prototype grid fragment shader has a manifest entry");
    expect_true(
        metal->relative_path.generic_string() == "metal/fs_grid.bin",
        "Metal shader path uses the Metal compiled target folder");

    const auto tone_map = library.shader_binary(
        crimson::ShaderProgramId::ToneMap,
        crimson::gpu::ShaderStage::Fragment,
        crimson::gpu::ShaderTarget::Vulkan);
    expect_true(tone_map.has_value(), "tone-map fragment shader has a manifest entry");
    expect_true(
        tone_map->relative_path.generic_string() == "vulkan/tone_map.bin",
        "tone-map shader path uses the Vulkan compiled target folder");

    const auto present = library.shader_binary(
        crimson::ShaderProgramId::Present,
        crimson::gpu::ShaderStage::Fragment,
        crimson::gpu::ShaderTarget::Direct3D12);
    expect_true(present.has_value(), "present fragment shader has a manifest entry");
    expect_true(
        present->relative_path.generic_string() == "dx12/present.bin",
        "present shader path uses the Direct3D12 compiled target folder");

    const auto opaque_pbr = library.shader_binary(
        crimson::ShaderProgramId::OpaquePbr,
        crimson::gpu::ShaderStage::Fragment,
        crimson::gpu::ShaderTarget::Vulkan);
    expect_true(opaque_pbr.has_value(), "OpaquePbr fragment shader has a manifest entry");
    expect_true(
        opaque_pbr->relative_path.generic_string() == "vulkan/opaque_pbr.fs.bin",
        "OpaquePbr shader path uses the Vulkan compiled target folder");

    const auto cutout_pbr = library.shader_binary(
        crimson::ShaderProgramId::AlphaCutoutPbr,
        crimson::gpu::ShaderStage::Vertex,
        crimson::gpu::ShaderTarget::Direct3D11);
    expect_true(cutout_pbr.has_value(), "AlphaCutoutPbr vertex shader has a manifest entry");
    expect_true(
        cutout_pbr->relative_path.generic_string() == "dx11/alpha_cutout_pbr.vs.bin",
        "AlphaCutoutPbr shader path uses the Direct3D11 compiled target folder");

    const auto transparent_pbr = library.shader_binary(
        crimson::ShaderProgramId::TransparentPbr,
        crimson::gpu::ShaderStage::Fragment,
        crimson::gpu::ShaderTarget::Metal);
    expect_true(transparent_pbr.has_value(), "TransparentPbr fragment shader has a manifest entry");
    expect_true(
        transparent_pbr->relative_path.generic_string() == "metal/transparent_pbr.fs.bin",
        "TransparentPbr shader path uses the Metal compiled target folder");

    const auto overlay_unlit = library.shader_binary(
        crimson::ShaderProgramId::OverlayUnlit,
        crimson::gpu::ShaderStage::Fragment,
        crimson::gpu::ShaderTarget::Vulkan);
    expect_true(overlay_unlit.has_value(), "OverlayUnlit fragment shader has a manifest entry");
    expect_true(
        overlay_unlit->relative_path.generic_string() == "vulkan/overlay_unlit.fs.bin",
        "OverlayUnlit shader path uses the Vulkan compiled target folder");

    const auto picking = library.shader_binary(
        crimson::ShaderProgramId::Picking,
        crimson::gpu::ShaderStage::Fragment,
        crimson::gpu::ShaderTarget::Direct3D11);
    expect_true(picking.has_value(), "Picking fragment shader has a manifest entry");
    expect_true(
        picking->relative_path.generic_string() == "dx11/picking.fs.bin",
        "Picking shader path uses the Direct3D11 compiled target folder");
}

TEST(ShaderManifest, RuntimeLookupNeverPointsToShaderSources)
{
    crimson::gpu::ShaderLibrary library("runtime/shaders");
    const auto binary = library.shader_binary(
        crimson::ShaderProgramId::PrototypeLitCube,
        crimson::gpu::ShaderStage::Vertex,
        crimson::gpu::ShaderTarget::Vulkan);

    expect_true(binary.has_value(), "shader binary ref exists");
    expect_true(binary->relative_path.extension() == ".bin", "shader lookup returns compiled binaries only");
    expect_true(binary->relative_path.extension() != ".sc", "shader lookup never returns shader source files");
}

} // namespace
