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

#include "crimson/native_surface.hpp"
#include "crimson/renderer_config.hpp"
#include "crimson/renderer_diagnostics.hpp"

#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace crimson::gpu {

/// Concrete graphics backend selected by the GPU layer.
enum class GraphicsBackend {
	/// Vulkan backend.
	Vulkan,
	/// Metal backend.
	Metal,
	/// Direct3D 12 backend.
	Direct3D12,
	/// Direct3D 11 backend.
	Direct3D11,
};

/**
 * Return the stable graphics-backend name.
 *
 * @param backend Backend to name.
 * @return Static backend name.
 */
constexpr std::string_view graphics_backend_name(GraphicsBackend backend) noexcept {
	switch (backend) {
		case GraphicsBackend::Vulkan:
			return "Vulkan";
		case GraphicsBackend::Metal:
			return "Metal";
		case GraphicsBackend::Direct3D12:
			return "Direct3D12";
		case GraphicsBackend::Direct3D11:
			return "Direct3D11";
	}

	return "Unknown";
}

/// Capability flags reported by the graphics runtime.
struct GpuCapabilityInput {
	/// Hardware instancing support.
	bool instancing = false;
	/// 2D texture support.
	bool texture_2d = false;
	/// Cube texture support.
	bool texture_cube = false;
	/// Floating-point render target support.
	bool floating_point_render_target = false;
	/// Render-to-texture support.
	bool render_to_texture = false;
	/// Depth texture support.
	bool depth_texture = false;
	/// Hardware sRGB texture sampling support.
	bool srgb_texture_sampling = false;
	/// sRGB backbuffer support.
	bool srgb_backbuffer = false;
	/// Manual final sRGB conversion support.
	bool manual_srgb_final_conversion = false;
	/// Integer picking target support.
	bool integer_picking_target = false;
	/// RGBA8 encoded picking target support.
	bool rgba8_picking_target = false;
	/// GPU readback support.
	bool readback = false;
};

/// Result of validating runtime capabilities against Crimson V1 requirements.
struct GpuCapabilityValidation {
	/// Capability support rows for diagnostics.
	std::vector<RendererCapabilityStatus> statuses;
	/// Diagnostics for missing required capabilities.
	std::vector<RendererDiagnostic> diagnostics;

	/**
	 * Check whether validation produced no diagnostics.
	 *
	 * @return True when all required capabilities are satisfied.
	 */
	[[nodiscard]] bool ok() const noexcept;
};

/**
 * Choose a backend for a platform and preference.
 *
 * @param platform Native surface platform.
 * @param preference User/backend preference.
 * @param supported_backends Backends reported as available.
 * @return Selected backend, or empty when none satisfy the request.
 */
std::optional<GraphicsBackend> choose_backend(
		NativeSurfacePlatform platform,
		GraphicsBackendPreference preference,
		std::span<const GraphicsBackend> supported_backends) noexcept;

/**
 * Build a diagnostic for an unsupported backend request.
 *
 * @param platform Native surface platform.
 * @param preference Requested preference.
 * @return Renderer diagnostic describing the unsupported backend.
 */
RendererDiagnostic make_backend_unsupported_diagnostic(
		NativeSurfacePlatform platform,
		GraphicsBackendPreference preference);

/**
 * Validate required V1 GPU capabilities.
 *
 * @param input Runtime capability flags.
 * @param display_conversion Display conversion policy.
 * @return Capability status rows and missing-capability diagnostics.
 */
GpuCapabilityValidation validate_v1_capabilities(
		const GpuCapabilityInput &input,
		const DisplayConversionSettings &display_conversion);

} // namespace crimson::gpu
