#include "ui/delegates/property_value_delegate.hpp"

#include "ui/models/property_descriptor.hpp"

#include <QAbstractItemModel>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QModelIndex>
#include <QVariant>

#include <limits>

namespace quader::ui {
namespace {

[[nodiscard]] PropertyValueKind value_kind_for_index(const QModelIndex &index) {
  return index.data(PropertyItemRoles::ValueKindRole)
      .value<PropertyValueKind>();
}

[[nodiscard]] PropertyDescriptor
descriptor_for_index(const QModelIndex &index) {
  return index.data(PropertyItemRoles::DescriptorRole)
      .value<PropertyDescriptor>();
}

[[nodiscard]] bool editable_for_index(const QModelIndex &index) {
  return index.data(PropertyItemRoles::EditableRole).toBool();
}

} // namespace

PropertyValueDelegate::PropertyValueDelegate(QObject *parent)
    : QStyledItemDelegate(parent) {}

QWidget *PropertyValueDelegate::createEditor(QWidget *parent,
                                             const QStyleOptionViewItem &option,
                                             const QModelIndex &index) const {
  (void)option;
  if (!editable_for_index(index)) {
    return nullptr;
  }

  switch (value_kind_for_index(index)) {
  case PropertyValueKind::String: {
    auto *editor = new QLineEdit(parent);
    editor->setFrame(false);
    return editor;
  }
  case PropertyValueKind::Float: {
    const auto descriptor = descriptor_for_index(index);
    auto *editor = new QDoubleSpinBox(parent);
    editor->setFrame(false);
    editor->setKeyboardTracking(false);
    editor->setDecimals(descriptor.decimals);
    editor->setMinimum(descriptor.minimum.isValid()
                           ? descriptor.minimum.toDouble()
                           : -std::numeric_limits<double>::max());
    editor->setMaximum(descriptor.maximum.isValid()
                           ? descriptor.maximum.toDouble()
                           : std::numeric_limits<double>::max());
    return editor;
  }
  case PropertyValueKind::Int:
  case PropertyValueKind::Vector3:
  case PropertyValueKind::ReadOnlyText:
    return nullptr;
  }

  return nullptr;
}

void PropertyValueDelegate::setEditorData(QWidget *editor,
                                          const QModelIndex &index) const {
  if (auto *line_edit = qobject_cast<QLineEdit *>(editor)) {
    line_edit->setText(index.data(Qt::EditRole).toString());
    return;
  }

  if (auto *spin_box = qobject_cast<QDoubleSpinBox *>(editor)) {
    spin_box->setValue(index.data(Qt::EditRole).toDouble());
    return;
  }

  QStyledItemDelegate::setEditorData(editor, index);
}

void PropertyValueDelegate::setModelData(QWidget *editor,
                                         QAbstractItemModel *model,
                                         const QModelIndex &index) const {
  if (auto *line_edit = qobject_cast<QLineEdit *>(editor)) {
    model->setData(index, line_edit->text(), Qt::EditRole);
    return;
  }

  if (auto *spin_box = qobject_cast<QDoubleSpinBox *>(editor)) {
    model->setData(index, spin_box->value(), Qt::EditRole);
    return;
  }

  QStyledItemDelegate::setModelData(editor, model, index);
}

QString PropertyValueDelegate::displayText(const QVariant &value,
                                           const QLocale &locale) const {
  if (value.metaType().id() == QMetaType::Double ||
      value.canConvert<double>()) {
    bool ok = false;
    const double number = value.toDouble(&ok);
    if (ok) {
      return locale.toString(number, 'f', 3);
    }
  }

  return QStyledItemDelegate::displayText(value, locale);
}

} // namespace quader::ui
