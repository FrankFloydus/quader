/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
#include "crimson/pipeline/instancing_batcher.hpp"

#include <algorithm>
#include <tuple>

namespace crimson {
namespace {

[[nodiscard]] bool instance_batch_key_less(const InstanceBatchKey &left, const InstanceBatchKey &right) noexcept {
	return std::tuple{
		left.queue,
		left.program,
		left.base_shader,
		left.material.index,
		left.material.generation,
		left.mesh.index,
		left.mesh.generation,
		left.submesh_index,
		left.double_sided,
	} < std::tuple{
		right.queue,
		right.program,
		right.base_shader,
		right.material.index,
		right.material.generation,
		right.mesh.index,
		right.mesh.generation,
		right.submesh_index,
		right.double_sided,
	};
}

} // namespace

bool render_queue_supports_instancing(RenderQueue queue) noexcept {
	return queue == RenderQueue::Opaque || queue == RenderQueue::AlphaCutout || queue == RenderQueue::Transparent;
}

InstanceBatchKey instance_batch_key_for(const DrawPacket &packet) noexcept {
	return InstanceBatchKey{
		.mesh = packet.mesh,
		.material = packet.material,
		.base_shader = packet.base_shader,
		.program = packet.program,
		.queue = packet.queue,
		.submesh_index = packet.submesh_index,
		.double_sided = packet.double_sided,
	};
}

std::vector<InstanceBatch> build_instance_batches(std::span<const DrawPacket> packets) {
	std::vector<DrawPacket> sorted_packets;
	sorted_packets.reserve(packets.size());
	for (const DrawPacket &packet : packets) {
		if (render_queue_supports_instancing(packet.queue)) {
			sorted_packets.push_back(packet);
		}
	}
	std::sort(sorted_packets.begin(), sorted_packets.end(), [](const DrawPacket &left, const DrawPacket &right) {
		return instance_batch_key_less(instance_batch_key_for(left), instance_batch_key_for(right));
	});

	std::vector<InstanceBatch> batches;
	for (const DrawPacket &packet : sorted_packets) {
		const InstanceBatchKey kEy = instance_batch_key_for(packet);
		if (batches.empty() || !(batches.back().key == kEy)) {
			batches.push_back(InstanceBatch{ .key = kEy });
		}
		batches.back().instances.push_back(packet);
	}
	return batches;
}

FrameInstancingStats make_instancing_stats(std::span<const InstanceBatch> batches) noexcept {
	FrameInstancingStats stats;
	stats.batch_count = static_cast<std::uint32_t>(batches.size());
	for (const InstanceBatch &batch : batches) {
		const std::uint32_t kInstanceCount = static_cast<std::uint32_t>(batch.instances.size());
		stats.submitted_instance_count += kInstanceCount;
		if (kInstanceCount > 1) {
			++stats.instanced_batch_count;
			stats.saved_draw_call_count += kInstanceCount - 1;
		} else {
			++stats.single_draw_batch_count;
		}
	}
	return stats;
}

} // namespace crimson
