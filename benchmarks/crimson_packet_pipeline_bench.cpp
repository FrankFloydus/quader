#include "benchmark_main.hpp"

#include "crimson/material/material_system.hpp"
#include "crimson/pipeline/draw_packet.hpp"
#include "crimson/pipeline/instancing_batcher.hpp"

#include <vector>

namespace quader::benchmarks {
namespace {

[[nodiscard]] quader::math::Aabb bounds_for(std::uint32_t index)
{
    const float x = static_cast<float>(index % 128) * 0.05F;
    const float y = static_cast<float>((index / 128) % 64) * 0.05F;
    const float z = -5.0F - static_cast<float>(index % 256) * 0.01F;
    return quader::math::Aabb{
        .min = {x - 0.25F, y - 0.25F, z - 0.25F},
        .max = {x + 0.25F, y + 0.25F, z + 0.25F},
    };
}

[[nodiscard]] std::vector<crimson::RenderObject> make_objects(
    std::uint32_t count,
    crimson::RenderMaterialHandle opaque,
    crimson::RenderMaterialHandle transparent)
{
    std::vector<crimson::RenderObject> objects;
    objects.reserve(count);
    for (std::uint32_t index = 0; index < count; ++index) {
        const bool transparent_object = index % 11 == 0;
        const quader::math::Aabb bounds = bounds_for(index);
        crimson::RenderObject object;
        object.object_id = index + 1;
        object.mesh = crimson::RenderMeshHandle{1 + (index % 8), 1};
        object.material = transparent_object ? transparent : opaque;
        object.base_shader = transparent_object ? crimson::BaseShaderId::TransparentPbr : crimson::BaseShaderId::OpaquePbr;
        object.queue = transparent_object ? crimson::RenderQueue::Transparent : crimson::RenderQueue::Opaque;
        object.world_bounds = bounds;
        object.world_from_object[12] = quader::math::center(bounds).x;
        object.world_from_object[13] = quader::math::center(bounds).y;
        object.world_from_object[14] = quader::math::center(bounds).z;
        objects.push_back(object);
    }
    return objects;
}

} // namespace

BenchmarkResult run_crimson_packet_pipeline_benchmark(const BenchmarkRunConfig& config)
{
    crimson::MaterialSystem materials;
    const crimson::RenderMaterialHandle opaque =
        materials.create_default_material(crimson::BaseShaderId::OpaquePbr).value();
    const crimson::RenderMaterialHandle transparent =
        materials.create_default_material(crimson::BaseShaderId::TransparentPbr).value();
    const std::vector<crimson::RenderObject> objects = make_objects(config.fixture_size, opaque, transparent);
    const crimson::RenderCamera camera{
        .eye = {0.0F, 0.0F, 0.0F},
        .target = {0.0F, 0.0F, -1.0F},
        .forward = {0.0F, 0.0F, -1.0F},
        .far_plane_m = 1000.0F,
    };

    return run_benchmark("crimson_packet_pipeline", config, [&]() {
        const crimson::DrawPacketBuildResult packets = crimson::build_draw_packets(
            objects,
            materials.registry(),
            materials,
            camera,
            crimson::RenderMeshHandle{1, 1},
            opaque);
        std::vector<crimson::DrawPacket> lit_packets;
        lit_packets.reserve(packets.draw_packet_count());
        lit_packets.insert(lit_packets.end(), packets.opaque.begin(), packets.opaque.end());
        lit_packets.insert(lit_packets.end(), packets.alpha_cutout.begin(), packets.alpha_cutout.end());
        lit_packets.insert(lit_packets.end(), packets.transparent.begin(), packets.transparent.end());
        const std::vector<crimson::InstanceBatch> batches = crimson::build_instance_batches(lit_packets);
        const crimson::FrameStats stats = crimson::make_frame_stats(crimson::FrameStatsInput{
            .queues = crimson::FrameQueueStats{.draw_packet_count = static_cast<std::uint32_t>(packets.draw_packet_count())},
            .culling = packets.culling,
            .packets = packets.packets,
            .instancing = crimson::make_instancing_stats(batches),
        });
        return crimson::make_renderer_counters(stats);
    });
}

} // namespace quader::benchmarks
