/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
#include "crimson/diagnostics/diagnostic_ring.hpp"
#include "crimson/diagnostics/renderer_diagnostics_snapshot.hpp"
#include "crimson/frame/frame_stats.hpp"
#include "crimson/renderer_diagnostics.hpp"

#include <gtest/gtest.h>

#include <iostream>
#include <string_view>
#include <vector>

namespace {

void expect_true(bool condition, std::string_view message) {
	EXPECT_TRUE(condition) << message;
}

TEST(FrameStatsDiagnostics, FrameStatsReportsQueuePickingAndResourceCounters) {
	const crimson::FrameStats kStats = crimson::make_frame_stats(crimson::FrameStatsInput{
			.frame_index = 42,
			.width_px = 1280,
			.height_px = 720,
			.view_count = 2,
			.graph_pass_count = 12,
			.visible_object_count = 3,
			.queues = crimson::FrameQueueStats{
					.draw_packet_count = 9,
					.opaque_draw_count = 2,
					.alpha_cutout_draw_count = 1,
					.transparent_draw_count = 1,
					.overlay_draw_count = 2,
			},
			.picking_request_count = 1,
			.picking_readback_count = 1,
			.resources = crimson::FrameResourceStats{
					.live_mesh_count = 4,
					.live_material_count = 5,
					.live_texture_count = 6,
					.live_program_count = 7,
					.frame_target_count = 8,
					.frame_target_recreate_count = 10,
			},
			.culling = crimson::FrameCullingStats{
					.input_object_count = 6,
					.visible_object_count = 3,
					.culled_object_count = 2,
					.invalid_bounds_count = 1,
			},
			.packets = crimson::FramePacketStats{
					.draw_packet_count = 7,
					.opaque_packet_count = 2,
					.alpha_cutout_packet_count = 1,
					.transparent_packet_count = 1,
					.overlay_packet_count = 3,
					.sorted_packet_count = 4,
			},
			.instancing = crimson::FrameInstancingStats{
					.batch_count = 3,
					.instanced_batch_count = 1,
					.single_draw_batch_count = 2,
					.submitted_instance_count = 5,
					.saved_draw_call_count = 2,
			},
			.uploads = crimson::FrameUploadStats{
					.mesh_create_count = 1,
					.mesh_update_count = 2,
					.texture_upload_count = 4,
					.skipped_clean_resource_count = 3,
					.uploaded_vertex_bytes = 144,
					.uploaded_index_bytes = 36,
					.uploaded_texture_bytes = 16,
			},
			.timings = crimson::FrameTimingStats{
					.cpu_frame_us = 100,
					.cpu_packet_build_us = 20,
					.cpu_resource_upload_us = 10,
					.cpu_submit_us = 30,
			},
			.diagnostic_count = 2,
			.fps = 59.5,
	});

	expect_true(kStats.frame_index == 42, "frame index is copied into stats");
	expect_true(kStats.width_px == 1280 && kStats.height_px == 720, "frame extent is reported");
	expect_true(kStats.view_count == 2, "view count is reported");
	expect_true(kStats.graph_pass_count == 12, "graph pass count is reported");
	expect_true(kStats.draw_packet_count == 9, "draw packet count is reported separately");
	expect_true(kStats.draw_call_count == 6, "visual draw calls sum PBR and overlay queues");
	expect_true(kStats.opaque_draw_count == 2, "opaque draw count is reported");
	expect_true(kStats.alpha_cutout_draw_count == 1, "alpha cutout draw count is reported");
	expect_true(kStats.transparent_draw_count == 1, "transparent draw count is reported");
	expect_true(kStats.overlay_draw_count == 2, "overlay draw count is reported");
	expect_true(kStats.picking_request_count == 1, "picking request count is reported");
	expect_true(kStats.picking_readback_count == 1, "picking readback count is reported");
	expect_true(kStats.visible_object_count == 3, "visible object count is reported");
	expect_true(kStats.live_mesh_count == 4, "live mesh count is reported");
	expect_true(kStats.live_material_count == 5, "live material count is reported");
	expect_true(kStats.live_texture_count == 6, "live texture count is reported");
	expect_true(kStats.live_program_count == 7, "live program count is reported");
	expect_true(kStats.frame_target_count == 8, "frame target count is reported");
	expect_true(kStats.frame_target_recreate_count == 10, "frame target recreation count is reported");
	expect_true(kStats.culling.input_object_count == 6, "culling input object count is reported");
	expect_true(kStats.culled_object_count == 2, "culled object count is flattened for status labels");
	expect_true(kStats.invalid_bounds_count == 1, "invalid bounds count is flattened for status labels");
	expect_true(kStats.packets.draw_packet_count == 7, "packet stats preserve renderer packet count");
	expect_true(kStats.packets.overlay_packet_count == 3, "overlay packet count is reported separately");
	expect_true(kStats.instancing.batch_count == 3, "instancing batch count is reported");
	expect_true(kStats.saved_draw_call_count == 2, "saved draw calls are flattened for status labels");
	expect_true(kStats.uploads.mesh_create_count == 1, "mesh upload create count is reported");
	expect_true(kStats.uploads.texture_upload_count == 4, "texture upload count is reported");
	expect_true(kStats.skipped_clean_resource_count == 3, "clean upload skip count is flattened for status labels");
	expect_true(kStats.uploaded_vertex_bytes == 144, "uploaded vertex bytes are flattened for status labels");
	expect_true(kStats.uploads.uploaded_texture_bytes == 16, "uploaded texture bytes are reported");
	expect_true(kStats.timings.cpu_packet_build_us == 20, "packet build timing is copied without thresholds");
	expect_true(kStats.diagnostic_count == 2, "diagnostic count is reported");
	expect_true(kStats.fps == 59.5, "fps is reported");
}

TEST(FrameStatsDiagnostics, RendererCountersAreTypedForDiagnosticsSnapshots) {
	const crimson::FrameStats kStats = crimson::make_frame_stats(crimson::FrameStatsInput{
			.frame_index = 7,
			.graph_pass_count = 12,
			.queues = crimson::FrameQueueStats{ .draw_packet_count = 2, .opaque_draw_count = 1 },
			.culling = crimson::FrameCullingStats{ .input_object_count = 2, .visible_object_count = 1 },
			.uploads = crimson::FrameUploadStats{ .texture_upload_count = 5, .uploaded_vertex_bytes = 96 },
			.timings = crimson::FrameTimingStats{ .cpu_frame_us = 5 },
	});
	const std::vector<crimson::RendererCounter> kCounters = crimson::make_renderer_counters(kStats);

	bool saw_frame_index = false;
	bool saw_upload_bytes = false;
	bool saw_texture_uploads = false;
	for (const crimson::RendererCounter &counter : kCounters) {
		if (counter.domain == crimson::RendererCounterDomain::Frame && counter.name == "frame_index") {
			saw_frame_index = counter.value == 7 && counter.unit == crimson::RendererMetricUnit::Frames;
		}
		if (counter.domain == crimson::RendererCounterDomain::Upload && counter.name == "uploaded_vertex_bytes") {
			saw_upload_bytes = counter.value == 96 && counter.unit == crimson::RendererMetricUnit::Bytes;
		}
		if (counter.domain == crimson::RendererCounterDomain::Upload && counter.name == "texture_upload_count") {
			saw_texture_uploads = counter.value == 5 && counter.unit == crimson::RendererMetricUnit::Count;
		}
	}

	expect_true(saw_frame_index, "typed counters expose frame index with frame units");
	expect_true(saw_upload_bytes, "typed counters expose upload bytes with byte units");
	expect_true(saw_texture_uploads, "typed counters expose live texture upload counts");
	expect_true(
			crimson::renderer_counter_domain_name(crimson::RendererCounterDomain::Instancing) == "Instancing",
			"counter domain names are stable");
	expect_true(
			crimson::renderer_metric_unit_name(crimson::RendererMetricUnit::Microseconds) == "Microseconds",
			"metric unit names are stable");
}

TEST(FrameStatsDiagnostics, DiagnosticRingCapsAndPreservesNewestEntries) {
	crimson::DiagnosticRing ring(3);
	for (std::uint64_t index = 0; index < 5; ++index) {
		ring.push(crimson::RendererDiagnostic{
				.severity = crimson::RendererDiagnosticSeverity::Warning,
				.code = crimson::RendererDiagnosticCode::PerformanceCounterInvalid,
				.message = "diagnostic",
				.frame_index = index,
		});
	}

	const std::vector<crimson::RendererDiagnostic> kDiagnostics = ring.snapshot();
	expect_true(kDiagnostics.size() == 3, "diagnostic ring caps entries");
	expect_true(kDiagnostics[0].frame_index == 2, "diagnostic ring keeps the oldest retained entry first");
	expect_true(kDiagnostics[2].frame_index == 4, "diagnostic ring keeps newest entry last");
}

TEST(FrameStatsDiagnostics, DiagnosticsStatusSnapshotUsesBoundedRecentEntries) {
	crimson::RendererStatus status;
	status.backend_name = "test";
	status.initialized = true;
	for (std::uint64_t index = 0; index < 5; ++index) {
		status.diagnostics.push_back(crimson::RendererDiagnostic{
				.severity = crimson::RendererDiagnosticSeverity::Warning,
				.code = crimson::RendererDiagnosticCode::PerformanceCounterInvalid,
				.message = "diagnostic",
				.frame_index = index,
		});
	}

	const std::vector<crimson::RendererDiagnostic> kRecent =
			crimson::make_diagnostic_ring_from_recent(status.diagnostics, 2).snapshot();
	const crimson::RendererStatus kSnapshot =
			crimson::make_bounded_renderer_status_snapshot(status, kRecent);

	expect_true(kSnapshot.backend_name == "test", "bounded status keeps backend identity");
	expect_true(kSnapshot.initialized, "bounded status keeps initialization state");
	expect_true(kSnapshot.diagnostics.size() == 2, "bounded status avoids copying unbounded diagnostics");
	expect_true(kSnapshot.diagnostics[0].frame_index == 3, "bounded status keeps oldest retained diagnostic first");
	expect_true(kSnapshot.diagnostics[1].frame_index == 4, "bounded status keeps newest diagnostic last");
}

TEST(FrameStatsDiagnostics, DiagnosticContextDistinguishesStructuredRendererErrors) {
	const crimson::RendererDiagnostic kMissingContext{
		.severity = crimson::RendererDiagnosticSeverity::Error,
		.code = crimson::RendererDiagnosticCode::ResourceLifetimeError,
		.message = "missing context",
	};
	expect_true(!crimson::has_structured_context(kMissingContext), "diagnostic without context is detected");

	const crimson::RendererDiagnostic kWithContext{
		.severity = crimson::RendererDiagnosticSeverity::Error,
		.code = crimson::RendererDiagnosticCode::ResourceLifetimeError,
		.message = "stale handle",
		.subsystem = "gpu.material",
		.resource_name = "MaterialSystem",
		.frame_index = 7,
	};
	expect_true(crimson::has_structured_context(kWithContext), "diagnostic with subsystem and resource is structured");
	expect_true(kWithContext.frame_index == 7, "diagnostic frame index is available for frame-scoped errors");
}

TEST(FrameStatsDiagnostics, DiagnosticNamesRemainStableForA43Codes) {
	expect_true(
			crimson::renderer_diagnostic_code_name(crimson::RendererDiagnosticCode::PickingReadbackFailed) == "PickingReadbackFailed",
			"picking diagnostic code name is stable");
	expect_true(
			crimson::renderer_diagnostic_code_name(crimson::RendererDiagnosticCode::FrameBufferUnsupported) == "FrameBufferUnsupported",
			"framebuffer diagnostic code name is stable");
	expect_true(
			crimson::renderer_diagnostic_code_name(crimson::RendererDiagnosticCode::ResourceLifetimeError) == "ResourceLifetimeError",
			"resource lifetime diagnostic code name is stable");
	expect_true(
			crimson::renderer_diagnostic_code_name(crimson::RendererDiagnosticCode::PerformanceCounterInvalid) == "PerformanceCounterInvalid",
			"performance counter diagnostic code name is stable");
	expect_true(
			crimson::renderer_diagnostic_code_name(crimson::RendererDiagnosticCode::UploadSkippedInvalidResource) == "UploadSkippedInvalidResource",
			"upload diagnostic code name is stable");
	expect_true(
			crimson::renderer_diagnostic_code_name(crimson::RendererDiagnosticCode::BenchmarkConfigurationInvalid) == "BenchmarkConfigurationInvalid",
			"benchmark diagnostic code name is stable");
	expect_true(
			crimson::renderer_capability_name(crimson::RendererCapability::ManualSrgbFinalConversion) == "ManualSrgbFinalConversion",
			"manual sRGB fallback capability name is stable");
}

} // namespace
