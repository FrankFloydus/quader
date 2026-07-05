/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
#include "crimson/material/base_shader.hpp"

namespace crimson {

const MaterialParameterDesc *find_parameter_desc(
		const BaseShaderDefinition &definition,
		std::string_view name) noexcept {
	for (const MaterialParameterDesc &parameter : definition.parameters) {
		if (parameter.name == name) {
			return &parameter;
		}
	}

	return nullptr;
}

const MaterialTextureSlotDesc *find_texture_slot_desc(
		const BaseShaderDefinition &definition,
		std::string_view name) noexcept {
	for (const MaterialTextureSlotDesc &slot : definition.texture_slots) {
		if (slot.name == name) {
			return &slot;
		}
	}

	return nullptr;
}

const char *depth_mode_name(DepthMode mode) noexcept {
	switch (mode) {
		case DepthMode::Disabled:
			return "Disabled";
		case DepthMode::TestWrite:
			return "TestWrite";
		case DepthMode::TestReadOnly:
			return "TestReadOnly";
		case DepthMode::OverlayCommand:
			return "OverlayCommand";
	}

	return "Unknown";
}

const char *blend_mode_name(BlendMode mode) noexcept {
	switch (mode) {
		case BlendMode::Off:
			return "Off";
		case BlendMode::AlphaBlend:
			return "AlphaBlend";
	}

	return "Unknown";
}

const char *cull_mode_name(CullMode mode) noexcept {
	switch (mode) {
		case CullMode::Back:
			return "Back";
		case CullMode::None:
			return "None";
	}

	return "Unknown";
}

const char *shadow_mode_name(ShadowMode mode) noexcept {
	switch (mode) {
		case ShadowMode::None:
			return "None";
		case ShadowMode::CastAndReceive:
			return "CastAndReceive";
		case ShadowMode::AlphaTestedCastAndReceive:
			return "AlphaTestedCastAndReceive";
	}

	return "Unknown";
}

} // namespace crimson
