/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
#include "document_test_helpers.hpp"

#include <gtest/gtest.h>

#include "foundation/logging.hpp"
#include "mesh/core/mesh_validation.hpp"

#include <limits>
#include <string>
#include <vector>

namespace {

bool contains_change(const std::vector<quader::document::DocumentChange> &changes,
		quader::document::DocumentChangeKind kind) {
	for (const auto &change : changes) {
		if (change.kind == kind) {
			return true;
		}
	}

	return false;
}

void discard_pending_changes(quader::document::Document &document) {
	const auto kChanges = document.take_pending_changes();
	(void)kChanges;
}

TEST(Document, CreateMeshObjectStoresMeshTransformDirtyFlagsAndEvents) {
	quader::document::Document document;
	auto mesh_fixture = quader::tests::document_fixtures::make_triangle_mesh();

	quader::document::Transform transform;
	transform.translation = quader::math::Vec3{ 2.0F, 3.0F, 4.0F };
	transform.rotation_euler = quader::math::Vec3{ 0.0F, 90.0F, 0.0F };
	transform.scale = quader::math::Vec3{ 1.0F, 2.0F, 3.0F };

	const auto kObject = document.create_mesh_object("Triangle", std::move(mesh_fixture.mesh), transform);
	EXPECT_TRUE(kObject);
	EXPECT_TRUE(kObject.value().is_valid());
	EXPECT_EQ(document.object_count(), 1U);

	const auto *stored = document.find_mesh_object(kObject.value());
	EXPECT_TRUE(stored != nullptr);
	EXPECT_EQ(stored->name, std::string("Triangle"));
	EXPECT_TRUE(stored->transform == transform);
	EXPECT_EQ(stored->material, quader::document::default_box_material());
	EXPECT_EQ(stored->mesh.vertex_count(), 3U);
	EXPECT_EQ(stored->mesh.face_count(), 1U);
	EXPECT_TRUE(quader::mesh::validate_mesh(stored->mesh).ok());

	EXPECT_TRUE(document.is_dirty());
	EXPECT_TRUE(document.has_dirty_flag(quader::document::DocumentDirtyFlag::Objects));

	const auto kChanges = document.take_pending_changes();
	EXPECT_EQ(kChanges.size(), 2U);
	EXPECT_EQ(kChanges[0].kind, quader::document::DocumentChangeKind::ObjectCreated);
	EXPECT_EQ(kChanges[0].object, kObject.value());
	EXPECT_EQ(kChanges[1].kind, quader::document::DocumentChangeKind::DirtyStateChanged);
	EXPECT_TRUE(quader::document::has_dirty_flag(kChanges[1].dirty_flags,
			quader::document::DocumentDirtyFlag::Objects));
}

TEST(Document, CreateMeshObjectStoresExplicitMaterialValues) {
	quader::document::Document document;
	auto mesh_fixture = quader::tests::document_fixtures::make_triangle_mesh();
	const quader::document::PbrMaterial kMaterial{
		.base_color = { 0.25F, 0.5F, 0.75F },
		.roughness = 0.8F,
		.metallic = 0.1F,
	};

	const auto kObject = document.create_mesh_object("Material Object",
			std::move(mesh_fixture.mesh),
			quader::document::Transform{},
			kMaterial);
	EXPECT_TRUE(kObject);
	const auto *stored = document.find_mesh_object(kObject.value());
	EXPECT_TRUE(stored != nullptr);
	EXPECT_EQ(stored->material, kMaterial);

	auto invalid_mesh = quader::tests::document_fixtures::make_triangle_mesh();
	quader::document::PbrMaterial invalid_material = kMaterial;
	invalid_material.roughness = std::numeric_limits<float>::infinity();
	const auto kInvalid = document.create_mesh_object("Invalid Material",
			std::move(invalid_mesh.mesh),
			quader::document::Transform{},
			invalid_material);
	EXPECT_FALSE(kInvalid);
	EXPECT_EQ(kInvalid.error().code, quader::document::DocumentErrorCode::InvalidMaterial);
}

TEST(Document, DeleteObjectPrunesSelectionInvalidatesStaleIdAndReusesGeneration) {
	auto fixture = quader::tests::document_fixtures::make_document_with_triangle_object();
	auto second_mesh = quader::tests::document_fixtures::make_triangle_mesh();
	const auto kSecond = fixture.document.create_mesh_object("Second", std::move(second_mesh.mesh));
	EXPECT_TRUE(kSecond);

	quader::document::Selection selection;
	EXPECT_TRUE(selection.set_objects({ fixture.object, kSecond.value() }));
	EXPECT_TRUE(fixture.document.set_selection(selection));
	fixture.document.clear_dirty();
	discard_pending_changes(fixture.document);

	EXPECT_TRUE(fixture.document.delete_object(fixture.object));
	EXPECT_EQ(fixture.document.object_count(), 1U);
	EXPECT_TRUE(fixture.document.find_mesh_object(fixture.object) == nullptr);
	EXPECT_EQ(fixture.document.selection().selected_objects().size(), 1U);
	EXPECT_EQ(fixture.document.selection().selected_objects().front(), kSecond.value());

	const auto kChanges = fixture.document.take_pending_changes();
	EXPECT_TRUE(contains_change(kChanges, quader::document::DocumentChangeKind::ObjectDeleted));
	EXPECT_TRUE(contains_change(kChanges, quader::document::DocumentChangeKind::SelectionChanged));
	EXPECT_TRUE(contains_change(kChanges, quader::document::DocumentChangeKind::DirtyStateChanged));

	const auto kStaleDelete = fixture.document.delete_object(fixture.object);
	EXPECT_FALSE(kStaleDelete);
	EXPECT_EQ(kStaleDelete.error().code, quader::document::DocumentErrorCode::InvalidObject);

	auto replacement_mesh = quader::tests::document_fixtures::make_triangle_mesh();
	const auto kReplacement = fixture.document.create_mesh_object("Replacement",
			std::move(replacement_mesh.mesh));
	EXPECT_TRUE(kReplacement);
	EXPECT_EQ(kReplacement.value().index(), fixture.object.index());
	EXPECT_NE(kReplacement.value().generation(), fixture.object.generation());
}

TEST(Document, SetTransformValidatesIdsAndRejectsNonFiniteValuesWithoutMutation) {
	auto fixture = quader::tests::document_fixtures::make_document_with_triangle_object();
	fixture.document.clear_dirty();
	discard_pending_changes(fixture.document);

	quader::document::Transform transform;
	transform.translation = quader::math::Vec3{ 4.0F, 5.0F, 6.0F };
	transform.scale = quader::math::Vec3{ 2.0F, 2.0F, 2.0F };

	EXPECT_TRUE(fixture.document.set_transform(fixture.object, transform));
	const auto *object = fixture.document.find_mesh_object(fixture.object);
	EXPECT_TRUE(object != nullptr);
	EXPECT_TRUE(object->transform == transform);

	auto changes = fixture.document.take_pending_changes();
	EXPECT_EQ(changes.size(), 2U);
	EXPECT_EQ(changes[0].kind, quader::document::DocumentChangeKind::TransformChanged);
	EXPECT_EQ(changes[1].kind, quader::document::DocumentChangeKind::DirtyStateChanged);
	EXPECT_TRUE(fixture.document.has_dirty_flag(quader::document::DocumentDirtyFlag::Transforms));

	fixture.document.clear_dirty();
	discard_pending_changes(fixture.document);
	const auto kBefore = quader::tests::document_fixtures::capture_document_state(fixture.document);

	quader::document::Transform invalid_transform = transform;
	invalid_transform.translation.x = std::numeric_limits<float>::infinity();
	const auto kInvalidResult = fixture.document.set_transform(fixture.object, invalid_transform);
	EXPECT_FALSE(kInvalidResult);
	EXPECT_EQ(kInvalidResult.error().code, quader::document::DocumentErrorCode::InvalidTransform);
	EXPECT_EQ(quader::tests::document_fixtures::capture_document_state(fixture.document), kBefore);
	EXPECT_TRUE(fixture.document.take_pending_changes().empty());

	const auto kInvalidObjectResult = fixture.document.set_transform(
			quader::document::ObjectId{ 999U, 1U },
			transform);
	EXPECT_FALSE(kInvalidObjectResult);
	EXPECT_EQ(kInvalidObjectResult.error().code, quader::document::DocumentErrorCode::InvalidObject);
}

TEST(Document, SelectionRejectsInvalidObjectAndComponentIdsAndAcceptsValidComponents) {
	auto fixture = quader::tests::document_fixtures::make_document_with_triangle_object();
	fixture.document.clear_dirty();
	discard_pending_changes(fixture.document);

	quader::document::Selection invalid_object_selection;
	EXPECT_TRUE(invalid_object_selection.set_objects({ quader::document::ObjectId{ 999U, 1U } }));
	const auto kInvalidObject = fixture.document.set_selection(invalid_object_selection);
	EXPECT_FALSE(kInvalidObject);
	EXPECT_EQ(kInvalidObject.error().code, quader::document::DocumentErrorCode::InvalidObject);
	EXPECT_TRUE(fixture.document.selection().empty());
	EXPECT_TRUE(fixture.document.take_pending_changes().empty());

	quader::document::Selection invalid_component_selection;
	EXPECT_TRUE(invalid_component_selection.set_components({
			quader::document::ComponentRef{
					fixture.object,
					quader::mesh::VertexId{ 999U, 1U },
			},
	}));
	const auto kInvalidComponent = fixture.document.set_selection(invalid_component_selection);
	EXPECT_FALSE(kInvalidComponent);
	EXPECT_EQ(kInvalidComponent.error().code, quader::document::DocumentErrorCode::InvalidVertex);
	EXPECT_TRUE(fixture.document.selection().empty());

	quader::document::Selection mixed_component_selection;
	EXPECT_FALSE(mixed_component_selection.set_components({
			quader::document::ComponentRef{ fixture.object, fixture.face },
			quader::document::ComponentRef{ fixture.object, fixture.vertices[0] },
	}));

	quader::document::Selection component_selection;
	EXPECT_TRUE(component_selection.set_components({
			quader::document::ComponentRef{ fixture.object, fixture.face },
			quader::document::ComponentRef{ fixture.object, fixture.face },
	}));
	EXPECT_TRUE(fixture.document.set_selection(component_selection));
	EXPECT_EQ(fixture.document.selection().mode(), quader::document::SelectionMode::Face);
	ASSERT_EQ(fixture.document.selection().selected_objects().size(), 1U);
	EXPECT_EQ(fixture.document.selection().selected_objects().front(), fixture.object);
	EXPECT_EQ(fixture.document.selection().selected_components().size(), 1U);
	EXPECT_TRUE(fixture.document.has_dirty_flag(quader::document::DocumentDirtyFlag::Selection));

	const auto kChanges = fixture.document.take_pending_changes();
	EXPECT_TRUE(contains_change(kChanges, quader::document::DocumentChangeKind::SelectionChanged));
	EXPECT_TRUE(contains_change(kChanges, quader::document::DocumentChangeKind::DirtyStateChanged));

	quader::document::Selection object_selection;
	EXPECT_TRUE(object_selection.set_objects({ fixture.object, fixture.object }));
	EXPECT_TRUE(fixture.document.set_selection(object_selection));
	EXPECT_EQ(fixture.document.selection().selected_objects().size(), 1U);
	EXPECT_TRUE(fixture.document.selection().selected_components().empty());
}

TEST(Document, DirtyFlagsClearIdempotentlyAndMeshDirtyEventsAreTyped) {
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

	const auto kInvalidTopology = fixture.document.mark_mesh_topology_changed(
			quader::document::ObjectId{ 999U, 1U });
	EXPECT_FALSE(kInvalidTopology);
	EXPECT_EQ(kInvalidTopology.error().code, quader::document::DocumentErrorCode::InvalidObject);
}

TEST(Document, SemanticStateHelperCapturesUserVisibleDocumentState) {
	auto fixture = quader::tests::document_fixtures::make_document_with_triangle_object();
	fixture.document.clear_dirty();
	discard_pending_changes(fixture.document);

	const auto kBefore = quader::tests::document_fixtures::capture_document_state(fixture.document);
	const auto kSame = quader::tests::document_fixtures::capture_document_state(fixture.document);
	EXPECT_EQ(kBefore, kSame);

	quader::document::Transform transform;
	transform.translation = quader::math::Vec3{ 8.0F, 0.0F, 0.0F };
	EXPECT_TRUE(fixture.document.set_transform(fixture.object, transform));

	const auto kAfterTransform = quader::tests::document_fixtures::capture_document_state(
			fixture.document);
	EXPECT_NE(kBefore, kAfterTransform);
}

} // namespace
