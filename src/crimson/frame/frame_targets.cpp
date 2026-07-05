#include "crimson/frame/frame_targets.hpp"

#include <string>

namespace crimson {
namespace {

[[nodiscard]] RenderResourceFormat picking_format(PickingIdTargetFormat format) noexcept
{
    return format == PickingIdTargetFormat::R32Uint
        ? RenderResourceFormat::R32Uint
        : RenderResourceFormat::Rgba8Unorm;
}

[[nodiscard]] RenderResourceDesc to_resource_desc(
    const FrameTargetDescriptor& target,
    ViewportExtent extent)
{
    return RenderResourceDesc{
        .name = std::string(target.name),
        .format = target.format,
        .extent = extent,
        .external = target.external,
        .resize_dependent = target.resize_dependent,
        .sample_count = target.sample_count,
    };
}

} // namespace

std::array<FrameTargetDescriptor, 6> v1_frame_target_descriptors(PickingIdTargetFormat format) noexcept
{
    return {
        FrameTargetDescriptor{
            .kind = FrameTargetKind::BackbufferColor,
            .name = kBackbufferColorTargetName,
            .format = RenderResourceFormat::BackbufferColor,
            .external = true,
            .resize_dependent = true,
            .color_managed = true,
        },
        FrameTargetDescriptor{
            .kind = FrameTargetKind::BackbufferDepth,
            .name = kBackbufferDepthTargetName,
            .format = RenderResourceFormat::BackbufferDepth,
            .external = true,
            .resize_dependent = true,
            .data = true,
        },
        FrameTargetDescriptor{
            .kind = FrameTargetKind::HdrSceneColor,
            .name = kHdrSceneColorTargetName,
            .format = RenderResourceFormat::Rgba16Float,
            .resize_dependent = true,
            .color_managed = true,
            .hdr = true,
        },
        FrameTargetDescriptor{
            .kind = FrameTargetKind::SceneDepth,
            .name = kSceneDepthTargetName,
            .format = RenderResourceFormat::D24S8,
            .resize_dependent = true,
            .data = true,
        },
        FrameTargetDescriptor{
            .kind = FrameTargetKind::PickingId,
            .name = kPickingIdTargetName,
            .format = picking_format(format),
            .resize_dependent = true,
            .data = true,
        },
        FrameTargetDescriptor{
            .kind = FrameTargetKind::ToneMappedColor,
            .name = kToneMappedColorTargetName,
            .format = RenderResourceFormat::Rgba16Float,
            .resize_dependent = true,
            .color_managed = true,
        },
    };
}

std::vector<RenderResourceDesc> make_v1_frame_target_resource_descs(
    ViewportExtent extent,
    PickingIdTargetFormat picking_format)
{
    std::vector<RenderResourceDesc> resources;
    const auto targets = v1_frame_target_descriptors(picking_format);
    resources.reserve(targets.size());
    for (const FrameTargetDescriptor& target : targets) {
        resources.push_back(to_resource_desc(target, extent));
    }
    return resources;
}

bool is_color_managed_frame_target(std::string_view name) noexcept
{
    for (const FrameTargetDescriptor& target : v1_frame_target_descriptors()) {
        if (target.name == name) {
            return target.color_managed;
        }
    }
    return false;
}

bool is_hdr_frame_target(std::string_view name) noexcept
{
    for (const FrameTargetDescriptor& target : v1_frame_target_descriptors()) {
        if (target.name == name) {
            return target.hdr;
        }
    }
    return false;
}

bool is_data_frame_target(std::string_view name) noexcept
{
    for (const FrameTargetDescriptor& target : v1_frame_target_descriptors()) {
        if (target.name == name) {
            return target.data;
        }
    }
    return false;
}

} // namespace crimson
