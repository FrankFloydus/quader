/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
#pragma once

#include <cstdint>

namespace crimson {

/// CPU culling counters for one frame.
struct FrameCullingStats {
	/// Objects considered by culling.
	std::uint32_t input_object_count = 0;
	/// Objects accepted for rendering.
	std::uint32_t visible_object_count = 0;
	/// Objects rejected by culling.
	std::uint32_t culled_object_count = 0;
	/// Objects skipped because their bounds were invalid.
	std::uint32_t invalid_bounds_count = 0;
};

/// Draw-packet counters for one frame.
struct FramePacketStats {
	/// Total draw packets produced.
	std::uint32_t draw_packet_count = 0;
	/// Opaque queue packet count.
	std::uint32_t opaque_packet_count = 0;
	/// Alpha-cutout queue packet count.
	std::uint32_t alpha_cutout_packet_count = 0;
	/// Transparent queue packet count.
	std::uint32_t transparent_packet_count = 0;
	/// Overlay queue packet count.
	std::uint32_t overlay_packet_count = 0;
	/// Packets that entered sorting.
	std::uint32_t sorted_packet_count = 0;
};

/// Instancing and batching counters for one frame.
struct FrameInstancingStats {
	/// Total instance batches emitted.
	std::uint32_t batch_count = 0;
	/// Batches submitted with more than one instance.
	std::uint32_t instanced_batch_count = 0;
	/// Batches submitted as one draw per object.
	std::uint32_t single_draw_batch_count = 0;
	/// Total submitted instances.
	std::uint32_t submitted_instance_count = 0;
	/// Draw calls avoided by instancing.
	std::uint32_t saved_draw_call_count = 0;
};

/// Resource upload counters for one frame.
struct FrameUploadStats {
	/// Mesh resources created.
	std::uint32_t mesh_create_count = 0;
	/// Mesh resources updated.
	std::uint32_t mesh_update_count = 0;
	/// Mesh resources destroyed.
	std::uint32_t mesh_destroy_count = 0;
	/// Material uploads performed.
	std::uint32_t material_upload_count = 0;
	/// Texture uploads performed.
	std::uint32_t texture_upload_count = 0;
	/// Resources skipped because cached data was clean.
	std::uint32_t skipped_clean_resource_count = 0;
	/// Vertex bytes uploaded.
	std::uint64_t uploaded_vertex_bytes = 0;
	/// Index bytes uploaded.
	std::uint64_t uploaded_index_bytes = 0;
	/// Texture bytes uploaded.
	std::uint64_t uploaded_texture_bytes = 0;
};

/// CPU timing counters in microseconds.
struct FrameTimingStats {
	/// Total CPU frame time.
	std::uint64_t cpu_frame_us = 0;
	/// CPU time spent validating snapshots.
	std::uint64_t cpu_snapshot_validation_us = 0;
	/// CPU time spent building draw packets.
	std::uint64_t cpu_packet_build_us = 0;
	/// CPU time spent uploading resources.
	std::uint64_t cpu_resource_upload_us = 0;
	/// CPU time spent submitting work.
	std::uint64_t cpu_submit_us = 0;
};

/// Aggregated diagnostics and performance counters for one frame.
struct FrameStats {
	/// Monotonic frame index.
	std::uint64_t frame_index = 0;
	/// Frame target width in pixels.
	std::uint32_t width_px = 0;
	/// Frame target height in pixels.
	std::uint32_t height_px = 0;
	/// Number of render views.
	std::uint32_t view_count = 0;
	/// Number of render graph passes.
	std::uint32_t graph_pass_count = 0;
	/// Submitted draw calls.
	std::uint32_t draw_call_count = 0;
	/// Total draw packets.
	std::uint32_t draw_packet_count = 0;
	/// Opaque draw count.
	std::uint32_t opaque_draw_count = 0;
	/// Alpha-cutout draw count.
	std::uint32_t alpha_cutout_draw_count = 0;
	/// Transparent draw count.
	std::uint32_t transparent_draw_count = 0;
	/// Overlay draw count.
	std::uint32_t overlay_draw_count = 0;
	/// Picking requests received.
	std::uint32_t picking_request_count = 0;
	/// Picking readbacks scheduled.
	std::uint32_t picking_readback_count = 0;
	/// Visible objects after culling.
	std::uint32_t visible_object_count = 0;
	/// Live GPU mesh count.
	std::uint32_t live_mesh_count = 0;
	/// Live GPU material count.
	std::uint32_t live_material_count = 0;
	/// Live GPU texture count.
	std::uint32_t live_texture_count = 0;
	/// Live GPU program count.
	std::uint32_t live_program_count = 0;
	/// Frame target count.
	std::uint32_t frame_target_count = 0;
	/// Frame target recreations.
	std::uint32_t frame_target_recreate_count = 0;
	/// Renderer diagnostic count.
	std::uint32_t diagnostic_count = 0;
	/// Objects culled this frame.
	std::uint32_t culled_object_count = 0;
	/// Objects skipped for invalid bounds.
	std::uint32_t invalid_bounds_count = 0;
	/// Instance batch count.
	std::uint32_t instancing_batch_count = 0;
	/// Draw calls avoided by instancing.
	std::uint32_t saved_draw_call_count = 0;
	/// Clean resources skipped during upload.
	std::uint32_t skipped_clean_resource_count = 0;
	/// Vertex bytes uploaded.
	std::uint64_t uploaded_vertex_bytes = 0;
	/// Index bytes uploaded.
	std::uint64_t uploaded_index_bytes = 0;
	/// Detailed culling counters.
	FrameCullingStats culling;
	/// Detailed packet counters.
	FramePacketStats packets;
	/// Detailed instancing counters.
	FrameInstancingStats instancing;
	/// Detailed upload counters.
	FrameUploadStats uploads;
	/// Detailed CPU timings.
	FrameTimingStats timings;
	/// Frames per second estimate.
	double fps = 0.0;
};

/// Queue-level draw counts used to assemble `FrameStats`.
struct FrameQueueStats {
	/// Total draw packets.
	std::uint32_t draw_packet_count = 0;
	/// Opaque draw count.
	std::uint32_t opaque_draw_count = 0;
	/// Alpha-cutout draw count.
	std::uint32_t alpha_cutout_draw_count = 0;
	/// Transparent draw count.
	std::uint32_t transparent_draw_count = 0;
	/// Overlay draw count.
	std::uint32_t overlay_draw_count = 0;
};

/// Live resource counts used to assemble `FrameStats`.
struct FrameResourceStats {
	/// Live GPU mesh count.
	std::uint32_t live_mesh_count = 0;
	/// Live GPU material count.
	std::uint32_t live_material_count = 0;
	/// Live GPU texture count.
	std::uint32_t live_texture_count = 0;
	/// Live GPU program count.
	std::uint32_t live_program_count = 0;
	/// Frame target count.
	std::uint32_t frame_target_count = 0;
	/// Frame target recreation count.
	std::uint32_t frame_target_recreate_count = 0;
};

/// Input bundle used to construct an aggregated frame stats record.
struct FrameStatsInput {
	/// Monotonic frame index.
	std::uint64_t frame_index = 0;
	/// Frame target width in pixels.
	std::uint32_t width_px = 0;
	/// Frame target height in pixels.
	std::uint32_t height_px = 0;
	/// Number of render views.
	std::uint32_t view_count = 0;
	/// Number of render graph passes.
	std::uint32_t graph_pass_count = 0;
	/// Visible object count.
	std::uint32_t visible_object_count = 0;
	/// Queue draw counts.
	FrameQueueStats queues;
	/// Picking requests received.
	std::uint32_t picking_request_count = 0;
	/// Picking readbacks scheduled.
	std::uint32_t picking_readback_count = 0;
	/// Live resource counts.
	FrameResourceStats resources;
	/// Culling counters.
	FrameCullingStats culling;
	/// Packet counters.
	FramePacketStats packets;
	/// Instancing counters.
	FrameInstancingStats instancing;
	/// Upload counters.
	FrameUploadStats uploads;
	/// CPU timing counters.
	FrameTimingStats timings;
	/// Renderer diagnostic count.
	std::uint32_t diagnostic_count = 0;
	/// Frames per second estimate.
	double fps = 0.0;
};

/**
 * Combine per-domain frame counters into one public stats record.
 *
 * @param input Source counters collected during frame submission.
 * @return Aggregated frame stats.
 */
[[nodiscard]] FrameStats make_frame_stats(const FrameStatsInput &input) noexcept;

} // namespace crimson
