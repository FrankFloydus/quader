#pragma once

#include "document/document_error.hpp"
#include "document/object_id.hpp"
#include "foundation/result.hpp"
#include "mesh/core/mesh_ids.hpp"

#include <span>
#include <variant>
#include <vector>

namespace quader::document {

enum class ComponentKind {
	Vertex,
	Edge,
	Face,
};

struct ComponentRef {
	ObjectId object;
	std::variant<quader::mesh::VertexId, quader::mesh::EdgeId, quader::mesh::FaceId> component;

	friend bool operator==(const ComponentRef &, const ComponentRef &) = default;
};

[[nodiscard]] ComponentKind component_kind(const ComponentRef &ref) noexcept;
[[nodiscard]] bool component_id_is_valid(const ComponentRef &ref);

class Selection final {
public:
	[[nodiscard]] bool empty() const noexcept;
	void clear();

	[[nodiscard]] quader::foundation::Result<void, DocumentError> set_objects(
			std::vector<ObjectId> objects);
	[[nodiscard]] quader::foundation::Result<void, DocumentError> set_components(
			std::vector<ComponentRef> components);

	[[nodiscard]] std::span<const ObjectId> selected_objects() const noexcept;
	[[nodiscard]] std::span<const ComponentRef> selected_components() const noexcept;
	[[nodiscard]] bool references_object(ObjectId object) const noexcept;

	void remove_object(ObjectId object);

	friend bool operator==(const Selection &, const Selection &) = default;

private:
	std::vector<ObjectId> objects_;
	std::vector<ComponentRef> components_;
};

} // namespace quader::document
