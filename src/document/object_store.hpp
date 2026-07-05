#pragma once

#include "foundation/result.hpp"
#include "document/document_error.hpp"
#include "document/object_id.hpp"
#include "document/scene_object.hpp"

#include <cstddef>
#include <vector>

namespace quader::document {

class ObjectStore final {
public:
    [[nodiscard]] quader::foundation::Result<ObjectId, DocumentError> create_mesh_object(
        std::string name,
        quader::mesh::Polyhedron mesh,
        Transform transform);

    [[nodiscard]] quader::foundation::Result<void, DocumentError> restore_mesh_object(
        MeshObject object);
    [[nodiscard]] quader::foundation::Result<MeshObject, DocumentError> remove_mesh_object(
        ObjectId id);

    [[nodiscard]] MeshObject* find_mesh_object(ObjectId id) noexcept;
    [[nodiscard]] const MeshObject* find_mesh_object(ObjectId id) const noexcept;
    [[nodiscard]] bool contains(ObjectId id) const noexcept;

    [[nodiscard]] std::vector<ObjectId> object_ids() const;
    [[nodiscard]] std::size_t size() const noexcept;

private:
    struct ObjectSlot {
        quader::foundation::IdGeneration generation = 1U;
        bool alive = false;
        MeshObject object;
    };

    [[nodiscard]] quader::foundation::IdIndex allocate_slot();
    [[nodiscard]] static quader::foundation::IdGeneration next_generation(
        quader::foundation::IdGeneration generation) noexcept;

    std::vector<ObjectSlot> slots_;
    std::vector<quader::foundation::IdIndex> free_slots_;
};

} // namespace quader::document
