#include "../document/document_test_helpers.hpp"

#include <gtest/gtest.h>

#include "commands/command_history.hpp"
#include "commands/document_commands.hpp"
#include "commands/selection_commands.hpp"
#include "document/document.hpp"
#include "document/selection.hpp"
#include "ui/actions/action_registry.hpp"
#include "ui/actions/action_state_updater.hpp"
#include "ui/actions/editor_state_snapshot.hpp"
#include "ui/delegates/property_value_delegate.hpp"
#include "ui/models/document_item_model.hpp"
#include "ui/models/document_selection_adapter.hpp"
#include "ui/models/item_model_roles.hpp"
#include "ui/models/property_descriptor.hpp"
#include "ui/models/property_item_model.hpp"
#include "ui/services/document_ui_controller.hpp"
#include "ui/services/notification_service.hpp"

#include <QAbstractItemModelTester>
#include <QApplication>
#include <QItemSelectionModel>
#include <QModelIndex>
#include <QString>
#include <QVariant>

#include <cmath>
#include <cstdio>
#include <limits>
#include <memory>
#include <string>
#include <utility>

namespace {

struct UiModelFixture {
	quader::document::Document document;
	quader::commands::CommandHistory command_history;
	quader::ui::ActionRegistry actions;
	quader::ui::NullEditorStateProvider editor_state;
	quader::ui::ActionStateUpdater action_state_updater;
	quader::ui::NotificationService notifications;
	quader::ui::DocumentUiController document_ui;

	UiModelFixture() : action_state_updater(actions, editor_state),
					   document_ui(document, command_history, action_state_updater,
							   notifications) {
		quader::ui::register_standard_actions(actions);
		action_state_updater.refresh();
	}

	quader::document::ObjectId
	add_triangle_object(std::string name = "Triangle") {
		auto mesh = quader::tests::document_fixtures::make_triangle_mesh();
		auto created =
				document.create_mesh_object(std::move(name), std::move(mesh.mesh));
		EXPECT_TRUE(created);
		discard_pending_changes();
		document.clear_dirty();
		discard_pending_changes();
		return created.value();
	}

	quader::tests::document_fixtures::DocumentMeshFixture
	add_triangle_object_with_ids(std::string name = "Triangle") {
		auto mesh = quader::tests::document_fixtures::make_triangle_mesh();
		auto created =
				document.create_mesh_object(std::move(name), std::move(mesh.mesh));
		EXPECT_TRUE(created);
		discard_pending_changes();
		document.clear_dirty();
		discard_pending_changes();
		return quader::tests::document_fixtures::DocumentMeshFixture{
			quader::document::Document{},
			created.value(),
			mesh.vertices,
			mesh.edge,
			mesh.face,
		};
	}

	void select_objects_direct(std::vector<quader::document::ObjectId> objects) {
		quader::document::Selection selection;
		EXPECT_TRUE(selection.set_objects(std::move(objects)));
		auto selected = document.set_selection(std::move(selection));
		EXPECT_TRUE(selected);
		discard_pending_changes();
		document.clear_dirty();
		discard_pending_changes();
	}

