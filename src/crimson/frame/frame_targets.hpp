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

#include "crimson/graph/render_resource_desc.hpp"

#include <array>
#include <string_view>
#include <vector>

namespace crimson {

/// Name of the external color target presented by the viewport.
inline constexpr std::string_view kBackbufferColorTargetName = "BackbufferColor";
/// Name of the external depth target paired with the backbuffer.
inline constexpr std::string_view kBackbufferDepthTargetName = "BackbufferDepth";
/// Name of the linear HDR scene color target.
inline constexpr std::string_view kHdrSceneColorTargetName = "HdrSceneColor";
/// Name of the scene depth target.
inline constexpr std::string_view kSceneDepthTargetName = "SceneDepth";
/// Name of the picking-id data target.
inline constexpr std::string_view kPickingIdTargetName = "PickingId";
/// Name of the SDR tone-mapped scene color target.
inline constexpr std::string_view kToneMappedColorTargetName = "ToneMappedColor";

/// Logical frame targets used by the V1 render graph.
enum class FrameTargetKind {
	BackbufferColor, ///< External color output.
	BackbufferDepth, ///< External depth output.
	HdrSceneColor, ///< Linear HDR scene color.
	SceneDepth, ///< Scene depth.
	PickingId, ///< Data target storing picking ids.
	ToneMappedColor, ///< SDR scene color after tone mapping.
};

/// GPU format policy for picking-id targets.
enum class PickingIdTargetFormat {
	R32Uint,   ///< Use an unsigned integer target.
	Rgba8Data, ///< Use RGBA8-encoded ids.
};

/// Render-resource descriptor plus semantic frame-target metadata.
struct FrameTargetDescriptor {
	/// Logical target kind.
	FrameTargetKind kind = FrameTargetKind::BackbufferColor;
	/// Graph resource name.
	std::string_view name;
	/// Resource storage format.
	RenderResourceFormat format = RenderResourceFormat::Rgba8Unorm;
	/// True when the target is owned by the host surface.
	bool external = false;
	/// True when the target dimensions follow the viewport extent.
	bool resize_dependent = true;
	/// True when color-space conversion rules apply.
	bool color_managed = false;
	/// True when the target stores linear HDR color.
	bool hdr = false;
	/// True when the target stores non-color data.
	bool data = false;
	/// Sample count for the target.
	std::uint8_t sample_count = 1;
};

/// Return V1 frame target descriptors for the requested picking format.
[[nodiscard]] std::array<FrameTargetDescriptor, 6> v1_frame_target_descriptors(
		PickingIdTargetFormat picking_format = PickingIdTargetFormat::R32Uint) noexcept;
/// Convert V1 frame targets into render graph resource descriptors.
[[nodiscard]] std::vector<RenderResourceDesc> make_v1_frame_target_resource_descs(
		ViewportExtent extent,
		PickingIdTargetFormat picking_format = PickingIdTargetFormat::R32Uint);
/// Return true when the target name denotes a color-managed target.
[[nodiscard]] bool is_color_managed_frame_target(std::string_view name) noexcept;
/// Return true when the target name denotes a linear HDR target.
[[nodiscard]] bool is_hdr_frame_target(std::string_view name) noexcept;
/// Return true when the target name denotes a data target.
[[nodiscard]] bool is_data_frame_target(std::string_view name) noexcept;

} // namespace crimson
