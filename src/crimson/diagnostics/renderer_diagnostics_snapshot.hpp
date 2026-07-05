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

enum class RendererMetricUnit : std::uint8_t {
    Count,
    Bytes,
    Microseconds,
    Frames,
};

enum class RendererCounterDomain : std::uint8_t {
    Frame,
    Graph,
    Culling,
    Packets,
    Instancing,
    Upload,
    Resources,
    Picking,
    Overlay,
    Diagnostics,
};

struct RendererCounter {
    RendererCounterDomain domain = RendererCounterDomain::Frame;
    std::string name;
    std::uint64_t value = 0;
    RendererMetricUnit unit = RendererMetricUnit::Count;
};

struct RenderPassStats {
    std::string pass_name;
    std::uint32_t resource_read_count = 0;
    std::uint32_t resource_write_count = 0;
    std::uint32_t draw_call_count = 0;
    std::uint32_t draw_packet_count = 0;
    std::uint64_t cpu_time_us = 0;
};

struct RendererDiagnosticsSnapshot {
    RendererStatus status;
    std::optional<FrameStats> latest_frame_stats;
    std::vector<RenderPassStats> pass_stats;
    std::vector<RendererCounter> counters;
    std::vector<RendererDiagnostic> recent_diagnostics;
    std::string frame_graph_dump;
};

[[nodiscard]] constexpr std::string_view renderer_metric_unit_name(RendererMetricUnit unit) noexcept
{
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

[[nodiscard]] constexpr std::string_view renderer_counter_domain_name(RendererCounterDomain domain) noexcept
{
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

[[nodiscard]] std::vector<RenderPassStats> make_render_pass_stats(
    const RenderGraph& graph,
    const FrameStats* stats = nullptr);
[[nodiscard]] std::vector<RendererCounter> make_renderer_counters(const FrameStats& stats);
[[nodiscard]] RendererStatus make_bounded_renderer_status_snapshot(
    const RendererStatus& status,
    std::vector<RendererDiagnostic> recent_diagnostics);

} // namespace crimson
