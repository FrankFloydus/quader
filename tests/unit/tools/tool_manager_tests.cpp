#include "../document/document_test_helpers.hpp"

#include <gtest/gtest.h>

#include "foundation/logging.hpp"
#include "commands/document_commands.hpp"
#include "tools/tool_manager.hpp"

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace {

static_assert(std::is_same_v<decltype(std::declval<const quader::tools::ToolContext&>().document()),
                             const quader::document::Document&>);

using quader::tests::document_fixtures::DocumentSemanticState;

void discard_pending_changes(quader::document::Document& document)
{
    const auto changes = document.take_pending_changes();
    (void)changes;
}

DocumentSemanticState capture_tool_state(const quader::document::Document& document)
{
    auto state = quader::tests::document_fixtures::capture_document_state(document);
    state.dirty = false;
    state.dirty_flags = quader::document::kDocumentDirtyNone;
    return state;
}

bool vec2_exactly_equal(quader::math::Vec2 left, quader::math::Vec2 right)
{
    return left.x == right.x && left.y == right.y;
}

class RecordingTool final : public quader::tools::ITool {
public:
    explicit RecordingTool(quader::tools::ToolId tool_id,
                           std::optional<quader::document::ObjectId> command_object = std::nullopt)
        : tool_id_(tool_id)
        , command_object_(command_object)
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
        preview_data.clear();
    }

    void cancel(quader::tools::ToolContext& context) override
    {
        (void)context;
        ++cancellations;
        preview_data.clear();
    }

    [[nodiscard]] bool on_pointer_event(const quader::tools::PointerEvent& event,
                                        quader::tools::ToolContext& context) override
    {
        (void)context;
        ++pointer_events;
        last_pointer = event;
        preview_data.clear();
        preview_data.active = true;
        preview_data.status_text = "pointer";
        preview_data.points.push_back(event.position);
        return true;
    }

    [[nodiscard]] bool on_key_event(const quader::tools::KeyEvent& event,
                                    quader::tools::ToolContext& context) override
    {
        ++key_events;
        last_key = event;
        preview_data.clear();
        preview_data.active = true;
        preview_data.status_text = "key";

        if (command_object_.has_value() && event.pressed && !event.auto_repeat) {
            auto result = context.execute_command(
                std::make_unique<quader::commands::RenameObjectCommand>(*command_object_,
                                                                         "Renamed By Tool"));
            last_command_status = result.status;
            return result.is_applied();
        }

        return true;
    }

    [[nodiscard]] quader::tools::ToolPreview preview() const override
    {
        return preview_data;
    }

    int activations = 0;
    int deactivations = 0;
    int cancellations = 0;
    int pointer_events = 0;
    int key_events = 0;
    std::optional<quader::tools::PointerEvent> last_pointer;
    std::optional<quader::tools::KeyEvent> last_key;
    std::optional<quader::commands::CommandStatus> last_command_status;
    quader::tools::ToolPreview preview_data;

private:
    quader::tools::ToolId tool_id_;
    std::optional<quader::document::ObjectId> command_object_;
};

TEST(ToolManager, ToolActivationRegistersAndReusesActiveTool)
{
    quader::document::Document document;
    quader::commands::CommandHistory history;
    quader::tools::ToolManager manager{quader::tools::ToolContext{document, history}};

    auto select_tool = std::make_unique<RecordingTool>(quader::tools::ToolId::Select);
    auto* select = select_tool.get();

    EXPECT_TRUE(manager.register_tool(std::move(select_tool)));
    EXPECT_TRUE(manager.has_tool(quader::tools::ToolId::Select));
    EXPECT_FALSE(manager.has_tool(quader::tools::ToolId::Move));
    EXPECT_FALSE(manager.register_tool(nullptr));
    EXPECT_FALSE(manager.register_tool(
        std::make_unique<RecordingTool>(quader::tools::ToolId::Select)));

    EXPECT_TRUE(manager.set_active_tool(quader::tools::ToolId::Select));
    EXPECT_TRUE(manager.active_tool_id().has_value());
    EXPECT_EQ(*manager.active_tool_id(), quader::tools::ToolId::Select);
    EXPECT_EQ(manager.active_tool(), static_cast<quader::tools::ITool*>(select));
    EXPECT_EQ(select->activations, 1);
    EXPECT_EQ(select->deactivations, 0);

    EXPECT_TRUE(manager.set_active_tool(quader::tools::ToolId::Select));
    EXPECT_EQ(select->activations, 1);
    EXPECT_EQ(select->deactivations, 0);
    EXPECT_FALSE(manager.set_active_tool(quader::tools::ToolId::Move));
    EXPECT_EQ(*manager.active_tool_id(), quader::tools::ToolId::Select);
}

