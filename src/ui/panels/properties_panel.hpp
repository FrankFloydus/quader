#pragma once

#include "ui/panels/panel_context.hpp"

#include <QWidget>

class QLabel;
class QTableView;

namespace quader::ui {

class PropertyItemModel;
class PropertyValueDelegate;

class PropertiesPanel final : public QWidget {
    Q_OBJECT

public:
    explicit PropertiesPanel(PanelContext context, QWidget* parent = nullptr);

    [[nodiscard]] PropertyItemModel& model() noexcept;
    [[nodiscard]] QTableView& table_view() noexcept;

private:
    void update_empty_state();

    PanelContext context_;
    PropertyItemModel* model_ = nullptr;
    QTableView* table_view_ = nullptr;
    PropertyValueDelegate* delegate_ = nullptr;
    QLabel* empty_label_ = nullptr;
};

} // namespace quader::ui
