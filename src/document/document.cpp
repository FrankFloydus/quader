#include "document/document.hpp"

#include "mesh/core/mesh_validation.hpp"

#include <type_traits>
#include <utility>

namespace quader::document {
namespace {

[[nodiscard]] quader::foundation::Result<void, DocumentError> invalid_component_result(
		ComponentKind kind,
		const char *message) {
	switch (kind) {
		case ComponentKind::Vertex:
			return quader::foundation::Result<void, DocumentError>::failure(
					make_document_error(DocumentErrorCode::InvalidVertex, message));
		case ComponentKind::Edge:
			return quader::foundation::Result<void, DocumentError>::failure(
					make_document_error(DocumentErrorCode::InvalidEdge, message));
		case ComponentKind::Face:
			return quader::foundation::Result<void, DocumentError>::failure(
					make_document_error(DocumentErrorCode::InvalidFace, message));
	}

	return quader::foundation::Result<void, DocumentError>::failure(
			make_document_error(DocumentErrorCode::InvalidSelection, message));
}

} // namespace

quader::foundation::Result<ObjectId, DocumentError> Document::create_mesh_object(
		std::string name,
		quader::mesh::Polyhedron mesh,
		Transform transform) {
	if (!is_finite(transform)) {
		return quader::foundation::Result<ObjectId, DocumentError>::failure(
				make_document_error(DocumentErrorCode::InvalidTransform,
						"cannot create a mesh object with a non-finite transform"));
	}

	auto mesh_validation = validate_mesh(mesh);
	if (!mesh_validation) {
		return quader::foundation::Result<ObjectId, DocumentError>::failure(
				std::move(mesh_validation).error());
	}

	auto created = objects_.create_mesh_object(std::move(name), std::move(mesh), transform);
	if (!created) {
		return created;
	}

	emit_change(DocumentChangeKind::ObjectCreated, created.value());
	mark_dirty(to_dirty_flags(DocumentDirtyFlag::Objects));
	return created;
}

quader::foundation::Result<void, DocumentError> Document::restore_mesh_object(MeshObject object) {
	const ObjectId kId = object.id;
	auto validation = validate_mesh_object(object);
	if (!validation) {
		return validation;
	}

	auto restored = objects_.restore_mesh_object(std::move(object));
	if (!restored) {
		return restored;
	}

	emit_change(DocumentChangeKind::ObjectCreated, kId);
	mark_dirty(to_dirty_flags(DocumentDirtyFlag::Objects));
	return quader::foundation::Result<void, DocumentError>::success();
}

quader::foundation::Result<void, DocumentError> Document::delete_object(ObjectId id) {
	auto removed = extract_mesh_object(id);
	if (!removed) {
		return quader::foundation::Result<void, DocumentError>::failure(std::move(removed).error());
	}

	return quader::foundation::Result<void, DocumentError>::success();
}

quader::foundation::Result<MeshObject, DocumentError> Document::extract_mesh_object(ObjectId id) {
	auto removed = objects_.remove_mesh_object(id);
	if (!removed) {
		return removed;
	}

	const bool kSelectionChanged = selection_.references_object(id);
	if (kSelectionChanged) {
		selection_.remove_object(id);
	}

	emit_change(DocumentChangeKind::ObjectDeleted, id);
	if (kSelectionChanged) {
		emit_change(DocumentChangeKind::SelectionChanged, id);
	}

	auto dirty = to_dirty_flags(DocumentDirtyFlag::Objects);
	if (kSelectionChanged) {
		dirty |= to_dirty_flags(DocumentDirtyFlag::Selection);
	}
	mark_dirty(dirty);

	return removed;
}

quader::foundation::Result<void, DocumentError> Document::rename_object(ObjectId id,
		std::string name) {
	auto *object = find_mesh_object(id);
	if (object == nullptr) {
		return quader::foundation::Result<void, DocumentError>::failure(
				make_document_error(DocumentErrorCode::InvalidObject,
						"cannot rename an unknown mesh object id"));
	}

	if (object->name == name) {
		return quader::foundation::Result<void, DocumentError>::success();
	}

	object->name = std::move(name);
	emit_change(DocumentChangeKind::ObjectRenamed, id);
	mark_dirty(to_dirty_flags(DocumentDirtyFlag::ObjectProperties));
	return quader::foundation::Result<void, DocumentError>::success();
}

quader::foundation::Result<void, DocumentError> Document::set_transform(ObjectId id,
		Transform transform) {
	auto *object = find_mesh_object(id);
	if (object == nullptr) {
		return quader::foundation::Result<void, DocumentError>::failure(
				make_document_error(DocumentErrorCode::InvalidObject,
						"cannot transform an unknown mesh object id"));
	}

	if (!is_finite(transform)) {
		return quader::foundation::Result<void, DocumentError>::failure(
				make_document_error(DocumentErrorCode::InvalidTransform,
						"cannot apply a non-finite object transform"));
	}

	if (object->transform == transform) {
		return quader::foundation::Result<void, DocumentError>::success();
	}

	object->transform = transform;
	emit_change(DocumentChangeKind::TransformChanged, id);
	mark_dirty(to_dirty_flags(DocumentDirtyFlag::Transforms));
	return quader::foundation::Result<void, DocumentError>::success();
}

MeshObject *Document::find_mesh_object(ObjectId id) noexcept {
	return objects_.find_mesh_object(id);
}

const MeshObject *Document::find_mesh_object(ObjectId id) const noexcept {
	return objects_.find_mesh_object(id);
}

quader::foundation::Result<void, DocumentError> Document::validate_object_id(ObjectId id) const {
	if (find_mesh_object(id) == nullptr) {
		return quader::foundation::Result<void, DocumentError>::failure(
				make_document_error(DocumentErrorCode::InvalidObject,
						"document does not contain the requested object id"));
	}

	return quader::foundation::Result<void, DocumentError>::success();
}

quader::foundation::Result<void, DocumentError> Document::validate_component_ref(
		const ComponentRef &component) const {
	const auto *object = find_mesh_object(component.object);
	if (object == nullptr) {
		return quader::foundation::Result<void, DocumentError>::failure(
				make_document_error(DocumentErrorCode::InvalidObject,
						"component reference points at an unknown object id"));
	}

	if (!component_id_is_valid(component)) {
		return invalid_component_result(component_kind(component),
				"component reference contains an invalid component id");
	}

	return std::visit(
			[&object](auto id) -> quader::foundation::Result<void, DocumentError> {
				using ComponentId = std::decay_t<decltype(id)>;
				if constexpr (std::is_same_v<ComponentId, quader::mesh::VertexId>) {
					if (!object->mesh.is_valid(id)) {
						return quader::foundation::Result<void, DocumentError>::failure(
								make_document_error(DocumentErrorCode::InvalidVertex,
										"component reference points at an unknown vertex id"));
					}
				} else if constexpr (std::is_same_v<ComponentId, quader::mesh::EdgeId>) {
					if (!object->mesh.is_valid(id)) {
						return quader::foundation::Result<void, DocumentError>::failure(
								make_document_error(DocumentErrorCode::InvalidEdge,
										"component reference points at an unknown edge id"));
					}
				} else if constexpr (std::is_same_v<ComponentId, quader::mesh::FaceId>) {
					if (!object->mesh.is_valid(id)) {
						return quader::foundation::Result<void, DocumentError>::failure(
								make_document_error(DocumentErrorCode::InvalidFace,
										"component reference points at an unknown face id"));
					}
				}

				return quader::foundation::Result<void, DocumentError>::success();
			},
			component.component);
}

const Selection &Document::selection() const noexcept {
	return selection_;
}

quader::foundation::Result<void, DocumentError> Document::set_selection(Selection selection) {
	auto validation = validate_selection(selection);
	if (!validation) {
		return validation;
	}

	if (selection_ == selection) {
		return quader::foundation::Result<void, DocumentError>::success();
	}

	selection_ = std::move(selection);
	emit_change(DocumentChangeKind::SelectionChanged);
	mark_dirty(to_dirty_flags(DocumentDirtyFlag::Selection));
	return quader::foundation::Result<void, DocumentError>::success();
}

void Document::clear_selection() {
	if (selection_.empty()) {
		return;
	}

	selection_.clear();
	emit_change(DocumentChangeKind::SelectionChanged);
	mark_dirty(to_dirty_flags(DocumentDirtyFlag::Selection));
}

quader::foundation::Result<void, DocumentError> Document::mark_mesh_topology_changed(ObjectId id) {
	auto validation = validate_object_id(id);
	if (!validation) {
		return validation;
	}

	emit_change(DocumentChangeKind::MeshTopologyChanged, id);
	mark_dirty(to_dirty_flags(DocumentDirtyFlag::MeshTopology));
	return quader::foundation::Result<void, DocumentError>::success();
}

quader::foundation::Result<void, DocumentError> Document::mark_mesh_geometry_changed(ObjectId id) {
	auto validation = validate_object_id(id);
	if (!validation) {
		return validation;
	}

	emit_change(DocumentChangeKind::MeshGeometryChanged, id);
	mark_dirty(to_dirty_flags(DocumentDirtyFlag::MeshGeometry));
	return quader::foundation::Result<void, DocumentError>::success();
}

bool Document::is_dirty() const noexcept {
	return dirty_flags_ != kDocumentDirtyNone;
}

DocumentDirtyFlags Document::dirty_flags() const noexcept {
	return dirty_flags_;
}

bool Document::has_dirty_flag(DocumentDirtyFlag flag) const noexcept {
	return quader::document::has_dirty_flag(dirty_flags_, flag);
}

void Document::clear_dirty() {
	if (dirty_flags_ == kDocumentDirtyNone) {
		return;
	}

	dirty_flags_ = kDocumentDirtyNone;
	emit_change(DocumentChangeKind::DirtyStateChanged, ObjectId::invalid(), dirty_flags_);
}

std::vector<DocumentChange> Document::take_pending_changes() {
	auto changes = std::move(pending_changes_);
	pending_changes_.clear();
	return changes;
}

std::size_t Document::object_count() const noexcept {
	return objects_.size();
}

std::vector<ObjectId> Document::object_ids() const {
	return objects_.object_ids();
}

quader::foundation::Result<void, DocumentError> Document::validate_mesh_object(
		const MeshObject &object) const {
	if (!object.id.is_valid()) {
		return quader::foundation::Result<void, DocumentError>::failure(
				make_document_error(DocumentErrorCode::InvalidObject,
						"mesh object has an invalid object id"));
	}

	if (!is_finite(object.transform)) {
		return quader::foundation::Result<void, DocumentError>::failure(
				make_document_error(DocumentErrorCode::InvalidTransform,
						"mesh object has a non-finite transform"));
	}

	return validate_mesh(object.mesh);
}

quader::foundation::Result<void, DocumentError> Document::validate_mesh(
		const quader::mesh::Polyhedron &mesh) const {
	const auto kReport = quader::mesh::validate_mesh(mesh);
	if (kReport.ok()) {
		return quader::foundation::Result<void, DocumentError>::success();
	}

	auto message = std::string("mesh object failed validation");
	if (!kReport.issues().empty()) {
		message += ": ";
		message += kReport.issues().front().diagnostic.message;
	}

	return quader::foundation::Result<void, DocumentError>::failure(
			make_document_error(DocumentErrorCode::InvalidMesh, std::move(message)));
}

quader::foundation::Result<void, DocumentError> Document::validate_selection(
		const Selection &selection) const {
	for (const auto kObject : selection.selected_objects()) {
		auto validation = validate_object_id(kObject);
		if (!validation) {
			return validation;
		}
	}

	for (const auto &component : selection.selected_components()) {
		auto validation = validate_component_ref(component);
		if (!validation) {
			return validation;
		}
	}

	return quader::foundation::Result<void, DocumentError>::success();
}

void Document::emit_change(DocumentChangeKind kind, ObjectId object, DocumentDirtyFlags flags) {
	pending_changes_.push_back(DocumentChange{
			kind,
			object,
			flags,
	});
}

void Document::mark_dirty(DocumentDirtyFlags flags) {
	const auto kNewFlags = dirty_flags_ | flags;
	if (kNewFlags == dirty_flags_) {
		return;
	}

	dirty_flags_ = kNewFlags;
	emit_change(DocumentChangeKind::DirtyStateChanged, ObjectId::invalid(), dirty_flags_);
}

} // namespace quader::document
