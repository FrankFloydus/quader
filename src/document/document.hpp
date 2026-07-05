#pragma once

#include "document/document_error.hpp"
#include "document/document_events.hpp"
#include "document/object_store.hpp"
#include "document/scene_object.hpp"
#include "document/selection.hpp"
#include "document/transform.hpp"
#include "foundation/result.hpp"

#include <string>
#include <vector>

namespace quader::document {

class Document final {
public:
	[[nodiscard]] quader::foundation::Result<ObjectId, DocumentError> create_mesh_object(
			std::string name,
			quader::mesh::Polyhedron mesh,
			Transform transform = {});
	[[nodiscard]] quader::foundation::Result<void, DocumentError> restore_mesh_object(
			MeshObject object);
	[[nodiscard]] quader::foundation::Result<void, DocumentError> delete_object(ObjectId id);
	[[nodiscard]] quader::foundation::Result<MeshObject, DocumentError> extract_mesh_object(
			ObjectId id);
	[[nodiscard]] quader::foundation::Result<void, DocumentError> rename_object(ObjectId id,
			std::string name);
	[[nodiscard]] quader::foundation::Result<void, DocumentError> set_transform(ObjectId id,
			Transform transform);

	[[nodiscard]] MeshObject *find_mesh_object(ObjectId id) noexcept;
	[[nodiscard]] const MeshObject *find_mesh_object(ObjectId id) const noexcept;
	[[nodiscard]] quader::foundation::Result<void, DocumentError> validate_object_id(
			ObjectId id) const;
	[[nodiscard]] quader::foundation::Result<void, DocumentError> validate_component_ref(
			const ComponentRef &component) const;

	[[nodiscard]] const Selection &selection() const noexcept;
	[[nodiscard]] quader::foundation::Result<void, DocumentError> set_selection(
			Selection selection);
	void clear_selection();

	[[nodiscard]] quader::foundation::Result<void, DocumentError> mark_mesh_topology_changed(
			ObjectId id);
	[[nodiscard]] quader::foundation::Result<void, DocumentError> mark_mesh_geometry_changed(
			ObjectId id);

	[[nodiscard]] bool is_dirty() const noexcept;
	[[nodiscard]] DocumentDirtyFlags dirty_flags() const noexcept;
	[[nodiscard]] bool has_dirty_flag(DocumentDirtyFlag flag) const noexcept;
	void clear_dirty();

	[[nodiscard]] std::vector<DocumentChange> take_pending_changes();
	[[nodiscard]] std::size_t object_count() const noexcept;
	[[nodiscard]] std::vector<ObjectId> object_ids() const;

private:
	[[nodiscard]] quader::foundation::Result<void, DocumentError> validate_mesh_object(
			const MeshObject &object) const;
	[[nodiscard]] quader::foundation::Result<void, DocumentError> validate_mesh(
			const quader::mesh::Polyhedron &mesh) const;
	[[nodiscard]] quader::foundation::Result<void, DocumentError> validate_selection(
			const Selection &selection) const;

	void emit_change(DocumentChangeKind kind,
			ObjectId object = ObjectId::invalid(),
			DocumentDirtyFlags flags = kDocumentDirtyNone);
	void mark_dirty(DocumentDirtyFlags flags);

	ObjectStore objects_;
	Selection selection_;
	std::vector<DocumentChange> pending_changes_;
	DocumentDirtyFlags dirty_flags_ = kDocumentDirtyNone;
};

} // namespace quader::document
