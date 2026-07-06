/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
#include "crimson/gpu/gpu_device.hpp"

#include "crimson/diagnostics/renderer_diagnostics_snapshot.hpp"
#include "crimson/gpu/gpu_frame_resources.hpp"
#include "crimson/gpu/gpu_handles.hpp"
#include "crimson/gpu/gpu_material_cache.hpp"
#include "crimson/gpu/gpu_mesh_cache.hpp"
#include "crimson/gpu/gpu_overlay_renderer.hpp"
#include "crimson/gpu/gpu_pbr_pass.hpp"
#include "crimson/gpu/gpu_picking.hpp"
#include "crimson/gpu/gpu_program_cache.hpp"
#include "crimson/gpu/gpu_view_policy.hpp"
#include "crimson/gpu/shader_library_runtime.hpp"
#include "crimson/material/default_material.hpp"
#include "crimson/material/material_system.hpp"
#include "crimson/overlays/overlay_system.hpp"
#include "crimson/pipeline/instancing_batcher.hpp"
#include "crimson/post/exposure.hpp"
#include "crimson/post/tone_mapping.hpp"
#include "crimson/scene/render_camera_projection.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <bx/math.h>

namespace crimson::gpu {
namespace {

constexpr bgfx::ViewId kBackgroundView = 0;
constexpr bgfx::ViewId kFirstDepthView = 1;
constexpr bgfx::ViewId kFirstSceneView = 64;
constexpr bgfx::ViewId kToneMapView = 128;
constexpr bgfx::ViewId kFirstOverlayView = 160;
constexpr bgfx::ViewId kPresentView = 240;
constexpr std::uint32_t kViewportClearColor = 0x020202ff;
constexpr std::uint32_t kSceneViewStride = 4;
constexpr std::uint32_t kOverlayViewStride = 3;

struct ViewportResources {
	bgfx::UniformHandle pbr_model_uniform = BGFX_INVALID_HANDLE;
	bool ready = false;
};

struct FullscreenVertex {
	float x;
	float y;
	float z;
	float u;
	float v;
};

struct PostProcessResources {
	bgfx::VertexLayout fullscreen_vertex_layout{};
	UniqueVertexBufferHandle fullscreen_vertex_buffer;
	UniqueUniformHandle hdr_scene_sampler;
	UniqueUniformHandle tone_mapped_sampler;
	UniqueUniformHandle tone_map_params_uniform;
	UniqueUniformHandle present_params_uniform;
	bool ready = false;
};

[[nodiscard]] bgfx::RendererType::Enum to_bgfx_renderer_type(GraphicsBackend backend) noexcept {
	switch (backend) {
		case GraphicsBackend::Vulkan:
			return bgfx::RendererType::Vulkan;
		case GraphicsBackend::Metal:
			return bgfx::RendererType::Metal;
		case GraphicsBackend::Direct3D12:
			return bgfx::RendererType::Direct3D12;
		case GraphicsBackend::Direct3D11:
			return bgfx::RendererType::Direct3D11;
	}

	return bgfx::RendererType::Count;
}

[[nodiscard]] std::optional<GraphicsBackend> from_bgfx_renderer_type(bgfx::RendererType::Enum renderer) noexcept {
	switch (renderer) {
		case bgfx::RendererType::Vulkan:
			return GraphicsBackend::Vulkan;
		case bgfx::RendererType::Metal:
			return GraphicsBackend::Metal;
		case bgfx::RendererType::Direct3D12:
			return GraphicsBackend::Direct3D12;
		case bgfx::RendererType::Direct3D11:
			return GraphicsBackend::Direct3D11;
		default:
			return std::nullopt;
	}
}

[[nodiscard]] ShaderTarget shader_target_for_backend(GraphicsBackend backend) noexcept {
	switch (backend) {
		case GraphicsBackend::Vulkan:
			return ShaderTarget::Vulkan;
		case GraphicsBackend::Metal:
			return ShaderTarget::Metal;
		case GraphicsBackend::Direct3D12:
			return ShaderTarget::Direct3D12;
		case GraphicsBackend::Direct3D11:
			return ShaderTarget::Direct3D11;
	}

	return ShaderTarget::Vulkan;
}

[[nodiscard]] PickingIdTargetFormat picking_target_format_from_status(const RendererStatus &status) noexcept {
	for (const RendererCapabilityStatus &capability : status.capabilities) {
		if (capability.capability == RendererCapability::IntegerPickingTarget && capability.supported) {
			return PickingIdTargetFormat::R32Uint;
		}
	}
	return PickingIdTargetFormat::Rgba8Data;
}

[[nodiscard]] std::vector<GraphicsBackend> supported_runtime_backends() {
	std::array<bgfx::RendererType::Enum, bgfx::RendererType::Count> renderers{};
	const std::uint8_t kCount = bgfx::getSupportedRenderers(
			static_cast<std::uint8_t>(renderers.size()),
			renderers.data());

	std::vector<GraphicsBackend> backends;
	backends.reserve(kCount);
	for (std::uint8_t index = 0; index < kCount; ++index) {
		if (std::optional<GraphicsBackend> backend = from_bgfx_renderer_type(renderers[index])) {
			backends.push_back(*backend);
		}
	}

	return backends;
}

[[nodiscard]] bgfx::PlatformData to_bgfx_platform_data(const NativeSurfaceDescriptor &surface) noexcept {
	bgfx::PlatformData data{};
	data.ndt = surface.native_display_handle;
	data.nwh = surface.native_window_handle;
	data.context = nullptr;
	data.backBuffer = nullptr;
	data.backBufferDS = nullptr;
	data.type = surface.platform == NativeSurfacePlatform::LinuxWayland
			? bgfx::NativeWindowHandleType::Wayland
			: bgfx::NativeWindowHandleType::Default;
	return data;
}

[[nodiscard]] bool format_supports(std::uint16_t format_caps, std::uint32_t required_flag) noexcept {
	return (static_cast<std::uint32_t>(format_caps) & required_flag) == required_flag;
}

[[nodiscard]] GpuCapabilityInput runtime_capabilities_from_bgfx(
		const bgfx::Caps &caps,
		const RendererConfig &config) noexcept {
	const std::uint16_t kRgba8Caps = caps.formats[bgfx::TextureFormat::RGBA8];
	const std::uint16_t kRgba16fCaps = caps.formats[bgfx::TextureFormat::RGBA16F];
	const std::uint16_t kR32uCaps = caps.formats[bgfx::TextureFormat::R32U];
	const std::uint16_t kD24s8Caps = caps.formats[bgfx::TextureFormat::D24S8];
	const std::uint16_t kD32fCaps = caps.formats[bgfx::TextureFormat::D32F];

	return GpuCapabilityInput{
		.instancing = (caps.supported & BGFX_CAPS_INSTANCING) != 0,
		.texture_2d = format_supports(kRgba8Caps, BGFX_CAPS_FORMAT_TEXTURE_2D),
		.texture_cube = format_supports(kRgba8Caps, BGFX_CAPS_FORMAT_TEXTURE_CUBE),
		.floating_point_render_target = format_supports(kRgba16fCaps, BGFX_CAPS_FORMAT_TEXTURE_FRAMEBUFFER),
		.render_to_texture = format_supports(kRgba8Caps, BGFX_CAPS_FORMAT_TEXTURE_FRAMEBUFFER) || format_supports(kRgba16fCaps, BGFX_CAPS_FORMAT_TEXTURE_FRAMEBUFFER),
		.depth_texture = format_supports(kD24s8Caps, BGFX_CAPS_FORMAT_TEXTURE_FRAMEBUFFER) || format_supports(kD32fCaps, BGFX_CAPS_FORMAT_TEXTURE_FRAMEBUFFER),
		.srgb_texture_sampling = format_supports(kRgba8Caps, BGFX_CAPS_FORMAT_TEXTURE_2D_SRGB),
		.srgb_backbuffer = false,
		.manual_srgb_final_conversion = config.display_conversion.allow_manual_final_srgb,
		.integer_picking_target = format_supports(kR32uCaps, BGFX_CAPS_FORMAT_TEXTURE_FRAMEBUFFER),
		.rgba8_picking_target = format_supports(kRgba8Caps, BGFX_CAPS_FORMAT_TEXTURE_FRAMEBUFFER),
		.readback = (caps.supported & BGFX_CAPS_TEXTURE_READ_BACK) != 0,
	};
}

[[nodiscard]] std::uint32_t reset_flags_for_config(const RendererConfig &config) noexcept {
	std::uint32_t flags = BGFX_RESET_MAXANISOTROPY;
	if (config.vsync) {
		flags |= BGFX_RESET_VSYNC;
	}
	return flags;
}

void push_diagnostic(RendererStatus &status, RendererDiagnostic diagnostic) {
	if (diagnostic.subsystem.empty()) {
		diagnostic.subsystem = "gpu";
	}
	if (diagnostic.resource_name.empty()) {
		diagnostic.resource_name = "GpuDevice";
	}
	status.diagnostics.push_back(std::move(diagnostic));
}

[[nodiscard]] std::uint64_t elapsed_us_since(std::chrono::steady_clock::time_point start) {
	return static_cast<std::uint64_t>(
			std::chrono::duration_cast<std::chrono::microseconds>(
					std::chrono::steady_clock::now() - start)
					.count());
}

void destroy_handle(bgfx::UniformHandle &handle) noexcept {
	if (bgfx::isValid(handle)) {
		bgfx::destroy(handle);
		handle = BGFX_INVALID_HANDLE;
	}
}

void destroy_viewport_resources(ViewportResources &resources) noexcept {
	destroy_handle(resources.pbr_model_uniform);
	resources.ready = false;
}

void destroy_post_process_resources(PostProcessResources &resources) noexcept {
	resources.fullscreen_vertex_buffer.reset();
	resources.hdr_scene_sampler.reset();
	resources.tone_mapped_sampler.reset();
	resources.tone_map_params_uniform.reset();
	resources.present_params_uniform.reset();
	resources.ready = false;
}

[[nodiscard]] bool are_viewport_resources_ready(const ViewportResources &resources) noexcept {
	return bgfx::isValid(resources.pbr_model_uniform);
}

[[nodiscard]] ScreenOrigin runtime_screen_origin() noexcept {
	const bgfx::Caps *caps = bgfx::getCaps();
	return screen_origin_from_bgfx(caps != nullptr && caps->originBottomLeft);
}

[[nodiscard]] bool runtime_homogeneous_depth() noexcept {
	const bgfx::Caps *caps = bgfx::getCaps();
	return caps != nullptr && caps->homogeneousDepth;
}

[[nodiscard]] std::array<FullscreenVertex, 3> make_fullscreen_triangle(ScreenOrigin origin) noexcept {
	const FullscreenTriangleUvRange kUv = fullscreen_triangle_uv_range(origin);
	return std::array<FullscreenVertex, 3>{
		FullscreenVertex{ -1.0F, 0.0F, 0.0F, kUv.min_u, kUv.min_v },
		FullscreenVertex{ 1.0F, 0.0F, 0.0F, kUv.max_u, kUv.min_v },
		FullscreenVertex{ 1.0F, 2.0F, 0.0F, kUv.max_u, kUv.max_v },
	};
}

[[nodiscard]] bool create_viewport_resources(
		ViewportResources &resources,
		RendererStatus &status) {
	destroy_viewport_resources(resources);

	resources.pbr_model_uniform = bgfx::createUniform("u_pbrModel", bgfx::UniformType::Mat4);

	resources.ready = are_viewport_resources_ready(resources);
	if (!resources.ready) {
		push_diagnostic(status, RendererDiagnostic{
										.severity = RendererDiagnosticSeverity::Error,
										.code = RendererDiagnosticCode::ResourceCreationFailed,
										.message = "Crimson failed to create viewport GPU resources.",
										.subsystem = "gpu.viewport",
										.resource_name = "ViewportResources",
								});
		destroy_viewport_resources(resources);
	}

	return resources.ready;
}

[[nodiscard]] bool create_post_process_resources(
		PostProcessResources &resources,
		RendererStatus &status) {
	destroy_post_process_resources(resources);

	resources.fullscreen_vertex_layout
			.begin()
			.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
			.add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
			.end();
	const std::array<FullscreenVertex, 3> kFullscreenTriangle =
			make_fullscreen_triangle(runtime_screen_origin());
	resources.fullscreen_vertex_buffer.reset(bgfx::createVertexBuffer(
			bgfx::copy(kFullscreenTriangle.data(), sizeof(FullscreenVertex) * kFullscreenTriangle.size()),
			resources.fullscreen_vertex_layout));
	resources.hdr_scene_sampler.reset(bgfx::createUniform("s_hdrSceneColor", bgfx::UniformType::Sampler));
	resources.tone_mapped_sampler.reset(bgfx::createUniform("s_toneMappedColor", bgfx::UniformType::Sampler));
	resources.tone_map_params_uniform.reset(bgfx::createUniform("u_toneMapParams", bgfx::UniformType::Vec4));
	resources.present_params_uniform.reset(bgfx::createUniform("u_presentParams", bgfx::UniformType::Vec4));

	resources.ready = resources.fullscreen_vertex_buffer.valid() && resources.hdr_scene_sampler.valid() && resources.tone_mapped_sampler.valid() && resources.tone_map_params_uniform.valid() && resources.present_params_uniform.valid();
	if (!resources.ready) {
		push_diagnostic(status, RendererDiagnostic{
										.severity = RendererDiagnosticSeverity::Error,
										.code = RendererDiagnosticCode::ResourceCreationFailed,
										.message = "Crimson failed to create post-process GPU resources.",
										.subsystem = "gpu.post",
										.resource_name = "PostProcessResources",
								});
		destroy_post_process_resources(resources);
	}

	return resources.ready;
}

[[nodiscard]] std::uint16_t safe_view_dimension(std::uint32_t value) noexcept {
	return static_cast<std::uint16_t>(std::clamp<std::uint32_t>(value, 1U, 65535U));
}

[[nodiscard]] float view_aspect_ratio(const RenderView &view) noexcept {
	return static_cast<float>(safe_view_dimension(view.rect.width)) / static_cast<float>(safe_view_dimension(view.rect.height));
}

[[nodiscard]] RendererDiagnostic invalid_snapshot_diagnostic(std::string detail, std::uint64_t frame_index = 0) {
	return RendererDiagnostic{
		.severity = RendererDiagnosticSeverity::Error,
		.code = RendererDiagnosticCode::InvalidFrameSnapshot,
		.message = "Crimson received an invalid frame snapshot.",
		.detail = std::move(detail),
		.subsystem = "frame",
		.resource_name = "FrameSnapshot",
		.frame_index = frame_index,
	};
}

void push_material_error(RendererStatus &status, RendererDiagnostic diagnostic) {
	if (diagnostic.subsystem.empty()) {
		diagnostic.subsystem = "material";
	}
	if (diagnostic.resource_name.empty()) {
		diagnostic.resource_name = "MaterialSystem";
	}
	status.diagnostics.push_back(std::move(diagnostic));
}

[[nodiscard]] bgfx::ViewId scene_view_id(std::size_t view_index, std::uint32_t queue_offset) noexcept {
	return static_cast<bgfx::ViewId>(kFirstSceneView + view_index * kSceneViewStride + queue_offset);
}

[[nodiscard]] bgfx::ViewId depth_view_id(std::size_t view_index) noexcept {
	return static_cast<bgfx::ViewId>(kFirstDepthView + view_index);
}

[[nodiscard]] bgfx::ViewId overlay_view_id(std::size_t view_index, std::uint32_t queue_offset) noexcept {
	return static_cast<bgfx::ViewId>(kFirstOverlayView + view_index * kOverlayViewStride + queue_offset);
}

[[nodiscard]] std::uint32_t overlay_queue_offset(OverlayDepthMode depth_mode) noexcept {
	switch (depth_mode) {
		case OverlayDepthMode::DepthTested:
			return 0;
		case OverlayDepthMode::XRay:
			return 1;
		case OverlayDepthMode::AlwaysOnTop:
			return 2;
	}

	return 0;
}

[[nodiscard]] bool grid_uses_scene_underlay_pass(
		const PreparedGridOverlayCommand &grid,
		const RenderView &view) noexcept {
	return grid.command.depth_mode == OverlayDepthMode::AlwaysOnTop &&
			grid.grid.depth_mode == OverlayDepthMode::AlwaysOnTop &&
			view.camera.projection != CameraProjection::Orthographic;
}

enum class PbrSubmitMode {
	DepthOnly,
	Lit,
};

enum class ViewClearMode {
	None,
	Color,
	Depth,
};

[[nodiscard]] std::uint16_t clear_flags_for(ViewClearMode mode) noexcept {
	switch (mode) {
		case ViewClearMode::None:
			return BGFX_CLEAR_NONE;
		case ViewClearMode::Color:
			return BGFX_CLEAR_COLOR;
		case ViewClearMode::Depth:
			return BGFX_CLEAR_DEPTH;
	}

	return BGFX_CLEAR_NONE;
}

[[nodiscard]] std::uint64_t state_for_packet(const PbrDrawPacket &packet, PbrSubmitMode mode) noexcept {
	const std::uint64_t kCullState = packet.double_sided ? 0 : BGFX_STATE_CULL_CCW;
	if (mode == PbrSubmitMode::DepthOnly) {
		return BGFX_STATE_WRITE_Z | BGFX_STATE_DEPTH_TEST_LESS | BGFX_STATE_MSAA | kCullState;
	}

	std::uint64_t state = BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_DEPTH_TEST_LEQUAL | BGFX_STATE_MSAA | kCullState;
	if (packet.queue == RenderQueue::Opaque || packet.queue == RenderQueue::AlphaCutout) {
		state |= BGFX_STATE_WRITE_Z;
	}
	if (packet.queue == RenderQueue::Transparent) {
		state |= BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_INV_SRC_ALPHA);
	}
	return state;
}

void configure_scene_view(
		bgfx::ViewId view_id,
		const RenderView &view,
		const std::string &view_name,
		ViewClearMode clear_mode,
		bgfx::FrameBufferHandle framebuffer = BGFX_INVALID_HANDLE) {
	const RenderCamera &camera = view.camera;
	bgfx::setViewName(view_id, view_name.c_str());
	bgfx::setViewMode(view_id, bgfx::ViewMode::Sequential);
	bgfx::setViewRect(view_id, view.rect.x, view.rect.y, view.rect.width, view.rect.height);
	bgfx::setViewClear(view_id, clear_flags_for(clear_mode), kViewportClearColor, 1.0F, 0);
	if (bgfx::isValid(framebuffer)) {
		bgfx::setViewFrameBuffer(view_id, framebuffer);
	}

	const float kAspect = static_cast<float>(std::max<std::uint16_t>(1, view.rect.width)) / static_cast<float>(std::max<std::uint16_t>(1, view.rect.height));
	const RenderCameraMatrices kMatrices = render_camera_matrices(camera, kAspect, current_render_homogeneous_depth());
	bgfx::setViewTransform(view_id, kMatrices.view.data(), kMatrices.projection.data());
	bgfx::touch(view_id);
}

[[nodiscard]] bool capability_supported(const RendererStatus &status, RendererCapability capability) noexcept {
	for (const RendererCapabilityStatus &current : status.capabilities) {
		if (current.capability == capability) {
			return current.supported;
		}
	}
	return false;
}

[[nodiscard]] bool manual_final_srgb_conversion_enabled(
		const RendererStatus &status,
		const RendererConfig &config) noexcept {
	const std::optional<DisplayConversionPath> kPath = choose_display_conversion_path(
			capability_supported(status, RendererCapability::SrgbBackbuffer),
			config.display_conversion);
	return kPath == DisplayConversionPath::ManualFinalShader;
}

[[nodiscard]] float tone_mapper_shader_id(ToneMapper mapper) noexcept {
	switch (mapper) {
		case ToneMapper::AcesFitted:
			return 0.0F;
		case ToneMapper::AgxApprox:
			return 1.0F;
		case ToneMapper::Reinhard:
			return 2.0F;
		case ToneMapper::Linear:
			return 3.0F;
	}

	return 0.0F;
}

std::uint32_t submit_fullscreen_pass(
		bgfx::ViewId view_id,
		const std::string &view_name,
		ViewportExtent extent,
		bgfx::FrameBufferHandle framebuffer,
		bgfx::TextureHandle source_texture,
		bgfx::ProgramHandle program,
		bgfx::UniformHandle sampler,
		bgfx::UniformHandle params_uniform,
		const std::array<float, 4> &params,
		const PostProcessResources &resources) {
	if (!resources.ready || !bgfx::isValid(source_texture) || !bgfx::isValid(program) || !bgfx::isValid(sampler) || !bgfx::isValid(params_uniform)) {
		return 0;
	}

	bgfx::setViewName(view_id, view_name.c_str());
	bgfx::setViewMode(view_id, bgfx::ViewMode::Sequential);
	bgfx::setViewRect(
			view_id,
			0,
			0,
			safe_view_dimension(extent.width_px),
			safe_view_dimension(extent.height_px));
	bgfx::setViewClear(view_id, BGFX_CLEAR_NONE, 0x00000000, 1.0F, 0);
	bgfx::setViewFrameBuffer(view_id, framebuffer);
	float ortho[16];
	bx::mtxOrtho(
			ortho,
			0.0F,
			1.0F,
			1.0F,
			0.0F,
			0.0F,
			100.0F,
			0.0F,
			runtime_homogeneous_depth());
	bgfx::setViewTransform(view_id, nullptr, ortho);
	bgfx::touch(view_id);

	float model[16];
	bx::mtxIdentity(model);
	bgfx::discard(BGFX_DISCARD_ALL);
	bgfx::setTexture(0, sampler, source_texture);
	bgfx::setUniform(params_uniform, params.data());
	bgfx::setTransform(model);
	bgfx::setVertexBuffer(0, resources.fullscreen_vertex_buffer.get(), 0, 3);
	bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A);
	bgfx::submit(view_id, program);
	return 1;
}