	void discard_pending_changes() { (void)document.take_pending_changes(); }
};

[[nodiscard]] int row_for_field(const quader::ui::PropertyItemModel &model,
		quader::ui::PropertyField field) {
	for (int row = 0; row < model.rowCount(); ++row) {
		const auto kPath =
				model
						.index(row, static_cast<int>(quader::ui::PropertyItemColumn::Value))
						.data(quader::ui::property_item_roles::kPropertyPathRole)
						.value<quader::ui::PropertyPath>();
		if (kPath.field == field) {
			return row;
		}
	}

	return -1;
}

[[nodiscard]] QModelIndex value_index(quader::ui::PropertyItemModel &model,
		quader::ui::PropertyField field) {
	const int kRow = row_for_field(model, field);
	if (kRow < 0) {
		return {};
	}

	return model.index(kRow,
			static_cast<int>(quader::ui::PropertyItemColumn::Value));
}

TEST(UiModel, DocumentItemModelEmptyDocumentIsConsistent) {
	UiModelFixture fixture;
	quader::ui::DocumentItemModel model(fixture.document_ui);
	QAbstractItemModelTester tester(
			&model, QAbstractItemModelTester::FailureReportingMode::Fatal);

	EXPECT_EQ(model.rowCount(), 0);
	EXPECT_EQ(model.columnCount(), 2);
}

TEST(UiModel, DocumentItemModelExposesObjectRolesAndFlags) {
	UiModelFixture fixture;
	const auto kObject = fixture.add_triangle_object();
	quader::ui::DocumentItemModel model(fixture.document_ui);
	QAbstractItemModelTester tester(
			&model, QAbstractItemModelTester::FailureReportingMode::Fatal);

	const QModelIndex kName =
			model.index(0, static_cast<int>(quader::ui::DocumentItemColumn::Name));
	const QModelIndex kInd =
			model.index(0, static_cast<int>(quader::ui::DocumentItemColumn::Kind));

	EXPECT_TRUE(kName.isValid());
	EXPECT_EQ(model.rowCount(), 1);
	EXPECT_EQ(kName.data(Qt::DisplayRole).toString(), QStringLiteral("Triangle"));
	EXPECT_EQ(kInd.data(Qt::DisplayRole).toString(), QStringLiteral("Mesh"));
	EXPECT_EQ(kName.data(quader::ui::document_item_roles::kObjectIdRole)
					  .value<quader::document::ObjectId>(),
			kObject);
	EXPECT_EQ(kName.data(quader::ui::document_item_roles::kKindRole)
					  .value<quader::ui::DocumentItemKind>(),
			quader::ui::DocumentItemKind::MeshObject);
	EXPECT_FALSE(kName.data(quader::ui::document_item_roles::kSelectedRole).toBool());
	EXPECT_TRUE((model.flags(kName) & Qt::ItemIsEditable) != 0);
	EXPECT_FALSE((model.flags(kInd) & Qt::ItemIsEditable) != 0);
	EXPECT_TRUE(model.index_for_object(kObject).isValid());
}

TEST(UiModel, DocumentItemModelNameEditDispatchesCommandAndUndoRefreshes) {
	UiModelFixture fixture;
	const auto kObject = fixture.add_triangle_object();
	quader::ui::DocumentItemModel model(fixture.document_ui);
	QAbstractItemModelTester tester(
			&model, QAbstractItemModelTester::FailureReportingMode::Fatal);

	const QModelIndex kName = model.index_for_object(kObject);
	EXPECT_TRUE(model.setData(kName, QStringLiteral("Renamed"), Qt::EditRole));
	EXPECT_EQ(fixture.document.find_mesh_object(kObject)->name,
			std::string("Renamed"));
	EXPECT_TRUE(fixture.command_history.can_undo());
	EXPECT_EQ(kName.data(Qt::DisplayRole).toString(), QStringLiteral("Renamed"));

	EXPECT_TRUE(fixture.document_ui.undo(quader::ui::CommandFeedback::Silent)
					.is_applied());
	EXPECT_EQ(fixture.document.find_mesh_object(kObject)->name,
			std::string("Triangle"));
	EXPECT_EQ(kName.data(Qt::DisplayRole).toString(), QStringLiteral("Triangle"));
}

TEST(UiModel, DocumentItemModelRejectsNoopRenameAndResetsAfterDelete) {
	UiModelFixture fixture;
	const auto kObject = fixture.add_triangle_object();
	quader::ui::DocumentItemModel model(fixture.document_ui);
	QAbstractItemModelTester tester(
			&model, QAbstractItemModelTester::FailureReportingMode::Fatal);

	EXPECT_FALSE(model.setData(model.index_for_object(kObject),
			QStringLiteral("Triangle"), Qt::EditRole));
	EXPECT_FALSE(fixture.command_history.can_undo());

	auto result = fixture.document_ui.execute_command(
			std::make_unique<quader::commands::DeleteObjectCommand>(kObject),
			quader::ui::CommandFeedback::Silent);
	EXPECT_TRUE(result.is_applied());
	EXPECT_EQ(model.rowCount(), 0);
	EXPECT_FALSE(model.index_for_object(kObject).isValid());
}

TEST(UiModel, PropertyItemModelEmptySelectionHasNoRows) {
	UiModelFixture fixture;
	fixture.add_triangle_object();
	quader::ui::PropertyItemModel model(fixture.document_ui);
	QAbstractItemModelTester tester(
			&model, QAbstractItemModelTester::FailureReportingMode::Fatal);

	EXPECT_EQ(model.rowCount(), 0);
	EXPECT_EQ(model.columnCount(), 2);
}

TEST(UiModel, PropertyItemModelSingleSelectionDescriptors) {
	UiModelFixture fixture;
	const auto kObject = fixture.add_triangle_object();
	fixture.select_objects_direct({ kObject });

	quader::ui::PropertyItemModel model(fixture.document_ui);
	QAbstractItemModelTester tester(
			&model, QAbstractItemModelTester::FailureReportingMode::Fatal);

	EXPECT_TRUE(row_for_field(model, quader::ui::PropertyField::ObjectName) >= 0);
	EXPECT_TRUE(row_for_field(model, quader::ui::PropertyField::ObjectKind) >= 0);
	EXPECT_TRUE(row_for_field(model, quader::ui::PropertyField::VertexCount) >=
			0);
	EXPECT_TRUE(row_for_field(model, quader::ui::PropertyField::EdgeCount) >= 0);
	EXPECT_TRUE(row_for_field(model, quader::ui::PropertyField::FaceCount) >= 0);
	EXPECT_TRUE(row_for_field(model, quader::ui::PropertyField::TranslationX) >=
			0);
	EXPECT_TRUE(row_for_field(model, quader::ui::PropertyField::RotationZ) >= 0);
	EXPECT_TRUE(row_for_field(model, quader::ui::PropertyField::ScaleY) >= 0);

	const QModelIndex kName =
			value_index(model, quader::ui::PropertyField::ObjectName);
	const QModelIndex kInd =
			value_index(model, quader::ui::PropertyField::ObjectKind);
	EXPECT_TRUE((model.flags(kName) & Qt::ItemIsEditable) != 0);
	EXPECT_FALSE((model.flags(kInd) & Qt::ItemIsEditable) != 0);
	EXPECT_EQ(kName.data(Qt::EditRole).toString(), QStringLiteral("Triangle"));
	EXPECT_EQ(kInd.data(Qt::DisplayRole).toString(), QStringLiteral("Mesh"));
}

TEST(UiModel, PropertyItemModelNameEditDispatchesCommand) {
	UiModelFixture fixture;
	const auto kObject = fixture.add_triangle_object();
	fixture.select_objects_direct({ kObject });

	quader::ui::PropertyItemModel model(fixture.document_ui);
	QAbstractItemModelTester tester(
			&model, QAbstractItemModelTester::FailureReportingMode::Fatal);

	const QModelIndex kName =
			value_index(model, quader::ui::PropertyField::ObjectName);
	EXPECT_TRUE(
			model.setData(kName, QStringLiteral("Property Renamed"), Qt::EditRole));
	EXPECT_EQ(fixture.document.find_mesh_object(kObject)->name,
			std::string("Property Renamed"));
	EXPECT_TRUE(fixture.command_history.can_undo());
	EXPECT_EQ(fixture.command_history.undo_name(),
			std::string_view("Rename Object"));
}

TEST(UiModel, PropertyItemModelTransformEditPreservesOtherFields) {
	UiModelFixture fixture;
	const auto kObject = fixture.add_triangle_object();
	fixture.select_objects_direct({ kObject });

	quader::ui::PropertyItemModel model(fixture.document_ui);
	QAbstractItemModelTester tester(
			&model, QAbstractItemModelTester::FailureReportingMode::Fatal);

	const auto kBefore = fixture.document.find_mesh_object(kObject)->transform;
	const QModelIndex kTranslationX =
			value_index(model, quader::ui::PropertyField::TranslationX);
	EXPECT_TRUE(model.setData(kTranslationX, 12.5, Qt::EditRole));

	const auto kAfter = fixture.document.find_mesh_object(kObject)->transform;
	EXPECT_NEAR((kAfter.translation.x), (12.5F), 0.0001F);
	EXPECT_NEAR((kAfter.translation.y), (kBefore.translation.y), 0.0001F);
	EXPECT_NEAR((kAfter.translation.z), (kBefore.translation.z), 0.0001F);
	EXPECT_NEAR((kAfter.scale.x), (kBefore.scale.x), 0.0001F);
	EXPECT_TRUE(fixture.command_history.can_undo());
	EXPECT_EQ(fixture.command_history.undo_name(),
			std::string_view("Set Object Transform"));
}

TEST(UiModel, PropertyItemModelRejectsReadonlyAndNonfiniteValues) {
	UiModelFixture fixture;
	const auto kObject = fixture.add_triangle_object();
	fixture.select_objects_direct({ kObject });

	quader::ui::PropertyItemModel model(fixture.document_ui);
	QAbstractItemModelTester tester(
			&model, QAbstractItemModelTester::FailureReportingMode::Fatal);

	const QModelIndex kInd =
			value_index(model, quader::ui::PropertyField::ObjectKind);
	EXPECT_FALSE(model.setData(kInd, QStringLiteral("Other"), Qt::EditRole));

	const auto kBefore = fixture.document.find_mesh_object(kObject)->transform;
	const QModelIndex kTranslationX =
			value_index(model, quader::ui::PropertyField::TranslationX);
	EXPECT_FALSE(model.setData(
			kTranslationX, std::numeric_limits<double>::infinity(), Qt::EditRole));
	EXPECT_EQ(fixture.document.find_mesh_object(kObject)->transform, kBefore);
	EXPECT_FALSE(fixture.command_history.can_undo());
}

TEST(UiModel, PropertyItemModelMultiSelectionIsReadonlySummary) {
	UiModelFixture fixture;
	const auto kFirst = fixture.add_triangle_object("First");
	const auto kSecond = fixture.add_triangle_object("Second");
	fixture.select_objects_direct({ kFirst, kSecond });

	quader::ui::PropertyItemModel model(fixture.document_ui);
	QAbstractItemModelTester tester(
			&model, QAbstractItemModelTester::FailureReportingMode::Fatal);

	EXPECT_EQ(model.rowCount(), 1);
	const QModelIndex kSummary =
			model.index(0, static_cast<int>(quader::ui::PropertyItemColumn::Value));
	EXPECT_EQ(kSummary.data(quader::ui::property_item_roles::kPropertyPathRole)
					  .value<quader::ui::PropertyPath>()
					  .field,
			quader::ui::PropertyField::SelectionSummary);
	EXPECT_TRUE(
			kSummary.data(Qt::DisplayRole).toString().contains(QStringLiteral("2")));
	EXPECT_FALSE((model.flags(kSummary) & Qt::ItemIsEditable) != 0);
}

TEST(UiModel, SelectionAdapterSyncsViewSelectionThroughCommandsAndUndo) {
	UiModelFixture fixture;
	const auto kObject = fixture.add_triangle_object();
	quader::ui::DocumentItemModel model(fixture.document_ui);
	QAbstractItemModelTester tester(
			&model, QAbstractItemModelTester::FailureReportingMode::Fatal);
	QItemSelectionModel selection_model(&model);
	quader::ui::DocumentSelectionAdapter adapter(model, selection_model,
			fixture.document_ui);

	const QModelIndex kRow = model.index_for_object(kObject);
	selection_model.select(kRow, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);

	EXPECT_EQ(fixture.document.selection().selected_objects().size(), 1U);
	EXPECT_EQ(fixture.document.selection().selected_objects().front(), kObject);
	EXPECT_TRUE(fixture.command_history.can_undo());

	EXPECT_TRUE(fixture.document_ui.undo(quader::ui::CommandFeedback::Silent)
					.is_applied());
	EXPECT_TRUE(fixture.document.selection().empty());
	EXPECT_TRUE(selection_model.selectedIndexes().empty());
}

TEST(UiModel, SelectionAdapterSyncsDirectDocumentSelectionToView) {
	UiModelFixture fixture;
	const auto kObject = fixture.add_triangle_object();
	quader::ui::DocumentItemModel model(fixture.document_ui);
	QItemSelectionModel selection_model(&model);
	quader::ui::DocumentSelectionAdapter adapter(model, selection_model,
			fixture.document_ui);

	quader::document::Selection selection;
	EXPECT_TRUE(selection.set_objects({ kObject }));
	auto result = fixture.document_ui.execute_command(
			std::make_unique<quader::commands::SetSelectionCommand>(
					std::move(selection)),
			quader::ui::CommandFeedback::Silent);
	EXPECT_TRUE(result.is_applied());
	EXPECT_TRUE(selection_model.isSelected(model.index_for_object(kObject)));
}

TEST(UiModel, SelectionAdapterComponentSelectionClearsObjectViewSelection) {
	UiModelFixture fixture;
	const auto kAdded = fixture.add_triangle_object_with_ids();
	quader::ui::DocumentItemModel model(fixture.document_ui);
	QItemSelectionModel selection_model(&model);
	quader::ui::DocumentSelectionAdapter adapter(model, selection_model,
			fixture.document_ui);

	selection_model.select(model.index_for_object(kAdded.object),
			QItemSelectionModel::ClearAndSelect |
					QItemSelectionModel::Rows);
	EXPECT_FALSE(selection_model.selectedIndexes().empty());

	quader::document::Selection component_selection;
	EXPECT_TRUE(component_selection.set_components({
			quader::document::ComponentRef{ kAdded.object, kAdded.vertices[0] },
	}));
	auto result = fixture.document_ui.execute_command(
			std::make_unique<quader::commands::SetSelectionCommand>(
					std::move(component_selection)),
			quader::ui::CommandFeedback::Silent);
	EXPECT_TRUE(result.is_applied());
	EXPECT_TRUE(fixture.document.selection().selected_objects().empty());
	EXPECT_EQ(fixture.document.selection().selected_components().size(), 1U);
	EXPECT_TRUE(selection_model.selectedIndexes().empty());
}

TEST(UiModel, PropertyValueDelegateCreatesEditorsWithoutDocumentAccess) {
	UiModelFixture fixture;
	const auto kObject = fixture.add_triangle_object();
	fixture.select_objects_direct({ kObject });

	quader::ui::PropertyItemModel model(fixture.document_ui);
	quader::ui::PropertyValueDelegate delegate;
	QWidget parent;

	const QModelIndex kName =
			value_index(model, quader::ui::PropertyField::ObjectName);
	const QModelIndex kTranslationX =
			value_index(model, quader::ui::PropertyField::TranslationX);
	const QModelIndex kInd =
			value_index(model, quader::ui::PropertyField::ObjectKind);

	auto *name_editor = delegate.createEditor(&parent, {}, kName);
	auto *float_editor = delegate.createEditor(&parent, {}, kTranslationX);
	auto *readonly_editor = delegate.createEditor(&parent, {}, kInd);

	EXPECT_TRUE(name_editor != nullptr);
	EXPECT_TRUE(float_editor != nullptr);
	EXPECT_TRUE(readonly_editor == nullptr);
}

} // namespace
