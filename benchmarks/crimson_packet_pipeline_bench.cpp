/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
#include "benchmark_main.hpp"

#include "crimson/material/material_system.hpp"
#include "crimson/pipeline/draw_packet.hpp"
#include "crimson/pipeline/instancing_batcher.hpp"

#include <vector>

namespace quader::benchmarks {
namespace {

[[nodiscard]] quader::math::Aabb bounds_for(std::uint32_t index) {
	const float kX = static_cast<float>(index % 128) * 0.05F;
	const float kY = static_cast<float>((index / 128) % 64) * 0.05F;
	const float kZ = -5.0F - static_cast<float>(index % 256) * 0.01F;
	return quader::math::Aabb{
		.min = { kX - 0.25F, kY - 0.25F, kZ - 0.25F },
		.max = { kX + 0.25F, kY + 0.25F, kZ + 0.25F },
	};
}

[[nodiscard]] std::vector<crimson::RenderObject> make_objects(
		std::uint32_t count,
		crimson::RenderMaterialHandle opaque,
		crimson::RenderMaterialHandle transparent) {
	std::vector<crimson::RenderObject> objects;
	objects.reserve(count);
	for (std::uint32_t index = 0; index < count; ++index) {
		const bool kTransparentObject = index % 11 == 0;
		const quader::math::Aabb kBounds = bounds_for(index);
		crimson::RenderObject object;
		object.object_id = index + 1;
		object.mesh = crimson::RenderMeshHandle{ 1 + (index % 8), 1 };
		object.material = kTransparentObject ? transparent : opaque;
		object.base_shader = kTransparentObject ? crimson::BaseShaderId::TransparentPbr : crimson::BaseShaderId::OpaquePbr;
		object.queue = kTransparentObject ? crimson::RenderQueue::Transparent : crimson::RenderQueue::Opaque;
		object.world_bounds = kBounds;
		object.world_from_object[12] = quader::math::center(kBounds).x;
		object.world_from_object[13] = quader::math::center(kBounds).y;
		object.world_from_object[14] = quader::math::center(kBounds).z;
		objects.push_back(object);
	}
	return objects;
}

} // namespace

BenchmarkResult run_crimson_packet_pipeline_benchmark(const BenchmarkRunConfig &config) {
	crimson::MaterialSystem materials;
	const crimson::RenderMaterialHandle kOpaque =
			materials.create_default_material(crimson::BaseShaderId::OpaquePbr).value();
	const crimson::RenderMaterialHandle kTransparent =
			materials.create_default_material(crimson::BaseShaderId::TransparentPbr).value();
	const std::vector<crimson::RenderObject> kObjects = make_objects(config.fixture_size, kOpaque, kTransparent);
	const crimson::RenderCamera kCamera{
		.eye = { 0.0F, 0.0F, 0.0F },
		.target = { 0.0F, 0.0F, -1.0F },
		.forward = { 0.0F, 0.0F, -1.0F },
		.far_plane_m = 1000.0F,
	};

	return run_benchmark("crimson_packet_pipeline", config, [&]() {
		const crimson::DrawPacketBuildResult kPackets = crimson::build_draw_packets(
				kObjects,
				materials.registry(),
				materials,
				kCamera,
				crimson::RenderMeshHandle{ 1, 1 },
				kOpaque);
		std::vector<crimson::DrawPacket> lit_packets;
		lit_packets.reserve(kPackets.draw_packet_count());
		lit_packets.insert(lit_packets.end(), kPackets.opaque.begin(), kPackets.opaque.end());
		lit_packets.insert(lit_packets.end(), kPackets.alpha_cutout.begin(), kPackets.alpha_cutout.end());
		lit_packets.insert(lit_packets.end(), kPackets.transparent.begin(), kPackets.transparent.end());
		const std::vector<crimson::InstanceBatch> kBatches = crimson::build_instance_batches(lit_packets);
		const crimson::FrameStats kStats = crimson::make_frame_stats(crimson::FrameStatsInput{
				.queues = crimson::FrameQueueStats{ .draw_packet_count = static_cast<std::uint32_t>(kPackets.draw_packet_count()) },
				.culling = kPackets.culling,
				.packets = kPackets.packets,
				.instancing = crimson::make_instancing_stats(kBatches),
		});
		return crimson::make_renderer_counters(kStats);
	});
}

} // namespace quader::benchmarks
