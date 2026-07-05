#pragma once

#include "crimson/diagnostics/renderer_diagnostics_snapshot.hpp"
#include "crimson/frame/frame_render_result.hpp"
#include "crimson/frame/frame_snapshot.hpp"
#include "crimson/gpu/gpu_capabilities.hpp"
#include "crimson/graph/render_graph.hpp"
#include "crimson/native_surface.hpp"
#include "crimson/renderer_config.hpp"
#include "crimson/renderer_diagnostics.hpp"
#include "foundation/result.hpp"

#include <memory>
#include <optional>
#include <vector>

namespace crimson::gpu {

class GpuDevice final {
public:
	GpuDevice();
	~GpuDevice();

	GpuDevice(const GpuDevice &) = delete;
	GpuDevice &operator=(const GpuDevice &) = delete;

	bool initialize(const RendererConfig &config, const NativeSurfaceDescriptor &surface);
	void shutdown() noexcept;
	bool resize(const ViewportExtent &extent);

	[[nodiscard]] quader::foundation::Result<FrameRenderResult, RendererDiagnostic> render_frame(
			const FrameSnapshot &snapshot,
			const RenderGraph &graph);
	[[nodiscard]] std::optional<FrameStats> latest_frame_stats() const noexcept;
	[[nodiscard]] std::vector<RenderPassStats> latest_pass_stats() const;

	bool initialized() const noexcept;
	std::optional<GraphicsBackend> selected_backend() const noexcept;
	const RendererStatus &status() const noexcept;

private:
	struct Impl;

	RendererStatus status_;
	std::optional<GraphicsBackend> selected_backend_;
	RendererConfig config_;
	ViewportExtent extent_;
	std::unique_ptr<Impl> impl_;
};

} // namespace crimson::gpu
