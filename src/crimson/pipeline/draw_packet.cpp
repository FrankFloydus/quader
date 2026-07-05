/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
#include "crimson/pipeline/draw_packet.hpp"

#include "crimson/pipeline/draw_packet_sort.hpp"
#include "crimson/pipeline/frustum_culler.hpp"

namespace crimson {
namespace {

[[nodiscard]] RenderMaterialHandle material_or_fallback(
		const MaterialSystem &materials,
		RenderMaterialHandle handle,
		RenderMaterialHandle fallback) noexcept {
	return materials.get(handle) == nullptr ? fallback : handle;
}

[[nodiscard]] const BaseShaderDefinition *definition_for_object(
		const BaseShaderRegistry &registry,
		const MaterialSystem &materials,
		const RenderObject &object,
		RenderMaterialHandle resolved_material) noexcept {
	if (const MaterialInstance *material = materials.get(resolved_material)) {
		if (const BaseShaderDefinition *definition = registry.find(material->base_shader_id)) {
			return definition;
		}
	}
	return registry.find(object.base_shader);
}

[[nodiscard]] DrawPacket packet_from_object(
		const RenderObject &object,
		const BaseShaderDefinition &definition,
		RenderMaterialHandle material,
		RenderMeshHandle unit_box_mesh,
		const RenderCamera &camera) {
	const RenderQueue kQueue = object.queue == RenderQueue::PrototypeOpaque
			? RenderQueue::Opaque
			: object.queue;
	const RenderMeshHandle kMesh = is_valid_handle(object.mesh)	 ? object.mesh
			: object.built_in_mesh == BuiltInRenderMesh::UnitBox ? unit_box_mesh
																 : RenderMeshHandle{};

	return DrawPacket{
		.object_id = object.object_id,
		.mesh = kMesh,
		.material = material,
		.base_shader = definition.id,
		.queue = kQueue,
		.program = definition.program,
		.world_from_object = object.world_from_object,
		.world_bounds = object.world_bounds,
		.submesh_index = object.submesh_index,
		.camera_distance_sq = object_camera_distance_sq(object, camera),
		.double_sided = object.pickable && definition.cull_mode == CullMode::None,
	};
}

} // namespace

std::size_t DrawPacketBuildResult::draw_packet_count() const noexcept {
	return opaque.size() + alpha_cutout.size() + transparent.size();
}

DrawPacketBuildResult build_draw_packets(
		std::span<const RenderObject> objects,
		const BaseShaderRegistry &registry,
		const MaterialSystem &materials,
		const RenderCamera &camera,
		RenderMeshHandle unit_box_mesh,
		RenderMaterialHandle fallback_material,
		float view_aspect_ratio) {
	DrawPacketBuildResult result;
	result.opaque.reserve(objects.size());
	result.alpha_cutout.reserve(objects.size());
	result.transparent.reserve(objects.size());
	result.culling.input_object_count = static_cast<std::uint32_t>(objects.size());

	const CameraFrustum kFrustum = make_camera_frustum(camera, view_aspect_ratio);
	for (const RenderObject &object : objects) {
		if (!object.visible) {
			++result.culling.culled_object_count;
			continue;
		}

		const CullingDecision kCulling = cull_object_bounds(kFrustum, object.world_bounds);
		if (kCulling.invalid_bounds) {
			++result.culling.invalid_bounds_count;
		}
		if (!kCulling.visible) {
			++result.culling.culled_object_count;
			continue;
		}

		const RenderMaterialHandle kMaterial = material_or_fallback(materials, object.material, fallback_material);
		const BaseShaderDefinition *definition = definition_for_object(registry, materials, object, kMaterial);
		if (definition == nullptr) {
			continue;
		}
		if (definition->domain != RenderDomain::LitSurface && definition->domain != RenderDomain::TransparentSurface) {
			continue;
		}
		if (!is_valid_handle(object.mesh) && object.built_in_mesh == BuiltInRenderMesh::None) {
			continue;
		}

		DrawPacket packet = packet_from_object(object, *definition, kMaterial, unit_box_mesh, camera);
		switch (packet.queue) {
			case RenderQueue::Opaque:
				result.opaque.push_back(packet);
				break;
			case RenderQueue::AlphaCutout:
				result.alpha_cutout.push_back(packet);
				break;
			case RenderQueue::Transparent:
				result.transparent.push_back(packet);
				break;
			case RenderQueue::PrototypeOpaque:
				packet.queue = RenderQueue::Opaque;
				result.opaque.push_back(packet);
				break;
			case RenderQueue::OverlayDepthTested:
			case RenderQueue::OverlayXRay:
			case RenderQueue::OverlayAlwaysOnTop:
			case RenderQueue::Picking:
				break;
		}
	}

	sort_opaque_draw_packets(result.opaque);
	sort_opaque_draw_packets(result.alpha_cutout);
	sort_transparent_draw_packets(result.transparent);

	result.packets.opaque_packet_count = static_cast<std::uint32_t>(result.opaque.size());
	result.packets.alpha_cutout_packet_count = static_cast<std::uint32_t>(result.alpha_cutout.size());
	result.packets.transparent_packet_count = static_cast<std::uint32_t>(result.transparent.size());
	result.packets.draw_packet_count = static_cast<std::uint32_t>(result.draw_packet_count());
	result.packets.sorted_packet_count = result.packets.draw_packet_count;
	result.culling.visible_object_count = result.packets.draw_packet_count;
	return result;
}

} // namespace crimson
