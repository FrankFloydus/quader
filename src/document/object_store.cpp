#include "document/object_store.hpp"

#include <algorithm>
#include <utility>

namespace quader::document {

quader::foundation::Result<ObjectId, DocumentError> ObjectStore::create_mesh_object(
    std::string name,
    quader::mesh::Polyhedron mesh,
    Transform transform)
{
    const auto index = allocate_slot();
    auto& slot = slots_[index];
    const ObjectId id{index, slot.generation};

    slot.alive = true;
    slot.object = MeshObject{
        id,
        std::move(name),
        transform,
        std::move(mesh),
    };

    return quader::foundation::Result<ObjectId, DocumentError>::success(id);
}

quader::foundation::Result<void, DocumentError> ObjectStore::restore_mesh_object(MeshObject object)
{
    if (!object.id.is_valid() || object.id.index() >= slots_.size()) {
        return quader::foundation::Result<void, DocumentError>::failure(
            make_document_error(DocumentErrorCode::InvalidObject,
                                "cannot restore a mesh object with an invalid object id"));
    }

    auto& slot = slots_[object.id.index()];
    if (slot.alive) {
        return quader::foundation::Result<void, DocumentError>::failure(
            make_document_error(DocumentErrorCode::ObjectAlreadyExists,
                                "cannot restore a mesh object over a live object slot"));
    }

    const auto free_it = std::find(free_slots_.begin(), free_slots_.end(), object.id.index());
    if (free_it == free_slots_.end()) {
        return quader::foundation::Result<void, DocumentError>::failure(
            make_document_error(DocumentErrorCode::InvalidObject,
                                "cannot restore a mesh object into an unavailable object slot"));
    }

    free_slots_.erase(free_it);
    slot.generation = object.id.generation();
    slot.alive = true;
    slot.object = std::move(object);

    return quader::foundation::Result<void, DocumentError>::success();
}

quader::foundation::Result<MeshObject, DocumentError> ObjectStore::remove_mesh_object(ObjectId id)
{
    auto* object = find_mesh_object(id);
    if (object == nullptr) {
        return quader::foundation::Result<MeshObject, DocumentError>::failure(
            make_document_error(DocumentErrorCode::InvalidObject,
                                "cannot remove an unknown mesh object id"));
    }

    MeshObject removed = std::move(*object);
    auto& slot = slots_[id.index()];
    slot.object = MeshObject{};
    slot.alive = false;
    slot.generation = next_generation(slot.generation);
    free_slots_.push_back(id.index());

    return quader::foundation::Result<MeshObject, DocumentError>::success(std::move(removed));
}

MeshObject* ObjectStore::find_mesh_object(ObjectId id) noexcept
{
    if (!id.is_valid() || id.index() >= slots_.size()) {
        return nullptr;
    }

    auto& slot = slots_[id.index()];
    if (!slot.alive || slot.generation != id.generation()) {
        return nullptr;
    }

    return &slot.object;
}

const MeshObject* ObjectStore::find_mesh_object(ObjectId id) const noexcept
{
    if (!id.is_valid() || id.index() >= slots_.size()) {
        return nullptr;
    }

    const auto& slot = slots_[id.index()];
    if (!slot.alive || slot.generation != id.generation()) {
        return nullptr;
    }

    return &slot.object;
}

bool ObjectStore::contains(ObjectId id) const noexcept
{
    return find_mesh_object(id) != nullptr;
}

std::vector<ObjectId> ObjectStore::object_ids() const
{
    std::vector<ObjectId> ids;
    ids.reserve(size());

    for (quader::foundation::IdIndex index = 0; index < slots_.size(); ++index) {
        const auto& slot = slots_[index];
        if (slot.alive) {
            ids.push_back(ObjectId{index, slot.generation});
        }
    }

    return ids;
}

std::size_t ObjectStore::size() const noexcept
{
    return slots_.size() - free_slots_.size();
}

quader::foundation::IdIndex ObjectStore::allocate_slot()
{
    if (!free_slots_.empty()) {
        const auto index = free_slots_.back();
        free_slots_.pop_back();
        slots_[index].object = MeshObject{};
        return index;
    }

    const auto index = static_cast<quader::foundation::IdIndex>(slots_.size());
    slots_.push_back(ObjectSlot{});
    return index;
}

quader::foundation::IdGeneration ObjectStore::next_generation(
    quader::foundation::IdGeneration generation) noexcept
{
    auto next = static_cast<quader::foundation::IdGeneration>(generation + 1U);
    if (next == 0U) {
        next = 1U;
    }
    return next;
}

} // namespace quader::document
