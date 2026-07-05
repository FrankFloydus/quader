#include "../document/document_test_helpers.hpp"

#include <gtest/gtest.h>

#include "commands/command_history.hpp"
#include "commands/document_commands.hpp"
#include "document/selection.hpp"
#include "io/import_export_registry.hpp"
#include "tools/tool.hpp"
#include "tools/tool_manager.hpp"
#include "ui/actions/action_registry.hpp"
#include "ui/actions/action_state_updater.hpp"
#include "ui/actions/editor_action_controller.hpp"
#include "ui/actions/editor_action_state_provider.hpp"
#include "ui/services/document_ui_controller.hpp"
#include "ui/services/notification_service.hpp"

#include <QAction>
#include <QApplication>

#include <cstdio>
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

DocumentSemanticState capture_ui_document_state(const quader::document::Document& document)
{
    auto state = quader::tests::document_fixtures::capture_document_state(document);
    state.dirty = false;
    state.dirty_flags = quader::document::kDocumentDirtyNone;
    return state;
}

quader::document::ObjectId add_triangle_object(quader::document::Document& document)
{
    auto mesh = quader::tests::document_fixtures::make_triangle_mesh();
    auto created = document.create_mesh_object("Triangle", std::move(mesh.mesh));
    EXPECT_TRUE(created);
    return created.value();
}

class CountingTool final : public quader::tools::ITool {
public:
    explicit CountingTool(quader::tools::ToolId tool_id) noexcept
        : tool_id_(tool_id)
    {
    }

    [[nodiscard]] quader::tools::ToolId id() const noexcept override
    {
        return tool_id_;
    }

    void activate(quader::tools::ToolContext& context) override
    {
        (void)context;
        ++activations;
    }

    void deactivate(quader::tools::ToolContext& context) override
    {
        (void)context;
        ++deactivations;
    }

    int activations = 0;
    int deactivations = 0;

private:
    quader::tools::ToolId tool_id_;
};

struct UiActionFixture {
    quader::document::Document document;
    quader::commands::CommandHistory command_history;
    quader::tools::ToolManager tool_manager;
    quader::io::ImportExportRegistry io_registry;
    quader::ui::ActionRegistry actions;
    quader::ui::EditorActionStateProvider editor_state;
    quader::ui::ActionStateUpdater action_state_updater;
    quader::ui::NotificationService notifications;
    quader::ui::DocumentUiController document_ui;
    quader::ui::EditorActionController editor_actions;

    UiActionFixture()
        : tool_manager(quader::tools::ToolContext{document, command_history})
        , editor_state(document, command_history, tool_manager, io_registry)
        , action_state_updater(actions, editor_state)
        , document_ui(document, command_history, action_state_updater, notifications)
        , editor_actions(actions,
                         document_ui,
                         tool_manager,
                         action_state_updater,
                         notifications)
    {
        quader::ui::register_standard_actions(actions);
        action_state_updater.refresh();
    }
};

TEST(UiActionWiring, StateProviderDrivesUndoRedoTextAndKeepsUnavailableActionsDisabled)
{
    UiActionFixture fixture;
    const auto object = add_triangle_object(fixture.document);
    fixture.document.clear_dirty();
    discard_pending_changes(fixture.document);

    auto result = fixture.command_history.execute(
        std::make_unique<quader::commands::RenameObjectCommand>(object, "Renamed"),
        fixture.document);
    EXPECT_EQ((result).status, quader::commands::CommandStatus::Applied);

    fixture.action_state_updater.refresh();

    EXPECT_TRUE(fixture.actions.action(quader::ui::ActionId::Undo).isEnabled());
    EXPECT_EQ(fixture.actions.action(quader::ui::ActionId::Undo).text(),
              QStringLiteral("Undo Rename Object"));
    EXPECT_FALSE(fixture.actions.action(quader::ui::ActionId::Redo).isEnabled());
    EXPECT_EQ(fixture.actions.action(quader::ui::ActionId::Redo).text(), QStringLiteral("&Redo"));

    EXPECT_FALSE(fixture.actions.action(quader::ui::ActionId::NewScene).isEnabled());
    EXPECT_FALSE(fixture.actions.action(quader::ui::ActionId::OpenScene).isEnabled());
    EXPECT_FALSE(fixture.actions.action(quader::ui::ActionId::SaveScene).isEnabled());
    EXPECT_FALSE(fixture.actions.action(quader::ui::ActionId::SaveSceneAs).isEnabled());
    EXPECT_FALSE(fixture.actions.action(quader::ui::ActionId::CreateCube).isEnabled());
    EXPECT_FALSE(fixture.actions.action(quader::ui::ActionId::CreateLight).isEnabled());
    EXPECT_FALSE(fixture.actions.action(quader::ui::ActionId::CreateCamera).isEnabled());
    EXPECT_FALSE(fixture.actions.action(quader::ui::ActionId::DuplicateSelection).isEnabled());
}