std::uint32_t submit_pbr_packets(
		bgfx::ViewId view_id,
		std::span<const PbrDrawPacket> packets,
		const BaseShaderRegistry &registry,
		const MaterialSystem &materials,
		const GpuMeshCache &mesh_cache,
		const GpuProgramCache &program_cache,
		const GpuMaterialCache &material_cache,
		const ViewportResources &resources,
		PbrSubmitMode mode = PbrSubmitMode::Lit) {
	std::uint32_t draw_calls = 0;
	for (const PbrDrawPacket &packet : packets) {
		const GpuMeshResource *mesh = mesh_cache.get(packet.mesh);
		const BaseShaderDefinition *definition = registry.find(packet.base_shader);
		const bgfx::ProgramHandle kProgram = program_cache.program(program_cache.cached_handle(packet.program));
		if (mesh == nullptr || !mesh->valid() || definition == nullptr || !bgfx::isValid(kProgram)) {
			continue;
		}

		bgfx::setTransform(packet.world_from_object.data());
		bgfx::setUniform(resources.pbr_model_uniform, packet.world_from_object.data());
		material_cache.bind_pbr_material(material_cache.material_block(materials, packet.material, *definition));
		material_cache.bind_pbr_textures(materials, packet.material, *definition);
		bgfx::setVertexBuffer(0, mesh->vertex_buffer.get());
		bgfx::setIndexBuffer(mesh->index_buffer.get());
		bgfx::setState(state_for_packet(packet, mode));
		bgfx::submit(view_id, kProgram);
		++draw_calls;
	}
	return draw_calls;
}

