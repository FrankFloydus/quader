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

#include <cstdint>
#include <filesystem>
#include <string_view>

namespace crimson {

/// Requested graphics backend selection policy for Crimson startup.
enum class GraphicsBackendPreference {
	/// Select the best supported backend for the platform.
	Auto,
	/// Prefer Vulkan.
	Vulkan,
	/// Prefer Metal.
	Metal,
	/// Prefer Direct3D 12.
	Direct3D12,
	/// Prefer Direct3D 11.
	Direct3D11,
};

/// Tone-mapping operator applied during display conversion.
enum class ToneMapper : std::uint8_t {
	/// ACES fitted approximation.
	AcesFitted,
	/// AgX-inspired approximation.
	AgxApprox,
	/// Reinhard tone mapper.
	Reinhard,
	/// Linear pass-through clamp path.
	Linear,
};

/**
 * Return the stable display name for a backend preference.
 *
 * @param preference Backend preference to name.
 * @return Static display name.
 */
constexpr std::string_view graphics_backend_preference_name(GraphicsBackendPreference preference) noexcept {
	switch (preference) {
		case GraphicsBackendPreference::Auto:
			return "Auto";
		case GraphicsBackendPreference::Vulkan:
			return "Vulkan";
		case GraphicsBackendPreference::Metal:
			return "Metal";
		case GraphicsBackendPreference::Direct3D12:
			return "Direct3D12";
		case GraphicsBackendPreference::Direct3D11:
			return "Direct3D11";
	}

	return "Unknown";
}

/**
 * Return the stable display name for a tone mapper.
 *
 * @param tone_mapper Tone mapper to name.
 * @return Static display name.
 */
constexpr std::string_view tone_mapper_name(ToneMapper tone_mapper) noexcept {
	switch (tone_mapper) {
		case ToneMapper::AcesFitted:
			return "AcesFitted";
		case ToneMapper::AgxApprox:
			return "AgxApprox";
		case ToneMapper::Reinhard:
			return "Reinhard";
		case ToneMapper::Linear:
			return "Linear";
	}

	return "Unknown";
}

/// Settings that choose the final linear-to-sRGB display path.
struct DisplayConversionSettings {
	/// Prefer an sRGB-capable backbuffer when the runtime supports it.
	bool prefer_srgb_backbuffer = true;
	/// Allow a final shader pass to perform manual sRGB conversion.
	bool allow_manual_final_srgb = true;
};

/// Startup configuration for a Crimson renderer instance.
struct RendererConfig {
	/// Preferred graphics backend, or automatic platform selection.
	GraphicsBackendPreference backend_preference = GraphicsBackendPreference::Auto;
	/// Root directory containing compiled shader binaries.
	std::filesystem::path shader_root;
	/// Root directory containing renderer runtime assets such as default materials.
	std::filesystem::path asset_root;
	/// Whether presentation should synchronize to display refresh.
	bool vsync = true;
	/// Whether renderer debug text is enabled when supported.
	bool enable_debug_text = true;
	/// Tone mapper used by frame settings when no override is supplied.
	ToneMapper default_tone_mapper = ToneMapper::Linear;
	/// Display conversion policy for the selected backend.
	DisplayConversionSettings display_conversion;
};

} // namespace crimson
