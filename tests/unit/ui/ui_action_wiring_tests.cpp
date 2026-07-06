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
#include "document/selection.hpp"
#include "io/import_export_registry.hpp"
#include "mesh/core/mesh_traversal.hpp"
#include "tools/box_tool.hpp"
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
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace {

using quader::tests::document_fixtures::DocumentSemanticState;

void discard_pending_changes(quader::document::Document &document) {
	const auto kChanges = document.take_pending_changes();
	(void)kChanges;
}

DocumentSemanticState capture_ui_document_state(const quader::document::Document &document) {
	auto state = quader::tests::document_fixtures::capture_document_state(document);
	state.dirty = false;
	state.dirty_flags = quader::document::kDocumentDirtyNone;
	return state;
}

quader::document::ObjectId add_triangle_object(quader::document::Document &document) {
	auto mesh = quader::tests::document_fixtures::make_triangle_mesh();
	auto created = document.create_mesh_object("Triangle", std::move(mesh.mesh));
	EXPECT_TRUE(created);
	return created.value();
}

quader::tools::PointerEvent box_grid_event(quader::math::Vec3 point,
		quader::math::Vec2 screen,
		quader::tools::PointerPhase phase,
		quader::tools::PointerButton button = quader::tools::PointerButton::None,
		bool pressed = false) {
	return quader::tools::PointerEvent{
		.position = screen,
		.button = button,
		.pressed = pressed,
		.phase = phase,
		.snap_to_grid = true,
		.grid_size = 1.0F,
		.ray = quader::tools::ViewportRay{
				.origin = point + quader::math::Vec3{ 0.0F, 10.0F, 0.0F },
				.direction = { 0.0F, -1.0F, 0.0F },
		},
	};
}

std::vector<quader::mesh::VertexId> face_vertices_for(
		const quader::document::Document &document,
		quader::document::ObjectId object) {
	const auto *mesh_object = document.find_mesh_object(object);
	EXPECT_TRUE(mesh_object != nullptr);
	if (mesh_object == nullptr) {
		return {};
	}

	const auto kFaces = mesh_object->mesh.face_ids();
	EXPECT_FALSE(kFaces.empty());
	if (kFaces.empty()) {
		return {};
	}

	auto vertices = quader::mesh::face_vertices(mesh_object->mesh, kFaces.front());
	EXPECT_TRUE(vertices);
	return vertices ? vertices.value() : std::vector<quader::mesh::VertexId>{};
}

bool same_cyclic_vertex_order(const std::vector<quader::mesh::VertexId> &actual,
		const std::vector<quader::mesh::VertexId> &expected) {
	if (actual.size() != expected.size() || actual.empty()) {
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

bool reversed_cyclic_vertex_order(const std::vector<quader::mesh::VertexId> &actual,
		const std::vector<quader::mesh::VertexId> &original) {
	if (original.size() != 3U) {
		return false;
	}

	return same_cyclic_vertex_order(actual, {
											 original[0],
											 original[2],
											 original[1],
									 });
}

class CountingTool final : public quader::tools::ITool {
public:
	explicit CountingTool(quader::tools::ToolId tool_id) noexcept
			: tool_id_(tool_id) {
	}

	[[nodiscard]] quader::tools::ToolId id() const noexcept override {
		return tool_id_;
	}

	void activate(quader::tools::ToolContext &context) override {
		(void)context;
		++activations;
	}

	void deactivate(quader::tools::ToolContext &context) override {
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

	UiActionFixture() : tool_manager(quader::tools::ToolContext{ document, command_history }), editor_state(document, command_history, tool_manager, io_registry), action_state_updater(actions, editor_state), document_ui(document, command_history, action_state_updater, notifications), editor_actions(actions, document_ui, tool_manager, action_state_updater, notifications) {
		quader::ui::register_standard_actions(actions);
		tool_manager.context().set_after_command_applied([this]() {
			document_ui.refresh_from_document();
		});
		tool_manager.set_after_active_tool_changed([this]() {
			action_state_updater.refresh();
		});
		action_state_updater.refresh();
	}
};

TEST(UiActionWiring, StateProviderDrivesUndoRedoTextAndKeepsUnavailableActionsDisabled) {
	UiActionFixture fixture;
	const auto kObject = add_triangle_object(fixture.document);
	fixture.document.clear_dirty();
	discard_pending_changes(fixture.document);

	auto result = fixture.command_history.execute(
			std::make_unique<quader::commands::RenameObjectCommand>(kObject, "Renamed"),
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
	EXPECT_FALSE(fixture.actions.action(quader::ui::ActionId::CreateLight).isEnabled());
	EXPECT_FALSE(fixture.actions.action(quader::ui::ActionId::CreateCamera).isEnabled());
	EXPECT_FALSE(fixture.actions.action(quader::ui::ActionId::DuplicateSelection).isEnabled());
}

TEST(UiActionWiring, UndoAndRedoActionsExecuteThroughCommandHistory) {
	UiActionFixture fixture;
	const auto kObject = add_triangle_object(fixture.document);
	fixture.document.clear_dirty();
	discard_pending_changes(fixture.document);

	const auto kBefore = capture_ui_document_state(fixture.document);
	auto result = fixture.command_history.execute(
			std::make_unique<quader::commands::RenameObjectCommand>(kObject, "Renamed"),
			fixture.document);
	EXPECT_EQ((result).status, quader::commands::CommandStatus::Applied);
	const auto kAfter = capture_ui_document_state(fixture.document);
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
	EXPECT_EQ(capture_ui_document_state(fixture.document), kBefore);
	EXPECT_EQ(command_history_changes, 1);
	EXPECT_EQ(drained_events, 1);
	EXPECT_FALSE(fixture.actions.action(quader::ui::ActionId::Undo).isEnabled());
	EXPECT_TRUE(fixture.actions.action(quader::ui::ActionId::Redo).isEnabled());
	EXPECT_EQ(fixture.actions.action(quader::ui::ActionId::Redo).text(),
			QStringLiteral("Redo Rename Object"));

	fixture.actions.action(quader::ui::ActionId::Redo).trigger();
	EXPECT_EQ(capture_ui_document_state(fixture.document), kAfter);
	EXPECT_EQ(command_history_changes, 2);
	EXPECT_EQ(drained_events, 2);
	EXPECT_TRUE(fixture.actions.action(quader::ui::ActionId::Undo).isEnabled());
	EXPECT_FALSE(fixture.actions.action(quader::ui::ActionId::Redo).isEnabled());
}

TEST(UiActionWiring, DeleteSelectionActionDeletesOneSelectedObjectThroughCommandHistory) {
	UiActionFixture fixture;
	const auto kObject = add_triangle_object(fixture.document);

	quader::document::Selection selection;
	EXPECT_TRUE(selection.set_objects({ kObject }));
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
	EXPECT_EQ(fixture.document.selection().selected_objects().front(), kObject);
	EXPECT_EQ(object_list_changes, 2);
	EXPECT_EQ(selection_changes, 2);
	EXPECT_EQ(command_history_changes, 2);
	EXPECT_TRUE(fixture.actions.action(quader::ui::ActionId::DeleteSelection).isEnabled());
}

TEST(UiActionWiring, SelectionSetActionsSelectClearAndInvertObjectsThroughCommands) {
	UiActionFixture fixture;
	const auto kFirst = add_triangle_object(fixture.document);
	const auto kSecond = add_triangle_object(fixture.document);
	fixture.document.clear_dirty();
	discard_pending_changes(fixture.document);

	fixture.action_state_updater.refresh();
	EXPECT_TRUE(fixture.actions.action(quader::ui::ActionId::SelectAll).isEnabled());
	EXPECT_TRUE(fixture.actions.action(quader::ui::ActionId::InvertSelection).isEnabled());
	EXPECT_FALSE(fixture.actions.action(quader::ui::ActionId::ClearSelection).isEnabled());

	fixture.actions.action(quader::ui::ActionId::SelectAll).trigger();
	ASSERT_EQ(fixture.document.selection().selected_objects().size(), 2U);
	EXPECT_EQ(fixture.document.selection().selected_objects()[0], kFirst);
	EXPECT_EQ(fixture.document.selection().selected_objects()[1], kSecond);
	EXPECT_TRUE(fixture.command_history.can_undo());
	EXPECT_EQ(fixture.command_history.undo_name(), std::string_view("Select All"));
	EXPECT_TRUE(fixture.actions.action(quader::ui::ActionId::ClearSelection).isEnabled());

	fixture.actions.action(quader::ui::ActionId::InvertSelection).trigger();
	EXPECT_TRUE(fixture.document.selection().empty());
	EXPECT_TRUE(fixture.actions.action(quader::ui::ActionId::ClearSelection).isEnabled() == false);

	fixture.actions.action(quader::ui::ActionId::Undo).trigger();
	EXPECT_EQ(fixture.document.selection().selected_objects().size(), 2U);

	fixture.actions.action(quader::ui::ActionId::ClearSelection).trigger();
	EXPECT_TRUE(fixture.document.selection().empty());
	EXPECT_EQ(fixture.command_history.undo_name(), std::string_view("Clear Selection"));
}

TEST(UiActionWiring, FlipMeshNormalsActionExecutesThroughCommandHistory) {
	UiActionFixture fixture;
	const auto kObject = add_triangle_object(fixture.document);

	quader::document::Selection selection;
	EXPECT_TRUE(selection.set_objects({ kObject }));
	EXPECT_TRUE(fixture.document.set_selection(selection));
	fixture.document.clear_dirty();
	discard_pending_changes(fixture.document);

	const auto kBefore = face_vertices_for(fixture.document, kObject);
	ASSERT_EQ(kBefore.size(), 3U);

	fixture.action_state_updater.refresh();
	EXPECT_TRUE(fixture.actions.action(quader::ui::ActionId::FlipMeshNormals).isEnabled());
	fixture.actions.action(quader::ui::ActionId::FlipMeshNormals).trigger();

	const auto kAfter = face_vertices_for(fixture.document, kObject);
	EXPECT_TRUE(reversed_cyclic_vertex_order(kAfter, kBefore));
	EXPECT_TRUE(fixture.command_history.can_undo());
	EXPECT_EQ(fixture.command_history.undo_name(), std::string_view("Flip Mesh Normals"));

	fixture.actions.action(quader::ui::ActionId::Undo).trigger();
	EXPECT_TRUE(same_cyclic_vertex_order(face_vertices_for(fixture.document, kObject), kBefore));
	EXPECT_TRUE(fixture.actions.action(quader::ui::ActionId::FlipMeshNormals).isEnabled());
}

TEST(UiActionWiring, ToolActionsRouteToToolManagerAndRefreshCheckedState) {
	UiActionFixture fixture;
	auto select_tool = std::make_unique<CountingTool>(quader::tools::ToolId::Select);
	auto move_tool = std::make_unique<CountingTool>(quader::tools::ToolId::Move);
	auto rotate_tool = std::make_unique<CountingTool>(quader::tools::ToolId::Rotate);
	auto scale_tool = std::make_unique<CountingTool>(quader::tools::ToolId::Scale);
	auto box_tool = std::make_unique<CountingTool>(quader::tools::ToolId::Box);
	auto *select = select_tool.get();
	auto *move = move_tool.get();
	auto *scale = scale_tool.get();
	auto *box = box_tool.get();

	EXPECT_TRUE(fixture.tool_manager.register_tool(std::move(select_tool)));
	EXPECT_TRUE(fixture.tool_manager.register_tool(std::move(move_tool)));
	EXPECT_TRUE(fixture.tool_manager.register_tool(std::move(rotate_tool)));
	EXPECT_TRUE(fixture.tool_manager.register_tool(std::move(scale_tool)));
	EXPECT_TRUE(fixture.tool_manager.register_tool(std::move(box_tool)));
	EXPECT_TRUE(fixture.tool_manager.set_active_tool(quader::tools::ToolId::Select));

	fixture.action_state_updater.refresh();
	EXPECT_TRUE(fixture.actions.action(quader::ui::ActionId::SelectTool).isEnabled());
	EXPECT_TRUE(fixture.actions.action(quader::ui::ActionId::SelectTool).isChecked());
	EXPECT_FALSE(fixture.actions.action(quader::ui::ActionId::MoveTool).isChecked());
	EXPECT_FALSE(fixture.actions.action(quader::ui::ActionId::BoxTool).isChecked());

	fixture.actions.action(quader::ui::ActionId::MoveTool).trigger();
	const auto kActiveToolId = fixture.tool_manager.active_tool_id();
	if (!kActiveToolId.has_value()) {
		ADD_FAILURE() << "move action did not set an active tool";
		return;
	}
	EXPECT_EQ(*kActiveToolId, quader::tools::ToolId::Move);
	EXPECT_EQ(select->deactivations, 1);
	EXPECT_EQ(move->activations, 1);
	EXPECT_FALSE(fixture.actions.action(quader::ui::ActionId::SelectTool).isChecked());
	EXPECT_TRUE(fixture.actions.action(quader::ui::ActionId::MoveTool).isChecked());

	fixture.actions.action(quader::ui::ActionId::ScaleTool).trigger();
	const auto kScaleToolId = fixture.tool_manager.active_tool_id();
	ASSERT_TRUE(kScaleToolId.has_value());
	EXPECT_EQ(*kScaleToolId, quader::tools::ToolId::Scale);
	EXPECT_EQ(scale->activations, 1);
	EXPECT_FALSE(fixture.actions.action(quader::ui::ActionId::MoveTool).isChecked());
	EXPECT_TRUE(fixture.actions.action(quader::ui::ActionId::ScaleTool).isChecked());

	fixture.actions.action(quader::ui::ActionId::BoxTool).trigger();
	const auto kBoxToolId = fixture.tool_manager.active_tool_id();
	ASSERT_TRUE(kBoxToolId.has_value());
	EXPECT_EQ(*kBoxToolId, quader::tools::ToolId::Box);
	EXPECT_EQ(box->activations, 1);
	EXPECT_FALSE(fixture.actions.action(quader::ui::ActionId::MoveTool).isChecked());
	EXPECT_FALSE(fixture.actions.action(quader::ui::ActionId::ScaleTool).isChecked());
	EXPECT_TRUE(fixture.actions.action(quader::ui::ActionId::BoxTool).isChecked());
	EXPECT_TRUE(fixture.actions.action(quader::ui::ActionId::SelectTool).shortcuts().contains(QKeySequence(Qt::Key_Q)));
	EXPECT_TRUE(fixture.actions.action(quader::ui::ActionId::MoveTool).shortcuts().contains(QKeySequence(Qt::Key_W)));
	EXPECT_TRUE(fixture.actions.action(quader::ui::ActionId::RotateTool).shortcuts().contains(QKeySequence(Qt::Key_R)));
	EXPECT_TRUE(fixture.actions.action(quader::ui::ActionId::ScaleTool).shortcuts().contains(QKeySequence(Qt::Key_S)));
	EXPECT_TRUE(fixture.actions.action(quader::ui::ActionId::BoxTool).shortcuts().contains(QKeySequence(Qt::Key_B)));
	EXPECT_TRUE(fixture.actions.action(quader::ui::ActionId::FlipMeshNormals).shortcuts().contains(QKeySequence(Qt::Key_F)));
	EXPECT_EQ(fixture.actions.action(quader::ui::ActionId::BoxTool).shortcutContext(), Qt::WidgetWithChildrenShortcut);
	EXPECT_EQ(fixture.actions.action(quader::ui::ActionId::FlipMeshNormals).shortcutContext(), Qt::WidgetWithChildrenShortcut);
}

TEST(UiActionWiring, BoxToolCompletionReturnsToSelectAndRefreshesCheckedState) {
	UiActionFixture fixture;
	EXPECT_TRUE(fixture.tool_manager.register_tool(
			std::make_unique<CountingTool>(quader::tools::ToolId::Select)));
	EXPECT_TRUE(fixture.tool_manager.register_tool(std::make_unique<quader::tools::BoxTool>()));
	EXPECT_TRUE(fixture.tool_manager.set_active_tool(quader::tools::ToolId::Select));

	fixture.actions.action(quader::ui::ActionId::BoxTool).trigger();
	ASSERT_TRUE(fixture.tool_manager.active_tool_id().has_value());
	EXPECT_EQ(*fixture.tool_manager.active_tool_id(), quader::tools::ToolId::Box);
	EXPECT_TRUE(fixture.actions.action(quader::ui::ActionId::BoxTool).isChecked());

	EXPECT_TRUE(fixture.tool_manager.dispatch_pointer_event(box_grid_event({ 0.2F, 0.0F, 0.3F },
			{ 10.0F, 100.0F },
			quader::tools::PointerPhase::Press,
			quader::tools::PointerButton::Left,
			true)));
	EXPECT_TRUE(fixture.tool_manager.dispatch_pointer_event(box_grid_event({ 2.2F, 0.0F, 3.2F },
			{ 40.0F, 100.0F },
			quader::tools::PointerPhase::Release,
			quader::tools::PointerButton::Left,
			false)));

	ASSERT_TRUE(fixture.tool_manager.active_tool_id().has_value());
	EXPECT_EQ(*fixture.tool_manager.active_tool_id(), quader::tools::ToolId::Select);
	EXPECT_EQ(fixture.tool_manager.selection_mode(), quader::tools::SelectionMode::Object);
	EXPECT_TRUE(fixture.actions.action(quader::ui::ActionId::SelectTool).isChecked());
	EXPECT_FALSE(fixture.actions.action(quader::ui::ActionId::BoxTool).isChecked());
	EXPECT_EQ(fixture.document.object_count(), 1U);
	ASSERT_EQ(fixture.document.selection().selected_objects().size(), 1U);
}

TEST(UiActionWiring, SelectionModeActionsRouteToToolManagerAndComponentSelectionVerbs) {
	UiActionFixture fixture;
	auto select_tool = std::make_unique<CountingTool>(quader::tools::ToolId::Select);
	EXPECT_TRUE(fixture.tool_manager.register_tool(std::move(select_tool)));
	EXPECT_TRUE(fixture.tool_manager.set_active_tool(quader::tools::ToolId::Select));

	const auto kObject = add_triangle_object(fixture.document);
	quader::document::Selection object_selection;
	EXPECT_TRUE(object_selection.set_objects({ kObject }));
	EXPECT_TRUE(fixture.document.set_selection(object_selection));
	discard_pending_changes(fixture.document);

	fixture.action_state_updater.refresh();
	fixture.actions.action(quader::ui::ActionId::SelectFaceMode).trigger();
	EXPECT_EQ(fixture.tool_manager.selection_mode(), quader::tools::SelectionMode::Face);
	EXPECT_EQ(fixture.document.selection().mode(), quader::document::SelectionMode::Face);
	EXPECT_EQ(fixture.command_history.undo_name(), std::string_view("Set Selection Mode"));
	EXPECT_TRUE(fixture.actions.action(quader::ui::ActionId::SelectFaceMode).isChecked());
	EXPECT_FALSE(fixture.actions.action(quader::ui::ActionId::SelectObjectMode).isChecked());
	EXPECT_TRUE(fixture.actions.action(quader::ui::ActionId::SelectVertexMode).shortcuts().contains(QKeySequence(Qt::Key_1)));
	EXPECT_TRUE(fixture.actions.action(quader::ui::ActionId::SelectVertexMode).shortcuts().contains(QKeySequence(Qt::KeypadModifier | Qt::Key_1)));
	EXPECT_TRUE(fixture.actions.action(quader::ui::ActionId::SelectEdgeMode).shortcuts().contains(QKeySequence(Qt::Key_2)));
	EXPECT_TRUE(fixture.actions.action(quader::ui::ActionId::SelectEdgeMode).shortcuts().contains(QKeySequence(Qt::KeypadModifier | Qt::Key_2)));
	EXPECT_TRUE(fixture.actions.action(quader::ui::ActionId::SelectFaceMode).shortcuts().contains(QKeySequence(Qt::Key_3)));
	EXPECT_TRUE(fixture.actions.action(quader::ui::ActionId::SelectFaceMode).shortcuts().contains(QKeySequence(Qt::KeypadModifier | Qt::Key_3)));
	EXPECT_TRUE(fixture.actions.action(quader::ui::ActionId::SelectObjectMode).shortcuts().contains(QKeySequence(Qt::Key_4)));
	EXPECT_TRUE(fixture.actions.action(quader::ui::ActionId::SelectObjectMode).shortcuts().contains(QKeySequence(Qt::KeypadModifier | Qt::Key_4)));
	EXPECT_EQ(fixture.actions.action(quader::ui::ActionId::SelectFaceMode).shortcutContext(), Qt::WidgetWithChildrenShortcut);

	fixture.actions.action(quader::ui::ActionId::SelectAll).trigger();
	const auto kComponents = fixture.document.selection().selected_components();
	ASSERT_EQ(kComponents.size(), 1U);
	EXPECT_EQ(kComponents.front().object, kObject);
	EXPECT_EQ(quader::document::component_kind(kComponents.front()), quader::document::ComponentKind::Face);
	EXPECT_EQ(fixture.document.selection().mode(), quader::document::SelectionMode::Face);
	ASSERT_EQ(fixture.document.selection().selected_objects().size(), 1U);
	EXPECT_EQ(fixture.document.selection().selected_objects().front(), kObject);

	fixture.actions.action(quader::ui::ActionId::InvertSelection).trigger();
	EXPECT_EQ(fixture.tool_manager.selection_mode(), quader::tools::SelectionMode::Face);
	EXPECT_EQ(fixture.document.selection().mode(), quader::document::SelectionMode::Face);
	EXPECT_TRUE(fixture.document.selection().selected_components().empty());
	ASSERT_EQ(fixture.document.selection().selected_objects().size(), 1U);
	EXPECT_EQ(fixture.document.selection().selected_objects().front(), kObject);
	EXPECT_EQ(fixture.command_history.undo_name(), std::string_view("Invert Selection"));
}

} // namespace
