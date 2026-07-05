#pragma once

#include "crimson/renderer_types.hpp"
#include "math/aabb.hpp"

#include <array>
#include <cstdint>

namespace crimson {

using RenderObjectId = std::uint64_t;

[[nodiscard]] constexpr std::array<float, 16> identity_transform() noexcept
{
    return {
        1.0F, 0.0F, 0.0F, 0.0F,
        0.0F, 1.0F, 0.0F, 0.0F,
        0.0F, 0.0F, 1.0F, 0.0F,
        0.0F, 0.0F, 0.0F, 1.0F,
    };
}

struct RenderObject {
    RenderObjectId object_id = 0;
    RenderMeshHandle mesh;
    RenderMaterialHandle material;
    BaseShaderId base_shader = BaseShaderId::OpaquePbr;
    std::array<float, 16> world_from_object = identity_transform();
    quader::math::Aabb world_bounds;
    RenderQueue queue = RenderQueue::Opaque;
    std::uint32_t submesh_index = 0;
    bool visible = true;
    bool pickable = true;
};

} // namespace crimson
