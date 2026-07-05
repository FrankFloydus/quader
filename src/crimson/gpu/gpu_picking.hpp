#pragma once

#include "crimson/frame/frame_snapshot.hpp"
#include "crimson/frame/frame_targets.hpp"
#include "crimson/gpu/gpu_mesh_cache.hpp"
#include "crimson/gpu/gpu_program_cache.hpp"
#include "crimson/picking/picking.hpp"
#include "crimson/renderer_diagnostics.hpp"

#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

#include <bgfx/bgfx.h>

namespace crimson::gpu {

struct GpuPickingFrameResult {
    std::vector<PickingResult> completed_results;
    std::uint32_t scheduled_readbacks = 0;
};

class GpuPicking final {
public:
    GpuPicking();
    ~GpuPicking();

    GpuPicking(const GpuPicking&) = delete;
    GpuPicking& operator=(const GpuPicking&) = delete;

    [[nodiscard]] bool initialize(PickingIdTargetFormat target_format, RendererStatus& status);
    void shutdown() noexcept;

    [[nodiscard]] bool ready() const noexcept;

    [[nodiscard]] GpuPickingFrameResult submit_frame_requests(
        const FrameSnapshot& snapshot,
        const GpuMeshCache& mesh_cache,
        const GpuProgramCache& program_cache,
        RenderMeshHandle fallback_mesh,
        RenderProgramHandle picking_program,
        std::uint32_t completed_bgfx_frame);

private:
    struct Impl;

    std::unique_ptr<Impl> impl_;
};

} // namespace crimson::gpu
