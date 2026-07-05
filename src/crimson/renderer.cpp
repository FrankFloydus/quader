#include "crimson/renderer.hpp"

#include "crimson/diagnostics/diagnostic_ring.hpp"
#include "crimson/gpu/gpu_device.hpp"
#include "crimson/graph/render_graph.hpp"

#include <string>
#include <utility>

namespace crimson {
namespace {

[[nodiscard]] RendererDiagnostic frame_graph_diagnostic(std::string detail)
{
    return RendererDiagnostic{
        .severity = RendererDiagnosticSeverity::Error,
        .code = RendererDiagnosticCode::FrameGraphValidationFailed,
        .message = "Crimson render graph validation failed.",
        .detail = std::move(detail),
        .subsystem = "graph",
        .resource_name = "V1RenderGraph",
    };
}

[[nodiscard]] RendererDiagnostic invalid_snapshot_diagnostic(std::string detail, std::uint64_t frame_index = 0)
{
    return RendererDiagnostic{
        .severity = RendererDiagnosticSeverity::Error,
        .code = RendererDiagnosticCode::InvalidFrameSnapshot,
        .message = "Crimson received an invalid frame snapshot.",
        .detail = std::move(detail),
        .subsystem = "frame",
        .resource_name = "FrameSnapshot",
        .frame_index = frame_index,
    };
}

[[nodiscard]] quader::foundation::Result<void, RendererDiagnostic> validate_snapshot(
    const FrameSnapshot& snapshot)
{
    if (!is_valid_extent(snapshot.target_extent())) {
        return quader::foundation::Result<void, RendererDiagnostic>::failure(
            invalid_snapshot_diagnostic("Frame snapshots require a valid target extent.", snapshot.frame_index()));
    }
    if (snapshot.views().empty()) {
        return quader::foundation::Result<void, RendererDiagnostic>::failure(
            invalid_snapshot_diagnostic("Frame snapshots require at least one render view.", snapshot.frame_index()));
    }
    for (const RenderView& view : snapshot.views()) {
        if (!is_valid_render_view(view)) {
            return quader::foundation::Result<void, RendererDiagnostic>::failure(
                invalid_snapshot_diagnostic(
                    "A frame snapshot render view has an invalid rectangle.",
                    snapshot.frame_index()));
        }
    }

    return quader::foundation::Result<void, RendererDiagnostic>::success();
}

} // namespace

struct Renderer::Impl {
    RendererConfig config;
    gpu::GpuDevice device;
    RenderGraph frame_graph;
    ViewportExtent current_extent;
};

Renderer::Renderer(std::unique_ptr<Impl> impl)
    : impl_(std::move(impl))
{
}

Renderer::~Renderer() = default;

quader::foundation::Result<std::unique_ptr<Renderer>, RendererDiagnostic> Renderer::create(
    RendererConfig config,
    const NativeSurfaceDescriptor& surface)
{
    auto impl = std::make_unique<Impl>();
    impl->config = std::move(config);
    if (!impl->device.initialize(impl->config, surface)) {
        const RendererStatus status = impl->device.status();
        RendererDiagnostic diagnostic{
            .severity = RendererDiagnosticSeverity::Fatal,
            .code = RendererDiagnosticCode::RuntimeInitializationFailed,
            .message = "Crimson renderer initialization failed.",
            .subsystem = "gpu",
            .resource_name = "GpuDevice",
        };
        if (!status.diagnostics.empty()) {
            diagnostic = status.diagnostics.back();
        }
        return quader::foundation::Result<std::unique_ptr<Renderer>, RendererDiagnostic>::failure(
            std::move(diagnostic));
    }

    impl->frame_graph = make_v1_correctness_render_graph(surface.extent);
    impl->current_extent = surface.extent;

    return quader::foundation::Result<std::unique_ptr<Renderer>, RendererDiagnostic>::success(
        std::unique_ptr<Renderer>(new Renderer(std::move(impl))));
}

quader::foundation::Result<void, RendererDiagnostic> Renderer::resize(const ViewportExtent& extent)
{
    if (extent.width_px == impl_->current_extent.width_px
        && extent.height_px == impl_->current_extent.height_px
        && extent.device_pixel_ratio == impl_->current_extent.device_pixel_ratio) {
        return quader::foundation::Result<void, RendererDiagnostic>::success();
    }

    if (!impl_->device.resize(extent)) {
        const RendererStatus status = impl_->device.status();
        RendererDiagnostic diagnostic{
            .severity = RendererDiagnosticSeverity::Error,
            .code = RendererDiagnosticCode::ResizeFailed,
            .message = "Crimson renderer resize failed.",
            .subsystem = "gpu",
            .resource_name = "Backbuffer",
        };
        if (!status.diagnostics.empty()) {
            diagnostic = status.diagnostics.back();
        }
        return quader::foundation::Result<void, RendererDiagnostic>::failure(std::move(diagnostic));
    }

    impl_->frame_graph.resize(extent);
    impl_->current_extent = extent;
    return quader::foundation::Result<void, RendererDiagnostic>::success();
}

quader::foundation::Result<FrameRenderResult, RendererDiagnostic> Renderer::render(const FrameSnapshot& snapshot)
{
    auto snapshot_validation = validate_snapshot(snapshot);
    if (!snapshot_validation) {
        return quader::foundation::Result<FrameRenderResult, RendererDiagnostic>::failure(
            std::move(snapshot_validation).error());
    }

    const ViewportExtent extent = snapshot.target_extent();
    auto resized = resize(extent);
    if (!resized) {
        return quader::foundation::Result<FrameRenderResult, RendererDiagnostic>::failure(std::move(resized).error());
    }

    auto graph_validation = impl_->frame_graph.validate();
    if (!graph_validation) {
        return quader::foundation::Result<FrameRenderResult, RendererDiagnostic>::failure(
            frame_graph_diagnostic(std::move(graph_validation).error()));
    }

    return impl_->device.render_frame(snapshot, impl_->frame_graph);
}

RendererStatus Renderer::status() const
{
    return impl_->device.status();
}

std::optional<FrameStats> Renderer::latest_frame_stats() const
{
    return impl_->device.latest_frame_stats();
}

RendererDiagnosticsSnapshot Renderer::diagnostics_snapshot() const
{
    RendererDiagnosticsSnapshot snapshot;
    const RendererStatus& status = impl_->device.status();
    snapshot.recent_diagnostics = make_diagnostic_ring_from_recent(status.diagnostics).snapshot();
    snapshot.status = make_bounded_renderer_status_snapshot(status, snapshot.recent_diagnostics);
    snapshot.latest_frame_stats = impl_->device.latest_frame_stats();
    snapshot.pass_stats = impl_->device.latest_pass_stats();
    if (snapshot.pass_stats.empty()) {
        snapshot.pass_stats = make_render_pass_stats(
            impl_->frame_graph,
            snapshot.latest_frame_stats ? &*snapshot.latest_frame_stats : nullptr);
    }
    if (snapshot.latest_frame_stats) {
        snapshot.counters = make_renderer_counters(*snapshot.latest_frame_stats);
    }
    snapshot.frame_graph_dump = impl_->frame_graph.debug_dump();
    return snapshot;
}

std::string Renderer::debug_dump_last_frame_graph() const
{
    return impl_->frame_graph.debug_dump();
}

} // namespace crimson
