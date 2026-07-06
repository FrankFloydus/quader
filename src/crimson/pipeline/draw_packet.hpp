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
#include "crimson/material/base_shader_registry.hpp"
#include "crimson/material/material_system.hpp"
#include "crimson/scene/render_camera.hpp"
#include "crimson/scene/render_object.hpp"

#include <array>
#include <cstddef>
#include <span>
#include <vector>

namespace crimson {

/// CPU-side draw command prepared from render objects and material state.
struct DrawPacket {
	/// Source render object id.
	RenderObjectId object_id = 0;
	/// Mesh resource to draw.
	RenderMeshHandle mesh;
	/// Material resource to bind.
	RenderMaterialHandle material;
	/// Base shader schema used for the draw.
	BaseShaderId base_shader = BaseShaderId::OpaquePbr;
	/// Render queue used for routing and sorting.
	RenderQueue queue = RenderQueue::Opaque;
	/// Shader program used by the draw.
	ShaderProgramId program = ShaderProgramId::OpaquePbr;
	/// Object-to-world transform.
	std::array<float, 16> world_from_object = identity_transform();
	/// World-space bounds for culling and sorting.
	quader::math::Aabb world_bounds;
	/// Submesh index inside the mesh resource.
	std::uint32_t submesh_index = 0;
	/// Squared distance from the active camera.
	float camera_distance_sq = 0.0F;
	/// Whether back-face culling should be disabled for this draw.
	bool double_sided = false;
};

/// Result of building draw packets for one view.
struct DrawPacketBuildResult {
	/// Opaque queue packets.
	std::vector<DrawPacket> opaque;
	/// Alpha-cutout queue packets.
	std::vector<DrawPacket> alpha_cutout;
	/// Transparent queue packets.
	std::vector<DrawPacket> transparent;
	/// Culling counters accumulated while building packets.
	FrameCullingStats culling;
	/// Packet counters accumulated while building packets.
	FramePacketStats packets;

	/**
	 * Return the total packet count across all queues.
	 *
	 * @return Sum of opaque, alpha-cutout, and transparent packet counts.
	 */
	[[nodiscard]] std::size_t draw_packet_count() const noexcept;
};

/**
 * Build draw packets from visible render objects.
 *
 * @param objects Render objects to inspect.
 * @param registry Base shader registry used for shader schema lookup.
 * @param materials Material system used for material lookup.
 * @param camera Camera used for culling and distance sorting.
 * @param fallback_material Material handle substituted when an object has no material.
 * @param view_aspect_ratio View aspect ratio used for frustum culling.
 * @return Queue-separated draw packets and frame counters.
 */
[[nodiscard]] DrawPacketBuildResult build_draw_packets(
		std::span<const RenderObject> objects,
		const BaseShaderRegistry &registry,
		const MaterialSystem &materials,
		const RenderCamera &camera,
		RenderMaterialHandle fallback_material,
		float view_aspect_ratio = 1.0F);

} // namespace crimson
