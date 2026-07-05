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

#include "crimson/gpu/gpu_handles.hpp"
#include "crimson/graph/render_graph.hpp"
#include "crimson/renderer_diagnostics.hpp"

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <bgfx/bgfx.h>

namespace crimson::gpu {

/// Runtime GPU resource record for one render graph target.
struct GpuFrameTargetRecord {
	/// Graph resource descriptor.
	RenderResourceDesc desc;
	/// Render graph generation associated with the runtime handles.
	std::uint64_t graph_generation = 0;
	/// Number of times the runtime target has been recreated.
	std::uint32_t recreate_count = 0;
	/// Texture handle owned for non-external targets.
	UniqueTextureHandle texture;
	/// Framebuffer handle owned for non-external targets or composites.
	UniqueFrameBufferHandle framebuffer;

	/// Create an empty target record.
	GpuFrameTargetRecord() noexcept = default;
	/// Target records are move-only because they own GPU handles.
	GpuFrameTargetRecord(const GpuFrameTargetRecord &) = delete;
	/// Target records are move-only because they own GPU handles.
	GpuFrameTargetRecord &operator=(const GpuFrameTargetRecord &) = delete;
	/// Move a target record and its GPU handles.
	GpuFrameTargetRecord(GpuFrameTargetRecord &&) noexcept = default;
	/// Move-assign a target record and its GPU handles.
	GpuFrameTargetRecord &operator=(GpuFrameTargetRecord &&) noexcept = default;

	/**
	 * Check whether the target is externally owned.
	 *
	 * @return True when the descriptor is external.
	 */
	[[nodiscard]] bool external() const noexcept;
	/**
	 * Check whether runtime handles are ready.
	 *
	 * @return True when the external target needs no handle or owned handles are valid.
	 */
	[[nodiscard]] bool runtime_ready() const noexcept;
};

/// Owns GPU frame targets synchronized from the render graph.
class GpuFrameResources final {
public:
	/**
	 * Synchronize stored descriptors from a graph without creating runtime handles.
	 *
	 * @param graph Render graph source.
	 */
	void synchronize_descriptors(const RenderGraph &graph);
	/**
	 * Synchronize descriptors and runtime target handles.
	 *
	 * @param graph Render graph source.
	 * @param status Status receiving diagnostics.
	 * @return True when required runtime targets are ready.
	 */
	[[nodiscard]] bool synchronize(const RenderGraph &graph, RendererStatus &status);
	/// Release all frame target handles and descriptors.
	void clear() noexcept;

	/// Return target records in graph order.
	[[nodiscard]] std::span<const GpuFrameTargetRecord> targets() const noexcept;
	/**
	 * Find a target record by graph resource name.
	 *
	 * @param name Resource name.
	 * @return Borrowed record, or `nullptr` when absent.
	 */
	[[nodiscard]] const GpuFrameTargetRecord *find(std::string_view name) const noexcept;
	/// Return the graph resize generation currently synchronized.
	[[nodiscard]] std::uint64_t graph_resize_generation() const noexcept;
	/**
	 * Return a texture handle by resource name.
	 *
	 * @param name Resource name.
	 * @return Texture handle, or an invalid handle when absent.
	 */
	[[nodiscard]] bgfx::TextureHandle texture(std::string_view name) const noexcept;
	/**
	 * Return a framebuffer handle by resource name.
	 *
	 * @param name Resource name.
	 * @return Framebuffer handle, or an invalid handle when absent.
	 */
	[[nodiscard]] bgfx::FrameBufferHandle framebuffer(std::string_view name) const noexcept;
	/// Return the composite framebuffer for scene depth.
	[[nodiscard]] bgfx::FrameBufferHandle scene_depth_framebuffer() const noexcept;
	/// Return the composite framebuffer for HDR scene color.
	[[nodiscard]] bgfx::FrameBufferHandle hdr_scene_framebuffer() const noexcept;
	/// Return the composite framebuffer for tone-mapped color.
	[[nodiscard]] bgfx::FrameBufferHandle tone_mapped_color_framebuffer() const noexcept;
	/// Return the tone-mapped framebuffer alias.
	[[nodiscard]] bgfx::FrameBufferHandle tone_mapped_framebuffer() const noexcept;
	/// Return the picking framebuffer.
	[[nodiscard]] bgfx::FrameBufferHandle picking_framebuffer() const noexcept;

private:
	[[nodiscard]] bool create_runtime_targets(RendererStatus &status);
	[[nodiscard]] bool recreate_composite_framebuffers(RendererStatus &status);

	std::vector<GpuFrameTargetRecord> targets_;
	std::uint64_t graph_resize_generation_ = 0;
	UniqueFrameBufferHandle hdr_scene_framebuffer_;
	UniqueFrameBufferHandle tone_mapped_framebuffer_;
	UniqueFrameBufferHandle picking_framebuffer_;
};

} // namespace crimson::gpu
