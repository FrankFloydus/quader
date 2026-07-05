#pragma once

#include "crimson/frame/frame_targets.hpp"
#include "crimson/graph/render_pass.hpp"
#include "crimson/graph/render_resource_registry.hpp"
#include "foundation/result.hpp"

#include <span>
#include <string>
#include <vector>

namespace crimson {

class RenderGraph final {
public:
    [[nodiscard]] quader::foundation::Result<void, std::string> add_resource(RenderResourceDesc desc);
    void add_pass(RenderPass pass);

    [[nodiscard]] quader::foundation::Result<void, std::string> validate() const;
    [[nodiscard]] std::string debug_dump() const;

    void resize(ViewportExtent extent);
    void clear() noexcept;

    [[nodiscard]] const RenderResourceRegistry& resources() const noexcept;
    [[nodiscard]] std::span<const RenderPass> passes() const noexcept;
    [[nodiscard]] std::uint64_t resize_generation() const noexcept;

private:
    RenderResourceRegistry resources_;
    std::vector<RenderPass> passes_;
};

[[nodiscard]] RenderGraph make_minimal_prototype_render_graph(ViewportExtent extent);
[[nodiscard]] RenderGraph make_v1_correctness_render_graph(
    ViewportExtent extent,
    PickingIdTargetFormat picking_format = PickingIdTargetFormat::R32Uint);

} // namespace crimson