std::uint32_t submit_grid_overlay_bucket(
		const OverlayDrawBucket &bucket,
		OverlayDepthMode depth_mode,
		std::span<const RenderView> views,
		const GpuOverlayRenderer &overlay_renderer,
		const GpuProgramCache &program_cache,
		RenderProgramHandle overlay_program,
		bgfx::FrameBufferHandle framebuffer) {
	std::uint32_t draw_calls = 0;
	const bgfx::ProgramHandle kProgram = program_cache.program(overlay_program);
	const std::uint32_t kQueueOffset = overlay_queue_offset(depth_mode);
	for (std::size_t view_index = 0; view_index < views.size(); ++view_index) {
		const RenderView &view = views[view_index];
		const bgfx::ViewId kViewId = overlay_view_id(view_index, kQueueOffset);
		const std::string kViewName = std::string(render_queue_name(render_queue_for_overlay_depth_mode(depth_mode))) + ":" + view.debug_name;
		configure_scene_view(kViewId, view, kViewName, ViewClearMode::None, framebuffer);

		for (const PreparedGridOverlayCommand &grid : bucket.grid_commands) {
			if (grid.grid.view_index != view.view_index) {
				continue;
			}
			if (grid_uses_scene_underlay_pass(grid, view)) {
				continue;
			}
			draw_calls += overlay_renderer.submit_grid(kViewId, view, grid, kProgram);
		}
	}
	return draw_calls;
}

