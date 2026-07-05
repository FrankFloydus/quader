#pragma once

#include "ui/actions/action_id.hpp"

#include <QAction>
#include <QKeySequence>
#include <QObject>
#include <QString>

#include <cstddef>
#include <unordered_map>

namespace quader::ui {

struct ActionSpec {
    QString text;
    QString status_tip;
    QKeySequence shortcut;
    bool checkable = false;
    bool initially_enabled = true;
    QAction::MenuRole menu_role = QAction::NoRole;
};

class ActionRegistry final : public QObject {
    Q_OBJECT

public:
    explicit ActionRegistry(QObject* parent = nullptr);

    QAction& register_action(ActionId id, ActionSpec spec);
    [[nodiscard]] QAction& action(ActionId id) const;
    [[nodiscard]] bool contains(ActionId id) const noexcept;
    [[nodiscard]] std::size_t size() const noexcept;

Q_SIGNALS:
    void action_triggered(quader::ui::ActionId id);
    void action_toggled(quader::ui::ActionId id, bool checked);

private:
    std::unordered_map<ActionId, QAction*> actions_;
};

void register_standard_actions(ActionRegistry& registry);

} // namespace quader::ui
