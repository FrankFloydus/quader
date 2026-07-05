#pragma once

#include "crimson/frame/frame_stats.hpp"
#include "crimson/material/base_shader_registry.hpp"
#include "crimson/material/material_system.hpp"
#include "crimson/scene/render_camera.hpp"
#include "crimson/scene/render_object.hpp"

#include <array>
#include <cstddef>
#include <span>
#include <vector>

namespace crimson {

struct DrawPacket {
	RenderObjectId object_id = 0;
	RenderMeshHandle mesh;
	RenderMaterialHandle material;
	BaseShaderId base_shader = BaseShaderId::OpaquePbr;
	RenderQueue queue = RenderQueue::Opaque;
	ShaderProgramId program = ShaderProgramId::OpaquePbr;
	std::array<float, 16> world_from_object = identity_transform();
	quader::math::Aabb world_bounds;
	std::uint32_t submesh_index = 0;
	float camera_distance_sq = 0.0F;
	bool double_sided = false;
};

struct DrawPacketBuildResult {
	std::vector<DrawPacket> opaque;
	std::vector<DrawPacket> alpha_cutout;
	std::vector<DrawPacket> transparent;
	FrameCullingStats culling;
	FramePacketStats packets;

	[[nodiscard]] std::size_t draw_packet_count() const noexcept;
};

[[nodiscard]] DrawPacketBuildResult build_draw_packets(
		std::span<const RenderObject> objects,
		const BaseShaderRegistry &registry,
		const MaterialSystem &materials,
		const RenderCamera &camera,
		RenderMeshHandle fallback_mesh,
		RenderMaterialHandle fallback_material,
		float view_aspect_ratio = 1.0F);

} // namespace crimson