std::uint32_t submit_grid_scene_underlay_bucket(
		const OverlayDrawBucket &bucket,
		const RenderView &view,
		bgfx::ViewId view_id,
		const GpuOverlayRenderer &overlay_renderer,
		const GpuProgramCache &program_cache,
		RenderProgramHandle overlay_program) {
	std::uint32_t draw_calls = 0;
	const bgfx::ProgramHandle kProgram = program_cache.program(overlay_program);
	for (const PreparedGridOverlayCommand &grid : bucket.grid_commands) {
		if (grid.grid.view_index != view.view_index || !grid_uses_scene_underlay_pass(grid, view)) {
			continue;
		}
		draw_calls += overlay_renderer.submit_grid(view_id, view, grid, kProgram);
	}
	return draw_calls;
}

std::uint32_t submit_line_overlay_bucket(
		const OverlayDrawBucket &bucket,
		OverlayDepthMode depth_mode,
		std::span<const SourceWireDepthStampMetadata> source_wire_depth_stamps,
		std::span<const RenderView> views,
		const GpuOverlayRenderer &overlay_renderer,
		const GpuProgramCache &program_cache,
		RenderProgramHandle line_program,
		bgfx::FrameBufferHandle framebuffer) {
	std::uint32_t draw_calls = 0;
	const bgfx::ProgramHandle kProgram = program_cache.program(line_program);
	const std::uint32_t kQueueOffset = overlay_queue_offset(depth_mode);
	for (std::size_t view_index = 0; view_index < views.size(); ++view_index) {
		const RenderView &view = views[view_index];
		const bgfx::ViewId kViewId = overlay_view_id(view_index, kQueueOffset);
		const std::string kViewName = std::string(render_queue_name(render_queue_for_overlay_depth_mode(depth_mode))) + ":" + view.debug_name;
		configure_scene_view(kViewId, view, kViewName, ViewClearMode::None, framebuffer);

		for (const PreparedLineOverlayCommand &lines : bucket.line_commands) {
			if (lines.command.view_index != view.view_index) {
				continue;
			}
			const PreparedLineOverlayCommand kFilteredLines =
					make_source_wire_visibility_filtered_line_command(
							view,
							lines,
							source_wire_depth_stamps);
			draw_calls += overlay_renderer.submit_lines(kViewId, view, kFilteredLines, kProgram);
		}
	}
	return draw_calls;
}

std::uint32_t submit_triangle_overlay_bucket(
		const OverlayDrawBucket &bucket,
		OverlayDepthMode depth_mode,
		std::span<const RenderView> views,
		const GpuOverlayRenderer &overlay_renderer,
		const GpuProgramCache &program_cache,
		RenderProgramHandle line_program,
		bgfx::FrameBufferHandle framebuffer) {
	std::uint32_t draw_calls = 0;
	const bgfx::ProgramHandle kProgram = program_cache.program(line_program);
	const std::uint32_t kQueueOffset = overlay_queue_offset(depth_mode);
	for (std::size_t view_index = 0; view_index < views.size(); ++view_index) {
		const RenderView &view = views[view_index];
		const bgfx::ViewId kViewId = overlay_view_id(view_index, kQueueOffset);
		const std::string kViewName = std::string(render_queue_name(render_queue_for_overlay_depth_mode(depth_mode))) + ":" + view.debug_name;
		configure_scene_view(kViewId, view, kViewName, ViewClearMode::None, framebuffer);

		for (const PreparedTriangleOverlayCommand &triangles : bucket.triangle_commands) {
			if (triangles.command.view_index != view.view_index) {
				continue;
			}
			draw_calls += overlay_renderer.submit_triangles(kViewId, triangles, kProgram);
		}
	}
	return draw_calls;
}

std::uint32_t submit_point_overlay_bucket(
		const OverlayDrawBucket &bucket,
		OverlayDepthMode depth_mode,
		std::span<const SourceWireDepthStampMetadata> source_wire_depth_stamps,
		std::span<const RenderView> views,
		const GpuOverlayRenderer &overlay_renderer,
		const GpuProgramCache &program_cache,
		RenderProgramHandle line_program,
		bgfx::FrameBufferHandle framebuffer) {
	std::uint32_t draw_calls = 0;
	const bgfx::ProgramHandle kProgram = program_cache.program(line_program);
	const std::uint32_t kQueueOffset = overlay_queue_offset(depth_mode);
	for (std::size_t view_index = 0; view_index < views.size(); ++view_index) {
		const RenderView &view = views[view_index];
		const bgfx::ViewId kViewId = overlay_view_id(view_index, kQueueOffset);
		const std::string kViewName = std::string(render_queue_name(render_queue_for_overlay_depth_mode(depth_mode))) + ":" + view.debug_name;
		configure_scene_view(kViewId, view, kViewName, ViewClearMode::None, framebuffer);

		for (const PreparedPointOverlayCommand &points : bucket.point_commands) {
			if (points.command.view_index != view.view_index) {
				continue;
			}
			const PreparedPointOverlayCommand kFilteredPoints =
					make_source_wire_visibility_filtered_point_command(
							view,
							points,
							source_wire_depth_stamps);
			draw_calls += overlay_renderer.submit_points(kViewId, view, kFilteredPoints, kProgram);
		}
	}
	return draw_calls;
}

[[nodiscard]] std::uint32_t size_to_u32(std::size_t value) noexcept {
	return static_cast<std::uint32_t>(std::min<std::size_t>(
			value,
			std::numeric_limits<std::uint32_t>::max()));
}

[[nodiscard]] std::uint32_t count_visible_objects(std::span<const RenderObject> objects) noexcept {
	std::uint32_t count = 0;
	for (const RenderObject &object : objects) {
		if (object.visible) {
			++count;
		}
	}
	return count;
}

[[nodiscard]] FrameResourceStats frame_resource_stats(
		const GpuFrameResources &frame_resources,
		const MaterialSystem &materials,
		const GpuMeshCache &mesh_cache,
		const GpuMaterialCache &material_cache,
		const GpuProgramCache &program_cache) noexcept {
	std::uint32_t recreate_count = 0;
	for (const GpuFrameTargetRecord &target : frame_resources.targets()) {
		recreate_count += target.recreate_count;
	}

	return FrameResourceStats{
		.live_mesh_count = size_to_u32(mesh_cache.live_mesh_count()),
		.live_material_count = size_to_u32(materials.live_material_count()),
		.live_texture_count = size_to_u32(material_cache.live_texture_count()),
		.live_program_count = size_to_u32(program_cache.live_program_count()),
		.frame_target_count = size_to_u32(frame_resources.targets().size()),
		.frame_target_recreate_count = recreate_count,
	};
}

void accumulate_culling_stats(FrameCullingStats &total, const FrameCullingStats &current) noexcept {
	total.input_object_count += current.input_object_count;
	total.visible_object_count += current.visible_object_count;
	total.culled_object_count += current.culled_object_count;
	total.invalid_bounds_count += current.invalid_bounds_count;
}

void accumulate_packet_stats(FramePacketStats &total, const FramePacketStats &current) noexcept {
	total.draw_packet_count += current.draw_packet_count;
	total.opaque_packet_count += current.opaque_packet_count;
	total.alpha_cutout_packet_count += current.alpha_cutout_packet_count;
	total.transparent_packet_count += current.transparent_packet_count;
	total.overlay_packet_count += current.overlay_packet_count;
	total.sorted_packet_count += current.sorted_packet_count;
}

void accumulate_instancing_stats(FrameInstancingStats &total, const FrameInstancingStats &current) noexcept {
	total.batch_count += current.batch_count;
	total.instanced_batch_count += current.instanced_batch_count;
	total.single_draw_batch_count += current.single_draw_batch_count;
	total.submitted_instance_count += current.submitted_instance_count;
	total.saved_draw_call_count += current.saved_draw_call_count;
}

void accumulate_upload_stats(FrameUploadStats &total, const FrameUploadStats &current) noexcept {
	total.mesh_create_count += current.mesh_create_count;
	total.mesh_update_count += current.mesh_update_count;
	total.mesh_destroy_count += current.mesh_destroy_count;
	total.material_upload_count += current.material_upload_count;
	total.texture_upload_count += current.texture_upload_count;
	total.skipped_clean_resource_count += current.skipped_clean_resource_count;
	total.uploaded_vertex_bytes += current.uploaded_vertex_bytes;
	total.uploaded_index_bytes += current.uploaded_index_bytes;
	total.uploaded_texture_bytes += current.uploaded_texture_bytes;
}

