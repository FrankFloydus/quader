#include "crimson/diagnostics/renderer_diagnostics_snapshot.hpp"

#include <utility>

namespace crimson {
namespace {

[[nodiscard]] bool access_reads(RenderResourceAccess access) noexcept {
	return access == RenderResourceAccess::Read || access == RenderResourceAccess::ReadWrite;
}

[[nodiscard]] bool access_writes(RenderResourceAccess access) noexcept {
	return access == RenderResourceAccess::Write || access == RenderResourceAccess::ReadWrite;
}

void add_counter(
		std::vector<RendererCounter> &counters,
		RendererCounterDomain domain,
		std::string name,
		std::uint64_t value,
		RendererMetricUnit unit = RendererMetricUnit::Count) {
	counters.push_back(RendererCounter{
			.domain = domain,
			.name = std::move(name),
			.value = value,
			.unit = unit,
	});
}

} // namespace

std::vector<RenderPassStats> make_render_pass_stats(const RenderGraph &graph, const FrameStats *stats) {
	std::vector<RenderPassStats> pass_stats;
	pass_stats.reserve(graph.passes().size());

	for (const RenderPass &pass : graph.passes()) {
		RenderPassStats current;
		current.pass_name = pass.name;
		for (const ResourceUse &use : pass.resources) {
			if (access_reads(use.access)) {
				++current.resource_read_count;
			}
			if (access_writes(use.access)) {
				++current.resource_write_count;
			}
		}

		if (stats != nullptr) {
			if (pass.name == "OpaquePbrPass") {
				current.draw_call_count = stats->opaque_draw_count;
				current.draw_packet_count = stats->packets.opaque_packet_count;
			} else if (pass.name == "AlphaCutoutPbrPass") {
				current.draw_call_count = stats->alpha_cutout_draw_count;
				current.draw_packet_count = stats->packets.alpha_cutout_packet_count;
			} else if (pass.name == "TransparentPbrPass") {
				current.draw_call_count = stats->transparent_draw_count;
				current.draw_packet_count = stats->packets.transparent_packet_count;
			} else if (pass.name == "ToneMapPass" || pass.name == "PresentPass") {
				current.draw_call_count = 1;
				current.draw_packet_count = 1;
			} else if (pass.name.rfind("Overlay", 0) == 0) {
				current.draw_call_count = stats->overlay_draw_count;
				current.draw_packet_count = stats->packets.overlay_packet_count;
			} else if (pass.name == "ResourceUploadPass") {
				current.cpu_time_us = stats->timings.cpu_resource_upload_us;
			}
		}

		pass_stats.push_back(std::move(current));
	}

	return pass_stats;
}

std::vector<RendererCounter> make_renderer_counters(const FrameStats &stats) {
	std::vector<RendererCounter> counters;
	counters.reserve(40);

	add_counter(counters, RendererCounterDomain::Frame, "frame_index", stats.frame_index, RendererMetricUnit::Frames);
	add_counter(counters, RendererCounterDomain::Frame, "view_count", stats.view_count);
	add_counter(counters, RendererCounterDomain::Graph, "graph_pass_count", stats.graph_pass_count);
	add_counter(counters, RendererCounterDomain::Packets, "draw_packet_count", stats.draw_packet_count);
	add_counter(counters, RendererCounterDomain::Packets, "draw_call_count", stats.draw_call_count);
	add_counter(counters, RendererCounterDomain::Packets, "opaque_packet_count", stats.packets.opaque_packet_count);
	add_counter(counters, RendererCounterDomain::Packets, "alpha_cutout_packet_count", stats.packets.alpha_cutout_packet_count);
	add_counter(counters, RendererCounterDomain::Packets, "transparent_packet_count", stats.packets.transparent_packet_count);
	add_counter(counters, RendererCounterDomain::Packets, "sorted_packet_count", stats.packets.sorted_packet_count);
	add_counter(counters, RendererCounterDomain::Culling, "input_object_count", stats.culling.input_object_count);
	add_counter(counters, RendererCounterDomain::Culling, "visible_object_count", stats.culling.visible_object_count);
	add_counter(counters, RendererCounterDomain::Culling, "culled_object_count", stats.culling.culled_object_count);
	add_counter(counters, RendererCounterDomain::Culling, "invalid_bounds_count", stats.culling.invalid_bounds_count);
	add_counter(counters, RendererCounterDomain::Instancing, "batch_count", stats.instancing.batch_count);
	add_counter(counters, RendererCounterDomain::Instancing, "instanced_batch_count", stats.instancing.instanced_batch_count);
	add_counter(counters, RendererCounterDomain::Instancing, "single_draw_batch_count", stats.instancing.single_draw_batch_count);
	add_counter(counters, RendererCounterDomain::Instancing, "submitted_instance_count", stats.instancing.submitted_instance_count);
	add_counter(counters, RendererCounterDomain::Instancing, "saved_draw_call_count", stats.instancing.saved_draw_call_count);
	add_counter(counters, RendererCounterDomain::Upload, "mesh_create_count", stats.uploads.mesh_create_count);
	add_counter(counters, RendererCounterDomain::Upload, "mesh_update_count", stats.uploads.mesh_update_count);
	add_counter(counters, RendererCounterDomain::Upload, "mesh_destroy_count", stats.uploads.mesh_destroy_count);
	add_counter(counters, RendererCounterDomain::Upload, "material_upload_count", stats.uploads.material_upload_count);
	add_counter(counters, RendererCounterDomain::Upload, "texture_upload_count", stats.uploads.texture_upload_count);
	add_counter(counters, RendererCounterDomain::Upload, "skipped_clean_resource_count", stats.uploads.skipped_clean_resource_count);
	add_counter(counters, RendererCounterDomain::Upload, "uploaded_vertex_bytes", stats.uploads.uploaded_vertex_bytes, RendererMetricUnit::Bytes);
	add_counter(counters, RendererCounterDomain::Upload, "uploaded_index_bytes", stats.uploads.uploaded_index_bytes, RendererMetricUnit::Bytes);
	add_counter(counters, RendererCounterDomain::Upload, "uploaded_texture_bytes", stats.uploads.uploaded_texture_bytes, RendererMetricUnit::Bytes);
	add_counter(counters, RendererCounterDomain::Resources, "live_mesh_count", stats.live_mesh_count);
	add_counter(counters, RendererCounterDomain::Resources, "live_material_count", stats.live_material_count);
	add_counter(counters, RendererCounterDomain::Resources, "live_texture_count", stats.live_texture_count);
	add_counter(counters, RendererCounterDomain::Resources, "live_program_count", stats.live_program_count);
	add_counter(counters, RendererCounterDomain::Picking, "picking_request_count", stats.picking_request_count);
	add_counter(counters, RendererCounterDomain::Picking, "picking_readback_count", stats.picking_readback_count);
	add_counter(counters, RendererCounterDomain::Overlay, "overlay_packet_count", stats.packets.overlay_packet_count);
	add_counter(counters, RendererCounterDomain::Overlay, "overlay_draw_count", stats.overlay_draw_count);
	add_counter(counters, RendererCounterDomain::Diagnostics, "diagnostic_count", stats.diagnostic_count);
	add_counter(counters, RendererCounterDomain::Frame, "cpu_frame_us", stats.timings.cpu_frame_us, RendererMetricUnit::Microseconds);
	add_counter(counters, RendererCounterDomain::Packets, "cpu_packet_build_us", stats.timings.cpu_packet_build_us, RendererMetricUnit::Microseconds);
	add_counter(counters, RendererCounterDomain::Upload, "cpu_resource_upload_us", stats.timings.cpu_resource_upload_us, RendererMetricUnit::Microseconds);
	add_counter(counters, RendererCounterDomain::Frame, "cpu_submit_us", stats.timings.cpu_submit_us, RendererMetricUnit::Microseconds);

	return counters;
}

RendererStatus make_bounded_renderer_status_snapshot(
		const RendererStatus &status,
		std::vector<RendererDiagnostic> recent_diagnostics) {
	return RendererStatus{
		.backend_name = status.backend_name,
		.initialized = status.initialized,
		.capabilities = status.capabilities,
		.diagnostics = std::move(recent_diagnostics),
	};
}

} // namespace crimson
