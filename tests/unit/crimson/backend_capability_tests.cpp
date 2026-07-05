#include "crimson/gpu/gpu_capabilities.hpp"

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

[[nodiscard]] const crimson::RendererCapabilityStatus* find_status(
    const std::vector<crimson::RendererCapabilityStatus>& statuses,
    crimson::RendererCapability capability) noexcept
{
    for (const crimson::RendererCapabilityStatus& status : statuses) {
        if (status.capability == capability) {
            return &status;
        }
    }

    return nullptr;
}

[[nodiscard]] crimson::gpu::GpuCapabilityInput full_capabilities() noexcept
{
    return crimson::gpu::GpuCapabilityInput{
        .instancing = true,
        .texture_2d = true,
        .texture_cube = true,
        .floating_point_render_target = true,
        .render_to_texture = true,
        .depth_texture = true,
        .srgb_texture_sampling = true,
        .srgb_backbuffer = false,
        .manual_srgb_final_conversion = true,
        .integer_picking_target = true,
        .rgba8_picking_target = true,
        .readback = true,
    };
}

TEST(BackendCapability, BackendSelectionUsesV1PlatformPolicy)
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
        "Windows Auto chooses Vulkan first");

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
        "Windows Auto falls back to Direct3D12 before Direct3D11");

    const std::vector<GraphicsBackend> metal_only = {GraphicsBackend::Metal};
    expect_true(
        !crimson::gpu::choose_backend(NativeSurfacePlatform::MacOS, GraphicsBackendPreference::Vulkan, metal_only),
        "explicit unsupported backend preference fails");
}

TEST(BackendCapability, FullCapabilitySetPassesWithManualSrgbFallback)
{
    const crimson::DisplayConversionSettings display;
    const crimson::gpu::GpuCapabilityValidation validation =
        crimson::gpu::validate_v1_capabilities(full_capabilities(), display);

    expect_true(validation.ok(), "full V1 capability set passes validation");
    expect_true(validation.statuses.size() == 12, "validation reports every public capability");

    const crimson::RendererCapabilityStatus* srgb_backbuffer = find_status(
        validation.statuses,
        crimson::RendererCapability::SrgbBackbuffer);
    const crimson::RendererCapabilityStatus* manual_srgb = find_status(
        validation.statuses,
        crimson::RendererCapability::ManualSrgbFinalConversion);
    expect_true(
        srgb_backbuffer != nullptr && !srgb_backbuffer->supported,
        "sRGB backbuffer may be unavailable when manual fallback is present");
    expect_true(
        manual_srgb != nullptr && manual_srgb->supported,
        "manual final sRGB conversion fallback satisfies display conversion alternative");
}

TEST(BackendCapability, MissingRequiredCapabilityReportsFatalDiagnostic)
{
    crimson::gpu::GpuCapabilityInput input = full_capabilities();
    input.instancing = false;

    const crimson::gpu::GpuCapabilityValidation validation =
        crimson::gpu::validate_v1_capabilities(input, crimson::DisplayConversionSettings{});

    expect_true(!validation.ok(), "missing instancing fails V1 capability validation");
    expect_true(!validation.diagnostics.empty(), "missing capability produces a diagnostic");
    if (!validation.diagnostics.empty()) {
        const crimson::RendererDiagnostic& diagnostic = validation.diagnostics.front();
        expect_true(
            diagnostic.severity == crimson::RendererDiagnosticSeverity::Fatal,
            "missing required capability is fatal");
        expect_true(
            diagnostic.code == crimson::RendererDiagnosticCode::CapabilityMissing,
            "missing required capability uses CapabilityMissing code");
        expect_true(diagnostic.subsystem == "gpu", "capability diagnostic records gpu subsystem");
        expect_true(
            diagnostic.resource_name == "Instancing",
            "capability diagnostic records missing capability name");
    }
}

TEST(BackendCapability, FallbackGroupsRequireOneSupportedPath)
{
    crimson::gpu::GpuCapabilityInput no_display_path = full_capabilities();
    no_display_path.manual_srgb_final_conversion = false;

    crimson::DisplayConversionSettings no_manual_display;
    no_manual_display.allow_manual_final_srgb = false;

    const crimson::gpu::GpuCapabilityValidation display_validation =
        crimson::gpu::validate_v1_capabilities(no_display_path, no_manual_display);
    expect_true(!display_validation.ok(), "display conversion requires sRGB backbuffer or manual fallback");

    crimson::gpu::GpuCapabilityInput no_picking_target = full_capabilities();
    no_picking_target.integer_picking_target = false;
    no_picking_target.rgba8_picking_target = false;

    const crimson::gpu::GpuCapabilityValidation picking_validation =
        crimson::gpu::validate_v1_capabilities(no_picking_target, crimson::DisplayConversionSettings{});
    expect_true(!picking_validation.ok(), "picking requires R32U or RGBA8 target fallback");
}

} // namespace