[[nodiscard]] std::uint32_t upload_delta(std::uint32_t current, std::uint32_t previous) noexcept {
	return current >= previous ? current - previous : current;
}

[[nodiscard]] std::uint64_t upload_delta(std::uint64_t current, std::uint64_t previous) noexcept {
	return current >= previous ? current - previous : current;
}

[[nodiscard]] FrameUploadStats subtract_upload_stats(
		const FrameUploadStats &current,
		const FrameUploadStats &previous) noexcept {
	return FrameUploadStats{
		.mesh_create_count = upload_delta(current.mesh_create_count, previous.mesh_create_count),
		.mesh_update_count = upload_delta(current.mesh_update_count, previous.mesh_update_count),
		.mesh_destroy_count = upload_delta(current.mesh_destroy_count, previous.mesh_destroy_count),
		.material_upload_count = upload_delta(current.material_upload_count, previous.material_upload_count),
		.texture_upload_count = upload_delta(current.texture_upload_count, previous.texture_upload_count),
		.skipped_clean_resource_count = upload_delta(
				current.skipped_clean_resource_count,
				previous.skipped_clean_resource_count),
		.uploaded_vertex_bytes = upload_delta(current.uploaded_vertex_bytes, previous.uploaded_vertex_bytes),
		.uploaded_index_bytes = upload_delta(current.uploaded_index_bytes, previous.uploaded_index_bytes),
		.uploaded_texture_bytes = upload_delta(current.uploaded_texture_bytes, previous.uploaded_texture_bytes),
	};
}

[[nodiscard]] FrameUploadStats runtime_upload_stats(
		const GpuMeshCache &mesh_cache,
		const GpuMaterialCache &material_cache) noexcept {
	FrameUploadStats stats;
	accumulate_upload_stats(stats, mesh_cache.upload_stats());
	accumulate_upload_stats(stats, material_cache.upload_stats());
	return stats;
}

[[nodiscard]] std::vector<DrawPacket> collect_lit_packets(const PbrPassPackets &packets) {
	std::vector<DrawPacket> collected;
	collected.reserve(packets.draw_packet_count());
	collected.insert(collected.end(), packets.opaque.begin(), packets.opaque.end());
	collected.insert(collected.end(), packets.alpha_cutout.begin(), packets.alpha_cutout.end());
	collected.insert(collected.end(), packets.transparent.begin(), packets.transparent.end());
	return collected;
}

} // namespace

struct GpuDevice::Impl {
	GpuFrameResources frame_resources;
	MaterialSystem material_system;
	GpuMeshCache mesh_cache;
	GpuProgramCache program_cache;
	GpuMaterialCache material_cache;
	GpuOverlayRenderer overlay_renderer;
	GpuPicking picking;
	ViewportResources viewport_resources;
	PostProcessResources post_resources;
	RenderMaterialHandle default_opaque_material;
	RenderProgramHandle overlay_program;
	RenderProgramHandle overlay_line_program;
	RenderProgramHandle picking_program;
	RenderProgramHandle tone_map_program;
	RenderProgramHandle present_program;
	std::optional<FrameStats> latest_frame_stats;
	std::vector<RenderPassStats> latest_pass_stats;
	FrameUploadStats previous_runtime_upload_stats;
	std::uint64_t frame_index = 0;
	std::uint32_t completed_bgfx_frame = 0;
	int stats_frame_count = 0;
	double last_stats_seconds = 0.0;
};

GpuDevice::GpuDevice() : impl_(std::make_unique<Impl>()) {
}

GpuDevice::~GpuDevice() {
	shutdown();
}

bool GpuDevice::initialize(const RendererConfig &config, const NativeSurfaceDescriptor &surface) {
	shutdown();

	status_ = {};
	selected_backend_ = std::nullopt;
	config_ = config;
	extent_ = surface.extent;
	impl_->previous_runtime_upload_stats = {};

	if (!is_valid_native_surface_descriptor(surface)) {
		push_diagnostic(status_, RendererDiagnostic{
										 .severity = RendererDiagnosticSeverity::Fatal,
										 .code = RendererDiagnosticCode::SurfaceUnavailable,
										 .message = "Native viewport surface is unavailable.",
										 .detail = "Crimson requires a platform, native window handle, and non-zero extent.",
										 .subsystem = "gpu",
										 .resource_name = "NativeSurface",
								 });
		return false;
	}

	std::vector<GraphicsBackend> supported_backends = supported_runtime_backends();
	selected_backend_ = choose_backend(surface.platform, config.backend_preference, supported_backends);
	if (!selected_backend_) {
		push_diagnostic(status_, make_backend_unsupported_diagnostic(surface.platform, config.backend_preference));
		return false;
	}

	bgfx::Init init;
	init.type = to_bgfx_renderer_type(*selected_backend_);
	init.platformData = to_bgfx_platform_data(surface);
	init.resolution.width = surface.extent.width_px;
	init.resolution.height = surface.extent.height_px;
	init.resolution.reset = reset_flags_for_config(config);
	init.debug = config.enable_debug_text;

	if (!bgfx::init(init)) {
		push_diagnostic(status_, RendererDiagnostic{
										 .severity = RendererDiagnosticSeverity::Fatal,
										 .code = RendererDiagnosticCode::RuntimeInitializationFailed,
										 .message = "Crimson failed to initialize the graphics runtime.",
										 .detail = std::string("Requested backend=") + std::string(graphics_backend_name(*selected_backend_)),
										 .subsystem = "gpu",
										 .resource_name = "GpuDevice",
								 });
		selected_backend_ = std::nullopt;
		return false;
	}

	status_.initialized = true;
	status_.backend_name = bgfx::getRendererName(bgfx::getRendererType());
	bgfx::setDebug(config.enable_debug_text ? BGFX_DEBUG_TEXT : BGFX_DEBUG_NONE);
	bgfx::setViewClear(kBackgroundView, BGFX_CLEAR_NONE, kViewportClearColor, 1.0F, 0);

	const bgfx::Caps *runtime_caps = bgfx::getCaps();
	if (runtime_caps == nullptr) {
		push_diagnostic(status_, RendererDiagnostic{
										 .severity = RendererDiagnosticSeverity::Fatal,
										 .code = RendererDiagnosticCode::CapabilityMissing,
										 .message = "Crimson could not query graphics runtime capabilities.",
										 .subsystem = "gpu",
										 .resource_name = "RuntimeCapabilities",
								 });
		bgfx::shutdown();
		status_.initialized = false;
		selected_backend_ = std::nullopt;
		return false;
	}

	GpuCapabilityValidation capabilities = validate_v1_capabilities(
			runtime_capabilities_from_bgfx(*runtime_caps, config),
			config.display_conversion);
	status_.capabilities = std::move(capabilities.statuses);
	for (RendererDiagnostic &diagnostic : capabilities.diagnostics) {
		push_diagnostic(status_, std::move(diagnostic));
	}
	if (has_error_diagnostic(status_)) {
		bgfx::shutdown();
		status_.initialized = false;
		selected_backend_ = std::nullopt;
		return false;
	}

	impl_->material_system = MaterialSystem{};
	const BaseShaderDefinition *default_definition = impl_->material_system.registry().find(BaseShaderId::OpaquePbr);
	if (default_definition == nullptr) {
		push_diagnostic(status_, RendererDiagnostic{
									 .severity = RendererDiagnosticSeverity::Fatal,
									 .code = RendererDiagnosticCode::MaterialSchemaInvalid,
									 .message = "Crimson cannot create the default Quader material because OpaquePbr is missing.",
									 .subsystem = "gpu.material",
									 .resource_name = "DefaultQuaderMaterial",
							 });
		bgfx::shutdown();
		status_.initialized = false;
		selected_backend_ = std::nullopt;
		return false;
	}

	auto default_material = impl_->material_system.create_material(
			make_default_quader_material_instance(*default_definition, {}));
	if (!default_material) {
		push_material_error(status_, std::move(default_material).error());
		bgfx::shutdown();
		status_.initialized = false;
		selected_backend_ = std::nullopt;
		return false;
	}
	impl_->default_opaque_material = default_material.value();

	ShaderLibrary shader_library(config.shader_root);
	const ShaderTarget kShaderTarget = shader_target_for_backend(*selected_backend_);
	(void)impl_->program_cache.load_program(shader_library, ShaderProgramId::OpaquePbr, kShaderTarget, status_);
	(void)impl_->program_cache.load_program(shader_library, ShaderProgramId::AlphaCutoutPbr, kShaderTarget, status_);
	(void)impl_->program_cache.load_program(shader_library, ShaderProgramId::TransparentPbr, kShaderTarget, status_);
	(void)impl_->program_cache.load_program(shader_library, ShaderProgramId::UnlitSurface, kShaderTarget, status_);
	impl_->overlay_program = impl_->program_cache.load_program(
			shader_library,
			ShaderProgramId::OverlayUnlit,
			kShaderTarget,
			status_);
	impl_->overlay_line_program = impl_->program_cache.load_program(
			shader_library,
			ShaderProgramId::OverlayLine,
			kShaderTarget,
			status_);
	impl_->picking_program = impl_->program_cache.load_program(
			shader_library,
			ShaderProgramId::Picking,
			kShaderTarget,
			status_);
	impl_->tone_map_program = impl_->program_cache.load_program(
			shader_library,
			ShaderProgramId::ToneMap,
			kShaderTarget,
			status_);
	impl_->present_program = impl_->program_cache.load_program(
			shader_library,
			ShaderProgramId::Present,
			kShaderTarget,
			status_);
	if (!impl_->material_cache.initialize(config, status_) || has_error_diagnostic(status_)) {
		impl_->material_cache.shutdown();
		impl_->program_cache.clear();
		impl_->mesh_cache.clear();
		bgfx::shutdown();
		status_.initialized = false;
		selected_backend_ = std::nullopt;
		return false;
	}
	auto default_albedo_bound = impl_->material_system.bind_texture(
			impl_->default_opaque_material,
			"base_color",
			impl_->material_cache.default_albedo_texture_handle());
	if (!default_albedo_bound) {
		push_material_error(status_, std::move(default_albedo_bound).error());
		impl_->material_cache.shutdown();
		impl_->program_cache.clear();
		impl_->mesh_cache.clear();
		bgfx::shutdown();
		status_.initialized = false;
		selected_backend_ = std::nullopt;
		return false;
	}

	if (!impl_->overlay_renderer.initialize(status_) || has_error_diagnostic(status_)) {
		impl_->overlay_renderer.shutdown();
		impl_->material_cache.shutdown();
		impl_->program_cache.clear();
		impl_->mesh_cache.clear();
		bgfx::shutdown();
		status_.initialized = false;
		selected_backend_ = std::nullopt;
		return false;
	}

	if (!impl_->picking.initialize(picking_target_format_from_status(status_), status_) || has_error_diagnostic(status_)) {
		impl_->picking.shutdown();
		impl_->overlay_renderer.shutdown();
		impl_->material_cache.shutdown();
		impl_->program_cache.clear();
		impl_->mesh_cache.clear();
		bgfx::shutdown();
		status_.initialized = false;
		selected_backend_ = std::nullopt;
		return false;
	}

	if (!create_viewport_resources(impl_->viewport_resources, status_)) {
		impl_->picking.shutdown();
		impl_->overlay_renderer.shutdown();
		impl_->material_cache.shutdown();
		impl_->program_cache.clear();
		impl_->mesh_cache.clear();
		bgfx::shutdown();
		status_.initialized = false;
		selected_backend_ = std::nullopt;
		return false;
	}

	if (!create_post_process_resources(impl_->post_resources, status_)) {
		destroy_viewport_resources(impl_->viewport_resources);
		impl_->picking.shutdown();
		impl_->overlay_renderer.shutdown();
		impl_->material_cache.shutdown();
		impl_->program_cache.clear();
		impl_->mesh_cache.clear();
		bgfx::shutdown();
		status_.initialized = false;
		selected_backend_ = std::nullopt;
		return false;
	}

	return true;
}

