#include "crimson/gpu/gpu_frame_resources.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <optional>
#include <utility>

namespace crimson::gpu {
namespace {

[[nodiscard]] bool descriptors_match(const RenderResourceDesc &left, const RenderResourceDesc &right) noexcept {
	return left.name == right.name && left.format == right.format && left.extent.width_px == right.extent.width_px && left.extent.height_px == right.extent.height_px && left.extent.device_pixel_ratio == right.extent.device_pixel_ratio && left.external == right.external && left.resize_dependent == right.resize_dependent && left.sample_count == right.sample_count;
}

[[nodiscard]] std::uint16_t texture_dimension(std::uint32_t value) noexcept {
	return static_cast<std::uint16_t>(std::clamp<std::uint32_t>(value, 1U, 65535U));
}

[[nodiscard]] std::optional<bgfx::TextureFormat::Enum> bgfx_texture_format(RenderResourceFormat format) noexcept {
	switch (format) {
		case RenderResourceFormat::Rgba8Unorm:
			return bgfx::TextureFormat::RGBA8;
		case RenderResourceFormat::Rgba16Float:
			return bgfx::TextureFormat::RGBA16F;
		case RenderResourceFormat::D24S8:
			return bgfx::TextureFormat::D24S8;
		case RenderResourceFormat::D32Float:
			return bgfx::TextureFormat::D32F;
		case RenderResourceFormat::R32Uint:
			return bgfx::TextureFormat::R32U;
		case RenderResourceFormat::BackbufferColor:
		case RenderResourceFormat::BackbufferDepth:
			return std::nullopt;
	}

	return std::nullopt;
}

[[nodiscard]] std::uint64_t texture_flags_for(const RenderResourceDesc &desc) noexcept {
	std::uint64_t flags = BGFX_TEXTURE_RT | BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP;
	if (desc.format == RenderResourceFormat::R32Uint) {
		flags |= BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MAG_POINT | BGFX_SAMPLER_MIP_POINT;
	}
	return flags;
}

void push_frame_resource_error(
		RendererStatus &status,
		RendererDiagnosticCode code,
		std::string message,
		std::string detail,
		std::string resource_name) {
	status.diagnostics.push_back(RendererDiagnostic{
			.severity = RendererDiagnosticSeverity::Error,
			.code = code,
			.message = std::move(message),
			.detail = std::move(detail),
			.subsystem = "gpu.frame_resources",
			.resource_name = std::move(resource_name),
	});
}

void name_texture(bgfx::TextureHandle texture, const std::string &name) {
	if (bgfx::isValid(texture)) {
		bgfx::setName(texture, name.c_str());
	}
}

void name_framebuffer(bgfx::FrameBufferHandle framebuffer, const char *name) {
	if (bgfx::isValid(framebuffer)) {
		bgfx::setName(framebuffer, name);
	}
}

[[nodiscard]] GpuFrameTargetRecord *find_mutable(
		std::vector<GpuFrameTargetRecord> &targets,
		std::string_view name) noexcept {
	const auto kIt = std::find_if(targets.begin(), targets.end(), [name](const GpuFrameTargetRecord &record) {
		return record.desc.name == name;
	});
	return kIt == targets.end() ? nullptr : &*kIt;
}

} // namespace

bool GpuFrameTargetRecord::external() const noexcept {
	return desc.external;
}

bool GpuFrameTargetRecord::runtime_ready() const noexcept {
	return desc.external || (texture.valid() && framebuffer.valid());
}

void GpuFrameResources::synchronize_descriptors(const RenderGraph &graph) {
	const std::uint64_t kGraphGeneration = graph.resize_generation();
	std::vector<GpuFrameTargetRecord> synchronized;
	synchronized.reserve(graph.resources().resources().size());

	for (const RenderResourceRecord &graph_resource : graph.resources().resources()) {
		const auto kExisting = std::find_if(targets_.begin(), targets_.end(), [&](const GpuFrameTargetRecord &record) {
			return record.desc.name == graph_resource.desc.name;
		});

		if (kExisting == targets_.end()) {
			GpuFrameTargetRecord record;
			record.desc = graph_resource.desc;
			record.graph_generation = graph_resource.generation;
			record.recreate_count = 1;
			synchronized.push_back(std::move(record));
			continue;
		}

		GpuFrameTargetRecord record = std::move(*kExisting);
		if (!descriptors_match(record.desc, graph_resource.desc) || record.graph_generation != graph_resource.generation) {
			record.desc = graph_resource.desc;
			record.graph_generation = graph_resource.generation;
			++record.recreate_count;
			record.framebuffer.reset();
			record.texture.reset();
		}
		synchronized.push_back(std::move(record));
	}

	targets_ = std::move(synchronized);
	graph_resize_generation_ = kGraphGeneration;
}

bool GpuFrameResources::synchronize(const RenderGraph &graph, RendererStatus &status) {
	const std::uint64_t kPreviousGeneration = graph_resize_generation_;
	synchronize_descriptors(graph);
	const bool kGenerationChanged = kPreviousGeneration != graph_resize_generation_;

	if (!create_runtime_targets(status)) {
		return false;
	}
	if (kGenerationChanged) {
		hdr_scene_framebuffer_.reset();
		tone_mapped_framebuffer_.reset();
		picking_framebuffer_.reset();
	}
	return recreate_composite_framebuffers(status);
}

void GpuFrameResources::clear() noexcept {
	hdr_scene_framebuffer_.reset();
	tone_mapped_framebuffer_.reset();
	picking_framebuffer_.reset();
	targets_.clear();
	graph_resize_generation_ = 0;
}

std::span<const GpuFrameTargetRecord> GpuFrameResources::targets() const noexcept {
	return targets_;
}

const GpuFrameTargetRecord *GpuFrameResources::find(std::string_view name) const noexcept {
	const auto kIt = std::find_if(targets_.begin(), targets_.end(), [name](const GpuFrameTargetRecord &record) {
		return record.desc.name == name;
	});
	return kIt == targets_.end() ? nullptr : &*kIt;
}

std::uint64_t GpuFrameResources::graph_resize_generation() const noexcept {
	return graph_resize_generation_;
}

bgfx::TextureHandle GpuFrameResources::texture(std::string_view name) const noexcept {
	const GpuFrameTargetRecord *record = find(name);
	return record == nullptr ? bgfx::TextureHandle{ bgfx::kInvalidHandle } : record->texture.get();
}

bgfx::FrameBufferHandle GpuFrameResources::framebuffer(std::string_view name) const noexcept {
	const GpuFrameTargetRecord *record = find(name);
	return record == nullptr ? bgfx::FrameBufferHandle{ bgfx::kInvalidHandle } : record->framebuffer.get();
}

bgfx::FrameBufferHandle GpuFrameResources::scene_depth_framebuffer() const noexcept {
	return framebuffer(kSceneDepthTargetName);
}

bgfx::FrameBufferHandle GpuFrameResources::hdr_scene_framebuffer() const noexcept {
	return hdr_scene_framebuffer_.get();
}

bgfx::FrameBufferHandle GpuFrameResources::tone_mapped_color_framebuffer() const noexcept {
	return framebuffer(kToneMappedColorTargetName);
}

bgfx::FrameBufferHandle GpuFrameResources::tone_mapped_framebuffer() const noexcept {
	return tone_mapped_framebuffer_.get();
}

bgfx::FrameBufferHandle GpuFrameResources::picking_framebuffer() const noexcept {
	return picking_framebuffer_.get();
}

bool GpuFrameResources::create_runtime_targets(RendererStatus &status) {
	bool ok = true;
	for (GpuFrameTargetRecord &record : targets_) {
		if (record.desc.external) {
			continue;
		}
		if (record.texture.valid() && record.framebuffer.valid()) {
			continue;
		}

		const std::optional<bgfx::TextureFormat::Enum> kFormat = bgfx_texture_format(record.desc.format);
		if (!kFormat) {
			push_frame_resource_error(
					status,
					RendererDiagnosticCode::FrameBufferUnsupported,
					"Crimson cannot map a graph frame target to a runtime texture format.",
					render_resource_format_name(record.desc.format),
					record.desc.name);
			ok = false;
			continue;
		}

		record.framebuffer.reset();
		record.texture.reset(bgfx::createTexture2D(
				texture_dimension(record.desc.extent.width_px),
				texture_dimension(record.desc.extent.height_px),
				false,
				1,
				*kFormat,
				texture_flags_for(record.desc)));
		name_texture(record.texture.get(), record.desc.name);

		if (!record.texture.valid()) {
			push_frame_resource_error(
					status,
					RendererDiagnosticCode::ResourceCreationFailed,
					"Crimson failed to create a graph frame target texture.",
					render_resource_format_name(record.desc.format),
					record.desc.name);
			ok = false;
			continue;
		}

		const bgfx::TextureHandle kAttachment = record.texture.get();
		record.framebuffer.reset(bgfx::createFrameBuffer(1, &kAttachment, false));
		name_framebuffer(record.framebuffer.get(), record.desc.name.c_str());

		if (!record.framebuffer.valid()) {
			push_frame_resource_error(
					status,
					RendererDiagnosticCode::FrameBufferUnsupported,
					"Crimson failed to create a graph frame target framebuffer.",
					record.desc.name,
					record.desc.name);
			ok = false;
		}
	}

	return ok;
}

bool GpuFrameResources::recreate_composite_framebuffers(RendererStatus &status) {
	GpuFrameTargetRecord *hdr = find_mutable(targets_, kHdrSceneColorTargetName);
	GpuFrameTargetRecord *depth = find_mutable(targets_, kSceneDepthTargetName);
	GpuFrameTargetRecord *picking = find_mutable(targets_, kPickingIdTargetName);
	GpuFrameTargetRecord *tone = find_mutable(targets_, kToneMappedColorTargetName);

	if (hdr == nullptr || depth == nullptr || picking == nullptr || tone == nullptr) {
		push_frame_resource_error(
				status,
				RendererDiagnosticCode::ResourceLifetimeError,
				"Crimson V1 graph frame targets are incomplete.",
				"Expected HdrSceneColor, SceneDepth, PickingId, and ToneMappedColor.",
				"V1FrameTargets");
		return false;
	}
	if (!hdr->texture.valid() || !depth->texture.valid() || !picking->texture.valid() || !tone->texture.valid()) {
		push_frame_resource_error(
				status,
				RendererDiagnosticCode::ResourceLifetimeError,
				"Crimson cannot create composite framebuffers before graph target textures exist.",
				"One or more V1 frame target textures are invalid.",
				"V1FrameTargets");
		return false;
	}

	if (!hdr_scene_framebuffer_.valid()) {
		const std::array<bgfx::TextureHandle, 2> kTargets = { hdr->texture.get(), depth->texture.get() };
		hdr_scene_framebuffer_.reset(bgfx::createFrameBuffer(
				static_cast<std::uint8_t>(kTargets.size()),
				kTargets.data(),
				false));
		name_framebuffer(hdr_scene_framebuffer_.get(), "HdrSceneColor+SceneDepth");
	}
	if (!tone_mapped_framebuffer_.valid()) {
		const std::array<bgfx::TextureHandle, 2> kTargets = { tone->texture.get(), depth->texture.get() };
		tone_mapped_framebuffer_.reset(bgfx::createFrameBuffer(
				static_cast<std::uint8_t>(kTargets.size()),
				kTargets.data(),
				false));
		name_framebuffer(tone_mapped_framebuffer_.get(), "ToneMappedColor+SceneDepth");
	}
	if (!picking_framebuffer_.valid()) {
		const std::array<bgfx::TextureHandle, 2> kTargets = { picking->texture.get(), depth->texture.get() };
		picking_framebuffer_.reset(bgfx::createFrameBuffer(
				static_cast<std::uint8_t>(kTargets.size()),
				kTargets.data(),
				false));
		name_framebuffer(picking_framebuffer_.get(), "PickingId+SceneDepth");
	}

	const bool kOk = hdr_scene_framebuffer_.valid() && tone_mapped_framebuffer_.valid() && picking_framebuffer_.valid();
	if (!kOk) {
		push_frame_resource_error(
				status,
				RendererDiagnosticCode::FrameBufferUnsupported,
				"Crimson failed to create one or more V1 composite graph framebuffers.",
				"HDR/depth, tone-mapped/depth, or picking/depth framebuffer was invalid.",
				"V1FrameTargets");
	}
	return kOk;
}

} // namespace crimson::gpu