TEST(UiActionWiring, UndoAndRedoActionsExecuteThroughCommandHistory)
{
    UiActionFixture fixture;
    const auto object = add_triangle_object(fixture.document);
    fixture.document.clear_dirty();
    discard_pending_changes(fixture.document);

    const auto before = capture_ui_document_state(fixture.document);
    auto result = fixture.command_history.execute(
        std::make_unique<quader::commands::RenameObjectCommand>(object, "Renamed"),
        fixture.document);
    EXPECT_EQ((result).status, quader::commands::CommandStatus::Applied);
    const auto after = capture_ui_document_state(fixture.document);
    discard_pending_changes(fixture.document);

    int command_history_changes = 0;
    int drained_events = 0;
    QObject::connect(&fixture.document_ui,
                     &quader::ui::DocumentUiController::command_history_changed,
                     &fixture.document_ui,
                     [&command_history_changes]() {
                         ++command_history_changes;
                     });
    QObject::connect(&fixture.document_ui,
                     &quader::ui::DocumentUiController::document_events_drained,
                     &fixture.document_ui,
                     [&drained_events]() {
                         ++drained_events;
                     });

    fixture.action_state_updater.refresh();
    fixture.actions.action(quader::ui::ActionId::Undo).trigger();
    EXPECT_EQ(capture_ui_document_state(fixture.document), before);
    EXPECT_EQ(command_history_changes, 1);
    EXPECT_EQ(drained_events, 1);
    EXPECT_FALSE(fixture.actions.action(quader::ui::ActionId::Undo).isEnabled());
    EXPECT_TRUE(fixture.actions.action(quader::ui::ActionId::Redo).isEnabled());
    EXPECT_EQ(fixture.actions.action(quader::ui::ActionId::Redo).text(),
              QStringLiteral("Redo Rename Object"));

    fixture.actions.action(quader::ui::ActionId::Redo).trigger();
    EXPECT_EQ(capture_ui_document_state(fixture.document), after);
    EXPECT_EQ(command_history_changes, 2);
    EXPECT_EQ(drained_events, 2);
    EXPECT_TRUE(fixture.actions.action(quader::ui::ActionId::Undo).isEnabled());
    EXPECT_FALSE(fixture.actions.action(quader::ui::ActionId::Redo).isEnabled());
}

