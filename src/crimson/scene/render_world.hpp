#pragma once

#include "crimson/scene/render_object.hpp"

#include <span>
#include <vector>

namespace crimson {

class RenderWorld final {
public:
    RenderObjectId add_object(RenderObject object);
    [[nodiscard]] bool update_object(RenderObjectId id, RenderObject object);
    [[nodiscard]] bool remove_object(RenderObjectId id);

    [[nodiscard]] std::span<const RenderObject> objects() const noexcept;
    void clear() noexcept;

private:
    std::vector<RenderObject> objects_;
    RenderObjectId next_object_id_ = 1;
};

} // namespace crimson
