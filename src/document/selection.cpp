#include "document/selection.hpp"

#include <algorithm>
#include <tuple>

namespace quader::document {
namespace {

[[nodiscard]] DocumentError invalid_component_error(ComponentKind kind)
{
    switch (kind) {
    case ComponentKind::Vertex:
        return make_document_error(DocumentErrorCode::InvalidVertex,
                                   "selection contains an invalid vertex id");
    case ComponentKind::Edge:
        return make_document_error(DocumentErrorCode::InvalidEdge,
                                   "selection contains an invalid edge id");
    case ComponentKind::Face:
        return make_document_error(DocumentErrorCode::InvalidFace,
                                   "selection contains an invalid face id");
    }

    return make_document_error(DocumentErrorCode::InvalidSelection,
                               "selection contains an invalid component id");
}

[[nodiscard]] auto component_key(const ComponentRef& ref) noexcept
{
    return std::visit(
        [](auto id) {
            return std::tuple{
                id.index(),
                id.generation(),
            };
        },
        ref.component);
}

[[nodiscard]] bool component_less(const ComponentRef& left, const ComponentRef& right) noexcept
{
    if (left.object != right.object) {
        return left.object < right.object;
    }

    if (left.component.index() != right.component.index()) {
        return left.component.index() < right.component.index();
    }

    return component_key(left) < component_key(right);
}

} // namespace

ComponentKind component_kind(const ComponentRef& ref) noexcept
{
    switch (ref.component.index()) {
    case 0U:
        return ComponentKind::Vertex;
    case 1U:
        return ComponentKind::Edge;
    case 2U:
        return ComponentKind::Face;
    default:
        return ComponentKind::Vertex;
    }
}

bool component_id_is_valid(const ComponentRef& ref) noexcept
{
    return std::visit([](auto id) { return id.is_valid(); }, ref.component);
}

bool Selection::empty() const noexcept
{
    return objects_.empty() && components_.empty();
}

void Selection::clear()
{
    objects_.clear();
    components_.clear();
}

quader::foundation::Result<void, DocumentError> Selection::set_objects(std::vector<ObjectId> objects)
{
    for (const auto object : objects) {
        if (!object.is_valid()) {
            return quader::foundation::Result<void, DocumentError>::failure(
                make_document_error(DocumentErrorCode::InvalidObject,
                                    "selection contains an invalid object id"));
        }
    }

    std::sort(objects.begin(), objects.end());
    objects.erase(std::unique(objects.begin(), objects.end()), objects.end());

    objects_ = std::move(objects);
    components_.clear();
    return quader::foundation::Result<void, DocumentError>::success();
}

quader::foundation::Result<void, DocumentError> Selection::set_components(
    std::vector<ComponentRef> components)
{
    for (const auto& component : components) {
        if (!component.object.is_valid()) {
            return quader::foundation::Result<void, DocumentError>::failure(
                make_document_error(DocumentErrorCode::InvalidObject,
                                    "selection contains an invalid object id"));
        }

        if (!component_id_is_valid(component)) {
            return quader::foundation::Result<void, DocumentError>::failure(
                invalid_component_error(component_kind(component)));
        }
    }

    std::sort(components.begin(), components.end(), component_less);
    components.erase(std::unique(components.begin(), components.end()), components.end());

    objects_.clear();
    components_ = std::move(components);
    return quader::foundation::Result<void, DocumentError>::success();
}

std::span<const ObjectId> Selection::selected_objects() const noexcept
{
    return objects_;
}

std::span<const ComponentRef> Selection::selected_components() const noexcept
{
    return components_;
}

bool Selection::references_object(ObjectId object) const noexcept
{
    if (std::find(objects_.begin(), objects_.end(), object) != objects_.end()) {
        return true;
    }

    return std::any_of(components_.begin(), components_.end(), [object](const ComponentRef& component) {
        return component.object == object;
    });
}

void Selection::remove_object(ObjectId object)
{
    objects_.erase(std::remove(objects_.begin(), objects_.end(), object), objects_.end());
    components_.erase(std::remove_if(components_.begin(),
                                     components_.end(),
                                     [object](const ComponentRef& component) {
                                         return component.object == object;
                                     }),
                      components_.end());
}

} // namespace quader::document
