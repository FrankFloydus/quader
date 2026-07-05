#pragma once

#include "crimson/gpu/gpu_runtime_resources.hpp"
#include "crimson/gpu/shader_library.hpp"
#include "crimson/renderer_diagnostics.hpp"

#include <unordered_map>

#include <bgfx/bgfx.h>

namespace crimson::gpu {

class GpuProgramCache final {
public:
    [[nodiscard]] RenderProgramHandle load_program(
        const ShaderLibrary& library,
        ShaderProgramId program,
        ShaderTarget target,
        RendererStatus& status);
    [[nodiscard]] bgfx::ProgramHandle program(RenderProgramHandle handle) const noexcept;
    [[nodiscard]] RenderProgramHandle cached_handle(ShaderProgramId program) const noexcept;
    [[nodiscard]] std::size_t live_program_count() const noexcept;
    void clear() noexcept;

private:
    GpuProgramTable programs_;
    std::unordered_map<ShaderProgramId, RenderProgramHandle> handles_by_program_;
};

} // namespace crimson::gpu
