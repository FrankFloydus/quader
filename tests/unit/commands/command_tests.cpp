#include "../document/document_test_helpers.hpp"

#include <gtest/gtest.h>

#include "foundation/logging.hpp"
#include "commands/command_history.hpp"
#include "commands/document_commands.hpp"
#include "commands/selection_commands.hpp"

#include <limits>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

namespace {

using quader::tests::document_fixtures::DocumentSemanticState;

void discard_pending_changes(quader::document::Document& document)
{
    const auto changes = document.take_pending_changes();
    (void)changes;
}

DocumentSemanticState capture_command_state(const quader::document::Document& document)
{
    auto state = quader::tests::document_fixtures::capture_document_state(document);
    state.dirty = false;
    state.dirty_flags = quader::document::kDocumentDirtyNone;
    return state;
}

quader::document::Transform translated_transform(float x, float y, float z)
{
    quader::document::Transform transform;
    transform.translation = quader::math::Vec3{x, y, z};
    return transform;
}

TEST(Command, CreateMeshObjectCommandExecutesUndoesAndRedoesSemanticState)
{
    quader::document::Document document;
    quader::commands::CommandHistory history;
    const auto before = capture_command_state(document);

    auto mesh = quader::tests::document_fixtures::make_triangle_mesh();
    const auto transform = translated_transform(2.0F, 3.0F, 4.0F);

    auto result = history.execute(std::make_unique<quader::commands::CreateMeshObjectCommand>(
                                      "Triangle",
                                      std::move(mesh.mesh),
                                      transform),
                                  document);
    EXPECT_EQ((result).status, quader::commands::CommandStatus::Applied);
    EXPECT_TRUE(history.can_undo());
    EXPECT_FALSE(history.can_redo());
    EXPECT_EQ(history.undo_name(), std::string_view("Create Mesh Object"));

    const auto after = capture_command_state(document);
    EXPECT_NE(after, before);
    EXPECT_EQ(after.objects.size(), 1U);
    EXPECT_EQ(after.objects.front().name, std::string("Triangle"));
    EXPECT_TRUE(after.objects.front().transform == transform);
    EXPECT_EQ(after.objects.front().vertex_count, 3U);
    EXPECT_EQ(after.objects.front().face_count, 1U);

    result = history.undo(document);
    EXPECT_EQ((result).status, quader::commands::CommandStatus::Applied);
    EXPECT_EQ(capture_command_state(document), before);
    EXPECT_FALSE(history.can_undo());
    EXPECT_TRUE(history.can_redo());
    EXPECT_EQ(history.redo_name(), std::string_view("Create Mesh Object"));

    result = history.redo(document);
    EXPECT_EQ((result).status, quader::commands::CommandStatus::Applied);
    EXPECT_EQ(capture_command_state(document), after);
    EXPECT_TRUE(history.can_undo());
    EXPECT_FALSE(history.can_redo());
}

TEST(Command, DeleteObjectCommandRestoresObjectAndSelectionOnUndoRedo)
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

    quader::commands::CommandHistory history;
    const auto before = capture_command_state(fixture.document);

    auto result = history.execute(
        std::make_unique<quader::commands::DeleteObjectCommand>(fixture.object),
        fixture.document);
    EXPECT_EQ((result).status, quader::commands::CommandStatus::Applied);

    const auto after = capture_command_state(fixture.document);
    EXPECT_EQ(after.objects.size(), 1U);
    EXPECT_EQ(after.objects.front().id, second.value());
    EXPECT_EQ(after.selected_objects.size(), 1U);
    EXPECT_EQ(after.selected_objects.front(), second.value());

    result = history.undo(fixture.document);
    EXPECT_EQ((result).status, quader::commands::CommandStatus::Applied);
    EXPECT_EQ(capture_command_state(fixture.document), before);

    result = history.redo(fixture.document);
    EXPECT_EQ((result).status, quader::commands::CommandStatus::Applied);
    EXPECT_EQ(capture_command_state(fixture.document), after);
}

TEST(Command, RenameObjectCommandExecutesUndoesAndRedoesName)
{
    auto fixture = quader::tests::document_fixtures::make_document_with_triangle_object();
    fixture.document.clear_dirty();
    discard_pending_changes(fixture.document);

    quader::commands::CommandHistory history;
    const auto before = capture_command_state(fixture.document);

    auto result = history.execute(
        std::make_unique<quader::commands::RenameObjectCommand>(fixture.object, "Renamed"),
        fixture.document);
    EXPECT_EQ((result).status, quader::commands::CommandStatus::Applied);

    const auto after = capture_command_state(fixture.document);
    EXPECT_EQ(after.objects.size(), 1U);
    EXPECT_EQ(after.objects.front().name, std::string("Renamed"));

    result = history.undo(fixture.document);
    EXPECT_EQ((result).status, quader::commands::CommandStatus::Applied);
    EXPECT_EQ(capture_command_state(fixture.document), before);

    result = history.redo(fixture.document);
    EXPECT_EQ((result).status, quader::commands::CommandStatus::Applied);
    EXPECT_EQ(capture_command_state(fixture.document), after);
}

