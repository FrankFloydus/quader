#pragma once

#include "crimson/graph/render_resource_desc.hpp"
#include "foundation/result.hpp"

#include <span>
#include <string>
#include <vector>

namespace crimson {

class RenderResourceRegistry final {
public:
    [[nodiscard]] quader::foundation::Result<void, std::string> add(RenderResourceDesc desc);
    [[nodiscard]] const RenderResourceRecord* find(std::string_view name) const noexcept;
    [[nodiscard]] RenderResourceRecord* find(std::string_view name) noexcept;
    [[nodiscard]] std::span<const RenderResourceRecord> resources() const noexcept;
    [[nodiscard]] std::uint64_t resize_generation() const noexcept;

    void resize(ViewportExtent extent);
    void clear() noexcept;

private:
    std::vector<RenderResourceRecord> resources_;
    std::uint64_t resize_generation_ = 1;
};

} // namespace crimson
