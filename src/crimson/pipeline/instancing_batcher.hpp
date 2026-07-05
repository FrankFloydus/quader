#pragma once

#include "crimson/frame/frame_stats.hpp"
#include "crimson/pipeline/draw_packet.hpp"

#include <span>
#include <vector>

namespace crimson {

struct InstanceBatchKey {
    RenderMeshHandle mesh;
    RenderMaterialHandle material;
    BaseShaderId base_shader = BaseShaderId::OpaquePbr;
    ShaderProgramId program = ShaderProgramId::OpaquePbr;
    RenderQueue queue = RenderQueue::Opaque;
    std::uint32_t submesh_index = 0;
    bool double_sided = false;

    friend bool operator==(const InstanceBatchKey&, const InstanceBatchKey&) = default;
};

struct InstanceBatch {
    InstanceBatchKey key;
    std::vector<DrawPacket> instances;
};

[[nodiscard]] InstanceBatchKey instance_batch_key_for(const DrawPacket& packet) noexcept;
[[nodiscard]] std::vector<InstanceBatch> build_instance_batches(std::span<const DrawPacket> packets);
[[nodiscard]] FrameInstancingStats make_instancing_stats(std::span<const InstanceBatch> batches) noexcept;
[[nodiscard]] bool render_queue_supports_instancing(RenderQueue queue) noexcept;

} // namespace crimson
