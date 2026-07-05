#pragma once

#include "crimson/pipeline/draw_packet.hpp"

#include <span>
#include <vector>

namespace crimson::gpu {

using PbrDrawPacket = DrawPacket;
using PbrPassPackets = DrawPacketBuildResult;

[[nodiscard]] PbrPassPackets prepare_pbr_pass_packets(
    std::span<const RenderObject> objects,
    const BaseShaderRegistry& registry,
    const MaterialSystem& materials,
    const RenderCamera& camera,
    RenderMeshHandle fallback_mesh,
    RenderMaterialHandle fallback_material,
    float view_aspect_ratio = 1.0F);

} // namespace crimson::gpu
