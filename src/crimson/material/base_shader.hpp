#pragma once

#include "crimson/material/material_parameter.hpp"
#include "crimson/material/material_texture.hpp"
#include "crimson/material/material_ui_schema.hpp"
#include "crimson/renderer_types.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace crimson {

enum class DepthMode : std::uint8_t {
    Disabled,
    TestWrite,
    TestReadOnly,
    OverlayCommand,
};

enum class BlendMode : std::uint8_t {
    Off,
    AlphaBlend,
};

enum class CullMode : std::uint8_t {
    Back,
    None,
};

enum class ShadowMode : std::uint8_t {
    None,
    CastAndReceive,
    AlphaTestedCastAndReceive,
};

enum VertexAttribute : std::uint32_t {
    VertexAttributePosition = 1u << 0u,
    VertexAttributeNormal = 1u << 1u,
    VertexAttributeTangent = 1u << 2u,
    VertexAttributeUv0 = 1u << 3u,
    VertexAttributeUv1 = 1u << 4u,
    VertexAttributeColor0 = 1u << 5u,
};

using VertexAttributeMask = std::uint32_t;

struct BaseShaderDefinition {
    BaseShaderId id = BaseShaderId::OpaquePbr;
    std::string name;
    RenderDomain domain = RenderDomain::LitSurface;
    RenderQueue default_queue = RenderQueue::Opaque;
    DepthMode depth_mode = DepthMode::TestWrite;
    BlendMode blend_mode = BlendMode::Off;
    CullMode cull_mode = CullMode::Back;
    ShadowMode shadow_mode = ShadowMode::CastAndReceive;
    VertexAttributeMask required_attributes = VertexAttributePosition;
    std::vector<MaterialParameterDesc> parameters;
    std::vector<MaterialTextureSlotDesc> texture_slots;
    std::vector<MaterialUiFieldDesc> ui_schema;
    ShaderProgramId program = ShaderProgramId::OpaquePbr;
};

[[nodiscard]] const MaterialParameterDesc* find_parameter_desc(
    const BaseShaderDefinition& definition,
    std::string_view name) noexcept;
[[nodiscard]] const MaterialTextureSlotDesc* find_texture_slot_desc(
    const BaseShaderDefinition& definition,
    std::string_view name) noexcept;
[[nodiscard]] const char* depth_mode_name(DepthMode mode) noexcept;
[[nodiscard]] const char* blend_mode_name(BlendMode mode) noexcept;
[[nodiscard]] const char* cull_mode_name(CullMode mode) noexcept;
[[nodiscard]] const char* shadow_mode_name(ShadowMode mode) noexcept;

} // namespace crimson
