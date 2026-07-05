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

#include "crimson/frame/frame_stats.hpp"
#include "crimson/pipeline/draw_packet.hpp"

#include <span>
#include <vector>

namespace crimson {

/// Key used to group compatible draw packets into one instanced submission.
struct InstanceBatchKey {
	/// Mesh resource shared by the batch.
	RenderMeshHandle mesh;
	/// Material resource shared by the batch.
	RenderMaterialHandle material;
	/// Base shader shared by the batch.
	BaseShaderId base_shader = BaseShaderId::OpaquePbr;
	/// Shader program shared by the batch.
	ShaderProgramId program = ShaderProgramId::OpaquePbr;
	/// Render queue shared by the batch.
	RenderQueue queue = RenderQueue::Opaque;
	/// Submesh index shared by the batch.
	std::uint32_t submesh_index = 0;
	/// Culling-state flag shared by the batch.
	bool double_sided = false;

	/// Compare batch keys by all grouping fields.
	friend bool operator==(const InstanceBatchKey &, const InstanceBatchKey &) = default;
};

/// Instancing group containing compatible draw packets.
struct InstanceBatch {
	/// Batch key shared by all instances.
	InstanceBatchKey key;
	/// Draw packets submitted as instances.
	std::vector<DrawPacket> instances;
};

/**
 * Build an instancing key for one draw packet.
 *
 * @param packet Packet to convert.
 * @return Batch key containing the packet's grouping fields.
 */
[[nodiscard]] InstanceBatchKey instance_batch_key_for(const DrawPacket &packet) noexcept;
/**
 * Group draw packets into instancing batches.
 *
 * @param packets Packets to group.
 * @return Instance batches in deterministic order.
 */
[[nodiscard]] std::vector<InstanceBatch> build_instance_batches(std::span<const DrawPacket> packets);
/**
 * Build instancing stats from batches.
 *
 * @param batches Instance batches to count.
 * @return Frame instancing counters.
 */
[[nodiscard]] FrameInstancingStats make_instancing_stats(std::span<const InstanceBatch> batches) noexcept;
/**
 * Check whether a render queue supports instanced drawing.
 *
 * @param queue Queue to test.
 * @return True when the queue can be grouped into instance batches.
 */
[[nodiscard]] bool render_queue_supports_instancing(RenderQueue queue) noexcept;

} // namespace crimson
