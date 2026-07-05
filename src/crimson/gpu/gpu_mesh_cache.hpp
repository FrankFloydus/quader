#pragma once

#include "crimson/frame/frame_stats.hpp"
#include "crimson/gpu/gpu_runtime_resources.hpp"
#include "crimson/renderer_diagnostics.hpp"

namespace crimson::gpu {

class GpuMeshCache final {
public:
	[[nodiscard]] RenderMeshHandle create_prototype_cube(RendererStatus &status);
	[[nodiscard]] const GpuMeshResource *get(RenderMeshHandle handle) const noexcept;
	[[nodiscard]] std::size_t live_mesh_count() const noexcept;
	[[nodiscard]] FrameUploadStats upload_stats() const noexcept;
	void clear() noexcept;

private:
	GpuMeshTable meshes_;
	FrameUploadStats upload_stats_;
};

} // namespace crimson::gpu
