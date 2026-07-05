#include "crimson/gpu/gpu_capabilities.hpp"

#include <algorithm>
#include <array>
#include <string>
#include <utility>

namespace crimson::gpu {
namespace {

constexpr std::array<GraphicsBackend, 3> kWindowsBackendPriority = {
    GraphicsBackend::Vulkan,
    GraphicsBackend::Direct3D12,
    GraphicsBackend::Direct3D11,
};

constexpr std::array<GraphicsBackend, 1> kLinuxBackendPriority = {
    GraphicsBackend::Vulkan,
};

constexpr std::array<GraphicsBackend, 1> kMacBackendPriority = {
    GraphicsBackend::Metal,
};

[[nodiscard]] bool contains_backend(
    std::span<const GraphicsBackend> supported_backends,
    GraphicsBackend backend) noexcept
{
    return std::find(supported_backends.begin(), supported_backends.end(), backend) != supported_backends.end();
}

[[nodiscard]] std::optional<GraphicsBackend> backend_for_preference(GraphicsBackendPreference preference) noexcept
{
    switch (preference) {
    case GraphicsBackendPreference::Auto:
        return std::nullopt;
    case GraphicsBackendPreference::Vulkan:
        return GraphicsBackend::Vulkan;
    case GraphicsBackendPreference::Metal:
        return GraphicsBackend::Metal;
    case GraphicsBackendPreference::Direct3D12:
        return GraphicsBackend::Direct3D12;
    case GraphicsBackendPreference::Direct3D11:
        return GraphicsBackend::Direct3D11;
    }

    return std::nullopt;
}

[[nodiscard]] std::span<const GraphicsBackend> priority_for_platform(NativeSurfacePlatform platform) noexcept
{
    switch (platform) {
    case NativeSurfacePlatform::Windows:
        return kWindowsBackendPriority;
    case NativeSurfacePlatform::LinuxX11:
    case NativeSurfacePlatform::LinuxWayland:
        return kLinuxBackendPriority;
    case NativeSurfacePlatform::MacOS:
        return kMacBackendPriority;
    case NativeSurfacePlatform::Unknown:
        return {};
    }

    return {};
}

void push_status(
    std::vector<RendererCapabilityStatus>& statuses,
    RendererCapability capability,
    bool supported,
    std::string detail)
{
    statuses.push_back(RendererCapabilityStatus{
        .capability = capability,
        .supported = supported,
        .detail = std::move(detail),
    });
}

[[nodiscard]] RendererDiagnostic capability_missing_diagnostic(
    RendererCapability capability,
    std::string detail)
{
    return RendererDiagnostic{
        .severity = RendererDiagnosticSeverity::Fatal,
        .code = RendererDiagnosticCode::CapabilityMissing,
        .message = "Crimson graphics backend is missing a required V1 capability.",
        .detail = std::move(detail),
        .subsystem = "gpu",
        .resource_name = std::string(renderer_capability_name(capability)),
    };
}

void push_required_status(
    GpuCapabilityValidation& validation,
    RendererCapability capability,
    bool supported,
    std::string supported_detail,
    std::string missing_detail)
{
    push_status(
        validation.statuses,
        capability,
        supported,
        supported ? std::move(supported_detail) : missing_detail);

    if (!supported) {
        validation.diagnostics.push_back(capability_missing_diagnostic(capability, std::move(missing_detail)));
    }
}

} // namespace

bool GpuCapabilityValidation::ok() const noexcept
{
    return diagnostics.empty();
}

std::optional<GraphicsBackend> choose_backend(
    NativeSurfacePlatform platform,
    GraphicsBackendPreference preference,
    std::span<const GraphicsBackend> supported_backends) noexcept
{
    if (std::optional<GraphicsBackend> preferred_backend = backend_for_preference(preference)) {
        return contains_backend(supported_backends, *preferred_backend)
            ? preferred_backend
            : std::nullopt;
    }

    for (GraphicsBackend backend : priority_for_platform(platform)) {
        if (contains_backend(supported_backends, backend)) {
            return backend;
        }
    }

    return std::nullopt;
}

RendererDiagnostic make_backend_unsupported_diagnostic(
    NativeSurfacePlatform platform,
    GraphicsBackendPreference preference)
{
    return RendererDiagnostic{
        .severity = RendererDiagnosticSeverity::Fatal,
        .code = RendererDiagnosticCode::BackendUnsupported,
        .message = "No supported Crimson graphics backend is available.",
        .detail = "Platform="
            + std::string(native_surface_platform_name(platform))
            + ", requested="
            + std::string(graphics_backend_preference_name(preference)),
        .subsystem = "gpu",
        .resource_name = "BackendSelection",
    };
}

GpuCapabilityValidation validate_v1_capabilities(
    const GpuCapabilityInput& input,
    const DisplayConversionSettings& display_conversion)
{
    GpuCapabilityValidation validation;
    validation.statuses.reserve(12);

    push_required_status(
        validation,
        RendererCapability::Instancing,
        input.instancing,
        "Runtime reports instanced rendering support.",
        "Instancing is required for V1 draw packet batching.");
    push_required_status(
        validation,
        RendererCapability::Texture2D,
        input.texture_2d,
        "Runtime reports 2D texture support.",
        "2D textures are required for V1 materials and framebuffer resources.");
    push_required_status(
        validation,
        RendererCapability::TextureCube,
        input.texture_cube,
        "Runtime reports cube texture support.",
        "Cube textures are required by the Crimson V1 renderer contract.");
    push_required_status(
        validation,
        RendererCapability::FloatingPointRenderTarget,
        input.floating_point_render_target,
        "Runtime reports RGBA16F framebuffer support.",
        "Floating-point render targets are required for the linear HDR scene target.");
    push_required_status(
        validation,
        RendererCapability::RenderToTexture,
        input.render_to_texture,
        "Runtime reports texture framebuffer support.",
        "Render-to-texture is required for V1 scene, picking, and final targets.");
    push_required_status(
        validation,
        RendererCapability::DepthTexture,
        input.depth_texture,
        "Runtime reports a usable depth framebuffer format.",
        "Depth texture framebuffer support is required by V1 depth-aware passes.");
    push_required_status(
        validation,
        RendererCapability::SrgbTextureSampling,
        input.srgb_texture_sampling,
        "Runtime reports sRGB 2D texture sampling support.",
        "sRGB texture sampling is required for base color and emissive texture slots.");

    const bool srgb_backbuffer = display_conversion.prefer_srgb_backbuffer && input.srgb_backbuffer;
    push_status(
        validation.statuses,
        RendererCapability::SrgbBackbuffer,
        srgb_backbuffer,
        srgb_backbuffer
            ? "Runtime reports sRGB backbuffer support."
            : "sRGB backbuffer support is unavailable or not requested.");

    const bool manual_srgb = display_conversion.allow_manual_final_srgb
        && input.manual_srgb_final_conversion;
    push_status(
        validation.statuses,
        RendererCapability::ManualSrgbFinalConversion,
        manual_srgb,
        manual_srgb
            ? "Manual final linear-to-sRGB conversion fallback is enabled."
            : "Manual final linear-to-sRGB conversion fallback is disabled.");
    if (!srgb_backbuffer && !manual_srgb) {
        validation.diagnostics.push_back(capability_missing_diagnostic(
            RendererCapability::ManualSrgbFinalConversion,
            "Crimson requires either an sRGB backbuffer or the manual final conversion fallback."));
    }

    push_status(
        validation.statuses,
        RendererCapability::IntegerPickingTarget,
        input.integer_picking_target,
        input.integer_picking_target
            ? "Runtime reports R32U framebuffer support for picking IDs."
            : "R32U picking target is unavailable; RGBA8 data fallback may be used.");
    push_status(
        validation.statuses,
        RendererCapability::Rgba8PickingTarget,
        input.rgba8_picking_target,
        input.rgba8_picking_target
            ? "Runtime reports RGBA8 framebuffer support for picking fallback."
            : "RGBA8 picking fallback target is unavailable.");
    if (!input.integer_picking_target && !input.rgba8_picking_target) {
        validation.diagnostics.push_back(capability_missing_diagnostic(
            RendererCapability::IntegerPickingTarget,
            "Crimson requires either R32U picking IDs or RGBA8 data-encoded picking IDs."));
    }

    push_required_status(
        validation,
        RendererCapability::Readback,
        input.readback,
        "Runtime reports texture readback support.",
        "Texture readback is required for request-scoped picking results.");

    return validation;
}

} // namespace crimson::gpu
