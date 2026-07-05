#pragma once

#include "crimson/frame/frame_stats.hpp"
#include "crimson/mesh/render_mesh.hpp"
#include "crimson/renderer_diagnostics.hpp"
#include "foundation/result.hpp"

#include <span>
#include <vector>

namespace crimson {

[[nodiscard]] quader::foundation::Result<RenderMeshUploadRecord, RendererDiagnostic> validate_render_mesh_upload_desc(
    const RenderMeshUploadDesc& desc);

class RenderMeshUploadTracker final {
public:
    [[nodiscard]] quader::foundation::Result<FrameUploadStats, RendererDiagnostic> process_uploads(
        std::span<const RenderMeshUploadDesc> uploads);
    [[nodiscard]] bool destroy(RenderMeshHandle handle) noexcept;
    [[nodiscard]] std::size_t live_record_count() const noexcept;
    [[nodiscard]] const RenderMeshUploadRecord* find(RenderMeshHandle handle) const noexcept;
    void clear() noexcept;

private:
    std::vector<RenderMeshUploadRecord> records_;
};

} // namespace crimson
