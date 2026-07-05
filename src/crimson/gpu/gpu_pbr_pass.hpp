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

#include "crimson/pipeline/draw_packet.hpp"

#include <span>
#include <vector>

namespace crimson::gpu {

/// Draw packet type consumed by the GPU PBR pass.
using PbrDrawPacket = DrawPacket;
/// Queue-separated packet result consumed by the GPU PBR pass.
using PbrPassPackets = DrawPacketBuildResult;

/**
 * Prepare PBR draw packets for GPU submission.
 *
 * @param objects Render objects to inspect.
 * @param registry Base shader registry used for shader schema lookup.
 * @param materials Material system used for material lookup.
 * @param camera Camera used for culling and distance sorting.
 * @param unit_box_mesh Built-in unit box mesh for explicitly marked objects.
 * @param fallback_material Material handle substituted when an object has no material.
 * @param view_aspect_ratio View aspect ratio used for frustum culling.
 * @return Queue-separated PBR packets and counters.
 */
[[nodiscard]] PbrPassPackets prepare_pbr_pass_packets(
		std::span<const RenderObject> objects,
		const BaseShaderRegistry &registry,
		const MaterialSystem &materials,
		const RenderCamera &camera,
		RenderMeshHandle unit_box_mesh,
		RenderMaterialHandle fallback_material,
		float view_aspect_ratio = 1.0F);

} // namespace crimson::gpu
