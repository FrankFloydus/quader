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
#include <string_view>

namespace crimson {

/// CPU-side handle for a renderer mesh resource.
struct RenderMeshHandle {
	/// One-based resource-table slot; zero is invalid.
	std::uint32_t index = 0;
	/// Generation used to reject stale handles.
	std::uint32_t generation = 0;

	/// Compare handles by index and generation.
	friend bool operator==(const RenderMeshHandle &, const RenderMeshHandle &) = default;
};

/// CPU-side handle for a renderer texture resource.
struct RenderTextureHandle {
	/// One-based resource-table slot; zero is invalid.
	std::uint32_t index = 0;
	/// Generation used to reject stale handles.
	std::uint32_t generation = 0;

	/// Compare handles by index and generation.
	friend bool operator==(const RenderTextureHandle &, const RenderTextureHandle &) = default;
};

/// CPU-side handle for a renderer material resource.
struct RenderMaterialHandle {
	/// One-based resource-table slot; zero is invalid.
	std::uint32_t index = 0;
	/// Generation used to reject stale handles.
	std::uint32_t generation = 0;

	/// Compare handles by index and generation.
	friend bool operator==(const RenderMaterialHandle &, const RenderMaterialHandle &) = default;
};

/// CPU-side handle for a renderer shader program resource.
struct RenderProgramHandle {
	/// One-based resource-table slot; zero is invalid.
	std::uint32_t index = 0;
	/// Generation used to reject stale handles.
	std::uint32_t generation = 0;

	/// Compare handles by index and generation.
	friend bool operator==(const RenderProgramHandle &, const RenderProgramHandle &) = default;
};

/// CPU-side handle for a renderer framebuffer resource.
struct RenderFrameBufferHandle {
	/// One-based resource-table slot; zero is invalid.
	std::uint32_t index = 0;
	/// Generation used to reject stale handles.
	std::uint32_t generation = 0;

	/// Compare handles by index and generation.
	friend bool operator==(const RenderFrameBufferHandle &, const RenderFrameBufferHandle &) = default;
};

/// CPU-side handle for a renderer environment resource.
struct RenderEnvironmentHandle {
	/// One-based resource-table slot; zero is invalid.
	std::uint32_t index = 0;
	/// Generation used to reject stale handles.
	std::uint32_t generation = 0;

	/// Compare handles by index and generation.
	friend bool operator==(const RenderEnvironmentHandle &, const RenderEnvironmentHandle &) = default;
};

/**
 * Check whether a renderer handle is non-sentinel.
 *
 * @tparam Handle Handle type with `index` and `generation` members.
 * @param handle Handle to test.
 * @return True when both index and generation are non-zero.
 */
template <typename Handle>
constexpr bool is_valid_handle(const Handle &handle) noexcept {
	return handle.index != 0 && handle.generation != 0;
}

/// Pixel extent and scale of a renderer viewport target.
struct ViewportExtent {
	/// Width in physical pixels.
	std::uint32_t width_px = 1;
	/// Height in physical pixels.
	std::uint32_t height_px = 1;
	/// Device pixel ratio used by the host UI.
	float device_pixel_ratio = 1.0f;
};

/**
 * Check whether a viewport extent can be rendered.
 *
 * @param extent Extent to test.
 * @return True when dimensions and device pixel ratio are positive.
 */
constexpr bool is_valid_extent(const ViewportExtent &extent) noexcept {
	return extent.width_px > 0 && extent.height_px > 0 && extent.device_pixel_ratio > 0.0f;
}

/// Shader program identifiers known to the compiled shader manifest.
enum class ShaderProgramId : std::uint16_t {
	/// Legacy prototype lit cube program.
	PrototypeLitCube,
	/// Legacy prototype grid overlay program.
	PrototypeGridOverlay,
	/// Opaque PBR surface program.
	OpaquePbr,
	/// Alpha-cutout PBR surface program.
	AlphaCutoutPbr,
	/// Transparent PBR surface program.
	TransparentPbr,
	/// Unlit overlay program.
	OverlayUnlit,
	/// Unlit line-list overlay program.
	OverlayLine,
	/// Object and element picking program.
	Picking,
	/// Tone-mapping program.
	ToneMap,
	/// Present/composite program.
	Present,
};

/**
 * Return the stable shader program name.
 *
 * @param id Shader program identifier.
 * @return Static program name.
 */
constexpr std::string_view shader_program_id_name(ShaderProgramId id) noexcept {
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
		case ShaderProgramId::OverlayLine:
			return "OverlayLine";
		case ShaderProgramId::Picking:
			return "Picking";
		case ShaderProgramId::ToneMap:
			return "ToneMap";
		case ShaderProgramId::Present:
			return "Present";
	}

	return "Unknown";
}

/// Base shader schemas supported by V1 material instances.
enum class BaseShaderId : std::uint16_t {
	/// Opaque metallic/roughness PBR shader.
	OpaquePbr,
	/// Alpha-cutout metallic/roughness PBR shader.
	AlphaCutoutPbr,
	/// Transparent metallic/roughness PBR shader.
	TransparentPbr,
	/// Unlit editor/debug overlay shader.
	OverlayUnlit,
};

/**
 * Return the stable base shader name.
 *
 * @param id Base shader identifier.
 * @return Static base shader name.
 */
constexpr std::string_view base_shader_id_name(BaseShaderId id) noexcept {
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

/// High-level render domain used for material and pass separation.
enum class RenderDomain : std::uint8_t {
	/// Lit opaque or alpha-tested surface domain.
	LitSurface,
	/// Transparent lit surface domain.
	TransparentSurface,
	/// Editor overlay domain.
	Overlay,
	/// Picking/ID output domain.
	Picking,
	/// Post-processing domain.
	Post,
};

/**
 * Return the stable render-domain name.
 *
 * @param domain Render domain to name.
 * @return Static domain name.
 */
constexpr std::string_view render_domain_name(RenderDomain domain) noexcept {
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

/// Queue used for sorting and routing draw packets.
enum class RenderQueue : std::uint8_t {
	/// Legacy prototype opaque queue.
	PrototypeOpaque,
	/// Opaque surface queue.
	Opaque,
	/// Alpha-tested surface queue.
	AlphaCutout,
	/// Transparent surface queue.
	Transparent,
	/// Depth-tested overlay queue.
	OverlayDepthTested,
	/// X-ray overlay queue.
	OverlayXRay,
	/// Always-on-top overlay queue.
	OverlayAlwaysOnTop,
	/// Picking queue.
	Picking,
};

/**
 * Return the stable render-queue name.
 *
 * @param queue Render queue to name.
 * @return Static queue name.
 */
constexpr std::string_view render_queue_name(RenderQueue queue) noexcept {
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
