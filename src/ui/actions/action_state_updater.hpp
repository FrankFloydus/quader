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

#include "ui/actions/action_registry.hpp"
#include "ui/actions/editor_state_snapshot.hpp"

#include <QObject>

namespace quader::ui {

/// Applies editor state snapshots to registered action enabled/checked/text state.
class ActionStateUpdater final : public QObject {
	Q_OBJECT

public:
	/// Construct an updater over non-owning action and state-provider references.
	ActionStateUpdater(ActionRegistry &actions,
			IEditorStateProvider &state_provider,
			QObject *parent = nullptr);

public Q_SLOTS:
	/// Query the state provider and apply the resulting snapshot.
	void refresh();
	/// Apply an already-computed editor state snapshot.
	void refresh_from_snapshot(const quader::ui::EditorStateSnapshot &snapshot);

private:
	ActionRegistry &actions_;
	IEditorStateProvider &state_provider_;
};

} // namespace quader::ui
