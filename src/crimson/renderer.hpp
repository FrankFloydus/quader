#pragma once

#include "crimson/frame/frame_render_result.hpp"
#include "crimson/frame/frame_snapshot.hpp"
#include "crimson/diagnostics/renderer_diagnostics_snapshot.hpp"
#include "crimson/native_surface.hpp"
#include "crimson/renderer_config.hpp"
#include "crimson/renderer_diagnostics.hpp"
#include "foundation/result.hpp"

#include <memory>
#include <optional>
#include <string>

namespace crimson {

class Renderer final {
public:
    [[nodiscard]] static quader::foundation::Result<std::unique_ptr<Renderer>, RendererDiagnostic> create(
        RendererConfig config,
        const NativeSurfaceDescriptor& surface);

    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    [[nodiscard]] quader::foundation::Result<void, RendererDiagnostic> resize(const ViewportExtent& extent);
    [[nodiscard]] quader::foundation::Result<FrameRenderResult, RendererDiagnostic> render(
        const FrameSnapshot& snapshot);

    [[nodiscard]] RendererStatus status() const;
    [[nodiscard]] std::optional<FrameStats> latest_frame_stats() const;
    [[nodiscard]] RendererDiagnosticsSnapshot diagnostics_snapshot() const;
    [[nodiscard]] std::string debug_dump_last_frame_graph() const;

private:
    struct Impl;

    explicit Renderer(std::unique_ptr<Impl> impl);

    std::unique_ptr<Impl> impl_;
};

} // namespace crimson
