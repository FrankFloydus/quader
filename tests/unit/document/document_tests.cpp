#include "document_test_helpers.hpp"

#include <gtest/gtest.h>

#include "foundation/logging.hpp"
#include "mesh/core/mesh_validation.hpp"

#include <limits>
#include <string>
#include <vector>

namespace {

bool contains_change(const std::vector<quader::document::DocumentChange>& changes,
                     quader::document::DocumentChangeKind kind)
{
    for (const auto& change : changes) {
        if (change.kind == kind) {
            return true;
        }
    }

    return false;
}

void discard_pending_changes(quader::document::Document& document)
{
    const auto changes = document.take_pending_changes();
    (void)changes;
}

TEST(Document, CreateMeshObjectStoresMeshTransformDirtyFlagsAndEvents)
{
    quader::document::Document document;
    auto mesh_fixture = quader::tests::document_fixtures::make_triangle_mesh();

    quader::document::Transform transform;
    transform.translation = quader::math::Vec3{2.0F, 3.0F, 4.0F};
    transform.rotation_euler = quader::math::Vec3{0.0F, 90.0F, 0.0F};
    transform.scale = quader::math::Vec3{1.0F, 2.0F, 3.0F};

    const auto object = document.create_mesh_object("Triangle", std::move(mesh_fixture.mesh), transform);
    EXPECT_TRUE(object);
    EXPECT_TRUE(object.value().is_valid());
    EXPECT_EQ(document.object_count(), 1U);

    const auto* stored = document.find_mesh_object(object.value());
    EXPECT_TRUE(stored != nullptr);
    EXPECT_EQ(stored->name, std::string("Triangle"));
    EXPECT_TRUE(stored->transform == transform);
    EXPECT_EQ(stored->mesh.vertex_count(), 3U);
    EXPECT_EQ(stored->mesh.face_count(), 1U);
    EXPECT_TRUE(quader::mesh::validate_mesh(stored->mesh).ok());

    EXPECT_TRUE(document.is_dirty());
    EXPECT_TRUE(document.has_dirty_flag(quader::document::DocumentDirtyFlag::Objects));

    const auto changes = document.take_pending_changes();
    EXPECT_EQ(changes.size(), 2U);
    EXPECT_EQ(changes[0].kind, quader::document::DocumentChangeKind::ObjectCreated);
    EXPECT_EQ(changes[0].object, object.value());
    EXPECT_EQ(changes[1].kind, quader::document::DocumentChangeKind::DirtyStateChanged);
    EXPECT_TRUE(quader::document::has_dirty_flag(changes[1].dirty_flags,
                                                 quader::document::DocumentDirtyFlag::Objects));
}

TEST(Document, DeleteObjectPrunesSelectionInvalidatesStaleIdAndReusesGeneration)
{
    auto fixture = quader::tests::document_fixtures::make_document_with_triangle_object();
    auto second_mesh = quader::tests::document_fixtures::make_triangle_mesh();
    const auto second = fixture.document.create_mesh_object("Second", std::move(second_mesh.mesh));
    EXPECT_TRUE(second);

    quader::document::Selection selection;
    EXPECT_TRUE(selection.set_objects({fixture.object, second.value()}));
    EXPECT_TRUE(fixture.document.set_selection(selection));
    fixture.document.clear_dirty();
    discard_pending_changes(fixture.document);

    EXPECT_TRUE(fixture.document.delete_object(fixture.object));
    EXPECT_EQ(fixture.document.object_count(), 1U);
    EXPECT_TRUE(fixture.document.find_mesh_object(fixture.object) == nullptr);
    EXPECT_EQ(fixture.document.selection().selected_objects().size(), 1U);
    EXPECT_EQ(fixture.document.selection().selected_objects().front(), second.value());

    const auto changes = fixture.document.take_pending_changes();
    EXPECT_TRUE(contains_change(changes, quader::document::DocumentChangeKind::ObjectDeleted));
    EXPECT_TRUE(contains_change(changes, quader::document::DocumentChangeKind::SelectionChanged));
    EXPECT_TRUE(contains_change(changes, quader::document::DocumentChangeKind::DirtyStateChanged));

    const auto stale_delete = fixture.document.delete_object(fixture.object);
    EXPECT_FALSE(stale_delete);
    EXPECT_EQ(stale_delete.error().code, quader::document::DocumentErrorCode::InvalidObject);

    auto replacement_mesh = quader::tests::document_fixtures::make_triangle_mesh();
    const auto replacement = fixture.document.create_mesh_object("Replacement",
                                                                std::move(replacement_mesh.mesh));
    EXPECT_TRUE(replacement);
    EXPECT_EQ(replacement.value().index(), fixture.object.index());
    EXPECT_NE(replacement.value().generation(), fixture.object.generation());
}

TEST(Document, SetTransformValidatesIdsAndRejectsNonFiniteValuesWithoutMutation)
{
    auto fixture = quader::tests::document_fixtures::make_document_with_triangle_object();
    fixture.document.clear_dirty();
    discard_pending_changes(fixture.document);

    quader::document::Transform transform;
    transform.translation = quader::math::Vec3{4.0F, 5.0F, 6.0F};
    transform.scale = quader::math::Vec3{2.0F, 2.0F, 2.0F};

    EXPECT_TRUE(fixture.document.set_transform(fixture.object, transform));
    const auto* object = fixture.document.find_mesh_object(fixture.object);
    EXPECT_TRUE(object != nullptr);
    EXPECT_TRUE(object->transform == transform);

    auto changes = fixture.document.take_pending_changes();
    EXPECT_EQ(changes.size(), 2U);
    EXPECT_EQ(changes[0].kind, quader::document::DocumentChangeKind::TransformChanged);
    EXPECT_EQ(changes[1].kind, quader::document::DocumentChangeKind::DirtyStateChanged);
    EXPECT_TRUE(fixture.document.has_dirty_flag(quader::document::DocumentDirtyFlag::Transforms));

    fixture.document.clear_dirty();
    discard_pending_changes(fixture.document);
    const auto before = quader::tests::document_fixtures::capture_document_state(fixture.document);

    quader::document::Transform invalid_transform = transform;
    invalid_transform.translation.x = std::numeric_limits<float>::infinity();
    const auto invalid_result = fixture.document.set_transform(fixture.object, invalid_transform);
    EXPECT_FALSE(invalid_result);
    EXPECT_EQ(invalid_result.error().code, quader::document::DocumentErrorCode::InvalidTransform);
    EXPECT_EQ(quader::tests::document_fixtures::capture_document_state(fixture.document), before);
    EXPECT_TRUE(fixture.document.take_pending_changes().empty());

    const auto invalid_object_result = fixture.document.set_transform(
        quader::document::ObjectId{999U, 1U},
        transform);
    EXPECT_FALSE(invalid_object_result);
    EXPECT_EQ(invalid_object_result.error().code, quader::document::DocumentErrorCode::InvalidObject);
}

TEST(Document, SelectionRejectsInvalidObjectAndComponentIdsAndAcceptsValidComponents)
{
    auto fixture = quader::tests::document_fixtures::make_document_with_triangle_object();
    fixture.document.clear_dirty();
    discard_pending_changes(fixture.document);

    quader::document::Selection invalid_object_selection;
    EXPECT_TRUE(invalid_object_selection.set_objects({quader::document::ObjectId{999U, 1U}}));
    const auto invalid_object = fixture.document.set_selection(invalid_object_selection);
    EXPECT_FALSE(invalid_object);
    EXPECT_EQ(invalid_object.error().code, quader::document::DocumentErrorCode::InvalidObject);
    EXPECT_TRUE(fixture.document.selection().empty());
    EXPECT_TRUE(fixture.document.take_pending_changes().empty());

    quader::document::Selection invalid_component_selection;
    EXPECT_TRUE(invalid_component_selection.set_components({
        quader::document::ComponentRef{
            fixture.object,
            quader::mesh::VertexId{999U, 1U},
        },
    }));
    const auto invalid_component = fixture.document.set_selection(invalid_component_selection);
    EXPECT_FALSE(invalid_component);
    EXPECT_EQ(invalid_component.error().code, quader::document::DocumentErrorCode::InvalidVertex);
    EXPECT_TRUE(fixture.document.selection().empty());

    quader::document::Selection component_selection;
    EXPECT_TRUE(component_selection.set_components({
        quader::document::ComponentRef{fixture.object, fixture.face},
        quader::document::ComponentRef{fixture.object, fixture.vertices[0]},
        quader::document::ComponentRef{fixture.object, fixture.edge},
        quader::document::ComponentRef{fixture.object, fixture.edge},
    }));
    EXPECT_TRUE(fixture.document.set_selection(component_selection));
    EXPECT_TRUE(fixture.document.selection().selected_objects().empty());
    EXPECT_EQ(fixture.document.selection().selected_components().size(), 3U);
    EXPECT_TRUE(fixture.document.has_dirty_flag(quader::document::DocumentDirtyFlag::Selection));

    const auto changes = fixture.document.take_pending_changes();
    EXPECT_TRUE(contains_change(changes, quader::document::DocumentChangeKind::SelectionChanged));
    EXPECT_TRUE(contains_change(changes, quader::document::DocumentChangeKind::DirtyStateChanged));

    quader::document::Selection object_selection;
    EXPECT_TRUE(object_selection.set_objects({fixture.object, fixture.object}));
    EXPECT_TRUE(fixture.document.set_selection(object_selection));
    EXPECT_EQ(fixture.document.selection().selected_objects().size(), 1U);
    EXPECT_TRUE(fixture.document.selection().selected_components().empty());
}

TEST(Document, DirtyFlagsClearIdempotentlyAndMeshDirtyEventsAreTyped)
{
    auto fixture = quader::tests::document_fixtures::make_document_with_triangle_object();
    EXPECT_TRUE(fixture.document.is_dirty());
    discard_pending_changes(fixture.document);

    fixture.document.clear_dirty();
    EXPECT_FALSE(fixture.document.is_dirty());
    EXPECT_EQ(fixture.document.dirty_flags(), quader::document::kDocumentDirtyNone);

    auto changes = fixture.document.take_pending_changes();
    EXPECT_EQ(changes.size(), 1U);
    EXPECT_EQ(changes[0].kind, quader::document::DocumentChangeKind::DirtyStateChanged);
    EXPECT_EQ(changes[0].dirty_flags, quader::document::kDocumentDirtyNone);

    fixture.document.clear_dirty();
    EXPECT_TRUE(fixture.document.take_pending_changes().empty());

    EXPECT_TRUE(fixture.document.mark_mesh_geometry_changed(fixture.object));
    changes = fixture.document.take_pending_changes();
    EXPECT_EQ(changes.size(), 2U);
    EXPECT_EQ(changes[0].kind, quader::document::DocumentChangeKind::MeshGeometryChanged);
    EXPECT_EQ(changes[0].object, fixture.object);
    EXPECT_EQ(changes[1].kind, quader::document::DocumentChangeKind::DirtyStateChanged);
    EXPECT_TRUE(fixture.document.has_dirty_flag(quader::document::DocumentDirtyFlag::MeshGeometry));

    const auto invalid_topology = fixture.document.mark_mesh_topology_changed(
        quader::document::ObjectId{999U, 1U});
    EXPECT_FALSE(invalid_topology);
    EXPECT_EQ(invalid_topology.error().code, quader::document::DocumentErrorCode::InvalidObject);
}

TEST(Document, SemanticStateHelperCapturesUserVisibleDocumentState)
{
    auto fixture = quader::tests::document_fixtures::make_document_with_triangle_object();
    fixture.document.clear_dirty();
    discard_pending_changes(fixture.document);

    const auto before = quader::tests::document_fixtures::capture_document_state(fixture.document);
    const auto same = quader::tests::document_fixtures::capture_document_state(fixture.document);
    EXPECT_EQ(before, same);

    quader::document::Transform transform;
    transform.translation = quader::math::Vec3{8.0F, 0.0F, 0.0F};
    EXPECT_TRUE(fixture.document.set_transform(fixture.object, transform));

    const auto after_transform = quader::tests::document_fixtures::capture_document_state(
        fixture.document);
    EXPECT_NE(before, after_transform);
}

} // namespace
