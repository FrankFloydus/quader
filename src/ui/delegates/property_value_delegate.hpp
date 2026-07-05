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

#include <QStyledItemDelegate>

namespace quader::ui {

/// Delegate that edits property model values with kind-aware Qt editors.
class PropertyValueDelegate final : public QStyledItemDelegate {
	Q_OBJECT

public:
	/// Construct a delegate parented to `parent`.
	explicit PropertyValueDelegate(QObject *parent = nullptr);

	/// Create an editor widget for the property value at `index`.
	[[nodiscard]] QWidget *createEditor(QWidget *parent,
			const QStyleOptionViewItem &option,
			const QModelIndex &index) const override;
	/// Copy model data into an editor widget.
	void setEditorData(QWidget *editor, const QModelIndex &index) const override;
	/// Commit editor data back into the model.
	void setModelData(QWidget *editor, QAbstractItemModel *model,
			const QModelIndex &index) const override;
	/// Return localized display text for a property value.
	[[nodiscard]] QString displayText(const QVariant &value,
			const QLocale &locale) const override;
};

} // namespace quader::ui
