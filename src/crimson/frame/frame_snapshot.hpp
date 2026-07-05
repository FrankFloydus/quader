#pragma once

#include "crimson/frame/frame_stats.hpp"
#include "crimson/overlays/overlay_command.hpp"
#include "crimson/picking/picking.hpp"
#include "crimson/renderer_types.hpp"
#include "crimson/scene/render_camera.hpp"
#include "crimson/scene/render_environment.hpp"
#include "crimson/scene/render_light.hpp"
#include "crimson/scene/render_object.hpp"
#include "crimson/scene/viewport_settings.hpp"

#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace crimson {

enum class DebugViewMode {
    FinalColor,
};

struct RenderViewportRect {
    std::uint16_t x = 0;
    std::uint16_t y = 0;
    std::uint16_t width = 1;
    std::uint16_t height = 1;
};

struct RenderView {
    std::uint32_t view_index = 0;
    RenderViewportRect rect;
    RenderCamera camera;
    std::string debug_name = "Viewport";
};

class FrameSnapshot final {
public:
    FrameSnapshot(
        std::uint64_t frame_index,
        double elapsed_seconds,
        ViewportExtent target_extent,
        std::vector<RenderView> views,
        std::vector<RenderObject> objects,
        std::vector<RenderLight> lights,
        RenderEnvironment environment,
        std::vector<OverlayCommand> overlays,
        std::vector<GridOverlayCommand> grid_overlay_payloads,
        std::vector<PickingRequest> picking_requests,
        ViewportSettings viewport_settings,
        DebugViewMode debug_view = DebugViewMode::FinalColor);

    [[nodiscard]] std::uint64_t frame_index() const noexcept;
    [[nodiscard]] double elapsed_seconds() const noexcept;
    [[nodiscard]] ViewportExtent target_extent() const noexcept;
    [[nodiscard]] std::span<const RenderView> views() const noexcept;
    [[nodiscard]] std::span<const RenderObject> objects() const noexcept;
    [[nodiscard]] std::span<const RenderLight> lights() const noexcept;
    [[nodiscard]] const RenderEnvironment& environment() const noexcept;
    [[nodiscard]] std::span<const OverlayCommand> overlays() const noexcept;
    [[nodiscard]] std::span<const GridOverlayCommand> grid_overlay_payloads() const noexcept;
    [[nodiscard]] std::span<const PickingRequest> picking_requests() const noexcept;
    [[nodiscard]] ViewportSettings viewport_settings() const noexcept;
    [[nodiscard]] DebugViewMode debug_view() const noexcept;

private:
    std::uint64_t frame_index_ = 0;
    double elapsed_seconds_ = 0.0;
    ViewportExtent target_extent_;
    std::vector<RenderView> views_;
    std::vector<RenderObject> objects_;
    std::vector<RenderLight> lights_;
    RenderEnvironment environment_;
    std::vector<OverlayCommand> overlays_;
    std::vector<GridOverlayCommand> grid_overlay_payloads_;
    std::vector<PickingRequest> picking_requests_;
    ViewportSettings viewport_settings_;
    DebugViewMode debug_view_ = DebugViewMode::FinalColor;
};

[[nodiscard]] bool is_valid_render_view(const RenderView& view) noexcept;

} // namespace crimson
