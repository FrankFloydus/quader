#pragma once

#include "document/object_id.hpp"
#include "ui/models/property_descriptor.hpp"

#include <QAbstractTableModel>
#include <QModelIndex>
#include <QObject>

#include <vector>

namespace quader::ui {

class DocumentUiController;

enum class PropertyItemColumn : int {
	Label = 0,
	Value = 1,
};

class PropertyItemModel final : public QAbstractTableModel {
	Q_OBJECT

public:
	explicit PropertyItemModel(DocumentUiController &documents,
			QObject *parent = nullptr);

	[[nodiscard]] int rowCount(const QModelIndex &parent = {}) const override;
	[[nodiscard]] int columnCount(const QModelIndex &parent = {}) const override;
	[[nodiscard]] QVariant data(const QModelIndex &index,
			int role) const override;
	[[nodiscard]] bool setData(const QModelIndex &index, const QVariant &value,
			int role) override;
	[[nodiscard]] Qt::ItemFlags flags(const QModelIndex &index) const override;
	[[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation,
			int role) const override;

public Q_SLOTS:
	void rebuild_from_document();
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
