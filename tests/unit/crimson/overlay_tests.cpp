#include "crimson/frame/frame_builder.hpp"
#include "crimson/frame/frame_targets.hpp"
#include "crimson/graph/render_graph.hpp"
#include "crimson/overlays/grid_overlay.hpp"
#include "crimson/overlays/overlay_system.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <iostream>
#include <string_view>
#include <utility>

namespace {

void expect_true(bool condition, std::string_view message)
{
    EXPECT_TRUE(condition) << message;
}

[[nodiscard]] bool near(float left, float right, float tolerance = 0.001F) noexcept
{
    return std::fabs(left - right) <= tolerance;
}

[[nodiscard]] const crimson::RenderPass* find_pass(const crimson::RenderGraph& graph, std::string_view name)
{
    for (const crimson::RenderPass& pass : graph.passes()) {
        if (pass.name == name) {
            return &pass;
        }
    }
    return nullptr;
}

[[nodiscard]] bool pass_uses_resource(const crimson::RenderPass& pass, std::string_view resource_name)
{
    for (const crimson::ResourceUse& use : pass.resources) {
        if (use.resource_name == resource_name) {
            return true;
        }
    }
    return false;
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

TEST(Overlay, PrototypeGridIsOverlayCommandNotLitObject)
{
    const std::array<crimson::PrototypeCamera, 1> cameras = {make_camera()};
    const std::array<crimson::PrototypeViewportView, 1> views = {
        crimson::PrototypeViewportView{
            .rect = crimson::PrototypeViewportRect{0, 0, 320, 240},
            .camera_index = 0,
            .debug_name = "Perspective",
        },
    };
    const crimson::PrototypeViewportFrame frame{
        .target_extent = crimson::ViewportExtent{640, 480, 1.0F},
        .views = views,
        .cameras = cameras,
    };

    const crimson::FrameBuilder builder;
    auto built = builder.build_prototype_snapshot(frame);
    expect_true(built.has_value(), "valid prototype frame builds for overlay tests");
    if (!built) {
        return;
    }

    const crimson::FrameSnapshot snapshot = std::move(built).value();
    expect_true(snapshot.objects().size() == 1, "prototype still has one lit render object");
    expect_true(snapshot.objects()[0].queue == crimson::RenderQueue::Opaque, "prototype object remains opaque PBR");
    expect_true(snapshot.overlays().size() == 1, "prototype grid is emitted through overlay commands");
    expect_true(snapshot.grid_overlay_payloads().size() == 1, "prototype grid payload is snapshot-owned");
    expect_true(
        snapshot.overlays()[0].primitive == crimson::OverlayPrimitive::Grid,
        "prototype overlay command is a grid primitive");
    expect_true(
        snapshot.overlays()[0].base_shader == crimson::BaseShaderId::OverlayUnlit,
        "prototype grid uses OverlayUnlit schema");
    for (const crimson::RenderObject& object : snapshot.objects()) {
        expect_true(
            object.queue != crimson::RenderQueue::OverlayDepthTested
                && object.queue != crimson::RenderQueue::OverlayXRay
                && object.queue != crimson::RenderQueue::OverlayAlwaysOnTop,
            "overlay queues are not represented as lit render objects");
    }
}

TEST(Overlay, OverlaySystemConvertsSrgbToLinearSdr)
{
    const std::array<float, 4> converted = crimson::to_linear_sdr_array(
        crimson::ColorSrgb{0.5F, 0.0F, 0.0F, 0.8F},
        0.5F);
    expect_true(near(converted[0], 0.214F), "overlay sRGB color converts to linear SDR");
    expect_true(near(converted[1], 0.0F), "overlay green channel remains black");
    expect_true(near(converted[3], 0.4F), "overlay opacity multiplies alpha");
}

TEST(Overlay, OverlaySystemBucketsGridCommandsByDepthMode)
{
    crimson::GridOverlayCommand grid;
    grid.view_index = 7;
    grid.minor_color = crimson::ColorSrgb{0.5F, 0.0F, 0.0F, 1.0F};

    std::array<crimson::GridOverlayCommand, 3> grids = {grid, grid, grid};
    std::array<crimson::OverlayCommand, 3> commands = {
        crimson::OverlayCommand{
            .view_index = 7,
            .primitive = crimson::OverlayPrimitive::Grid,
            .depth_mode = crimson::OverlayDepthMode::DepthTested,
            .payload_offset = 0,
            .payload_count = 1,
        },
        crimson::OverlayCommand{
            .view_index = 7,
            .primitive = crimson::OverlayPrimitive::Grid,
            .depth_mode = crimson::OverlayDepthMode::XRay,
            .opacity = 0.5F,
            .payload_offset = 1,
            .payload_count = 1,
        },
        crimson::OverlayCommand{
            .view_index = 7,
            .primitive = crimson::OverlayPrimitive::Grid,
            .depth_mode = crimson::OverlayDepthMode::AlwaysOnTop,
            .payload_offset = 2,
            .payload_count = 1,
        },
    };

    const crimson::OverlayDrawLists lists = crimson::OverlaySystem{}.prepare(commands, grids);
    expect_true(lists.depth_tested.grid_commands.size() == 1, "depth-tested grid command is bucketed");
    expect_true(lists.xray.grid_commands.size() == 1, "XRay grid command is bucketed");
    expect_true(lists.always_on_top.grid_commands.size() == 1, "always-on-top grid command is bucketed");
    expect_true(lists.command_count() == 6, "overlay draw lists retain command and grid payload records");
    expect_true(
        near(lists.xray.grid_commands[0].minor_color_linear_sdr[3], 0.5F),
        "prepared grid payload alpha honors overlay command opacity");
}

TEST(Overlay, OverlayPassesStayAfterToneMapAndOutOfHdrSceneColor)
{
    const crimson::RenderGraph graph = crimson::make_v1_correctness_render_graph(
        crimson::ViewportExtent{800, 600, 1.0F});
    const crimson::RenderPass* tone_map = find_pass(graph, "ToneMapPass");
    const crimson::RenderPass* depth_tested = find_pass(graph, "OverlayDepthTestedPass");
    const crimson::RenderPass* xray = find_pass(graph, "OverlayXRayPass");
    const crimson::RenderPass* always_on_top = find_pass(graph, "OverlayAlwaysOnTopPass");
    expect_true(tone_map != nullptr, "tone-map pass exists before overlays");
    expect_true(depth_tested != nullptr && xray != nullptr && always_on_top != nullptr, "all overlay passes exist");

    bool seen_tone_map = false;
    for (const crimson::RenderPass& pass : graph.passes()) {
        if (pass.name == "ToneMapPass") {
            seen_tone_map = true;
        }
        if (pass.name == "OverlayDepthTestedPass"
            || pass.name == "OverlayXRayPass"
            || pass.name == "OverlayAlwaysOnTopPass") {
            expect_true(seen_tone_map, "overlay pass is ordered after ToneMapPass");
            expect_true(
                !pass_uses_resource(pass, crimson::kHdrSceneColorTargetName),
                "overlay pass never touches HdrSceneColor");
            expect_true(
                pass_uses_resource(pass, crimson::kToneMappedColorTargetName),
                "overlay pass composites into ToneMappedColor");
        }
    }
}

} // namespace
