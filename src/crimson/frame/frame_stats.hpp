#pragma once

#include <cstdint>

namespace crimson {

struct FrameCullingStats {
    std::uint32_t input_object_count = 0;
    std::uint32_t visible_object_count = 0;
    std::uint32_t culled_object_count = 0;
    std::uint32_t invalid_bounds_count = 0;
};

struct FramePacketStats {
    std::uint32_t draw_packet_count = 0;
    std::uint32_t opaque_packet_count = 0;
    std::uint32_t alpha_cutout_packet_count = 0;
    std::uint32_t transparent_packet_count = 0;
    std::uint32_t overlay_packet_count = 0;
    std::uint32_t sorted_packet_count = 0;
};

struct FrameInstancingStats {
    std::uint32_t batch_count = 0;
    std::uint32_t instanced_batch_count = 0;
    std::uint32_t single_draw_batch_count = 0;
    std::uint32_t submitted_instance_count = 0;
    std::uint32_t saved_draw_call_count = 0;
};

struct FrameUploadStats {
    std::uint32_t mesh_create_count = 0;
    std::uint32_t mesh_update_count = 0;
    std::uint32_t mesh_destroy_count = 0;
    std::uint32_t material_upload_count = 0;
    std::uint32_t texture_upload_count = 0;
    std::uint32_t skipped_clean_resource_count = 0;
    std::uint64_t uploaded_vertex_bytes = 0;
    std::uint64_t uploaded_index_bytes = 0;
    std::uint64_t uploaded_texture_bytes = 0;
};

struct FrameTimingStats {
    std::uint64_t cpu_frame_us = 0;
    std::uint64_t cpu_snapshot_validation_us = 0;
    std::uint64_t cpu_packet_build_us = 0;
    std::uint64_t cpu_resource_upload_us = 0;
    std::uint64_t cpu_submit_us = 0;
};

struct FrameStats {
    std::uint64_t frame_index = 0;
    std::uint32_t width_px = 0;
    std::uint32_t height_px = 0;
    std::uint32_t view_count = 0;
    std::uint32_t graph_pass_count = 0;
    std::uint32_t draw_call_count = 0;
    std::uint32_t draw_packet_count = 0;
    std::uint32_t opaque_draw_count = 0;
    std::uint32_t alpha_cutout_draw_count = 0;
    std::uint32_t transparent_draw_count = 0;
    std::uint32_t overlay_draw_count = 0;
    std::uint32_t picking_request_count = 0;
    std::uint32_t picking_readback_count = 0;
    std::uint32_t visible_object_count = 0;
    std::uint32_t live_mesh_count = 0;
    std::uint32_t live_material_count = 0;
    std::uint32_t live_texture_count = 0;
    std::uint32_t live_program_count = 0;
    std::uint32_t frame_target_count = 0;
    std::uint32_t frame_target_recreate_count = 0;
    std::uint32_t diagnostic_count = 0;
    std::uint32_t culled_object_count = 0;
    std::uint32_t invalid_bounds_count = 0;
    std::uint32_t instancing_batch_count = 0;
    std::uint32_t saved_draw_call_count = 0;
    std::uint32_t skipped_clean_resource_count = 0;
    std::uint64_t uploaded_vertex_bytes = 0;
    std::uint64_t uploaded_index_bytes = 0;
    FrameCullingStats culling;
    FramePacketStats packets;
    FrameInstancingStats instancing;
    FrameUploadStats uploads;
    FrameTimingStats timings;
    double fps = 0.0;
};

struct FrameQueueStats {
    std::uint32_t draw_packet_count = 0;
    std::uint32_t opaque_draw_count = 0;
    std::uint32_t alpha_cutout_draw_count = 0;
    std::uint32_t transparent_draw_count = 0;
    std::uint32_t overlay_draw_count = 0;
};

struct FrameResourceStats {
    std::uint32_t live_mesh_count = 0;
    std::uint32_t live_material_count = 0;
    std::uint32_t live_texture_count = 0;
    std::uint32_t live_program_count = 0;
    std::uint32_t frame_target_count = 0;
    std::uint32_t frame_target_recreate_count = 0;
};

struct FrameStatsInput {
    std::uint64_t frame_index = 0;
    std::uint32_t width_px = 0;
    std::uint32_t height_px = 0;
    std::uint32_t view_count = 0;
    std::uint32_t graph_pass_count = 0;
    std::uint32_t visible_object_count = 0;
    FrameQueueStats queues;
    std::uint32_t picking_request_count = 0;
    std::uint32_t picking_readback_count = 0;
    FrameResourceStats resources;
    FrameCullingStats culling;
    FramePacketStats packets;
    FrameInstancingStats instancing;
    FrameUploadStats uploads;
    FrameTimingStats timings;
    std::uint32_t diagnostic_count = 0;
    double fps = 0.0;
};

[[nodiscard]] FrameStats make_frame_stats(const FrameStatsInput& input) noexcept;

} // namespace crimson
