#pragma once

#include "crimson/graph/render_resource_desc.hpp"

#include <array>
#include <string_view>
#include <vector>

namespace crimson {

inline constexpr std::string_view kBackbufferColorTargetName = "BackbufferColor";
inline constexpr std::string_view kBackbufferDepthTargetName = "BackbufferDepth";
inline constexpr std::string_view kHdrSceneColorTargetName = "HdrSceneColor";
inline constexpr std::string_view kSceneDepthTargetName = "SceneDepth";
inline constexpr std::string_view kPickingIdTargetName = "PickingId";
inline constexpr std::string_view kToneMappedColorTargetName = "ToneMappedColor";

enum class FrameTargetKind {
	BackbufferColor,
	BackbufferDepth,
	HdrSceneColor,
	SceneDepth,
	PickingId,
	ToneMappedColor,
};

enum class PickingIdTargetFormat {
	R32Uint,
	Rgba8Data,
};

struct FrameTargetDescriptor {
	FrameTargetKind kind = FrameTargetKind::BackbufferColor;
	std::string_view name;
	RenderResourceFormat format = RenderResourceFormat::Rgba8Unorm;
	bool external = false;
	bool resize_dependent = true;
	bool color_managed = false;
	bool hdr = false;
	bool data = false;
	std::uint8_t sample_count = 1;
};

[[nodiscard]] std::array<FrameTargetDescriptor, 6> v1_frame_target_descriptors(
		PickingIdTargetFormat picking_format = PickingIdTargetFormat::R32Uint) noexcept;
[[nodiscard]] std::vector<RenderResourceDesc> make_v1_frame_target_resource_descs(
		ViewportExtent extent,
		PickingIdTargetFormat picking_format = PickingIdTargetFormat::R32Uint);
[[nodiscard]] bool is_color_managed_frame_target(std::string_view name) noexcept;
[[nodiscard]] bool is_hdr_frame_target(std::string_view name) noexcept;
[[nodiscard]] bool is_data_frame_target(std::string_view name) noexcept;

} // namespace crimson
