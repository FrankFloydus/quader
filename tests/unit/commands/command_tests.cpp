/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
#include "../document/document_test_helpers.hpp"

#include <gtest/gtest.h>

#include "commands/command_history.hpp"
#include "commands/document_commands.hpp"
#include "commands/selection_commands.hpp"
#include "foundation/logging.hpp"

#include <limits>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

namespace {

using quader::tests::document_fixtures::DocumentSemanticState;

void discard_pending_changes(quader::document::Document &document) {
	const auto kChanges = document.take_pending_changes();
	(void)kChanges;
}

DocumentSemanticState capture_command_state(const quader::document::Document &document) {
	auto state = quader::tests::document_fixtures::capture_document_state(document);
	state.dirty = false;
	state.dirty_flags = quader::document::kDocumentDirtyNone;
	return state;
}

quader::document::Transform translated_transform(float x, float y, float z) {
	quader::document::Transform transform;
	transform.translation = quader::math::Vec3{ x, y, z };
	return transform;
}

TEST(Command, CreateMeshObjectCommandExecutesUndoesAndRedoesSemanticState) {
	quader::document::Document document;
	quader::commands::CommandHistory history;
	const auto kBefore = capture_command_state(document);

	auto mesh = quader::tests::document_fixtures::make_triangle_mesh();
	const auto kTransform = translated_transform(2.0F, 3.0F, 4.0F);

	auto result = history.execute(std::make_unique<quader::commands::CreateMeshObjectCommand>(
										  "Triangle",
										  std::move(mesh.mesh),
										  kTransform),
			document);
	EXPECT_EQ((result).status, quader::commands::CommandStatus::Applied);
	EXPECT_TRUE(history.can_undo());
	EXPECT_FALSE(history.can_redo());
	EXPECT_EQ(history.undo_name(), std::string_view("Create Mesh Object"));

	const auto kAfter = capture_command_state(document);
	EXPECT_NE(kAfter, kBefore);
	EXPECT_EQ(kAfter.objects.size(), 1U);
	EXPECT_EQ(kAfter.objects.front().name, std::string("Triangle"));
	EXPECT_TRUE(kAfter.objects.front().transform == kTransform);
	EXPECT_EQ(kAfter.objects.front().material, quader::document::default_box_material());
	EXPECT_EQ(kAfter.objects.front().vertex_count, 3U);
	EXPECT_EQ(kAfter.objects.front().face_count, 1U);

	result = history.undo(document);
	EXPECT_EQ((result).status, quader::commands::CommandStatus::Applied);
	EXPECT_EQ(capture_command_state(document), kBefore);
	EXPECT_FALSE(history.can_undo());
	EXPECT_TRUE(history.can_redo());
	EXPECT_EQ(history.redo_name(), std::string_view("Create Mesh Object"));

	result = history.redo(document);
	EXPECT_EQ((result).status, quader::commands::CommandStatus::Applied);
	EXPECT_EQ(capture_command_state(document), kAfter);
	EXPECT_TRUE(history.can_undo());
	EXPECT_FALSE(history.can_redo());
}

TEST(Command, CreateMeshObjectCommandRoundTripsMaterialOnUndoRedo) {
	quader::document::Document document;
	quader::commands::CommandHistory history;
	auto mesh = quader::tests::document_fixtures::make_triangle_mesh();
	const quader::document::PbrMaterial kMaterial{
		.base_color = { 0.5F, 0.5F, 0.5F },
		.roughness = 1.0F,
		.metallic = 0.0F,
	};

	auto result = history.execute(std::make_unique<quader::commands::CreateMeshObjectCommand>(
										  "Box",
										  std::move(mesh.mesh),
										  quader::document::Transform{},
										  kMaterial),
			document);
	EXPECT_EQ((result).status, quader::commands::CommandStatus::Applied);
	const auto kCreatedState = capture_command_state(document);
	ASSERT_EQ(kCreatedState.objects.size(), 1U);
	EXPECT_EQ(kCreatedState.objects.front().material, kMaterial);

	result = history.undo(document);
	EXPECT_EQ((result).status, quader::commands::CommandStatus::Applied);
	EXPECT_TRUE(capture_command_state(document).objects.empty());

	result = history.redo(document);
	EXPECT_EQ((result).status, quader::commands::CommandStatus::Applied);
	const auto kRedoneState = capture_command_state(document);
	ASSERT_EQ(kRedoneState.objects.size(), 1U);
	EXPECT_EQ(kRedoneState.objects.front().material, kMaterial);
}

TEST(Command, DeleteObjectCommandRestoresObjectAndSelectionOnUndoRedo) {
	auto fixture = quader::tests::document_fixtures::make_document_with_triangle_object();
	auto second_mesh = quader::tests::document_fixtures::make_triangle_mesh();
	const auto kSecond = fixture.document.create_mesh_object("Second", std::move(second_mesh.mesh));
	EXPECT_TRUE(kSecond);

	quader::document::Selection selection;
	EXPECT_TRUE(selection.set_objects({ fixture.object, kSecond.value() }));
	EXPECT_TRUE(fixture.document.set_selection(selection));
	fixture.document.clear_dirty();
	discard_pending_changes(fixture.document);

	quader::commands::CommandHistory history;
	const auto kBefore = capture_command_state(fixture.document);

	auto result = history.execute(
			std::make_unique<quader::commands::DeleteObjectCommand>(fixture.object),
			fixture.document);
	EXPECT_EQ((result).status, quader::commands::CommandStatus::Applied);

	const auto kAfter = capture_command_state(fixture.document);
	EXPECT_EQ(kAfter.objects.size(), 1U);
	EXPECT_EQ(kAfter.objects.front().id, kSecond.value());
	EXPECT_EQ(kAfter.selected_objects.size(), 1U);
	EXPECT_EQ(kAfter.selected_objects.front(), kSecond.value());

	result = history.undo(fixture.document);
	EXPECT_EQ((result).status, quader::commands::CommandStatus::Applied);
	EXPECT_EQ(capture_command_state(fixture.document), kBefore);

	result = history.redo(fixture.document);
	EXPECT_EQ((result).status, quader::commands::CommandStatus::Applied);
	EXPECT_EQ(capture_command_state(fixture.document), kAfter);
}

TEST(Command, RenameObjectCommandExecutesUndoesAndRedoesName) {
	auto fixture = quader::tests::document_fixtures::make_document_with_triangle_object();
	fixture.document.clear_dirty();
	discard_pending_changes(fixture.document);

	quader::commands::CommandHistory history;
	const auto kBefore = capture_command_state(fixture.document);

	auto result = history.execute(
			std::make_unique<quader::commands::RenameObjectCommand>(fixture.object, "Renamed"),
			fixture.document);
	EXPECT_EQ((result).status, quader::commands::CommandStatus::Applied);

	const auto kAfter = capture_command_state(fixture.document);
	EXPECT_EQ(kAfter.objects.size(), 1U);
	EXPECT_EQ(kAfter.objects.front().name, std::string("Renamed"));

	result = history.undo(fixture.document);
	EXPECT_EQ((result).status, quader::commands::CommandStatus::Applied);
	EXPECT_EQ(capture_command_state(fixture.document), kBefore);

	result = history.redo(fixture.document);
	EXPECT_EQ((result).status, quader::commands::CommandStatus::Applied);
	EXPECT_EQ(capture_command_state(fixture.document), kAfter);
}

TEST(Command, TransformCommandExecutesUndoesAndRedoesTransform) {
	auto fixture = quader::tests::document_fixtures::make_document_with_triangle_object();
	fixture.document.clear_dirty();
	discard_pending_changes(fixture.document);

	quader::commands::CommandHistory history;
	const auto kBefore = capture_command_state(fixture.document);
	const auto kTransform = translated_transform(5.0F, 6.0F, 7.0F);

	auto result = history.execute(
			std::make_unique<quader::commands::SetObjectTransformCommand>(fixture.object, kTransform),
			fixture.document);
	EXPECT_EQ((result).status, quader::commands::CommandStatus::Applied);

	const auto kAfter = capture_command_state(fixture.document);
	EXPECT_EQ(kAfter.objects.size(), 1U);
	EXPECT_TRUE(kAfter.objects.front().transform == kTransform);

	result = history.undo(fixture.document);
	EXPECT_EQ((result).status, quader::commands::CommandStatus::Applied);
	EXPECT_EQ(capture_command_state(fixture.document), kBefore);

	result = history.redo(fixture.document);
	EXPECT_EQ((result).status, quader::commands::CommandStatus::Applied);
	EXPECT_EQ(capture_command_state(fixture.document), kAfter);
}

TEST(Command, SetSelectionCommandExecutesUndoesAndRedoesSelection) {
	auto fixture = quader::tests::document_fixtures::make_document_with_triangle_object();
	fixture.document.clear_dirty();
	discard_pending_changes(fixture.document);

	quader::document::Selection selection;
	EXPECT_TRUE(selection.set_objects({ fixture.object }));

	quader::commands::CommandHistory history;
	const auto kBefore = capture_command_state(fixture.document);

	auto result = history.execute(
			std::make_unique<quader::commands::SetSelectionCommand>(selection),
			fixture.document);
	EXPECT_EQ((result).status, quader::commands::CommandStatus::Applied);

	const auto kAfter = capture_command_state(fixture.document);
	EXPECT_EQ(kAfter.selected_objects.size(), 1U);
	EXPECT_EQ(kAfter.selected_objects.front(), fixture.object);

	result = history.undo(fixture.document);
	EXPECT_EQ((result).status, quader::commands::CommandStatus::Applied);
	EXPECT_EQ(capture_command_state(fixture.document), kBefore);

	result = history.redo(fixture.document);
	EXPECT_EQ((result).status, quader::commands::CommandStatus::Applied);
	EXPECT_EQ(capture_command_state(fixture.document), kAfter);
}

TEST(Command, ClearSelectionCommandExecutesUndoesAndRedoesSelection) {
	auto fixture = quader::tests::document_fixtures::make_document_with_triangle_object();
	quader::document::Selection selection;
	EXPECT_TRUE(selection.set_objects({ fixture.object }));
	EXPECT_TRUE(fixture.document.set_selection(selection));
	fixture.document.clear_dirty();
	discard_pending_changes(fixture.document);

	quader::commands::CommandHistory history;
	const auto kBefore = capture_command_state(fixture.document);

	auto result = history.execute(std::make_unique<quader::commands::ClearSelectionCommand>(),
			fixture.document);
	EXPECT_EQ((result).status, quader::commands::CommandStatus::Applied);

	const auto kAfter = capture_command_state(fixture.document);
	EXPECT_TRUE(kAfter.selected_objects.empty());
	EXPECT_TRUE(kAfter.selected_components.empty());

	result = history.undo(fixture.document);
	EXPECT_EQ((result).status, quader::commands::CommandStatus::Applied);
	EXPECT_EQ(capture_command_state(fixture.document), kBefore);

	result = history.redo(fixture.document);
	EXPECT_EQ((result).status, quader::commands::CommandStatus::Applied);
	EXPECT_EQ(capture_command_state(fixture.document), kAfter);
}

TEST(Command, RejectedCommandsDoNotMutateDocumentOrHistory) {
	auto fixture = quader::tests::document_fixtures::make_document_with_triangle_object();
	fixture.document.clear_dirty();
	discard_pending_changes(fixture.document);

	quader::commands::CommandHistory history;
	const auto kBefore = capture_command_state(fixture.document);

	auto invalid_transform = translated_transform(1.0F, 2.0F, 3.0F);
	invalid_transform.translation.x = std::numeric_limits<float>::infinity();

	auto result = history.execute(
			std::make_unique<quader::commands::SetObjectTransformCommand>(fixture.object,
					invalid_transform),
			fixture.document);
	EXPECT_EQ((result).status, quader::commands::CommandStatus::Rejected);
	EXPECT_FALSE(result.diagnostics.empty());
	EXPECT_FALSE(history.can_undo());
	EXPECT_FALSE(history.can_redo());
	EXPECT_EQ(capture_command_state(fixture.document), kBefore);

	quader::document::Selection invalid_selection;
	EXPECT_TRUE(invalid_selection.set_objects({ quader::document::ObjectId{ 999U, 1U } }));
	result = history.execute(
			std::make_unique<quader::commands::SetSelectionCommand>(invalid_selection),
			fixture.document);
	EXPECT_EQ((result).status, quader::commands::CommandStatus::Rejected);
	EXPECT_FALSE(history.can_undo());
	EXPECT_EQ(capture_command_state(fixture.document), kBefore);

	result = history.execute(
			std::make_unique<quader::commands::RenameObjectCommand>(
					quader::document::ObjectId{ 999U, 1U },
					"Missing"),
			fixture.document);
	EXPECT_EQ((result).status, quader::commands::CommandStatus::Rejected);
	EXPECT_FALSE(history.can_undo());
	EXPECT_EQ(capture_command_state(fixture.document), kBefore);

	result = history.execute(nullptr, fixture.document);
	EXPECT_EQ((result).status, quader::commands::CommandStatus::Rejected);
	EXPECT_FALSE(history.can_undo());
	EXPECT_EQ(capture_command_state(fixture.document), kBefore);
}

TEST(Command, HistoryTruncatesRedoAfterNewCommandAndReportsAvailability) {
	auto fixture = quader::tests::document_fixtures::make_document_with_triangle_object();
	fixture.document.clear_dirty();
	discard_pending_changes(fixture.document);

	quader::commands::CommandHistory history;
	EXPECT_FALSE(history.can_undo());
	EXPECT_FALSE(history.can_redo());
	EXPECT_EQ(history.undo_name(), std::string_view{});
	EXPECT_EQ(history.redo_name(), std::string_view{});

	auto result = history.undo(fixture.document);
	EXPECT_EQ((result).status, quader::commands::CommandStatus::Rejected);
	result = history.redo(fixture.document);
	EXPECT_EQ((result).status, quader::commands::CommandStatus::Rejected);

	result = history.execute(
			std::make_unique<quader::commands::RenameObjectCommand>(fixture.object, "First"),
			fixture.document);
	EXPECT_EQ((result).status, quader::commands::CommandStatus::Applied);
	EXPECT_TRUE(history.can_undo());
	EXPECT_EQ(history.undo_name(), std::string_view("Rename Object"));

	result = history.execute(
			std::make_unique<quader::commands::SetObjectTransformCommand>(
					fixture.object,
					translated_transform(10.0F, 0.0F, 0.0F)),
			fixture.document);
	EXPECT_EQ((result).status, quader::commands::CommandStatus::Applied);
	EXPECT_EQ(history.undo_name(), std::string_view("Set Object Transform"));

	result = history.undo(fixture.document);
	EXPECT_EQ((result).status, quader::commands::CommandStatus::Applied);
	EXPECT_TRUE(history.can_redo());
	EXPECT_EQ(history.redo_name(), std::string_view("Set Object Transform"));

	quader::document::Selection selection;
	EXPECT_TRUE(selection.set_objects({ fixture.object }));
	result = history.execute(
			std::make_unique<quader::commands::SetSelectionCommand>(selection),
			fixture.document);
	EXPECT_EQ((result).status, quader::commands::CommandStatus::Applied);
	EXPECT_FALSE(history.can_redo());
	EXPECT_EQ(history.redo_name(), std::string_view{});

	result = history.redo(fixture.document);
	EXPECT_EQ((result).status, quader::commands::CommandStatus::Rejected);
	EXPECT_TRUE(history.can_undo());

	history.clear();
	EXPECT_FALSE(history.can_undo());
	EXPECT_FALSE(history.can_redo());
}

TEST(Command, CommandMergeHooksDefaultToNotMergeable) {
	quader::commands::RenameObjectCommand first{ quader::document::ObjectId{ 0U, 1U }, "First" };
	quader::commands::RenameObjectCommand second{ quader::document::ObjectId{ 0U, 1U }, "Second" };

	EXPECT_FALSE(first.can_merge_with(second));
	auto result = first.merge_with(std::make_unique<quader::commands::RenameObjectCommand>(
			quader::document::ObjectId{ 0U, 1U },
			"Third"));
	EXPECT_EQ((result).status, quader::commands::CommandStatus::NotMergeable);
}

} // namespace
