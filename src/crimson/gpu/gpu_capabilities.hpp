#pragma once

#include "crimson/native_surface.hpp"
#include "crimson/renderer_config.hpp"
#include "crimson/renderer_diagnostics.hpp"

#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace crimson::gpu {

enum class GraphicsBackend {
	Vulkan,
	Metal,
	Direct3D12,
	Direct3D11,
};

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

struct GpuCapabilityInput {
	bool instancing = false;
	bool texture_2d = false;
	bool texture_cube = false;
	bool floating_point_render_target = false;
	bool render_to_texture = false;
	bool depth_texture = false;
	bool srgb_texture_sampling = false;
	bool srgb_backbuffer = false;
	bool manual_srgb_final_conversion = false;
	bool integer_picking_target = false;
	bool rgba8_picking_target = false;
	bool readback = false;
};

struct GpuCapabilityValidation {
	std::vector<RendererCapabilityStatus> statuses;
	std::vector<RendererDiagnostic> diagnostics;

	[[nodiscard]] bool ok() const noexcept;
};

std::optional<GraphicsBackend> choose_backend(
		NativeSurfacePlatform platform,
		GraphicsBackendPreference preference,
		std::span<const GraphicsBackend> supported_backends) noexcept;

RendererDiagnostic make_backend_unsupported_diagnostic(
		NativeSurfacePlatform platform,
		GraphicsBackendPreference preference);

GpuCapabilityValidation validate_v1_capabilities(
		const GpuCapabilityInput &input,
		const DisplayConversionSettings &display_conversion);

} // namespace crimson::gpu
