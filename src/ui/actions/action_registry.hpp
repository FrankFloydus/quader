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

#include "ui/actions/action_id.hpp"

#include <QAction>
#include <QKeySequence>
#include <QList>
#include <QObject>
#include <QString>
#include <Qt>

#include <cstddef>
#include <unordered_map>

namespace quader::ui {

/// Declarative configuration used when registering a canonical action.
struct ActionSpec {
	/// User-visible action text.
	QString text;
	/// Status bar text.
	QString status_tip;
	/// Optional keyboard shortcuts.
	QList<QKeySequence> shortcuts;
	/// Qt shortcut activation scope.
	Qt::ShortcutContext shortcut_context = Qt::WindowShortcut;
	/// True when the action carries checked state.
	bool checkable = false;
	/// Initial enabled state before an action-state refresh.
	bool initially_enabled = true;
	/// Qt menu role for platform menu integration.
	QAction::MenuRole menu_role = QAction::NoRole;
};

/**
 * Owns canonical `QAction` instances indexed by stable action id.
 *
 * Actions are parent-owned by the registry. Menus, toolbars, and shortcuts
 * borrow the same instances to keep enabled/checked state synchronized.
 */
class ActionRegistry final : public QObject {
	Q_OBJECT

public:
	/// Construct an empty registry.
	explicit ActionRegistry(QObject *parent = nullptr);

	/**
	 * Register or replace an action for an id.
	 *
	 * @param id Stable action id.
	 * @param spec Action metadata.
	 * @return Registered action owned by this registry.
	 */
	QAction &register_action(ActionId id, const ActionSpec &spec);
	/// Return a registered action; callers must only request known ids.
	[[nodiscard]] QAction &action(ActionId id) const;
	/// Return true when `id` has a registered action.
	[[nodiscard]] bool contains(ActionId id) const noexcept;
	/// Return the number of registered actions.
	[[nodiscard]] std::size_t size() const noexcept;

Q_SIGNALS:
	void action_triggered(quader::ui::ActionId id);
	void action_toggled(quader::ui::ActionId id, bool checked);

private:
	std::unordered_map<ActionId, QAction *> actions_;
};

/// Register the standard V1 action set.
void register_standard_actions(ActionRegistry &registry);

} // namespace quader::ui
