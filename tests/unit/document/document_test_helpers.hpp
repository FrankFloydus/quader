/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
#pragma once

#include "document/document.hpp"
#include "foundation/assert.hpp"
#include "mesh/core/mesh_traversal.hpp"

#include <array>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace quader::tests::document_fixtures {

/// Mesh fixture containing one triangular face and representative component ids.
struct TriangleMeshFixture {
	/// Mesh owning the component ids below.
	quader::mesh::Polyhedron mesh;
	/// Vertex ids of the triangle.
	std::array<quader::mesh::VertexId, 3> vertices;
	/// First edge found on the triangle face.
	quader::mesh::EdgeId edge;
	/// Triangle face id.
	quader::mesh::FaceId face;
};

/// Document fixture containing one mesh object built from a triangle.
struct DocumentMeshFixture {
	/// Document owning the mesh object.
	quader::document::Document document;
	/// Id of the triangle mesh object in `document`.
	quader::document::ObjectId object;
	/// Vertex ids of the stored triangle mesh.
	std::array<quader::mesh::VertexId, 3> vertices;
	/// First edge found on the stored triangle face.
	quader::mesh::EdgeId edge;
	/// Stored triangle face id.
	quader::mesh::FaceId face;
};

/// Semantic snapshot of a mesh object used for document command comparisons.
struct SemanticMeshObjectState {
	/// Object id.
	quader::document::ObjectId id;
	/// Object display name.
	std::string name;
	/// Object transform.
	quader::document::Transform transform;
	/// Object material.
	quader::document::PbrMaterial material;
	/// Number of vertices in the object's mesh.
	std::size_t vertex_count = 0U;
	/// Number of edges in the object's mesh.
	std::size_t edge_count = 0U;
	/// Number of faces in the object's mesh.
	std::size_t face_count = 0U;

	/// Compare semantic object snapshots by value.
	friend bool operator==(const SemanticMeshObjectState &, const SemanticMeshObjectState &) = default;
};

/// Semantic snapshot of document state used to compare undo/redo effects.
struct DocumentSemanticState {
	/// Object snapshots in document iteration order.
	std::vector<SemanticMeshObjectState> objects;
	/// Current document selection mode.
	quader::document::SelectionMode selection_mode = quader::document::SelectionMode::Object;
	/// Selected object ids.
	std::vector<quader::document::ObjectId> selected_objects;
	/// Selected component references.
	std::vector<quader::document::ComponentRef> selected_components;
	/// Dirty flag state.
	bool dirty = false;
	/// Dirty flag bitset.
	quader::document::DocumentDirtyFlags dirty_flags = quader::document::kDocumentDirtyNone;

	/// Compare semantic document snapshots by value.
	friend bool operator==(const DocumentSemanticState &, const DocumentSemanticState &) = default;
};

/// Build a simple triangle mesh fixture with stable ids for tests.
inline TriangleMeshFixture make_triangle_mesh() {
	quader::mesh::Polyhedron mesh;
	const std::array<quader::mesh::VertexId, 3> kVertices{
		mesh.create_vertex(quader::math::Vec3{ 0.0F, 0.0F, 0.0F }),
		mesh.create_vertex(quader::math::Vec3{ 1.0F, 0.0F, 0.0F }),
		mesh.create_vertex(quader::math::Vec3{ 0.0F, 1.0F, 0.0F }),
	};

	auto face = mesh.create_face(kVertices);
	QUADER_ASSERT(face);

	auto edges = quader::mesh::face_edges(mesh, face.value());
	QUADER_ASSERT(edges);
	QUADER_ASSERT(!edges.value().empty());

	return TriangleMeshFixture{
		std::move(mesh),
		kVertices,
		edges.value().front(),
		face.value(),
	};
}

/// Build a document containing one triangle mesh object.
inline DocumentMeshFixture make_document_with_triangle_object() {
	auto mesh_fixture = make_triangle_mesh();

	quader::document::Document document;
	auto object = document.create_mesh_object("Triangle", std::move(mesh_fixture.mesh));
	QUADER_ASSERT(object);

	return DocumentMeshFixture{
		std::move(document),
		object.value(),
		mesh_fixture.vertices,
		mesh_fixture.edge,
		mesh_fixture.face,
	};
}

/// Capture the document state used by command and document tests.
inline DocumentSemanticState capture_document_state(const quader::document::Document &document) {
	DocumentSemanticState state;
	state.dirty = document.is_dirty();
	state.dirty_flags = document.dirty_flags();

	for (const auto kObjectId : document.object_ids()) {
		const auto *object = document.find_mesh_object(kObjectId);
		QUADER_ASSERT(object != nullptr);

		state.objects.push_back(SemanticMeshObjectState{
				object->id,
				object->name,
				object->transform,
				object->material,
				object->mesh.vertex_count(),
				object->mesh.edge_count(),
				object->mesh.face_count(),
		});
	}

	state.selection_mode = document.selection().mode();
	state.selected_objects.assign(document.selection().selected_objects().begin(),
			document.selection().selected_objects().end());
	state.selected_components.assign(document.selection().selected_components().begin(),
			document.selection().selected_components().end());
	return state;
}

} // namespace quader::tests::document_fixtures
