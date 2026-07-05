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

struct GpuFrameTargetRecord {
	RenderResourceDesc desc;
	std::uint64_t graph_generation = 0;
	std::uint32_t recreate_count = 0;
	UniqueTextureHandle texture;
	UniqueFrameBufferHandle framebuffer;

	GpuFrameTargetRecord() noexcept = default;
	GpuFrameTargetRecord(const GpuFrameTargetRecord &) = delete;
	GpuFrameTargetRecord &operator=(const GpuFrameTargetRecord &) = delete;
	GpuFrameTargetRecord(GpuFrameTargetRecord &&) noexcept = default;
	GpuFrameTargetRecord &operator=(GpuFrameTargetRecord &&) noexcept = default;

	[[nodiscard]] bool external() const noexcept;
	[[nodiscard]] bool runtime_ready() const noexcept;
};

class GpuFrameResources final {
public:
	void synchronize_descriptors(const RenderGraph &graph);
	[[nodiscard]] bool synchronize(const RenderGraph &graph, RendererStatus &status);
	void clear() noexcept;

	[[nodiscard]] std::span<const GpuFrameTargetRecord> targets() const noexcept;
	[[nodiscard]] const GpuFrameTargetRecord *find(std::string_view name) const noexcept;
	[[nodiscard]] std::uint64_t graph_resize_generation() const noexcept;
	[[nodiscard]] bgfx::TextureHandle texture(std::string_view name) const noexcept;
	[[nodiscard]] bgfx::FrameBufferHandle framebuffer(std::string_view name) const noexcept;
	[[nodiscard]] bgfx::FrameBufferHandle scene_depth_framebuffer() const noexcept;
	[[nodiscard]] bgfx::FrameBufferHandle hdr_scene_framebuffer() const noexcept;
	[[nodiscard]] bgfx::FrameBufferHandle tone_mapped_color_framebuffer() const noexcept;
	[[nodiscard]] bgfx::FrameBufferHandle tone_mapped_framebuffer() const noexcept;
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
