/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
#include "ui/models/property_item_model.hpp"

#include "commands/command_result.hpp"
#include "commands/document_commands.hpp"
#include "document/document.hpp"
#include "document/scene_object.hpp"
#include "document/transform.hpp"
#include "math/vec3.hpp"
#include "ui/models/item_model_roles.hpp"
#include "ui/services/document_ui_controller.hpp"

#include <QList>
#include <QVariant>

#include <algorithm>
#include <cmath>
#include <memory>
#include <string>

namespace quader::ui {
namespace {

constexpr int kPropertyItemColumnCount = 2;

[[nodiscard]] bool is_value_column(int column) noexcept {
	return column == static_cast<int>(PropertyItemColumn::Value);
}

[[nodiscard]] bool is_label_column(int column) noexcept {
	return column == static_cast<int>(PropertyItemColumn::Label);
}

[[nodiscard]] bool
descriptor_matches_object(const PropertyDescriptor &descriptor,
		quader::document::ObjectId object) noexcept {
	return descriptor.path.object == object;
}

[[nodiscard]] bool same_descriptor_topology(
		const std::vector<PropertyDescriptor> &left,
		const std::vector<PropertyDescriptor> &right) noexcept {
	if (left.size() != right.size()) {
		return false;
	}

	for (std::size_t index = 0; index < left.size(); ++index) {
		if (left[index].path != right[index].path || left[index].kind != right[index].kind) {
			return false;
		}
	}
	return true;
}

[[nodiscard]] bool descriptor_display_data_equal(
		const PropertyDescriptor &left,
		const PropertyDescriptor &right) {
	return left.label == right.label &&
			left.value == right.value &&
			left.minimum == right.minimum &&
			left.maximum == right.maximum &&
			left.decimals == right.decimals &&
			left.editable == right.editable &&
			left.tooltip == right.tooltip;
}

[[nodiscard]] bool value_to_finite_float(const QVariant &value,
		float &out_value) {
	bool ok = false;
	const double kParsed = value.toDouble(&ok);
	if (!ok || !std::isfinite(kParsed)) {
		return false;
	}

	out_value = static_cast<float>(kParsed);
	return std::isfinite(out_value);
}

[[nodiscard]] bool
replace_transform_component(quader::document::Transform &transform,
		PropertyField field, float value) {
	float *target = nullptr;
	switch (field) {
		case PropertyField::TranslationX:
			target = &transform.translation.x;
			break;
		case PropertyField::TranslationY:
			target = &transform.translation.y;
			break;
		case PropertyField::TranslationZ:
			target = &transform.translation.z;
			break;
		case PropertyField::RotationX:
			target = &transform.rotation_euler.x;
			break;
		case PropertyField::RotationY:
			target = &transform.rotation_euler.y;
			break;
		case PropertyField::RotationZ:
			target = &transform.rotation_euler.z;
			break;
		case PropertyField::ScaleX:
			target = &transform.scale.x;
			break;
		case PropertyField::ScaleY:
			target = &transform.scale.y;
			break;
		case PropertyField::ScaleZ:
			target = &transform.scale.z;
			break;
		default:
			return false;
	}

	if (*target == value) {
		return false;
	}

	*target = value;
	return true;
}

} // namespace

PropertyItemModel::PropertyItemModel(DocumentUiController &documents,
		QObject *parent) : QAbstractTableModel(parent),
						   documents_(documents) {
	register_ui_model_metatypes();
	rebuild_from_document();

	connect(&documents_, &DocumentUiController::document_changed, this,
			&PropertyItemModel::refresh_from_change);
}

int PropertyItemModel::rowCount(const QModelIndex &parent_index) const {
	if (parent_index.isValid()) {
		return 0;
	}

	return static_cast<int>(descriptors_.size());
}

int PropertyItemModel::columnCount(const QModelIndex &parent_index) const {
	(void)parent_index;
	return kPropertyItemColumnCount;
}

QVariant PropertyItemModel::data(const QModelIndex &model_index,
		int role) const {
	const auto *descriptor = descriptor_for_row(model_index.row());
	if (descriptor == nullptr || !model_index.isValid()) {
		return {};
	}

	switch (role) {
		case Qt::DisplayRole:
			if (is_label_column(model_index.column())) {
				return descriptor->label;
			}
			if (is_value_column(model_index.column())) {
				return descriptor->value;
			}
			return {};
		case Qt::EditRole:
			if (is_value_column(model_index.column())) {
				return descriptor->value;
			}
			return {};
		case Qt::ToolTipRole:
			return descriptor->tooltip;
		case property_item_roles::kPropertyPathRole:
			return QVariant::fromValue(descriptor->path);
		case property_item_roles::kValueKindRole:
			return QVariant::fromValue(descriptor->kind);
		case property_item_roles::kEditableRole:
			return descriptor->editable;
		case property_item_roles::kDescriptorRole:
			return QVariant::fromValue(*descriptor);
		default:
			return {};
	}
}

bool PropertyItemModel::setData(const QModelIndex &model_index,
		const QVariant &value, int role) {
	if (role != Qt::EditRole || !model_index.isValid() ||
			!is_value_column(model_index.column())) {
		return false;
	}

	const auto *descriptor = descriptor_for_row(model_index.row());
	if (descriptor == nullptr || !descriptor->editable) {
		return false;
	}

	return apply_descriptor_edit(*descriptor, value);
}

Qt::ItemFlags PropertyItemModel::flags(const QModelIndex &model_index) const {
	if (!model_index.isValid()) {
		return Qt::NoItemFlags;
	}

	auto item_flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
	const auto *descriptor = descriptor_for_row(model_index.row());
	if (descriptor != nullptr && descriptor->editable &&
			is_value_column(model_index.column())) {
		item_flags |= Qt::ItemIsEditable;
	}

	return item_flags;
}

QVariant PropertyItemModel::headerData(int section, Qt::Orientation orientation,
		int role) const {
	if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
		return {};
	}

