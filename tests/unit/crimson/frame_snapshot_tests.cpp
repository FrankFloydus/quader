#include "crimson/frame/frame_builder.hpp"
#include "crimson/scene/render_world.hpp"

#include <gtest/gtest.h>

#include <array>
#include <iostream>
#include <string_view>

namespace {

void expect_true(bool condition, std::string_view message)
{
    EXPECT_TRUE(condition) << message;
}

[[nodiscard]] crimson::PrototypeCamera make_camera()
{
    return crimson::PrototypeCamera{
        .eye = {0.0F, 4.0F, 8.0F},
        .target = {0.0F, 0.0F, 0.0F},
        .up = {0.0F, 1.0F, 0.0F},
        .forward = {0.0F, -0.4F, -1.0F},
        .projection = crimson::PrototypeCameraProjection::Perspective,
    };
}

TEST(FrameSnapshot, BuilderRejectsInvalidViews)
{
    const std::array<crimson::PrototypeCamera, 1> cameras = {make_camera()};
    const std::array<crimson::PrototypeViewportView, 1> views = {
        crimson::PrototypeViewportView{
            .rect = crimson::PrototypeViewportRect{0, 0, 0, 100},
            .camera_index = 0,
            .debug_name = "Invalid",
        },
    };
    const crimson::PrototypeViewportFrame frame{
        .target_extent = crimson::ViewportExtent{100, 100, 1.0F},
        .views = views,
        .cameras = cameras,
    };

    const crimson::FrameBuilder builder;
    const auto snapshot = builder.build_prototype_snapshot(frame);
    expect_true(!snapshot, "builder rejects a zero-width view");
    expect_true(
        !snapshot && snapshot.error().code == crimson::RendererDiagnosticCode::InvalidFrameSnapshot,
        "invalid view failure reports InvalidFrameSnapshot");
}

TEST(FrameSnapshot, BuilderFreezesPrototypeFrameIntoImmutableSnapshot)
{
    std::array<crimson::PrototypeCamera, 1> cameras = {make_camera()};
    std::array<crimson::PrototypeViewportView, 1> views = {
        crimson::PrototypeViewportView{
            .rect = crimson::PrototypeViewportRect{4, 8, 320, 200},
            .camera_index = 0,
            .debug_name = "Perspective",
        },
    };
    std::array<crimson::PickingRequest, 1> picking_requests = {
        crimson::PickingRequest{.request_id = 77, .view_index = 0, .x_px = 12, .y_px = 24},
    };
    crimson::PrototypeViewportFrame frame{
        .target_extent = crimson::ViewportExtent{640, 480, 1.0F},
        .views = views,
        .cameras = cameras,
        .picking_requests = picking_requests,
        .animation_enabled = true,
        .elapsed_seconds = 2.0,
    };

    const crimson::FrameBuilder builder;
    auto built = builder.build_prototype_snapshot(frame);
    expect_true(built.has_value(), "valid prototype frame builds a snapshot");
    if (!built) {
        return;
    }

    crimson::FrameSnapshot snapshot = std::move(built).value();
    views[0].rect.width = 1;
    cameras[0].eye.x = 99.0F;
    picking_requests[0].x_px = 99;
    frame.elapsed_seconds = 99.0;

    expect_true(snapshot.target_extent().width_px == 640, "snapshot keeps copied target extent");
    expect_true(snapshot.views().size() == 1, "snapshot keeps copied view count");
    expect_true(snapshot.views()[0].rect.width == 320, "snapshot view rect is immutable from source changes");
    expect_true(snapshot.views()[0].camera.eye.x == 0.0F, "snapshot camera is immutable from source changes");
    expect_true(snapshot.elapsed_seconds() == 2.0, "snapshot keeps copied elapsed seconds");
    expect_true(snapshot.objects().size() == 1, "prototype snapshot contains one prepared object");
    expect_true(
        snapshot.objects()[0].base_shader == crimson::BaseShaderId::OpaquePbr
            && snapshot.objects()[0].queue == crimson::RenderQueue::Opaque,
        "prototype cube is prepared as an OpaquePbr render object");
    expect_true(snapshot.overlays().size() == 1, "prototype snapshot contains one overlay command");
    expect_true(snapshot.grid_overlay_payloads().size() == 1, "prototype snapshot contains one grid overlay payload");
    expect_true(snapshot.picking_requests().size() == 1, "prototype snapshot copies picking requests");
    expect_true(snapshot.picking_requests()[0].x_px == 12, "picking request is immutable from source changes");
    expect_true(
        snapshot.viewport_settings().exposure_ev100 == 0.0F,
        "prototype snapshot preserves non-physical exposure until fixture lighting is physical");
    expect_true(
        snapshot.overlays()[0].primitive == crimson::OverlayPrimitive::Grid
            && snapshot.overlays()[0].base_shader == crimson::BaseShaderId::OverlayUnlit,
        "prototype grid is emitted as an OverlayUnlit grid command");
    expect_true(
        snapshot.overlays()[0].payload_offset == 0 && snapshot.overlays()[0].payload_count == 1,
        "grid overlay command points at immutable grid payload data");
    expect_true(
        snapshot.objects()[0].world_from_object != crimson::identity_transform(),
        "animated prototype object has a prepared transform");
}

TEST(FrameSnapshot, RenderWorldStoresPreparedObjectsById)
{
    crimson::RenderWorld world;
    crimson::RenderObject object;
    object.object_id = 7;
    object.visible = true;

    const crimson::RenderObjectId id = world.add_object(object);
    expect_true(id == 7, "render world preserves explicit object ids");
    expect_true(world.objects().size() == 1, "render world stores added objects");

    object.visible = false;
    expect_true(world.update_object(id, object), "render world updates existing objects");
    expect_true(!world.objects()[0].visible, "render world exposes updated prepared object data");

    expect_true(world.remove_object(id), "render world removes existing objects");
    expect_true(world.objects().empty(), "render world is empty after removal");
}

} // namespace