TEST(Command, TransformCommandExecutesUndoesAndRedoesTransform)
{
    auto fixture = quader::tests::document_fixtures::make_document_with_triangle_object();
    fixture.document.clear_dirty();
    discard_pending_changes(fixture.document);

    quader::commands::CommandHistory history;
    const auto before = capture_command_state(fixture.document);
    const auto transform = translated_transform(5.0F, 6.0F, 7.0F);

    auto result = history.execute(
        std::make_unique<quader::commands::SetObjectTransformCommand>(fixture.object, transform),
        fixture.document);
    EXPECT_EQ((result).status, quader::commands::CommandStatus::Applied);

    const auto after = capture_command_state(fixture.document);
    EXPECT_EQ(after.objects.size(), 1U);
    EXPECT_TRUE(after.objects.front().transform == transform);

    result = history.undo(fixture.document);
    EXPECT_EQ((result).status, quader::commands::CommandStatus::Applied);
    EXPECT_EQ(capture_command_state(fixture.document), before);

    result = history.redo(fixture.document);
    EXPECT_EQ((result).status, quader::commands::CommandStatus::Applied);
    EXPECT_EQ(capture_command_state(fixture.document), after);
}

TEST(Command, SetSelectionCommandExecutesUndoesAndRedoesSelection)
{
    auto fixture = quader::tests::document_fixtures::make_document_with_triangle_object();
    fixture.document.clear_dirty();
    discard_pending_changes(fixture.document);

    quader::document::Selection selection;
    EXPECT_TRUE(selection.set_objects({fixture.object}));

    quader::commands::CommandHistory history;
    const auto before = capture_command_state(fixture.document);

    auto result = history.execute(
        std::make_unique<quader::commands::SetSelectionCommand>(selection),
        fixture.document);
    EXPECT_EQ((result).status, quader::commands::CommandStatus::Applied);

    const auto after = capture_command_state(fixture.document);
    EXPECT_EQ(after.selected_objects.size(), 1U);
    EXPECT_EQ(after.selected_objects.front(), fixture.object);

    result = history.undo(fixture.document);
    EXPECT_EQ((result).status, quader::commands::CommandStatus::Applied);
    EXPECT_EQ(capture_command_state(fixture.document), before);

    result = history.redo(fixture.document);
    EXPECT_EQ((result).status, quader::commands::CommandStatus::Applied);
    EXPECT_EQ(capture_command_state(fixture.document), after);
}

TEST(Command, ClearSelectionCommandExecutesUndoesAndRedoesSelection)
{
    auto fixture = quader::tests::document_fixtures::make_document_with_triangle_object();
    quader::document::Selection selection;
    EXPECT_TRUE(selection.set_objects({fixture.object}));
    EXPECT_TRUE(fixture.document.set_selection(selection));
    fixture.document.clear_dirty();
    discard_pending_changes(fixture.document);

    quader::commands::CommandHistory history;
    const auto before = capture_command_state(fixture.document);

    auto result = history.execute(std::make_unique<quader::commands::ClearSelectionCommand>(),
                                  fixture.document);
    EXPECT_EQ((result).status, quader::commands::CommandStatus::Applied);

    const auto after = capture_command_state(fixture.document);
    EXPECT_TRUE(after.selected_objects.empty());
    EXPECT_TRUE(after.selected_components.empty());

    result = history.undo(fixture.document);
    EXPECT_EQ((result).status, quader::commands::CommandStatus::Applied);
    EXPECT_EQ(capture_command_state(fixture.document), before);

    result = history.redo(fixture.document);
    EXPECT_EQ((result).status, quader::commands::CommandStatus::Applied);
    EXPECT_EQ(capture_command_state(fixture.document), after);
}

TEST(Command, RejectedCommandsDoNotMutateDocumentOrHistory)
{
    auto fixture = quader::tests::document_fixtures::make_document_with_triangle_object();
    fixture.document.clear_dirty();
    discard_pending_changes(fixture.document);

    quader::commands::CommandHistory history;
    const auto before = capture_command_state(fixture.document);

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
    EXPECT_EQ(capture_command_state(fixture.document), before);

    quader::document::Selection invalid_selection;
    EXPECT_TRUE(invalid_selection.set_objects({quader::document::ObjectId{999U, 1U}}));
    result = history.execute(
        std::make_unique<quader::commands::SetSelectionCommand>(invalid_selection),
        fixture.document);
    EXPECT_EQ((result).status, quader::commands::CommandStatus::Rejected);
    EXPECT_FALSE(history.can_undo());
    EXPECT_EQ(capture_command_state(fixture.document), before);

    result = history.execute(
        std::make_unique<quader::commands::RenameObjectCommand>(
            quader::document::ObjectId{999U, 1U},
            "Missing"),
        fixture.document);
    EXPECT_EQ((result).status, quader::commands::CommandStatus::Rejected);
    EXPECT_FALSE(history.can_undo());
    EXPECT_EQ(capture_command_state(fixture.document), before);

    result = history.execute(nullptr, fixture.document);
    EXPECT_EQ((result).status, quader::commands::CommandStatus::Rejected);
    EXPECT_FALSE(history.can_undo());
    EXPECT_EQ(capture_command_state(fixture.document), before);
}

TEST(Command, HistoryTruncatesRedoAfterNewCommandAndReportsAvailability)
{
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
    EXPECT_TRUE(selection.set_objects({fixture.object}));
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

TEST(Command, CommandMergeHooksDefaultToNotMergeable)
{
    quader::commands::RenameObjectCommand first{quader::document::ObjectId{0U, 1U}, "First"};
    quader::commands::RenameObjectCommand second{quader::document::ObjectId{0U, 1U}, "Second"};

    EXPECT_FALSE(first.can_merge_with(second));
    auto result = first.merge_with(std::make_unique<quader::commands::RenameObjectCommand>(
        quader::document::ObjectId{0U, 1U},
        "Third"));
    EXPECT_EQ((result).status, quader::commands::CommandStatus::NotMergeable);
}

} // namespace
