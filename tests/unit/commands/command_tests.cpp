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
#include "mesh/core/mesh_traversal.hpp"

#include <array>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

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

std::vector<quader::mesh::VertexId> face_vertices_for(
		const quader::document::Document &document,
		quader::document::ObjectId object,
		quader::mesh::FaceId face) {
	const auto *mesh_object = document.find_mesh_object(object);
	EXPECT_TRUE(mesh_object != nullptr);
	if (mesh_object == nullptr) {
		return {};
	}

	auto vertices = quader::mesh::face_vertices(mesh_object->mesh, face);
	EXPECT_TRUE(vertices);
	return vertices ? vertices.value() : std::vector<quader::mesh::VertexId>{};
}

bool is_same_cyclic_vertex_order(const std::vector<quader::mesh::VertexId> &actual,
		const std::array<quader::mesh::VertexId, 3> &expected) {
	if (actual.size() != expected.size()) {
		return false;
	}

	for (std::size_t offset = 0; offset < expected.size(); ++offset) {
		bool matches = true;
		for (std::size_t index = 0; index < expected.size(); ++index) {
			if (actual[(offset + index) % expected.size()] != expected[index]) {
				matches = false;
				break;
			}
		}
		if (matches) {
			return true;
		}
	}
	return false;
}

class MergeableRenameCommand final : public quader::commands::ICommand {
public:
	MergeableRenameCommand(quader::document::ObjectId object, std::string next_name)
			: object_(object), next_name_(std::move(next_name)) {
	}

	[[nodiscard]] std::string_view name() const noexcept override {
		return "Mergeable Rename";
	}

	[[nodiscard]] quader::commands::CommandResult execute(quader::document::Document &document) override {
		const auto *object = document.find_mesh_object(object_);
		if (object == nullptr) {
			return quader::commands::CommandResult::rejected("missing object");
		}
		if (!previous_name_.has_value()) {
			previous_name_ = object->name;
		}
		auto renamed = document.rename_object(object_, next_name_);
		return renamed ? quader::commands::CommandResult::applied()
					   : quader::commands::CommandResult::rejected("rename failed");
	}

	[[nodiscard]] quader::commands::CommandResult undo(quader::document::Document &document) override {
		if (!previous_name_.has_value()) {
			return quader::commands::CommandResult::rejected("rename command was not executed");
		}
		auto renamed = document.rename_object(object_, *previous_name_);
		return renamed ? quader::commands::CommandResult::applied()
					   : quader::commands::CommandResult::rejected("rename undo failed");
	}

	[[nodiscard]] bool can_merge_with(const quader::commands::ICommand &next) const noexcept override {
		const auto *rename = dynamic_cast<const MergeableRenameCommand *>(&next);
		return rename != nullptr && rename->object_ == object_;
	}

	[[nodiscard]] quader::commands::CommandResult merge_with(std::unique_ptr<quader::commands::ICommand> next) override {
		auto *rename = dynamic_cast<MergeableRenameCommand *>(next.get());
		if (rename == nullptr || rename->object_ != object_) {
			return quader::commands::CommandResult::not_mergeable();
		}

		next_name_ = std::move(rename->next_name_);
		return quader::commands::CommandResult::applied();
	}

private:
	quader::document::ObjectId object_;
	std::string next_name_;
	std::optional<std::string> previous_name_;
};

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
	ASSERT_EQ(kAfter.selected_objects.size(), 1U);
	EXPECT_EQ(kAfter.selected_objects.front(), kAfter.objects.front().id);
	EXPECT_TRUE(kAfter.selected_components.empty());

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
	ASSERT_EQ(kRedoneState.selected_objects.size(), 1U);
	EXPECT_EQ(kRedoneState.selected_objects.front(), kRedoneState.objects.front().id);
}

