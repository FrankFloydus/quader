#pragma once

#include "foundation/assert.hpp"
#include "mesh/core/mesh_traversal.hpp"
#include "document/document.hpp"

#include <array>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace quader::tests::document_fixtures {

struct TriangleMeshFixture {
    quader::mesh::Polyhedron mesh;
    std::array<quader::mesh::VertexId, 3> vertices;
    quader::mesh::EdgeId edge;
    quader::mesh::FaceId face;
};

struct DocumentMeshFixture {
    quader::document::Document document;
    quader::document::ObjectId object;
    std::array<quader::mesh::VertexId, 3> vertices;
    quader::mesh::EdgeId edge;
    quader::mesh::FaceId face;
};

struct SemanticMeshObjectState {
    quader::document::ObjectId id;
    std::string name;
    quader::document::Transform transform;
    std::size_t vertex_count = 0U;
    std::size_t edge_count = 0U;
    std::size_t face_count = 0U;

    friend bool operator==(const SemanticMeshObjectState&, const SemanticMeshObjectState&) = default;
};

struct DocumentSemanticState {
    std::vector<SemanticMeshObjectState> objects;
    std::vector<quader::document::ObjectId> selected_objects;
    std::vector<quader::document::ComponentRef> selected_components;
    bool dirty = false;
    quader::document::DocumentDirtyFlags dirty_flags = quader::document::kDocumentDirtyNone;

    friend bool operator==(const DocumentSemanticState&, const DocumentSemanticState&) = default;
};

inline TriangleMeshFixture make_triangle_mesh()
{
    quader::mesh::Polyhedron mesh;
    const std::array<quader::mesh::VertexId, 3> vertices{
        mesh.create_vertex(quader::math::Vec3{0.0F, 0.0F, 0.0F}),
        mesh.create_vertex(quader::math::Vec3{1.0F, 0.0F, 0.0F}),
        mesh.create_vertex(quader::math::Vec3{0.0F, 1.0F, 0.0F}),
    };

    auto face = mesh.create_face(vertices);
    QUADER_ASSERT(face);

    auto edges = quader::mesh::face_edges(mesh, face.value());
    QUADER_ASSERT(edges);
    QUADER_ASSERT(!edges.value().empty());

    return TriangleMeshFixture{
        std::move(mesh),
        vertices,
        edges.value().front(),
        face.value(),
    };
}

inline DocumentMeshFixture make_document_with_triangle_object()
{
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

inline DocumentSemanticState capture_document_state(const quader::document::Document& document)
{
    DocumentSemanticState state;
    state.dirty = document.is_dirty();
    state.dirty_flags = document.dirty_flags();

    for (const auto object_id : document.object_ids()) {
        const auto* object = document.find_mesh_object(object_id);
        QUADER_ASSERT(object != nullptr);

        state.objects.push_back(SemanticMeshObjectState{
            object->id,
            object->name,
            object->transform,
            object->mesh.vertex_count(),
            object->mesh.edge_count(),
            object->mesh.face_count(),
        });
    }

    state.selected_objects.assign(document.selection().selected_objects().begin(),
                                  document.selection().selected_objects().end());
    state.selected_components.assign(document.selection().selected_components().begin(),
                                     document.selection().selected_components().end());
    return state;
}

} // namespace quader::tests::document_fixtures
