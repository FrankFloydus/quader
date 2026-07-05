#pragma once

#include <cstdint>
#include <string_view>

namespace crimson {

struct RenderMeshHandle {
    std::uint32_t index = 0;
    std::uint32_t generation = 0;

    friend bool operator==(const RenderMeshHandle&, const RenderMeshHandle&) = default;
};

struct RenderTextureHandle {
    std::uint32_t index = 0;
    std::uint32_t generation = 0;

    friend bool operator==(const RenderTextureHandle&, const RenderTextureHandle&) = default;
};

struct RenderMaterialHandle {
    std::uint32_t index = 0;
    std::uint32_t generation = 0;

    friend bool operator==(const RenderMaterialHandle&, const RenderMaterialHandle&) = default;
};

struct RenderProgramHandle {
    std::uint32_t index = 0;
    std::uint32_t generation = 0;

    friend bool operator==(const RenderProgramHandle&, const RenderProgramHandle&) = default;
};

struct RenderFrameBufferHandle {
    std::uint32_t index = 0;
    std::uint32_t generation = 0;

    friend bool operator==(const RenderFrameBufferHandle&, const RenderFrameBufferHandle&) = default;
};

struct RenderEnvironmentHandle {
    std::uint32_t index = 0;
    std::uint32_t generation = 0;

    friend bool operator==(const RenderEnvironmentHandle&, const RenderEnvironmentHandle&) = default;
};

template <typename Handle>
constexpr bool is_valid_handle(const Handle& handle) noexcept
{
    return handle.index != 0 && handle.generation != 0;
}

struct ViewportExtent {
    std::uint32_t width_px = 1;
    std::uint32_t height_px = 1;
    float device_pixel_ratio = 1.0f;
};

constexpr bool is_valid_extent(const ViewportExtent& extent) noexcept
{
    return extent.width_px > 0 && extent.height_px > 0 && extent.device_pixel_ratio > 0.0f;
}

enum class ShaderProgramId : std::uint16_t {
    PrototypeLitCube,
    PrototypeGridOverlay,
    OpaquePbr,
    AlphaCutoutPbr,
    TransparentPbr,
    OverlayUnlit,
    Picking,
    ToneMap,
    Present,
};

constexpr std::string_view shader_program_id_name(ShaderProgramId id) noexcept
{
    switch (id) {
    case ShaderProgramId::PrototypeLitCube:
        return "PrototypeLitCube";
    case ShaderProgramId::PrototypeGridOverlay:
        return "PrototypeGridOverlay";
    case ShaderProgramId::OpaquePbr:
        return "OpaquePbr";
    case ShaderProgramId::AlphaCutoutPbr:
        return "AlphaCutoutPbr";
    case ShaderProgramId::TransparentPbr:
        return "TransparentPbr";
    case ShaderProgramId::OverlayUnlit:
        return "OverlayUnlit";
    case ShaderProgramId::Picking:
        return "Picking";
    case ShaderProgramId::ToneMap:
        return "ToneMap";
    case ShaderProgramId::Present:
        return "Present";
    }

    return "Unknown";
}

enum class BaseShaderId : std::uint16_t {
    OpaquePbr,
    AlphaCutoutPbr,
    TransparentPbr,
    OverlayUnlit,
};

constexpr std::string_view base_shader_id_name(BaseShaderId id) noexcept
{
    switch (id) {
    case BaseShaderId::OpaquePbr:
        return "OpaquePbr";
    case BaseShaderId::AlphaCutoutPbr:
        return "AlphaCutoutPbr";
    case BaseShaderId::TransparentPbr:
        return "TransparentPbr";
    case BaseShaderId::OverlayUnlit:
        return "OverlayUnlit";
    }

    return "Unknown";
}

enum class RenderDomain : std::uint8_t {
    LitSurface,
    TransparentSurface,
    Overlay,
    Picking,
    Post,
};

constexpr std::string_view render_domain_name(RenderDomain domain) noexcept
{
    switch (domain) {
    case RenderDomain::LitSurface:
        return "LitSurface";
    case RenderDomain::TransparentSurface:
        return "TransparentSurface";
    case RenderDomain::Overlay:
        return "Overlay";
    case RenderDomain::Picking:
        return "Picking";
    case RenderDomain::Post:
        return "Post";
    }

    return "Unknown";
}

enum class RenderQueue : std::uint8_t {
    PrototypeOpaque,
    Opaque,
    AlphaCutout,
    Transparent,
    OverlayDepthTested,
    OverlayXRay,
    OverlayAlwaysOnTop,
    Picking,
};

constexpr std::string_view render_queue_name(RenderQueue queue) noexcept
{
    switch (queue) {
    case RenderQueue::PrototypeOpaque:
        return "PrototypeOpaque";
    case RenderQueue::Opaque:
        return "Opaque";
    case RenderQueue::AlphaCutout:
        return "AlphaCutout";
    case RenderQueue::Transparent:
        return "Transparent";
    case RenderQueue::OverlayDepthTested:
        return "OverlayDepthTested";
    case RenderQueue::OverlayXRay:
        return "OverlayXRay";
    case RenderQueue::OverlayAlwaysOnTop:
        return "OverlayAlwaysOnTop";
    case RenderQueue::Picking:
        return "Picking";
    }

    return "Unknown";
}

} // namespace crimson