TEST(Command, CreateMeshObjectCommandSelectsOnlyNewObjectAndRestoresPriorSelection) {
	auto fixture = quader::tests::document_fixtures::make_document_with_triangle_object();
	quader::document::Selection selection;
	ASSERT_TRUE(selection.set_objects({ fixture.object }));
	ASSERT_TRUE(fixture.document.set_selection(selection));
	fixture.document.clear_dirty();
	discard_pending_changes(fixture.document);

	auto mesh = quader::tests::document_fixtures::make_triangle_mesh();
	quader::commands::CommandHistory history;
	auto command = std::make_unique<quader::commands::CreateMeshObjectCommand>(
			"Created",
			std::move(mesh.mesh));
	auto *create_command = command.get();
	auto result = history.execute(std::move(command), fixture.document);
	EXPECT_EQ((result).status, quader::commands::CommandStatus::Applied);
	ASSERT_TRUE(create_command->object_id().has_value());
	const quader::document::ObjectId kCreated = *create_command->object_id();
	EXPECT_NE(kCreated, fixture.object);
	ASSERT_EQ(fixture.document.selection().selected_objects().size(), 1U);
	EXPECT_EQ(fixture.document.selection().selected_objects().front(), kCreated);

	result = history.undo(fixture.document);
	EXPECT_EQ((result).status, quader::commands::CommandStatus::Applied);
	EXPECT_TRUE(fixture.document.find_mesh_object(kCreated) == nullptr);
	ASSERT_EQ(fixture.document.selection().selected_objects().size(), 1U);
	EXPECT_EQ(fixture.document.selection().selected_objects().front(), fixture.object);

	result = history.redo(fixture.document);
	EXPECT_EQ((result).status, quader::commands::CommandStatus::Applied);
	EXPECT_TRUE(fixture.document.find_mesh_object(kCreated) != nullptr);
	ASSERT_EQ(fixture.document.selection().selected_objects().size(), 1U);
	EXPECT_EQ(fixture.document.selection().selected_objects().front(), kCreated);
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

TEST(Command, BatchTransformCommandExecutesUndoesAndRedoesMultipleObjects) {
	auto fixture = quader::tests::document_fixtures::make_document_with_triangle_object();
	auto second_mesh = quader::tests::document_fixtures::make_triangle_mesh();
	const auto kSecond = fixture.document.create_mesh_object("Second", std::move(second_mesh.mesh));
	ASSERT_TRUE(kSecond);
	fixture.document.clear_dirty();
	discard_pending_changes(fixture.document);

	quader::commands::CommandHistory history;
	const auto kBefore = capture_command_state(fixture.document);
	quader::document::Transform first_transform;
	first_transform.translation = { 1.0F, 0.0F, 0.0F };
	quader::document::Transform second_transform;
	second_transform.scale = { 2.0F, 2.0F, 2.0F };

	auto result = history.execute(std::make_unique<quader::commands::BatchSetObjectTransformsCommand>(
										  std::vector<quader::commands::ObjectTransformEdit>{
												  { fixture.object, first_transform },
												  { kSecond.value(), second_transform },
										  }),
			fixture.document);
	EXPECT_EQ((result).status, quader::commands::CommandStatus::Applied);
	EXPECT_EQ(history.undo_name(), std::string_view("Transform Objects"));

	const auto kAfter = capture_command_state(fixture.document);
	ASSERT_EQ(kAfter.objects.size(), 2U);
	EXPECT_TRUE(kAfter.objects[0].transform == first_transform);
	EXPECT_TRUE(kAfter.objects[1].transform == second_transform);

	result = history.undo(fixture.document);
	EXPECT_EQ((result).status, quader::commands::CommandStatus::Applied);
	EXPECT_EQ(capture_command_state(fixture.document), kBefore);

	result = history.redo(fixture.document);
	EXPECT_EQ((result).status, quader::commands::CommandStatus::Applied);
	EXPECT_EQ(capture_command_state(fixture.document), kAfter);
}

TEST(Command, BatchTransformCommandRejectsInvalidOrNoopBatchesWithoutMutation) {
	auto fixture = quader::tests::document_fixtures::make_document_with_triangle_object();
	fixture.document.clear_dirty();
	discard_pending_changes(fixture.document);
	const auto kBefore = capture_command_state(fixture.document);

	quader::commands::CommandHistory history;
	auto result = history.execute(std::make_unique<quader::commands::BatchSetObjectTransformsCommand>(
										  std::vector<quader::commands::ObjectTransformEdit>{}),
			fixture.document);
	EXPECT_EQ((result).status, quader::commands::CommandStatus::Rejected);
	EXPECT_EQ(capture_command_state(fixture.document), kBefore);

	quader::document::Transform invalid_transform;
	invalid_transform.translation.x = std::numeric_limits<float>::infinity();
	result = history.execute(std::make_unique<quader::commands::BatchSetObjectTransformsCommand>(
									 std::vector<quader::commands::ObjectTransformEdit>{
											 { fixture.object, invalid_transform },
									 }),
			fixture.document);
	EXPECT_EQ((result).status, quader::commands::CommandStatus::Rejected);
	EXPECT_EQ(capture_command_state(fixture.document), kBefore);

	quader::document::Transform changed;
	changed.translation = { 1.0F, 0.0F, 0.0F };
	result = history.execute(std::make_unique<quader::commands::BatchSetObjectTransformsCommand>(
									 std::vector<quader::commands::ObjectTransformEdit>{
											 { fixture.object, changed },
											 { fixture.object, changed },
									 }),
			fixture.document);
	EXPECT_EQ((result).status, quader::commands::CommandStatus::Rejected);
	EXPECT_EQ(capture_command_state(fixture.document), kBefore);
	EXPECT_FALSE(history.can_undo());
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

TEST(Command, EditSelectionCommandAddsRemovesTogglesAndRestoresSelection) {
	auto fixture = quader::tests::document_fixtures::make_document_with_triangle_object();
	auto second_mesh = quader::tests::document_fixtures::make_triangle_mesh();
	const auto kSecond = fixture.document.create_mesh_object("Second", std::move(second_mesh.mesh));
	ASSERT_TRUE(kSecond);
	fixture.document.clear_dirty();
	discard_pending_changes(fixture.document);

	quader::commands::CommandHistory history;
	quader::document::Selection first_selection;
	ASSERT_TRUE(first_selection.set_objects({ fixture.object }));
	auto result = history.execute(std::make_unique<quader::commands::EditSelectionCommand>(
										  quader::commands::SelectionEditOperation::Replace,
										  first_selection),
			fixture.document);
	EXPECT_EQ((result).status, quader::commands::CommandStatus::Applied);

	quader::document::Selection second_selection;
	ASSERT_TRUE(second_selection.set_objects({ kSecond.value() }));
	result = history.execute(std::make_unique<quader::commands::EditSelectionCommand>(
									 quader::commands::SelectionEditOperation::Add,
									 second_selection),
			fixture.document);
	EXPECT_EQ((result).status, quader::commands::CommandStatus::Applied);
	EXPECT_EQ(fixture.document.selection().selected_objects().size(), 2U);

	result = history.undo(fixture.document);
	EXPECT_EQ((result).status, quader::commands::CommandStatus::Applied);
	ASSERT_EQ(fixture.document.selection().selected_objects().size(), 1U);
	EXPECT_EQ(fixture.document.selection().selected_objects().front(), fixture.object);

	result = history.redo(fixture.document);
	EXPECT_EQ((result).status, quader::commands::CommandStatus::Applied);
	EXPECT_EQ(fixture.document.selection().selected_objects().size(), 2U);

	result = history.execute(std::make_unique<quader::commands::EditSelectionCommand>(
									 quader::commands::SelectionEditOperation::Toggle,
									 second_selection),
			fixture.document);
	EXPECT_EQ((result).status, quader::commands::CommandStatus::Applied);
	ASSERT_EQ(fixture.document.selection().selected_objects().size(), 1U);
	EXPECT_EQ(fixture.document.selection().selected_objects().front(), fixture.object);

	result = history.execute(std::make_unique<quader::commands::EditSelectionCommand>(
									 quader::commands::SelectionEditOperation::Remove,
									 first_selection),
			fixture.document);
	EXPECT_EQ((result).status, quader::commands::CommandStatus::Applied);
	EXPECT_TRUE(fixture.document.selection().empty());
}

TEST(Command, SetSelectionModeCommandPreservesObjectTargetsAndUndoRedo) {
	auto fixture = quader::tests::document_fixtures::make_document_with_triangle_object();
	quader::document::Selection selection;
	ASSERT_TRUE(selection.set_objects({ fixture.object }));
	ASSERT_TRUE(fixture.document.set_selection(selection));
	fixture.document.clear_dirty();
	discard_pending_changes(fixture.document);

	quader::commands::CommandHistory history;
	const auto kBefore = capture_command_state(fixture.document);
	auto result = history.execute(std::make_unique<quader::commands::SetSelectionModeCommand>(
										  quader::document::SelectionMode::Face),
			fixture.document);
	EXPECT_EQ((result).status, quader::commands::CommandStatus::Applied);
	EXPECT_EQ(fixture.document.selection().mode(), quader::document::SelectionMode::Face);
	ASSERT_EQ(fixture.document.selection().selected_objects().size(), 1U);
	EXPECT_EQ(fixture.document.selection().selected_objects().front(), fixture.object);
	EXPECT_TRUE(fixture.document.selection().selected_components().empty());

	const auto kAfter = capture_command_state(fixture.document);
	result = history.undo(fixture.document);
	EXPECT_EQ((result).status, quader::commands::CommandStatus::Applied);
	EXPECT_EQ(capture_command_state(fixture.document), kBefore);

	result = history.redo(fixture.document);
	EXPECT_EQ((result).status, quader::commands::CommandStatus::Applied);
	EXPECT_EQ(capture_command_state(fixture.document), kAfter);
}

TEST(Command, SelectAllAndInvertSelectionRespectActiveMode) {
	auto fixture = quader::tests::document_fixtures::make_document_with_triangle_object();
	auto second_mesh = quader::tests::document_fixtures::make_triangle_mesh();
	const auto kSecond = fixture.document.create_mesh_object("Second", std::move(second_mesh.mesh));
	ASSERT_TRUE(kSecond);
	fixture.document.clear_dirty();
	discard_pending_changes(fixture.document);

	quader::commands::CommandHistory history;
	auto result = history.execute(std::make_unique<quader::commands::SelectAllCommand>(),
			fixture.document);
	EXPECT_EQ((result).status, quader::commands::CommandStatus::Applied);
	EXPECT_EQ(fixture.document.selection().mode(), quader::document::SelectionMode::Object);
	EXPECT_EQ(fixture.document.selection().selected_objects().size(), 2U);

	result = history.execute(std::make_unique<quader::commands::InvertSelectionCommand>(),
			fixture.document);
	EXPECT_EQ((result).status, quader::commands::CommandStatus::Applied);
	EXPECT_TRUE(fixture.document.selection().empty());

	quader::document::Selection vertex_targets;
	ASSERT_TRUE(vertex_targets.set_component_selection(
			quader::document::SelectionMode::Vertex,
			{ fixture.object },
			{}));
	ASSERT_TRUE(fixture.document.set_selection(vertex_targets));
	result = history.execute(std::make_unique<quader::commands::SelectAllCommand>(),
			fixture.document);
	EXPECT_EQ((result).status, quader::commands::CommandStatus::Applied);
	EXPECT_EQ(fixture.document.selection().mode(), quader::document::SelectionMode::Vertex);
	ASSERT_EQ(fixture.document.selection().selected_objects().size(), 1U);
	EXPECT_EQ(fixture.document.selection().selected_objects().front(), fixture.object);
	EXPECT_EQ(fixture.document.selection().selected_components().size(), 3U);
}

TEST(Command, SelectionCommandsRejectStaleCurrentSelectionWithoutMutation) {
	auto fixture = quader::tests::document_fixtures::make_document_with_triangle_object();
	quader::document::Selection face_selection;
	ASSERT_TRUE(face_selection.set_components({
			quader::document::ComponentRef{ fixture.object, fixture.face },
	}));
	ASSERT_TRUE(fixture.document.set_selection(face_selection));
	fixture.document.clear_dirty();
	discard_pending_changes(fixture.document);
	const auto kBefore = capture_command_state(fixture.document);

	auto *object = fixture.document.find_mesh_object(fixture.object);
	ASSERT_TRUE(object != nullptr);
	ASSERT_TRUE(object->mesh.delete_face(fixture.face));

	quader::commands::CommandHistory history;
	quader::document::Selection replacement;
	ASSERT_TRUE(replacement.set_objects({ fixture.object }));
	auto result = history.execute(std::make_unique<quader::commands::EditSelectionCommand>(
										  quader::commands::SelectionEditOperation::Replace,
										  replacement),
			fixture.document);
	EXPECT_EQ((result).status, quader::commands::CommandStatus::Rejected);
	EXPECT_FALSE(history.can_undo());
	EXPECT_EQ(capture_command_state(fixture.document).selected_components, kBefore.selected_components);
}

TEST(Command, FlipMeshNormalsCommandFlipsSelectedFacesAndUndoRedoRestoresWindingAndSelection) {
	auto fixture = quader::tests::document_fixtures::make_document_with_triangle_object();
	quader::document::Selection face_selection;
	ASSERT_TRUE(face_selection.set_components({
			quader::document::ComponentRef{ fixture.object, fixture.face },
	}));
	ASSERT_TRUE(fixture.document.set_selection(face_selection));
	fixture.document.clear_dirty();
	discard_pending_changes(fixture.document);

	quader::commands::CommandHistory history;
	const auto kBeforeState = capture_command_state(fixture.document);
	const std::array<quader::mesh::VertexId, 3> kOriginal{
		fixture.vertices[0],
		fixture.vertices[1],
		fixture.vertices[2],
	};
	const std::array<quader::mesh::VertexId, 3> kReversed{
		fixture.vertices[0],
		fixture.vertices[2],
		fixture.vertices[1],
	};
	EXPECT_TRUE(is_same_cyclic_vertex_order(
			face_vertices_for(fixture.document, fixture.object, fixture.face),
			kOriginal));

	auto result = history.execute(std::make_unique<quader::commands::FlipMeshNormalsCommand>(),
			fixture.document);
	EXPECT_EQ((result).status, quader::commands::CommandStatus::Applied);
	EXPECT_TRUE(is_same_cyclic_vertex_order(
			face_vertices_for(fixture.document, fixture.object, fixture.face),
			kReversed));
	EXPECT_EQ(capture_command_state(fixture.document).selected_components, kBeforeState.selected_components);

	result = history.undo(fixture.document);
	EXPECT_EQ((result).status, quader::commands::CommandStatus::Applied);
	EXPECT_TRUE(is_same_cyclic_vertex_order(
			face_vertices_for(fixture.document, fixture.object, fixture.face),
			kOriginal));
	EXPECT_EQ(capture_command_state(fixture.document), kBeforeState);

	result = history.redo(fixture.document);
	EXPECT_EQ((result).status, quader::commands::CommandStatus::Applied);
	EXPECT_TRUE(is_same_cyclic_vertex_order(
			face_vertices_for(fixture.document, fixture.object, fixture.face),
			kReversed));
	EXPECT_EQ(capture_command_state(fixture.document).selected_components, kBeforeState.selected_components);
}

TEST(Command, FlipMeshNormalsCommandFlipsSelectedMeshObjects) {
	auto fixture = quader::tests::document_fixtures::make_document_with_triangle_object();
	quader::document::Selection object_selection;
	ASSERT_TRUE(object_selection.set_objects({ fixture.object }));
	ASSERT_TRUE(fixture.document.set_selection(object_selection));
	fixture.document.clear_dirty();
	discard_pending_changes(fixture.document);

	const std::array<quader::mesh::VertexId, 3> kReversed{
		fixture.vertices[0],
		fixture.vertices[2],
		fixture.vertices[1],
	};

	quader::commands::CommandHistory history;
	auto result = history.execute(std::make_unique<quader::commands::FlipMeshNormalsCommand>(),
			fixture.document);
	EXPECT_EQ((result).status, quader::commands::CommandStatus::Applied);
	EXPECT_TRUE(is_same_cyclic_vertex_order(
			face_vertices_for(fixture.document, fixture.object, fixture.face),
			kReversed));
	ASSERT_EQ(fixture.document.selection().selected_objects().size(), 1U);
	EXPECT_EQ(fixture.document.selection().selected_objects().front(), fixture.object);
}

TEST(Command, FlipMeshNormalsCommandRejectsEmptySelectionWithoutMutation) {
	auto fixture = quader::tests::document_fixtures::make_document_with_triangle_object();
	fixture.document.clear_dirty();
	discard_pending_changes(fixture.document);
	const auto kBefore = capture_command_state(fixture.document);

	quader::commands::CommandHistory history;
	auto result = history.execute(std::make_unique<quader::commands::FlipMeshNormalsCommand>(),
			fixture.document);
	EXPECT_EQ((result).status, quader::commands::CommandStatus::Rejected);
	EXPECT_FALSE(history.can_undo());
	EXPECT_EQ(capture_command_state(fixture.document), kBefore);
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

TEST(Command, HistoryMergesExecutedCommandIntoPreviousUndoEntry) {
	auto fixture = quader::tests::document_fixtures::make_document_with_triangle_object();
	fixture.document.clear_dirty();
	discard_pending_changes(fixture.document);
	const auto kOriginal = capture_command_state(fixture.document);

	quader::commands::CommandHistory history;
	auto result = history.execute(std::make_unique<MergeableRenameCommand>(fixture.object, "First"),
			fixture.document);
	EXPECT_EQ((result).status, quader::commands::CommandStatus::Applied);
	result = history.execute(std::make_unique<MergeableRenameCommand>(fixture.object, "Second"),
			fixture.document);
	EXPECT_EQ((result).status, quader::commands::CommandStatus::Applied);

	auto *object = fixture.document.find_mesh_object(fixture.object);
	ASSERT_TRUE(object != nullptr);
	EXPECT_EQ(object->name, std::string("Second"));
	EXPECT_TRUE(history.can_undo());
	EXPECT_EQ(history.undo_name(), std::string_view("Mergeable Rename"));

	result = history.undo(fixture.document);
	EXPECT_EQ((result).status, quader::commands::CommandStatus::Applied);
	EXPECT_EQ(capture_command_state(fixture.document), kOriginal);
	EXPECT_FALSE(history.can_undo());
	EXPECT_TRUE(history.can_redo());

	result = history.redo(fixture.document);
	EXPECT_EQ((result).status, quader::commands::CommandStatus::Applied);
	object = fixture.document.find_mesh_object(fixture.object);
	ASSERT_TRUE(object != nullptr);
	EXPECT_EQ(object->name, std::string("Second"));
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
