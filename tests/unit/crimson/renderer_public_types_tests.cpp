#include "crimson/native_surface.hpp"
#include "crimson/renderer_config.hpp"
#include "crimson/renderer_diagnostics.hpp"
#include "crimson/renderer_types.hpp"
#include "crimson/scene/render_object.hpp"

#include <gtest/gtest.h>

#include <iostream>
#include <string_view>

namespace {

template <typename T>
concept HasPublicProgramMember = requires(T object) {
    object.program;
};

void expect_true(bool condition, std::string_view message)
{
    EXPECT_TRUE(condition) << message;
}

TEST(RendererPublicTypes, HandlesEncodeInvalidAndGenerationState)
{
    expect_true(!crimson::is_valid_handle(crimson::RenderMeshHandle{}), "default mesh handle is invalid");
    expect_true(!crimson::is_valid_handle(crimson::RenderTextureHandle{}), "default texture handle is invalid");
    expect_true(crimson::is_valid_handle(crimson::RenderMeshHandle{1, 1}), "non-zero mesh handle is valid");
    expect_true(
        crimson::RenderMeshHandle{7, 2} == crimson::RenderMeshHandle{7, 2},
        "matching mesh handles compare equal");
    expect_true(
        crimson::RenderMeshHandle{7, 2} != crimson::RenderMeshHandle{7, 3},
        "generation participates in mesh handle equality");
}

TEST(RendererPublicTypes, ExtentAndNativeSurfaceValidationRejectInvalidSurfaces)
{
    expect_true(crimson::is_valid_extent(crimson::ViewportExtent{}), "default extent is valid");
    expect_true(
        !crimson::is_valid_extent(crimson::ViewportExtent{0, 1, 1.0f}),
        "zero width is an invalid extent");
    expect_true(
        !crimson::is_valid_extent(crimson::ViewportExtent{1, 1, 0.0f}),
        "zero device pixel ratio is invalid");

    crimson::NativeSurfaceDescriptor surface;
    expect_true(!crimson::is_valid_native_surface_descriptor(surface), "default surface is invalid");

    surface.platform = crimson::NativeSurfacePlatform::Windows;
    surface.native_window_handle = reinterpret_cast<void*>(0x1);
    surface.extent = crimson::ViewportExtent{1280, 720, 1.0f};
    expect_true(crimson::is_valid_native_surface_descriptor(surface), "populated Windows surface is valid");
}

TEST(RendererPublicTypes, NamesAreStableForPublicDiagnosticsAndConfig)
{
    expect_true(
        crimson::shader_program_id_name(crimson::ShaderProgramId::PrototypeLitCube) == "PrototypeLitCube",
        "prototype cube program name is stable");
    expect_true(
        crimson::shader_program_id_name(crimson::ShaderProgramId::ToneMap) == "ToneMap",
        "tone-map program name is stable");
    expect_true(
        crimson::shader_program_id_name(crimson::ShaderProgramId::OpaquePbr) == "OpaquePbr",
        "OpaquePbr program name is stable");
    expect_true(
        crimson::shader_program_id_name(crimson::ShaderProgramId::Picking) == "Picking",
        "Picking program name is stable");
    expect_true(
        crimson::graphics_backend_preference_name(crimson::GraphicsBackendPreference::Direct3D12) == "Direct3D12",
        "backend preference name is stable");
    expect_true(
        crimson::renderer_diagnostic_code_name(crimson::RendererDiagnosticCode::ShaderFileMissing)
            == "ShaderFileMissing",
        "shader diagnostic code name is stable");
    expect_true(
        crimson::native_surface_platform_name(crimson::NativeSurfacePlatform::LinuxWayland) == "LinuxWayland",
        "native surface platform name is stable");
}

TEST(RendererPublicTypes, StatusReportsErrorSeverity)
{
    crimson::RendererStatus status;
    expect_true(!crimson::has_error_diagnostic(status), "empty status has no error diagnostics");

    status.diagnostics.push_back(crimson::RendererDiagnostic{
        .severity = crimson::RendererDiagnosticSeverity::Warning,
        .code = crimson::RendererDiagnosticCode::CapabilityMissing,
    });
    expect_true(!crimson::has_error_diagnostic(status), "warning status has no error diagnostics");

    status.diagnostics.push_back(crimson::RendererDiagnostic{
        .severity = crimson::RendererDiagnosticSeverity::Error,
        .code = crimson::RendererDiagnosticCode::SurfaceUnavailable,
    });
    expect_true(crimson::has_error_diagnostic(status), "error status is detected");
}

TEST(RendererPublicTypes, RenderObjectContractDoesNotExposeProgramSelection)
{
    expect_true(
        !HasPublicProgramMember<crimson::RenderObject>,
        "RenderObject does not expose arbitrary shader program selection");
}

} // namespace
