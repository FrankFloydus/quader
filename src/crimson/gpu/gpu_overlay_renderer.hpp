#pragma once

#include "crimson/frame/frame_snapshot.hpp"
#include "crimson/gpu/gpu_program_cache.hpp"
#include "crimson/overlays/overlay_system.hpp"
#include "crimson/renderer_diagnostics.hpp"

#include <cstdint>
#include <memory>

#include <bgfx/bgfx.h>

namespace crimson::gpu {

class GpuOverlayRenderer final {
public:
    GpuOverlayRenderer();
    ~GpuOverlayRenderer();

    GpuOverlayRenderer(const GpuOverlayRenderer&) = delete;
    GpuOverlayRenderer& operator=(const GpuOverlayRenderer&) = delete;

    [[nodiscard]] bool initialize(RendererStatus& status);
    void shutdown() noexcept;

    [[nodiscard]] bool ready() const noexcept;
    [[nodiscard]] std::uint32_t submit_grid(
        bgfx::ViewId view_id,
        const RenderView& view,
        const PreparedGridOverlayCommand& grid,
        bgfx::ProgramHandle program) const noexcept;

private:
    struct Impl;

    std::unique_ptr<Impl> impl_;
};

} // namespace crimson::gpu