void GpuDevice::shutdown() noexcept {
	if (!status_.initialized) {
		return;
	}

	destroy_post_process_resources(impl_->post_resources);
	destroy_viewport_resources(impl_->viewport_resources);
	impl_->picking.shutdown();
	impl_->overlay_renderer.shutdown();
	impl_->material_cache.shutdown();
	impl_->program_cache.clear();
	impl_->mesh_cache.clear();
	impl_->material_system = MaterialSystem{};
	impl_->default_opaque_material = {};
	impl_->overlay_program = {};
	impl_->overlay_line_program = {};
	impl_->picking_program = {};
	impl_->tone_map_program = {};
	impl_->present_program = {};
	impl_->frame_resources.clear();
	bgfx::shutdown();
	status_.initialized = false;
	selected_backend_ = std::nullopt;
	impl_->latest_frame_stats = std::nullopt;
	impl_->previous_runtime_upload_stats = {};
}

bool GpuDevice::resize(const ViewportExtent &extent) {
	if (!status_.initialized) {
		push_diagnostic(status_, RendererDiagnostic{
										 .severity = RendererDiagnosticSeverity::Error,
										 .code = RendererDiagnosticCode::ResizeFailed,
										 .message = "Cannot resize Crimson before the GPU device is initialized.",
										 .subsystem = "gpu",
										 .resource_name = "Backbuffer",
								 });
		return false;
	}

	if (!is_valid_extent(extent)) {
		push_diagnostic(status_, RendererDiagnostic{
										 .severity = RendererDiagnosticSeverity::Error,
										 .code = RendererDiagnosticCode::ResizeFailed,
										 .message = "Cannot resize Crimson to an invalid viewport extent.",
										 .subsystem = "gpu",
										 .resource_name = "Backbuffer",
								 });
		return false;
	}

	extent_ = extent;
	bgfx::reset(extent.width_px, extent.height_px, reset_flags_for_config(config_));
	return true;
}

