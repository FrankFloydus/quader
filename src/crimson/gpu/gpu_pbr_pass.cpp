#include "crimson/gpu/gpu_pbr_pass.hpp"

namespace crimson::gpu {

PbrPassPackets prepare_pbr_pass_packets(
		std::span<const RenderObject> objects,
		const BaseShaderRegistry &registry,
		const MaterialSystem &materials,
		const RenderCamera &camera,
		RenderMeshHandle fallback_mesh,
		RenderMaterialHandle fallback_material,
		float view_aspect_ratio) {
	return build_draw_packets(objects, registry, materials, camera, fallback_mesh, fallback_material, view_aspect_ratio);
}

} // namespace crimson::gpu
