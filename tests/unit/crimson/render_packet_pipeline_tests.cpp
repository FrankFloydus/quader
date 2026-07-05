#include "crimson/material/material_system.hpp"
#include "crimson/pipeline/draw_packet.hpp"
#include "crimson/pipeline/instancing_batcher.hpp"

#include <gtest/gtest.h>

#include <array>
#include <iostream>
#include <string_view>
#include <vector>

namespace {

void expect_true(bool condition, std::string_view message)
{
    EXPECT_TRUE(condition) << message;
}

[[nodiscard]] quader::math::Aabb bounds(float center_x, float center_y, float center_z, float half_extent = 0.5F)
{
    return quader::math::Aabb{
        .min = {center_x - half_extent, center_y - half_extent, center_z - half_extent},
        .max = {center_x + half_extent, center_y + half_extent, center_z + half_extent},
    };
}

[[nodiscard]] crimson::RenderObject object(
    crimson::RenderObjectId id,
    crimson::RenderMaterialHandle material,
    crimson::BaseShaderId shader,
    crimson::RenderQueue queue,
    quader::math::Aabb object_bounds)
{
    crimson::RenderObject render_object;
    render_object.object_id = id;
    render_object.mesh = crimson::RenderMeshHandle{1, 1};
    render_object.material = material;
    render_object.base_shader = shader;
    render_object.queue = queue;
    render_object.world_bounds = object_bounds;
    render_object.world_from_object[12] = quader::math::center(object_bounds).x;
    render_object.world_from_object[13] = quader::math::center(object_bounds).y;
    render_object.world_from_object[14] = quader::math::center(object_bounds).z;
    return render_object;
}

[[nodiscard]] crimson::RenderCamera camera()
{
    return crimson::RenderCamera{
        .eye = {0.0F, 0.0F, 0.0F},
        .target = {0.0F, 0.0F, -1.0F},
        .forward = {0.0F, 0.0F, -1.0F},
        .near_plane_m = 0.1F,
        .far_plane_m = 100.0F,
        .vertical_fov_degrees = 60.0F,
    };
}

TEST(RenderPacketPipeline, CullingKeepsVisibleRejectsOutsideAndCountsInvalidBounds)
{
    crimson::MaterialSystem materials;
    const crimson::RenderMaterialHandle opaque = materials.create_default_material(crimson::BaseShaderId::OpaquePbr).value();

    std::array<crimson::RenderObject, 3> objects = {
        object(1, opaque, crimson::BaseShaderId::OpaquePbr, crimson::RenderQueue::Opaque, bounds(0.0F, 0.0F, -5.0F)),
        object(2, opaque, crimson::BaseShaderId::OpaquePbr, crimson::RenderQueue::Opaque, bounds(100.0F, 0.0F, -5.0F)),
        object(3, opaque, crimson::BaseShaderId::OpaquePbr, crimson::RenderQueue::Opaque, quader::math::Aabb{}),
    };

    const crimson::DrawPacketBuildResult packets = crimson::build_draw_packets(
        objects,
        materials.registry(),
        materials,
        camera(),
        crimson::RenderMeshHandle{9, 1},
        opaque,
        1.0F);

    expect_true(packets.culling.input_object_count == 3, "culling records input object count");
    expect_true(packets.culling.visible_object_count == 2, "inside and invalid-bounds objects remain visible");
    expect_true(packets.culling.culled_object_count == 1, "outside object is culled");
    expect_true(packets.culling.invalid_bounds_count == 1, "invalid bounds are kept and counted");
    expect_true(packets.opaque.size() == 2, "only visible opaque packets are emitted");
}

TEST(RenderPacketPipeline, CullingUsesNonSquareViewAspect)
{
    crimson::MaterialSystem materials;
    const crimson::RenderMaterialHandle opaque = materials.create_default_material(crimson::BaseShaderId::OpaquePbr).value();
    const std::array<crimson::RenderObject, 1> objects = {
        object(1, opaque, crimson::BaseShaderId::OpaquePbr, crimson::RenderQueue::Opaque, bounds(5.5F, 0.0F, -5.0F)),
    };

    const crimson::DrawPacketBuildResult square_packets = crimson::build_draw_packets(
        objects,
        materials.registry(),
        materials,
        camera(),
        crimson::RenderMeshHandle{9, 1},
        opaque,
        1.0F);
    const crimson::DrawPacketBuildResult wide_packets = crimson::build_draw_packets(
        objects,
        materials.registry(),
        materials,
        camera(),
        crimson::RenderMeshHandle{9, 1},
        opaque,
        2.0F);

    expect_true(square_packets.culling.culled_object_count == 1, "square frustum culls object outside horizontal FOV");
    expect_true(wide_packets.culling.visible_object_count == 1, "wide viewport aspect keeps object inside horizontal FOV");
}

TEST(RenderPacketPipeline, OpaqueSortIsDeterministicAndStateFriendly)
{
    crimson::MaterialSystem materials;
    const crimson::RenderMaterialHandle material_a = materials.create_default_material(crimson::BaseShaderId::OpaquePbr).value();
    const crimson::RenderMaterialHandle material_b = materials.create_default_material(crimson::BaseShaderId::OpaquePbr).value();

    std::array<crimson::RenderObject, 3> objects = {
        object(30, material_b, crimson::BaseShaderId::OpaquePbr, crimson::RenderQueue::Opaque, bounds(0.0F, 0.0F, -6.0F)),
        object(10, material_a, crimson::BaseShaderId::OpaquePbr, crimson::RenderQueue::Opaque, bounds(0.0F, 0.0F, -5.0F)),
        object(20, material_a, crimson::BaseShaderId::OpaquePbr, crimson::RenderQueue::Opaque, bounds(0.0F, 0.0F, -7.0F)),
    };

    const crimson::DrawPacketBuildResult packets = crimson::build_draw_packets(
        objects,
        materials.registry(),
        materials,
        camera(),
        crimson::RenderMeshHandle{9, 1},
        material_a,
        1.0F);

    expect_true(packets.opaque.size() == 3, "all opaque objects build packets");
    expect_true(
        packets.opaque[0].object_id == 10 && packets.opaque[1].object_id == 20,
        "opaque packets sort by state and stable object id, not input order");
    expect_true(packets.packets.sorted_packet_count == 3, "sorted packet count is reported");
}

TEST(RenderPacketPipeline, TransparentSortIsBackToFrontWithStableTies)
{
    crimson::MaterialSystem materials;
    const crimson::RenderMaterialHandle transparent =
        materials.create_default_material(crimson::BaseShaderId::TransparentPbr).value();

    std::array<crimson::RenderObject, 3> objects = {
        object(20, transparent, crimson::BaseShaderId::TransparentPbr, crimson::RenderQueue::Transparent, bounds(0.0F, 0.0F, -5.0F)),
        object(10, transparent, crimson::BaseShaderId::TransparentPbr, crimson::RenderQueue::Transparent, bounds(0.0F, 0.0F, -5.0F)),
        object(30, transparent, crimson::BaseShaderId::TransparentPbr, crimson::RenderQueue::Transparent, bounds(0.0F, 0.0F, -9.0F)),
    };

    const crimson::DrawPacketBuildResult packets = crimson::build_draw_packets(
        objects,
        materials.registry(),
        materials,
        camera(),
        crimson::RenderMeshHandle{9, 1},
        transparent,
        1.0F);

    expect_true(packets.transparent.size() == 3, "transparent packets are emitted");
    expect_true(packets.transparent[0].object_id == 30, "far transparent packet is submitted first");
    expect_true(
        packets.transparent[1].object_id == 10 && packets.transparent[2].object_id == 20,
        "same-depth transparent packets use stable object id tie-breakers");
}

TEST(RenderPacketPipeline, InstancingKeysExcludeTransformAndObjectId)
{
    crimson::DrawPacket first;
    first.object_id = 1;
    first.mesh = crimson::RenderMeshHandle{1, 1};
    first.material = crimson::RenderMaterialHandle{2, 1};
    first.base_shader = crimson::BaseShaderId::OpaquePbr;
    first.program = crimson::ShaderProgramId::OpaquePbr;
    first.queue = crimson::RenderQueue::Opaque;

    crimson::DrawPacket second = first;
    second.object_id = 2;
    second.world_from_object[12] = 10.0F;

    crimson::DrawPacket overlay = first;
    overlay.object_id = 3;
    overlay.queue = crimson::RenderQueue::OverlayAlwaysOnTop;

    const std::array<crimson::DrawPacket, 3> packets = {second, overlay, first};
    const std::vector<crimson::InstanceBatch> batches = crimson::build_instance_batches(packets);
    const crimson::FrameInstancingStats stats = crimson::make_instancing_stats(batches);

    expect_true(batches.size() == 1, "overlay packets are excluded from PBR instancing batches");
    expect_true(batches[0].instances.size() == 2, "same mesh/material/program packets group despite transform/id differences");
    expect_true(stats.instanced_batch_count == 1, "multi-instance batch is counted");
    expect_true(stats.saved_draw_call_count == 1, "saved draw calls are counted for staged instancing");
}

} // namespace
