/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
#include "crimson/gpu/gpu_pbr_pass.hpp"

namespace crimson::gpu {

PbrPassPackets prepare_pbr_pass_packets(
		std::span<const RenderObject> objects,
		const BaseShaderRegistry &registry,
		const MaterialSystem &materials,
		const RenderCamera &camera,
		RenderMaterialHandle fallback_material,
		float view_aspect_ratio) {
	return build_draw_packets(objects, registry, materials, camera, fallback_material, view_aspect_ratio);
}

} // namespace crimson::gpu