TEST(ToolManager, NoToolIgnoresEventsAndPreservesDocumentState)
{
    quader::document::Document document;
    quader::commands::CommandHistory history;
    quader::tools::ToolManager manager{quader::tools::ToolContext{document, history}};
    const auto before = capture_tool_state(document);

    EXPECT_FALSE(manager.active_tool_id().has_value());
    EXPECT_TRUE(manager.active_tool() == nullptr);
    EXPECT_TRUE(manager.preview().empty());
    EXPECT_FALSE(manager.cancel_active_tool());
    EXPECT_FALSE(manager.dispatch_pointer_event(quader::tools::PointerEvent{
        quader::math::Vec2{1.0F, 2.0F},
        quader::tools::PointerButton::Left,
        true,
    }));
    EXPECT_FALSE(manager.dispatch_key_event(quader::tools::KeyEvent{65, true, false}));
    EXPECT_FALSE(manager.set_active_tool(quader::tools::ToolId::Select));
    EXPECT_EQ(capture_tool_state(document), before);
    EXPECT_FALSE(history.can_undo());
    EXPECT_FALSE(history.can_redo());
}

TEST(ToolManager, EventsForwardToActiveToolAndPreviewIsReported)
{
    quader::document::Document document;
    quader::commands::CommandHistory history;
    quader::tools::ToolManager manager{quader::tools::ToolContext{document, history}};

    auto tool = std::make_unique<RecordingTool>(quader::tools::ToolId::Select);
    auto* recording_tool = tool.get();
    EXPECT_TRUE(manager.register_tool(std::move(tool)));
    EXPECT_TRUE(manager.set_active_tool(quader::tools::ToolId::Select));

    const quader::tools::PointerEvent pointer{
        quader::math::Vec2{12.0F, 24.0F},
        quader::tools::PointerButton::Right,
        true,
    };
    EXPECT_TRUE(manager.dispatch_pointer_event(pointer));
    EXPECT_EQ(recording_tool->pointer_events, 1);
    EXPECT_TRUE(recording_tool->last_pointer.has_value());
    EXPECT_TRUE(vec2_exactly_equal(recording_tool->last_pointer->position, pointer.position));
    EXPECT_EQ(recording_tool->last_pointer->button, quader::tools::PointerButton::Right);
    EXPECT_TRUE(recording_tool->last_pointer->pressed);

    auto preview = manager.preview();
    EXPECT_FALSE(preview.empty());
    EXPECT_TRUE(preview.active);
    EXPECT_EQ(preview.status_text, std::string("pointer"));
    EXPECT_EQ(preview.points.size(), 1U);
    EXPECT_TRUE(vec2_exactly_equal(preview.points.front(), pointer.position));

    const quader::tools::KeyEvent key{90, true, false};
    EXPECT_TRUE(manager.dispatch_key_event(key));
    EXPECT_EQ(recording_tool->key_events, 1);
    EXPECT_TRUE(recording_tool->last_key.has_value());
    EXPECT_EQ(recording_tool->last_key->key_code, 90);
    EXPECT_TRUE(recording_tool->last_key->pressed);
    EXPECT_FALSE(recording_tool->last_key->auto_repeat);
    EXPECT_EQ(manager.preview().status_text, std::string("key"));
}

