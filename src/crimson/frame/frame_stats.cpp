#include "crimson/frame/frame_stats.hpp"

#include <limits>

namespace crimson {
namespace {

[[nodiscard]] std::uint32_t saturating_add(std::uint32_t left, std::uint32_t right) noexcept
{
    const std::uint32_t max = std::numeric_limits<std::uint32_t>::max();
    if (max - left < right) {
        return max;
    }
    return left + right;
}

} // namespace

FrameStats make_frame_stats(const FrameStatsInput& input) noexcept
{
    const FramePacketStats packets = input.packets.draw_packet_count == 0
        ? FramePacketStats{
            .draw_packet_count = input.queues.draw_packet_count,
            .opaque_packet_count = input.queues.opaque_draw_count,
            .alpha_cutout_packet_count = input.queues.alpha_cutout_draw_count,
            .transparent_packet_count = input.queues.transparent_draw_count,
            .overlay_packet_count = input.queues.overlay_draw_count,
            .sorted_packet_count = input.queues.draw_packet_count,
        }
        : input.packets;
    const std::uint32_t visual_draws = saturating_add(
        saturating_add(input.queues.opaque_draw_count, input.queues.alpha_cutout_draw_count),
        saturating_add(input.queues.transparent_draw_count, input.queues.overlay_draw_count));

    return FrameStats{
        .frame_index = input.frame_index,
        .width_px = input.width_px,
        .height_px = input.height_px,
        .view_count = input.view_count,
        .graph_pass_count = input.graph_pass_count,
        .draw_call_count = visual_draws,
        .draw_packet_count = input.queues.draw_packet_count,
        .opaque_draw_count = input.queues.opaque_draw_count,
        .alpha_cutout_draw_count = input.queues.alpha_cutout_draw_count,
        .transparent_draw_count = input.queues.transparent_draw_count,
        .overlay_draw_count = input.queues.overlay_draw_count,
        .picking_request_count = input.picking_request_count,
        .picking_readback_count = input.picking_readback_count,
        .visible_object_count = input.visible_object_count,
        .live_mesh_count = input.resources.live_mesh_count,
        .live_material_count = input.resources.live_material_count,
        .live_texture_count = input.resources.live_texture_count,
        .live_program_count = input.resources.live_program_count,
        .frame_target_count = input.resources.frame_target_count,
        .frame_target_recreate_count = input.resources.frame_target_recreate_count,
        .diagnostic_count = input.diagnostic_count,
        .culled_object_count = input.culling.culled_object_count,
        .invalid_bounds_count = input.culling.invalid_bounds_count,
        .instancing_batch_count = input.instancing.batch_count,
        .saved_draw_call_count = input.instancing.saved_draw_call_count,
        .skipped_clean_resource_count = input.uploads.skipped_clean_resource_count,
        .uploaded_vertex_bytes = input.uploads.uploaded_vertex_bytes,
        .uploaded_index_bytes = input.uploads.uploaded_index_bytes,
        .culling = input.culling,
        .packets = packets,
        .instancing = input.instancing,
        .uploads = input.uploads,
        .timings = input.timings,
        .fps = input.fps,
    };
}

} // namespace crimson
