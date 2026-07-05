#include "crimson/frame/frame_snapshot.hpp"

#include <utility>

namespace crimson {

FrameSnapshot::FrameSnapshot(
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
		DebugViewMode debug_view) : frame_index_(frame_index),
									elapsed_seconds_(elapsed_seconds),
									target_extent_(target_extent),
									views_(std::move(views)),
									objects_(std::move(objects)),
									lights_(std::move(lights)),
									environment_(environment),
									overlays_(std::move(overlays)),
									grid_overlay_payloads_(std::move(grid_overlay_payloads)),
									picking_requests_(std::move(picking_requests)),
									viewport_settings_(viewport_settings),
									debug_view_(debug_view) {
}

std::uint64_t FrameSnapshot::frame_index() const noexcept {
	return frame_index_;
}

double FrameSnapshot::elapsed_seconds() const noexcept {
	return elapsed_seconds_;
}

ViewportExtent FrameSnapshot::target_extent() const noexcept {
	return target_extent_;
}

std::span<const RenderView> FrameSnapshot::views() const noexcept {
	return views_;
}

std::span<const RenderObject> FrameSnapshot::objects() const noexcept {
	return objects_;
}

std::span<const RenderLight> FrameSnapshot::lights() const noexcept {
	return lights_;
}

const RenderEnvironment &FrameSnapshot::environment() const noexcept {
	return environment_;
}

std::span<const OverlayCommand> FrameSnapshot::overlays() const noexcept {
	return overlays_;
}

std::span<const GridOverlayCommand> FrameSnapshot::grid_overlay_payloads() const noexcept {
	return grid_overlay_payloads_;
}

std::span<const PickingRequest> FrameSnapshot::picking_requests() const noexcept {
	return picking_requests_;
}

ViewportSettings FrameSnapshot::viewport_settings() const noexcept {
	return viewport_settings_;
}

DebugViewMode FrameSnapshot::debug_view() const noexcept {
	return debug_view_;
}

bool is_valid_render_view(const RenderView &view) noexcept {
	return view.rect.width > 0 && view.rect.height > 0;
}

} // namespace crimson