TEST(ToolManager, ToolCommitsDocumentChangesOnlyThroughCommandHistory)
{
    auto fixture = quader::tests::document_fixtures::make_document_with_triangle_object();
    fixture.document.clear_dirty();
    discard_pending_changes(fixture.document);
    const auto before = capture_tool_state(fixture.document);

    quader::commands::CommandHistory history;
    quader::tools::ToolManager manager{quader::tools::ToolContext{fixture.document, history}};

    auto tool = std::make_unique<RecordingTool>(quader::tools::ToolId::Select, fixture.object);
    auto* recording_tool = tool.get();
    EXPECT_TRUE(manager.register_tool(std::move(tool)));
    EXPECT_TRUE(manager.set_active_tool(quader::tools::ToolId::Select));

    EXPECT_TRUE(manager.dispatch_pointer_event(quader::tools::PointerEvent{
        quader::math::Vec2{3.0F, 4.0F},
        quader::tools::PointerButton::Left,
        true,
    }));
    EXPECT_EQ(capture_tool_state(fixture.document), before);
    EXPECT_FALSE(history.can_undo());

    EXPECT_TRUE(manager.dispatch_key_event(quader::tools::KeyEvent{13, true, false}));
    EXPECT_TRUE(recording_tool->last_command_status.has_value());
    EXPECT_EQ(*recording_tool->last_command_status, quader::commands::CommandStatus::Applied);
    EXPECT_TRUE(history.can_undo());
    EXPECT_EQ(history.undo_name(), std::string_view("Rename Object"));

    const auto after = capture_tool_state(fixture.document);
    EXPECT_EQ(after.objects.size(), 1U);
    EXPECT_EQ(after.objects.front().name, std::string("Renamed By Tool"));

    auto result = history.undo(fixture.document);
    EXPECT_EQ((result).status, quader::commands::CommandStatus::Applied);
    EXPECT_EQ(capture_tool_state(fixture.document), before);

    result = history.redo(fixture.document);
    EXPECT_EQ((result).status, quader::commands::CommandStatus::Applied);
    EXPECT_EQ(capture_tool_state(fixture.document), after);
}

TEST(ToolManager, SwitchingAndCancelSemanticsAreExplicit)
{
    quader::document::Document document;
    quader::commands::CommandHistory history;
    quader::tools::ToolManager manager{quader::tools::ToolContext{document, history}};

    auto select_tool = std::make_unique<RecordingTool>(quader::tools::ToolId::Select);
    auto move_tool = std::make_unique<RecordingTool>(quader::tools::ToolId::Move);
    auto* select = select_tool.get();
    auto* move = move_tool.get();

    EXPECT_TRUE(manager.register_tool(std::move(select_tool)));
    EXPECT_TRUE(manager.register_tool(std::move(move_tool)));
    EXPECT_TRUE(manager.set_active_tool(quader::tools::ToolId::Select));

    EXPECT_TRUE(manager.dispatch_pointer_event(quader::tools::PointerEvent{
        quader::math::Vec2{5.0F, 6.0F},
        quader::tools::PointerButton::Left,
        true,
    }));
    EXPECT_FALSE(manager.preview().empty());

    EXPECT_TRUE(manager.cancel_active_tool());
    EXPECT_EQ(select->cancellations, 1);
    EXPECT_TRUE(manager.active_tool_id().has_value());
    EXPECT_EQ(*manager.active_tool_id(), quader::tools::ToolId::Select);
    EXPECT_TRUE(manager.preview().empty());

    EXPECT_TRUE(manager.set_active_tool(quader::tools::ToolId::Move));
    EXPECT_EQ(select->deactivations, 1);
    EXPECT_EQ(move->activations, 1);
    EXPECT_EQ(*manager.active_tool_id(), quader::tools::ToolId::Move);

    manager.clear_active_tool();
    EXPECT_EQ(move->deactivations, 1);
    EXPECT_FALSE(manager.active_tool_id().has_value());
    EXPECT_TRUE(manager.active_tool() == nullptr);

    manager.clear_active_tool();
    EXPECT_EQ(move->deactivations, 1);
    EXPECT_FALSE(manager.dispatch_key_event(quader::tools::KeyEvent{27, true, false}));
}

} // namespace
