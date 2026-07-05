#pragma once

#include <QStyledItemDelegate>

namespace quader::ui {

class PropertyValueDelegate final : public QStyledItemDelegate {
  Q_OBJECT

public:
  explicit PropertyValueDelegate(QObject *parent = nullptr);

  [[nodiscard]] QWidget *createEditor(QWidget *parent,
                                      const QStyleOptionViewItem &option,
                                      const QModelIndex &index) const override;
  void setEditorData(QWidget *editor, const QModelIndex &index) const override;
  void setModelData(QWidget *editor, QAbstractItemModel *model,
                    const QModelIndex &index) const override;
  [[nodiscard]] QString displayText(const QVariant &value,
                                    const QLocale &locale) const override;
};

} // namespace quader::ui
