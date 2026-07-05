#pragma once

#include "ui/actions/action_registry.hpp"
#include "ui/actions/editor_state_snapshot.hpp"

#include <QObject>

namespace quader::ui {

class ActionStateUpdater final : public QObject {
	Q_OBJECT

public:
	ActionStateUpdater(ActionRegistry &actions,
			IEditorStateProvider &state_provider,
			QObject *parent = nullptr);

public Q_SLOTS:
	void refresh();
	void refresh_from_snapshot(const quader::ui::EditorStateSnapshot &snapshot);

private:
	ActionRegistry &actions_;
	IEditorStateProvider &state_provider_;
};

} // namespace quader::ui
