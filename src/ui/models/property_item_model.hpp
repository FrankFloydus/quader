/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
#pragma once

#include "document/object_id.hpp"
#include "ui/models/property_descriptor.hpp"

#include <QAbstractTableModel>
#include <QModelIndex>
#include <QObject>

#include <vector>

namespace quader::ui {

class DocumentUiController;

/// Columns exposed by the property item model.
enum class PropertyItemColumn : int {
	Label = 0, ///< Property label.
	Value = 1, ///< Property value/editor.
};

/// Qt table model exposing editable document property descriptors.
class PropertyItemModel final : public QAbstractTableModel {
	Q_OBJECT

public:
	/// Construct a property model backed by document UI services.
	explicit PropertyItemModel(DocumentUiController &documents,
			QObject *parent = nullptr);

	/// Return property row count.
	[[nodiscard]] int rowCount(const QModelIndex &parent = {}) const override;
	/// Return property column count.
	[[nodiscard]] int columnCount(const QModelIndex &parent = {}) const override;
	/// Return data for a property cell.
	[[nodiscard]] QVariant data(const QModelIndex &index,
			int role) const override;
	/// Apply editable property changes through document commands.
	[[nodiscard]] bool setData(const QModelIndex &index, const QVariant &value,
			int role) override;
	/// Return item flags for a property cell.
	[[nodiscard]] Qt::ItemFlags flags(const QModelIndex &index) const override;
	/// Return horizontal header text.
	[[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation,
			int role) const override;

public Q_SLOTS:
	/// Rebuild descriptors from the current document state.
	void rebuild_from_document();
	/// Refresh descriptor rows associated with `object`.
	void refresh_object(quader::document::ObjectId object);

private:
	[[nodiscard]] const PropertyDescriptor *
	descriptor_for_row(int row) const noexcept;
	[[nodiscard]] bool apply_descriptor_edit(const PropertyDescriptor &descriptor,
			const QVariant &value);

	DocumentUiController &documents_;
	std::vector<PropertyDescriptor> descriptors_;
};

} // namespace quader::ui
