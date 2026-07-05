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
#include "crimson/gpu/gpu_runtime_resources.hpp"
#include "crimson/mesh/render_mesh.hpp"
#include "crimson/renderer_diagnostics.hpp"

namespace crimson::gpu {

/// Owns GPU mesh resources used by the runtime renderer path.
class GpuMeshCache final {
public:
	/**
	 * Create and store the built-in unit box mesh.
	 *
	 * @param status Status receiving resource diagnostics.
	 * @return Handle for the created mesh, or an invalid handle on failure.
	 */
	[[nodiscard]] RenderMeshHandle create_unit_box(RendererStatus &status);
	/**
	 * Upload or replace a caller-owned render mesh.
	 *
	 * @param desc Upload descriptor with stable public mesh handle.
	 * @param status Status receiving resource diagnostics.
	 * @return True when the mesh resource is resident after the call.
	 */
	[[nodiscard]] bool upload_mesh(const RenderMeshUploadDesc &desc, RendererStatus &status);
	/**
	 * Resolve an immutable mesh resource.
	 *
	 * @param handle Mesh handle to resolve.
	 * @return Borrowed resource, or `nullptr` for invalid/stale handles.
	 */
	[[nodiscard]] const GpuMeshResource *get(RenderMeshHandle handle) const noexcept;
	/// Return the number of live mesh resources.
	[[nodiscard]] std::size_t live_mesh_count() const noexcept;
	/// Return upload counters accumulated by the mesh cache.
	[[nodiscard]] FrameUploadStats upload_stats() const noexcept;
	/// Destroy all mesh resources.
	void clear() noexcept;

private:
	GpuMeshTable meshes_;
	FrameUploadStats upload_stats_;
};

} // namespace crimson::gpu