	if (section == static_cast<int>(PropertyItemColumn::Label)) {
		return QStringLiteral("Property");
	}

	if (section == static_cast<int>(PropertyItemColumn::Value)) {
		return QStringLiteral("Value");
	}

	return {};
}

void PropertyItemModel::rebuild_from_document() {
	beginResetModel();
	descriptors_ = build_property_descriptors(documents_.document());
	endResetModel();
}

void PropertyItemModel::refresh_object(quader::document::ObjectId object) {
	const bool kSelectedObjectChanged =
			documents_.document().selection().references_object(object);
	const bool kCurrentDescriptorsReferenceObject =
			std::any_of(descriptors_.begin(), descriptors_.end(),
					[object](const auto &descriptor) {
						return descriptor_matches_object(descriptor, object);
					});

	if (kSelectedObjectChanged || kCurrentDescriptorsReferenceObject) {
		refresh_current_descriptor_values();
	}
}

void PropertyItemModel::refresh_from_change(
		const quader::document::DocumentChange &change) {
	using quader::document::DocumentChangeKind;

	switch (change.kind) {
		case DocumentChangeKind::ObjectCreated:
		case DocumentChangeKind::ObjectDeleted:
		case DocumentChangeKind::SelectionChanged:
			rebuild_from_document();
			break;
		case DocumentChangeKind::ObjectRenamed:
		case DocumentChangeKind::TransformChanged:
		case DocumentChangeKind::MeshTopologyChanged:
		case DocumentChangeKind::MeshGeometryChanged:
			refresh_object(change.object);
			break;
		case DocumentChangeKind::DirtyStateChanged:
			break;
	}
}

const PropertyDescriptor *
PropertyItemModel::descriptor_for_row(int row) const noexcept {
	if (row < 0 || static_cast<std::size_t>(row) >= descriptors_.size()) {
		return nullptr;
	}

	return &descriptors_[static_cast<std::size_t>(row)];
}

bool PropertyItemModel::apply_descriptor_edit(
		const PropertyDescriptor &descriptor, const QVariant &value) {
	const auto *object =
			documents_.document().find_mesh_object(descriptor.path.object);
	if (object == nullptr) {
		return false;
	}

	if (descriptor.path.field == PropertyField::ObjectName) {
		const QString kNewName = value.toString();
		if (kNewName.isEmpty() || kNewName.toStdString() == object->name) {
			return false;
		}

		auto result = documents_.execute_command(
				std::make_unique<quader::commands::RenameObjectCommand>(
						descriptor.path.object, kNewName.toStdString()));
		return result.is_applied();
	}

	float numeric_value = 0.0F;
	if (!value_to_finite_float(value, numeric_value)) {
		return false;
	}

	auto transform = object->transform;
	if (!replace_transform_component(transform, descriptor.path.field,
				numeric_value)) {
		return false;
	}

	auto result = documents_.execute_command(
			std::make_unique<quader::commands::SetObjectTransformCommand>(
					descriptor.path.object, transform));
	return result.is_applied();
}

void PropertyItemModel::refresh_current_descriptor_values() {
	std::vector<PropertyDescriptor> next_descriptors =
			build_property_descriptors(documents_.document());
	if (!same_descriptor_topology(descriptors_, next_descriptors)) {
		beginResetModel();
		descriptors_ = std::move(next_descriptors);
		endResetModel();
		return;
	}

	const QList<int> kChangedRoles{
		Qt::DisplayRole,
		Qt::EditRole,
		Qt::ToolTipRole,
		property_item_roles::kPropertyPathRole,
		property_item_roles::kValueKindRole,
		property_item_roles::kEditableRole,
		property_item_roles::kDescriptorRole,
	};
	for (std::size_t index = 0; index < descriptors_.size(); ++index) {
		const bool kChanged =
				!descriptor_display_data_equal(descriptors_[index], next_descriptors[index]);
		descriptors_[index] = std::move(next_descriptors[index]);
		if (kChanged) {
			const int kRow = static_cast<int>(index);
			Q_EMIT dataChanged(
					this->index(kRow, 0),
					this->index(kRow, kPropertyItemColumnCount - 1),
					kChangedRoles);
		}
	}
}

} // namespace quader::ui
