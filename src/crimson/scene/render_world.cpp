#include "crimson/scene/render_world.hpp"

#include <algorithm>

namespace crimson {

RenderObjectId RenderWorld::add_object(RenderObject object)
{
    if (object.object_id == 0) {
        object.object_id = next_object_id_++;
    } else {
        next_object_id_ = std::max(next_object_id_, object.object_id + 1);
    }

    const RenderObjectId id = object.object_id;
    objects_.push_back(object);
    return id;
}

bool RenderWorld::update_object(RenderObjectId id, RenderObject object)
{
    const auto it = std::find_if(objects_.begin(), objects_.end(), [id](const RenderObject& current) {
        return current.object_id == id;
    });
    if (it == objects_.end()) {
        return false;
    }

    object.object_id = id;
    *it = object;
    return true;
}

bool RenderWorld::remove_object(RenderObjectId id)
{
    const auto old_size = objects_.size();
    objects_.erase(
        std::remove_if(objects_.begin(), objects_.end(), [id](const RenderObject& object) {
            return object.object_id == id;
        }),
        objects_.end());
    return objects_.size() != old_size;
}

std::span<const RenderObject> RenderWorld::objects() const noexcept
{
    return objects_;
}

void RenderWorld::clear() noexcept
{
    objects_.clear();
    next_object_id_ = 1;
}

} // namespace crimson
