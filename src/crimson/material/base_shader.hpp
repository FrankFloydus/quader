/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
#pragma once

#include "crimson/material/material_parameter.hpp"
#include "crimson/material/material_texture.hpp"
#include "crimson/material/material_ui_schema.hpp"
#include "crimson/renderer_types.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace crimson {

/// Depth-state policy declared by a base shader.
enum class DepthMode : std::uint8_t {
	Disabled,       ///< Disable depth testing and writes.
	TestWrite,      ///< Test against depth and write depth.
	TestReadOnly,   ///< Test against depth without writing depth.
	OverlayCommand, ///< Depth policy is selected per overlay command.
};

/// Blend-state policy declared by a base shader.
enum class BlendMode : std::uint8_t {
	Off,       ///< Disable blending.
	AlphaBlend,///< Use alpha blending.
};

/// Face-culling policy declared by a base shader.
enum class CullMode : std::uint8_t {
	Back, ///< Cull back faces.
	None, ///< Disable face culling.
};

/// Shadow behavior declared by a base shader.
enum class ShadowMode : std::uint8_t {
	None,                     ///< Do not cast or receive shadows.
	CastAndReceive,           ///< Cast and receive opaque shadows.
	AlphaTestedCastAndReceive,///< Cast alpha-tested shadows and receive shadows.
};

/// Vertex attribute bits required by base shaders.
enum VertexAttribute : std::uint32_t {
	VertexAttributePosition = 1u << 0u, ///< Position attribute.
	VertexAttributeNormal = 1u << 1u,   ///< Normal attribute.
	VertexAttributeTangent = 1u << 2u,  ///< Tangent attribute.
	VertexAttributeUv0 = 1u << 3u,      ///< First texture coordinate attribute.
	VertexAttributeUv1 = 1u << 4u,      ///< Second texture coordinate attribute.
	VertexAttributeColor0 = 1u << 5u,   ///< First vertex color attribute.
};

/// Bitmask of `VertexAttribute` values.
using VertexAttributeMask = std::uint32_t;

/// Static schema describing one Crimson base shader.
struct BaseShaderDefinition {
	/// Stable base shader id.
	BaseShaderId id = BaseShaderId::OpaquePbr;
	/// Developer-facing shader name.
	std::string name;
	/// Render domain that owns this shader.
	RenderDomain domain = RenderDomain::LitSurface;
	/// Default render queue for material instances.
	RenderQueue default_queue = RenderQueue::Opaque;
	/// Depth-state policy.
	DepthMode depth_mode = DepthMode::TestWrite;
	/// Blend-state policy.
	BlendMode blend_mode = BlendMode::Off;
	/// Face-culling policy.
	CullMode cull_mode = CullMode::Back;
	/// Shadow policy.
	ShadowMode shadow_mode = ShadowMode::CastAndReceive;
	/// Required vertex attributes.
	VertexAttributeMask required_attributes = VertexAttributePosition;
	/// Parameter schema for material instances.
	std::vector<MaterialParameterDesc> parameters;
	/// Texture slot schema for material instances.
	std::vector<MaterialTextureSlotDesc> texture_slots;
	/// UI fields exposed for this shader.
	std::vector<MaterialUiFieldDesc> ui_schema;
	/// Shader program used by this base shader.
	ShaderProgramId program = ShaderProgramId::OpaquePbr;
};

/// Find a parameter descriptor by name, or return `nullptr`.
[[nodiscard]] const MaterialParameterDesc *find_parameter_desc(
		const BaseShaderDefinition &definition,
		std::string_view name) noexcept;
/// Find a texture slot descriptor by name, or return `nullptr`.
[[nodiscard]] const MaterialTextureSlotDesc *find_texture_slot_desc(
		const BaseShaderDefinition &definition,
		std::string_view name) noexcept;
/// Return the stable debug name for a depth mode.
[[nodiscard]] const char *depth_mode_name(DepthMode mode) noexcept;
/// Return the stable debug name for a blend mode.
[[nodiscard]] const char *blend_mode_name(BlendMode mode) noexcept;
/// Return the stable debug name for a cull mode.
[[nodiscard]] const char *cull_mode_name(CullMode mode) noexcept;
/// Return the stable debug name for a shadow mode.
[[nodiscard]] const char *shadow_mode_name(ShadowMode mode) noexcept;

} // namespace crimson
