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

#include "crimson/frame/frame_stats.hpp"
#include "crimson/graph/render_graph.hpp"
#include "crimson/renderer_diagnostics.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace crimson {

/// Unit used to display a renderer counter.
enum class RendererMetricUnit : std::uint8_t {
	/// Unitless count.
	Count,
	/// Byte count.
	Bytes,
	/// Microsecond duration.
	Microseconds,
	/// Frame count.
	Frames,
};

/// Diagnostics domain associated with a renderer counter.
enum class RendererCounterDomain : std::uint8_t {
	/// Frame-level counters.
	Frame,
	/// Render graph counters.
	Graph,
	/// CPU culling counters.
	Culling,
	/// Draw-packet counters.
	Packets,
	/// Instancing counters.
	Instancing,
	/// Upload counters.
	Upload,
	/// Runtime resource counters.
	Resources,
	/// Picking counters.
	Picking,
	/// Overlay counters.
	Overlay,
	/// Diagnostic counters.
	Diagnostics,
};

/// One named renderer counter.
struct RendererCounter {
	/// Counter domain.
	RendererCounterDomain domain = RendererCounterDomain::Frame;
	/// Stable counter name.
	std::string name;
	/// Counter value.
	std::uint64_t value = 0;
	/// Counter display unit.
	RendererMetricUnit unit = RendererMetricUnit::Count;
};

/// Per-pass renderer statistics exposed to diagnostics UI/tests.
struct RenderPassStats {
	/// Render graph pass name.
	std::string pass_name;
	/// Number of read resources declared by the pass.
	std::uint32_t resource_read_count = 0;
	/// Number of write resources declared by the pass.
	std::uint32_t resource_write_count = 0;
	/// Draw calls submitted by the pass.
	std::uint32_t draw_call_count = 0;
	/// Draw packets consumed by the pass.
	std::uint32_t draw_packet_count = 0;
	/// CPU time for the pass in microseconds.
	std::uint64_t cpu_time_us = 0;
};

/// Value-copy diagnostics snapshot for UI and tests.
struct RendererDiagnosticsSnapshot {
	/// Renderer status snapshot.
	RendererStatus status;
	/// Latest frame stats when a frame has completed.
	std::optional<FrameStats> latest_frame_stats;
	/// Per-pass stats for the latest frame graph.
	std::vector<RenderPassStats> pass_stats;
	/// Flattened renderer counters.
	std::vector<RendererCounter> counters;
	/// Recent diagnostics in oldest-to-newest order.
	std::vector<RendererDiagnostic> recent_diagnostics;
	/// Human-readable frame graph dump.
	std::string frame_graph_dump;
};

/**
 * Return the stable metric-unit name.
 *
 * @param unit Metric unit to name.
 * @return Static unit name.
 */
[[nodiscard]] constexpr std::string_view renderer_metric_unit_name(RendererMetricUnit unit) noexcept {
	switch (unit) {
		case RendererMetricUnit::Count:
			return "Count";
		case RendererMetricUnit::Bytes:
			return "Bytes";
		case RendererMetricUnit::Microseconds:
			return "Microseconds";
		case RendererMetricUnit::Frames:
			return "Frames";
	}

	return "Unknown";
}

/**
 * Return the stable counter-domain name.
 *
 * @param domain Counter domain to name.
 * @return Static domain name.
 */
[[nodiscard]] constexpr std::string_view renderer_counter_domain_name(RendererCounterDomain domain) noexcept {
	switch (domain) {
		case RendererCounterDomain::Frame:
			return "Frame";
		case RendererCounterDomain::Graph:
			return "Graph";
		case RendererCounterDomain::Culling:
			return "Culling";
		case RendererCounterDomain::Packets:
			return "Packets";
		case RendererCounterDomain::Instancing:
			return "Instancing";
		case RendererCounterDomain::Upload:
			return "Upload";
		case RendererCounterDomain::Resources:
			return "Resources";
		case RendererCounterDomain::Picking:
			return "Picking";
		case RendererCounterDomain::Overlay:
			return "Overlay";
		case RendererCounterDomain::Diagnostics:
			return "Diagnostics";
	}

	return "Unknown";
}

/**
 * Build per-pass diagnostics rows from a render graph.
 *
 * @param graph Graph whose passes are reported.
 * @param stats Optional latest frame stats used to attach pass counters.
 * @return Per-pass stats rows.
 */
[[nodiscard]] std::vector<RenderPassStats> make_render_pass_stats(
		const RenderGraph &graph,
		const FrameStats *stats = nullptr);
/**
 * Flatten frame stats into named counters.
 *
 * @param stats Frame stats to convert.
 * @return Counter rows grouped by domain.
 */
[[nodiscard]] std::vector<RendererCounter> make_renderer_counters(const FrameStats &stats);
/**
 * Build a status snapshot with externally bounded recent diagnostics.
 *
 * @param status Source renderer status.
 * @param recent_diagnostics Bounded diagnostics to attach to the copied status.
 * @return Status copy using `recent_diagnostics`.
 */
[[nodiscard]] RendererStatus make_bounded_renderer_status_snapshot(
		const RendererStatus &status,
		std::vector<RendererDiagnostic> recent_diagnostics);

} // namespace crimson
