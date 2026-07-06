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

#include <variant>

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
		const RenderObject &object) noexcept {
	if (const MaterialInstance *material = materials.get(object.material)) {
		if (const BaseShaderDefinition *definition = registry.find(material->base_shader_id)) {
			return definition;
		}
	}
	return registry.find(object.base_shader);
}

[[nodiscard]] bool material_double_sided(const MaterialInstance *material) noexcept {
	if (material == nullptr) {
		return false;
	}

	for (const MaterialParameter &parameter : material->parameters) {
		if (parameter.name == "double_sided") {
			if (const auto *value = std::get_if<bool>(&parameter.value)) {
				return *value;
			}
			return material->overrides.double_sided;
		}
	}
	return material->overrides.double_sided;
}

[[nodiscard]] DrawPacket packet_from_object(
		const RenderObject &object,
		const BaseShaderDefinition &definition,
		const MaterialInstance *material_instance,
		RenderMaterialHandle material,
		const RenderCamera &camera) {
	const RenderQueue kQueue = object.queue == RenderQueue::ViewportOpaque
			? RenderQueue::Opaque
			: object.queue;

	return DrawPacket{
		.object_id = object.object_id,
		.mesh = object.mesh,
		.material = material,
		.base_shader = definition.id,
		.queue = kQueue,
		.program = definition.program,
		.world_from_object = object.world_from_object,
		.world_bounds = object.world_bounds,
		.submesh_index = object.submesh_index,
		.camera_distance_sq = object_camera_distance_sq(object, camera),
		.double_sided = definition.cull_mode == CullMode::None || material_double_sided(material_instance),
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
		const BaseShaderDefinition *definition = definition_for_object(registry, materials, object);
		if (definition == nullptr) {
			continue;
		}
		if (definition->domain != RenderDomain::LitSurface && definition->domain != RenderDomain::TransparentSurface) {
			continue;
		}
		if (!is_valid_handle(object.mesh)) {
			continue;
		}

		DrawPacket packet = packet_from_object(object,
				*definition,
				materials.get(kMaterial),
				kMaterial,
				camera);
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
			case RenderQueue::ViewportOpaque:
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