TEST(UiActionWiring, DeleteSelectionActionDeletesOneSelectedObjectThroughCommandHistory)
{
    UiActionFixture fixture;
    const auto object = add_triangle_object(fixture.document);

    quader::document::Selection selection;
    EXPECT_TRUE(selection.set_objects({object}));
    EXPECT_TRUE(fixture.document.set_selection(selection));
    fixture.document.clear_dirty();
    discard_pending_changes(fixture.document);

    fixture.action_state_updater.refresh();
    EXPECT_TRUE(fixture.actions.action(quader::ui::ActionId::DeleteSelection).isEnabled());
    EXPECT_FALSE(fixture.actions.action(quader::ui::ActionId::DuplicateSelection).isEnabled());

    int object_list_changes = 0;
    int selection_changes = 0;
    int command_history_changes = 0;
    QObject::connect(&fixture.document_ui,
                     &quader::ui::DocumentUiController::object_list_changed,
                     &fixture.document_ui,
                     [&object_list_changes]() {
                         ++object_list_changes;
                     });
    QObject::connect(&fixture.document_ui,
                     &quader::ui::DocumentUiController::selection_changed,
                     &fixture.document_ui,
                     [&selection_changes]() {
                         ++selection_changes;
                     });
    QObject::connect(&fixture.document_ui,
                     &quader::ui::DocumentUiController::command_history_changed,
                     &fixture.document_ui,
                     [&command_history_changes]() {
                         ++command_history_changes;
                     });

    fixture.actions.action(quader::ui::ActionId::DeleteSelection).trigger();
    EXPECT_EQ(fixture.document.object_count(), 0U);
    EXPECT_TRUE(fixture.command_history.can_undo());
    EXPECT_EQ(fixture.command_history.undo_name(), std::string_view("Delete Object"));
    EXPECT_EQ(object_list_changes, 1);
    EXPECT_EQ(selection_changes, 1);
    EXPECT_EQ(command_history_changes, 1);
    EXPECT_FALSE(fixture.actions.action(quader::ui::ActionId::DeleteSelection).isEnabled());
    EXPECT_TRUE(fixture.actions.action(quader::ui::ActionId::Undo).isEnabled());

    fixture.actions.action(quader::ui::ActionId::Undo).trigger();
    EXPECT_EQ(fixture.document.object_count(), 1U);
    EXPECT_EQ(fixture.document.selection().selected_objects().size(), 1U);
    EXPECT_EQ(fixture.document.selection().selected_objects().front(), object);
    EXPECT_EQ(object_list_changes, 2);
    EXPECT_EQ(selection_changes, 2);
    EXPECT_EQ(command_history_changes, 2);
    EXPECT_TRUE(fixture.actions.action(quader::ui::ActionId::DeleteSelection).isEnabled());
}

TEST(UiActionWiring, ToolActionsRouteToToolManagerAndRefreshCheckedState)
{
    UiActionFixture fixture;
    auto select_tool = std::make_unique<CountingTool>(quader::tools::ToolId::Select);
    auto move_tool = std::make_unique<CountingTool>(quader::tools::ToolId::Move);
    auto rotate_tool = std::make_unique<CountingTool>(quader::tools::ToolId::Rotate);
    auto scale_tool = std::make_unique<CountingTool>(quader::tools::ToolId::Scale);
    auto* select = select_tool.get();
    auto* move = move_tool.get();

    EXPECT_TRUE(fixture.tool_manager.register_tool(std::move(select_tool)));
    EXPECT_TRUE(fixture.tool_manager.register_tool(std::move(move_tool)));
    EXPECT_TRUE(fixture.tool_manager.register_tool(std::move(rotate_tool)));
    EXPECT_TRUE(fixture.tool_manager.register_tool(std::move(scale_tool)));
    EXPECT_TRUE(fixture.tool_manager.set_active_tool(quader::tools::ToolId::Select));

    fixture.action_state_updater.refresh();
    EXPECT_TRUE(fixture.actions.action(quader::ui::ActionId::SelectTool).isEnabled());
    EXPECT_TRUE(fixture.actions.action(quader::ui::ActionId::SelectTool).isChecked());
    EXPECT_FALSE(fixture.actions.action(quader::ui::ActionId::MoveTool).isChecked());

    fixture.actions.action(quader::ui::ActionId::MoveTool).trigger();
    EXPECT_TRUE(fixture.tool_manager.active_tool_id().has_value());
    EXPECT_EQ(*fixture.tool_manager.active_tool_id(), quader::tools::ToolId::Move);
    EXPECT_EQ(select->deactivations, 1);
    EXPECT_EQ(move->activations, 1);
    EXPECT_FALSE(fixture.actions.action(quader::ui::ActionId::SelectTool).isChecked());
    EXPECT_TRUE(fixture.actions.action(quader::ui::ActionId::MoveTool).isChecked());
}

} // namespace