quader::foundation::Result<FrameRenderResult, RendererDiagnostic> GpuDevice::render_frame(
		const FrameSnapshot &snapshot,
		const RenderGraph &graph) {
	const auto kFrameStart = std::chrono::steady_clock::now();
	FrameTimingStats timing_stats;

	if (!status_.initialized) {
		return quader::foundation::Result<FrameRenderResult, RendererDiagnostic>::failure(RendererDiagnostic{
				.severity = RendererDiagnosticSeverity::Error,
				.code = RendererDiagnosticCode::InvalidFrameSnapshot,
				.message = "Cannot render before Crimson is initialized.",
				.subsystem = "gpu",
				.resource_name = "GpuDevice",
				.frame_index = snapshot.frame_index(),
		});
	}

	if (!is_valid_extent(snapshot.target_extent()) || snapshot.views().empty()) {
		return quader::foundation::Result<FrameRenderResult, RendererDiagnostic>::failure(
				invalid_snapshot_diagnostic(
						"Frame snapshots require a valid extent and at least one render view.",
						snapshot.frame_index()));
	}

	const ViewportExtent kTargetExtent = snapshot.target_extent();
	if (kTargetExtent.width_px != extent_.width_px || kTargetExtent.height_px != extent_.height_px || kTargetExtent.device_pixel_ratio != extent_.device_pixel_ratio) {
		if (!resize(kTargetExtent)) {
			const RendererStatus kCurrentStatus = status();
			RendererDiagnostic diagnostic{
				.severity = RendererDiagnosticSeverity::Error,
				.code = RendererDiagnosticCode::ResizeFailed,
				.message = "Crimson failed to resize before rendering.",
			};
			if (!kCurrentStatus.diagnostics.empty()) {
				diagnostic = kCurrentStatus.diagnostics.back();
			}
			return quader::foundation::Result<FrameRenderResult, RendererDiagnostic>::failure(std::move(diagnostic));
		}
	}

	for (const RenderView &view : snapshot.views()) {
		if (!is_valid_render_view(view)) {
			return quader::foundation::Result<FrameRenderResult, RendererDiagnostic>::failure(
					invalid_snapshot_diagnostic("A render view has an invalid rectangle.", snapshot.frame_index()));
		}
	}
	const auto kUploadStart = std::chrono::steady_clock::now();
	if (!impl_->frame_resources.synchronize(graph, status_)) {
		RendererDiagnostic diagnostic{
			.severity = RendererDiagnosticSeverity::Error,
			.code = RendererDiagnosticCode::ResourceLifetimeError,
			.message = "Crimson failed to synchronize graph GPU frame resources.",
			.subsystem = "gpu.frame_resources",
			.resource_name = "V1FrameTargets",
			.frame_index = snapshot.frame_index(),
		};
		if (!status_.diagnostics.empty()) {
			diagnostic = status_.diagnostics.back();
			diagnostic.frame_index = snapshot.frame_index();
		}
		return quader::foundation::Result<FrameRenderResult, RendererDiagnostic>::failure(std::move(diagnostic));
	}
	timing_stats.cpu_resource_upload_us += elapsed_us_since(kUploadStart);

	const auto kWidthPx = safe_view_dimension(kTargetExtent.width_px);
	const auto kHeightPx = safe_view_dimension(kTargetExtent.height_px);
	bgfx::setViewRect(kBackgroundView, 0, 0, kWidthPx, kHeightPx);
	bgfx::setViewClear(kBackgroundView, BGFX_CLEAR_NONE, kViewportClearColor, 1.0F, 0);
	bgfx::touch(kBackgroundView);

	const bgfx::FrameBufferHandle kHdrSceneFramebuffer = impl_->frame_resources.hdr_scene_framebuffer();
	const bgfx::FrameBufferHandle kSceneDepthFramebuffer = impl_->frame_resources.scene_depth_framebuffer();
	const bgfx::FrameBufferHandle kToneMappedColorFramebuffer =
			impl_->frame_resources.tone_mapped_color_framebuffer();
	const bgfx::FrameBufferHandle kToneMappedFramebuffer = impl_->frame_resources.tone_mapped_framebuffer();
	const bgfx::TextureHandle kHdrSceneTexture = impl_->frame_resources.texture(kHdrSceneColorTargetName);
	const bgfx::TextureHandle kToneMappedTexture = impl_->frame_resources.texture(kToneMappedColorTargetName);

	GpuPickingFrameResult picking_frame;
	if (impl_->picking.ready()) {
		picking_frame = impl_->picking.submit_frame_requests(
				snapshot,
				impl_->mesh_cache,
				impl_->program_cache,
				impl_->picking_program,
				impl_->completed_bgfx_frame);
	}

	FrameQueueStats queue_stats;
	FrameCullingStats culling_stats;
	FramePacketStats packet_stats;
	FrameInstancingStats instancing_stats;
	FrameUploadStats upload_stats;
	const bool kDrawOverlayCommands = snapshot.viewport_settings().draw_overlays &&
			snapshot.viewport_settings().draw_grid_overlay &&
			impl_->overlay_renderer.ready();
	std::optional<OverlayDrawLists> overlay_draw_lists;
	if (kDrawOverlayCommands) {
		overlay_draw_lists = OverlaySystem{}.prepare(
				snapshot.overlays(),
				snapshot.grid_overlay_payloads(),
				snapshot.line_overlay_payloads(),
				snapshot.triangle_overlay_payloads(),
				snapshot.point_overlay_payloads());
		const std::uint32_t kOverlayPacketCount = size_to_u32(overlay_draw_lists->command_count());
		queue_stats.draw_packet_count += kOverlayPacketCount;
		packet_stats.draw_packet_count += kOverlayPacketCount;
		packet_stats.overlay_packet_count += kOverlayPacketCount;
	}
	for (const RenderMeshUpload &upload : snapshot.mesh_uploads()) {
		if (!impl_->mesh_cache.upload_mesh(upload.desc(), status_)) {
			RendererDiagnostic diagnostic{
				.severity = RendererDiagnosticSeverity::Error,
				.code = RendererDiagnosticCode::ResourceCreationFailed,
				.message = "Crimson failed to upload a frame mesh.",
				.subsystem = "gpu.mesh",
				.resource_name = "FrameMeshUpload",
				.frame_index = snapshot.frame_index(),
			};
			if (!status_.diagnostics.empty()) {
				diagnostic = status_.diagnostics.back();
				diagnostic.frame_index = snapshot.frame_index();
			}
			return quader::foundation::Result<FrameRenderResult, RendererDiagnostic>::failure(std::move(diagnostic));
		}
	}
	if (impl_->viewport_resources.ready) {
		const auto kSubmitStart = std::chrono::steady_clock::now();
		for (std::size_t index = 0; index < snapshot.views().size(); ++index) {
			const RenderView &view = snapshot.views()[index];
			const bgfx::ViewId kDepthPrepassViewId = depth_view_id(index);
			configure_scene_view(
					kDepthPrepassViewId,
					view,
					"DepthPrepass:" + view.debug_name,
					ViewClearMode::Depth,
					kSceneDepthFramebuffer);

			const auto kPacketBuildStart = std::chrono::steady_clock::now();
			PbrPassPackets packets = prepare_pbr_pass_packets(
					snapshot.objects(),
					impl_->material_system.registry(),
					impl_->material_system,
					view.camera,
					impl_->default_opaque_material,
					view_aspect_ratio(view));
			timing_stats.cpu_packet_build_us += elapsed_us_since(kPacketBuildStart);
			queue_stats.draw_packet_count += size_to_u32(packets.draw_packet_count());
			accumulate_culling_stats(culling_stats, packets.culling);
			accumulate_packet_stats(packet_stats, packets.packets);
			const std::vector<DrawPacket> kLitPackets = collect_lit_packets(packets);
			const std::vector<InstanceBatch> kBatches = build_instance_batches(kLitPackets);
			accumulate_instancing_stats(instancing_stats, make_instancing_stats(kBatches));

			const bgfx::ViewId kGridUnderlayViewId = scene_view_id(index, 0);
			configure_scene_view(
					kGridUnderlayViewId,
					view,
					"GridSceneUnderlayPass:" + view.debug_name,
					ViewClearMode::Color,
					kHdrSceneFramebuffer);
			const bgfx::ViewId kOpaqueViewId = scene_view_id(index, 1);
			configure_scene_view(
					kOpaqueViewId,
					view,
					"OpaquePbrPass:" + view.debug_name,
					ViewClearMode::None,
					kHdrSceneFramebuffer);
			const bgfx::ViewId kCutoutViewId = scene_view_id(index, 2);
			configure_scene_view(
					kCutoutViewId,
					view,
					"AlphaCutoutPbrPass:" + view.debug_name,
					ViewClearMode::None,
					kHdrSceneFramebuffer);
			const bgfx::ViewId kTransparentViewId = scene_view_id(index, 3);
			configure_scene_view(
					kTransparentViewId,
					view,
					"TransparentPbrPass:" + view.debug_name,
					ViewClearMode::None,
					kHdrSceneFramebuffer);

			if (overlay_draw_lists.has_value()) {
				queue_stats.overlay_draw_count += submit_grid_scene_underlay_bucket(
						overlay_draw_lists->always_on_top,
						view,
						kGridUnderlayViewId,
						impl_->overlay_renderer,
						impl_->program_cache,
						impl_->overlay_program);
			}

			if (snapshot.viewport_settings().draw_lit_surfaces) {
				(void)submit_pbr_packets(
						kDepthPrepassViewId,
						packets.opaque,
						impl_->material_system.registry(),
						impl_->material_system,
						impl_->mesh_cache,
						impl_->program_cache,
						impl_->material_cache,
						impl_->viewport_resources,
						PbrSubmitMode::DepthOnly);
				queue_stats.opaque_draw_count += submit_pbr_packets(
						kOpaqueViewId,
						packets.opaque,
						impl_->material_system.registry(),
						impl_->material_system,
						impl_->mesh_cache,
						impl_->program_cache,
						impl_->material_cache,
						impl_->viewport_resources);
				queue_stats.alpha_cutout_draw_count += submit_pbr_packets(
						kCutoutViewId,
						packets.alpha_cutout,
						impl_->material_system.registry(),
						impl_->material_system,
						impl_->mesh_cache,
						impl_->program_cache,
						impl_->material_cache,
						impl_->viewport_resources);
				queue_stats.transparent_draw_count += submit_pbr_packets(
						kTransparentViewId,
						packets.transparent,
						impl_->material_system.registry(),
						impl_->material_system,
						impl_->mesh_cache,
						impl_->program_cache,
						impl_->material_cache,
						impl_->viewport_resources);
			}
		}

		const bgfx::ProgramHandle kToneMapProgram = impl_->program_cache.program(impl_->tone_map_program);
		const std::array<float, 4> kToneMapParams = {
			tone_mapper_shader_id(snapshot.viewport_settings().tone_mapper),
			exposure_multiplier_from_ev100(
					snapshot.viewport_settings().exposure_ev100,
					snapshot.viewport_settings().exposure_compensation_ev),
			0.0F,
			0.0F,
		};
		queue_stats.draw_packet_count += submit_fullscreen_pass(
				kToneMapView,
				"ToneMapPass",
				kTargetExtent,
				kToneMappedColorFramebuffer,
				kHdrSceneTexture,
				kToneMapProgram,
				impl_->post_resources.hdr_scene_sampler.get(),
				impl_->post_resources.tone_map_params_uniform.get(),
				kToneMapParams,
				impl_->post_resources);

		if (overlay_draw_lists.has_value()) {
			const OverlayDrawLists &kOverlayDrawLists = *overlay_draw_lists;
			queue_stats.overlay_draw_count += submit_grid_overlay_bucket(
					kOverlayDrawLists.depth_tested,
					OverlayDepthMode::DepthTested,
					snapshot.views(),
					impl_->overlay_renderer,
					impl_->program_cache,
					impl_->overlay_program,
					kToneMappedFramebuffer);
			queue_stats.overlay_draw_count += submit_grid_overlay_bucket(
					kOverlayDrawLists.xray,
					OverlayDepthMode::XRay,
					snapshot.views(),
					impl_->overlay_renderer,
					impl_->program_cache,
					impl_->overlay_program,
					kToneMappedFramebuffer);
			queue_stats.overlay_draw_count += submit_grid_overlay_bucket(
					kOverlayDrawLists.always_on_top,
					OverlayDepthMode::AlwaysOnTop,
					snapshot.views(),
					impl_->overlay_renderer,
					impl_->program_cache,
					impl_->overlay_program,
					kToneMappedFramebuffer);
			queue_stats.overlay_draw_count += submit_triangle_overlay_bucket(
					kOverlayDrawLists.depth_tested,
					OverlayDepthMode::DepthTested,
					snapshot.views(),
					impl_->overlay_renderer,
					impl_->program_cache,
					impl_->overlay_line_program,
					kToneMappedFramebuffer);
			queue_stats.overlay_draw_count += submit_triangle_overlay_bucket(
					kOverlayDrawLists.xray,
					OverlayDepthMode::XRay,
					snapshot.views(),
					impl_->overlay_renderer,
					impl_->program_cache,
					impl_->overlay_line_program,
					kToneMappedFramebuffer);
			queue_stats.overlay_draw_count += submit_triangle_overlay_bucket(
					kOverlayDrawLists.always_on_top,
					OverlayDepthMode::AlwaysOnTop,
					snapshot.views(),
					impl_->overlay_renderer,
					impl_->program_cache,
					impl_->overlay_line_program,
					kToneMappedFramebuffer);
			queue_stats.overlay_draw_count += submit_line_overlay_bucket(
					kOverlayDrawLists.depth_tested,
					OverlayDepthMode::DepthTested,
					kOverlayDrawLists.source_wire_depth_stamps,
					snapshot.views(),
					impl_->overlay_renderer,
					impl_->program_cache,
					impl_->overlay_line_program,
					kToneMappedFramebuffer);
			queue_stats.overlay_draw_count += submit_line_overlay_bucket(
					kOverlayDrawLists.xray,
					OverlayDepthMode::XRay,
					kOverlayDrawLists.source_wire_depth_stamps,
					snapshot.views(),
					impl_->overlay_renderer,
					impl_->program_cache,
					impl_->overlay_line_program,
					kToneMappedFramebuffer);
			queue_stats.overlay_draw_count += submit_line_overlay_bucket(
					kOverlayDrawLists.always_on_top,
					OverlayDepthMode::AlwaysOnTop,
					kOverlayDrawLists.source_wire_depth_stamps,
					snapshot.views(),
					impl_->overlay_renderer,
					impl_->program_cache,
					impl_->overlay_line_program,
					kToneMappedFramebuffer);
			queue_stats.overlay_draw_count += submit_point_overlay_bucket(
					kOverlayDrawLists.depth_tested,
					OverlayDepthMode::DepthTested,
					kOverlayDrawLists.source_wire_depth_stamps,
					snapshot.views(),
					impl_->overlay_renderer,
					impl_->program_cache,
					impl_->overlay_line_program,
					kToneMappedFramebuffer);
			queue_stats.overlay_draw_count += submit_point_overlay_bucket(
					kOverlayDrawLists.xray,
					OverlayDepthMode::XRay,
					kOverlayDrawLists.source_wire_depth_stamps,
					snapshot.views(),
					impl_->overlay_renderer,
					impl_->program_cache,
					impl_->overlay_line_program,
					kToneMappedFramebuffer);
			queue_stats.overlay_draw_count += submit_point_overlay_bucket(
					kOverlayDrawLists.always_on_top,
					OverlayDepthMode::AlwaysOnTop,
					kOverlayDrawLists.source_wire_depth_stamps,
					snapshot.views(),
					impl_->overlay_renderer,
					impl_->program_cache,
					impl_->overlay_line_program,
					kToneMappedFramebuffer);
		}

		const bgfx::ProgramHandle kPresentProgram = impl_->program_cache.program(impl_->present_program);
		const std::array<float, 4> kPresentParams = {
			manual_final_srgb_conversion_enabled(status_, config_) ? 1.0F : 0.0F,
			0.0F,
			0.0F,
			0.0F,
		};
		queue_stats.draw_packet_count += submit_fullscreen_pass(
				kPresentView,
				"PresentPass",
				kTargetExtent,
				BGFX_INVALID_HANDLE,
				kToneMappedTexture,
				kPresentProgram,
				impl_->post_resources.tone_mapped_sampler.get(),
				impl_->post_resources.present_params_uniform.get(),
				kPresentParams,
				impl_->post_resources);
		timing_stats.cpu_submit_us += elapsed_us_since(kSubmitStart);
	}

	impl_->completed_bgfx_frame = bgfx::frame();
	timing_stats.cpu_frame_us = elapsed_us_since(kFrameStart);

	++impl_->frame_index;
	++impl_->stats_frame_count;

	const FrameUploadStats kCurrentRuntimeUploadStats =
			runtime_upload_stats(impl_->mesh_cache, impl_->material_cache);
	upload_stats = subtract_upload_stats(
			kCurrentRuntimeUploadStats,
			impl_->previous_runtime_upload_stats);
	impl_->previous_runtime_upload_stats = kCurrentRuntimeUploadStats;

	double fps = impl_->latest_frame_stats ? impl_->latest_frame_stats->fps : 0.0;
	const double kElapsedSeconds = snapshot.elapsed_seconds();
	if (kElapsedSeconds - impl_->last_stats_seconds >= 1.0) {
		const double kDelta = kElapsedSeconds - impl_->last_stats_seconds;
		fps = kDelta > 0.0 ? static_cast<double>(impl_->stats_frame_count) / kDelta : 0.0;
		impl_->last_stats_seconds = kElapsedSeconds;
		impl_->stats_frame_count = 0;
	}

	impl_->latest_frame_stats = make_frame_stats(FrameStatsInput{
			.frame_index = impl_->frame_index,
			.width_px = kTargetExtent.width_px,
			.height_px = kTargetExtent.height_px,
			.view_count = size_to_u32(snapshot.views().size()),
			.graph_pass_count = size_to_u32(graph.passes().size()),
			.visible_object_count = culling_stats.visible_object_count,
			.queues = queue_stats,
			.picking_request_count = size_to_u32(snapshot.picking_requests().size()),
			.picking_readback_count = picking_frame.scheduled_readbacks,
			.resources = frame_resource_stats(
					impl_->frame_resources,
					impl_->material_system,
					impl_->mesh_cache,
					impl_->material_cache,
					impl_->program_cache),
			.culling = culling_stats,
			.packets = packet_stats,
			.instancing = instancing_stats,
			.uploads = upload_stats,
			.timings = timing_stats,
			.diagnostic_count = size_to_u32(status_.diagnostics.size()),
			.fps = fps,
	});
	impl_->latest_pass_stats = make_render_pass_stats(graph, &*impl_->latest_frame_stats);

	return quader::foundation::Result<FrameRenderResult, RendererDiagnostic>::success(FrameRenderResult{
			.stats = *impl_->latest_frame_stats,
			.completed_picking_results = std::move(picking_frame.completed_results),
	});
}

std::optional<FrameStats> GpuDevice::latest_frame_stats() const noexcept {
	return impl_->latest_frame_stats;
}

std::vector<RenderPassStats> GpuDevice::latest_pass_stats() const {
	return impl_->latest_pass_stats;
}

bool GpuDevice::initialized() const noexcept {
	return status_.initialized;
}

std::optional<GraphicsBackend> GpuDevice::selected_backend() const noexcept {
	return selected_backend_;
}

const RendererStatus &GpuDevice::status() const noexcept {
	return status_;
}

} // namespace crimson::gpu
