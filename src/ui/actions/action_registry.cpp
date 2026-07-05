#include "ui/actions/action_registry.hpp"

#include "foundation/assert.hpp"

#include <Qt>

#include <utility>

namespace quader::ui {

ActionRegistry::ActionRegistry(QObject *parent) : QObject(parent) {
}

QAction &ActionRegistry::register_action(ActionId id, const ActionSpec &spec) {
	if (auto existing = actions_.find(id); existing != actions_.end()) {
		return *existing->second;
	}

	auto *action = new QAction(spec.text, this);
	action->setStatusTip(spec.status_tip);
	action->setShortcut(spec.shortcut);
	action->setCheckable(spec.checkable);
	action->setEnabled(spec.initially_enabled);
	action->setMenuRole(spec.menu_role);

	actions_.emplace(id, action);

	connect(action, &QAction::triggered, this, [this, id]() {
		Q_EMIT action_triggered(id);
	});
	connect(action, &QAction::toggled, this, [this, id](bool checked) {
		Q_EMIT action_toggled(id, checked);
	});

	return *action;
}

QAction &ActionRegistry::action(ActionId id) const {
	const auto kIt = actions_.find(id);
	QUADER_ASSERT(kIt != actions_.end());
	return *kIt->second;
}

bool ActionRegistry::contains(ActionId id) const noexcept {
	return actions_.find(id) != actions_.end();
}

std::size_t ActionRegistry::size() const noexcept {
	return actions_.size();
}

void register_standard_actions(ActionRegistry &registry) {
	registry.register_action(ActionId::NewScene, {
														 QStringLiteral("&New Scene"),
														 QStringLiteral("Create a new scene when document services are available."),
														 QKeySequence::New,
												 });
	registry.register_action(ActionId::OpenScene, {
														  QStringLiteral("&Open..."),
														  QStringLiteral("Open a scene when file I/O services are available."),
														  QKeySequence::Open,
												  });
	registry.register_action(ActionId::SaveScene, {
														  QStringLiteral("&Save"),
														  QStringLiteral("Save the active scene when persistence is available."),
														  QKeySequence::Save,
												  });
	registry.register_action(ActionId::SaveSceneAs, {
															QStringLiteral("Save &As..."),
															QStringLiteral("Save the active scene to a new path when persistence is available."),
															QKeySequence::SaveAs,
													});
	registry.register_action(ActionId::Exit, {
													 QStringLiteral("E&xit"),
													 QStringLiteral("Close Quader."),
													 QKeySequence::Quit,
													 false,
													 true,
													 QAction::QuitRole,
											 });

	registry.register_action(ActionId::Undo, {
													 QStringLiteral("&Undo"),
													 QStringLiteral("Undo the last command when command history is available."),
													 QKeySequence::Undo,
											 });
	registry.register_action(ActionId::Redo, {
													 QStringLiteral("&Redo"),
													 QStringLiteral("Redo the last undone command when command history is available."),
													 QKeySequence::Redo,
											 });
	registry.register_action(ActionId::DuplicateSelection, {
																   QStringLiteral("Duplicate"),
																   QStringLiteral("Duplicate the current selection when document commands are available."),
																   QKeySequence(QStringLiteral("Ctrl+D")),
														   });
	registry.register_action(ActionId::DeleteSelection, {
																QStringLiteral("Delete"),
																QStringLiteral("Delete the current selection when document commands are available."),
																QKeySequence::Delete,
														});

	registry.register_action(ActionId::SelectTool, {
														   QStringLiteral("Select"),
														   QStringLiteral("Activate the selection tool when tool services are available."),
														   {},
														   true,
												   });
	registry.register_action(ActionId::MoveTool, {
														 QStringLiteral("Move"),
														 QStringLiteral("Activate the move tool when tool services are available."),
														 {},
														 true,
												 });
	registry.register_action(ActionId::RotateTool, {
														   QStringLiteral("Rotate"),
														   QStringLiteral("Activate the rotate tool when tool services are available."),
														   {},
														   true,
												   });
	registry.register_action(ActionId::ScaleTool, {
														  QStringLiteral("Scale"),
														  QStringLiteral("Activate the scale tool when tool services are available."),
														  {},
														  true,
												  });

	registry.register_action(ActionId::CreateCube, {
														   QStringLiteral("Cube"),
														   QStringLiteral("Create a cube when document creation commands are available."),
												   });
	registry.register_action(ActionId::CreateLight, {
															QStringLiteral("Light"),
															QStringLiteral("Create a light when document creation commands are available."),
													});
	registry.register_action(ActionId::CreateCamera, {
															 QStringLiteral("Camera"),
															 QStringLiteral("Create a camera when document creation commands are available."),
													 });

	registry.register_action(ActionId::ViewPerspective, {
																QStringLiteral("Perspective"),
																QStringLiteral("Use the prototype perspective view."),
														});
	registry.register_action(ActionId::ViewShaded, {
														   QStringLiteral("Shaded"),
														   QStringLiteral("Use the prototype shaded view."),
												   });
	registry.register_action(ActionId::ToggleQuadViewports, {
																	QStringLiteral("Four Viewports"),
																	QStringLiteral("Toggle the prototype four-viewport layout."),
																	QKeySequence(Qt::Key_F1),
																	true,
															});
	registry.register_action(ActionId::FocusViewport, {
															  QStringLiteral("Focus Viewport"),
															  QStringLiteral("Move keyboard focus to the viewport."),
													  });

	registry.register_action(ActionId::ShowScenePanel, {
															   QStringLiteral("Scene Panel"),
															   QStringLiteral("Show or hide the Scene panel."),
															   {},
															   true,
													   });
	registry.register_action(ActionId::ShowPropertiesPanel, {
																	QStringLiteral("Properties Panel"),
																	QStringLiteral("Show or hide the Properties panel."),
																	{},
																	true,
															});
	registry.register_action(ActionId::ShowDiagnosticsPanel, {
																	 QStringLiteral("Diagnostics Panel"),
																	 QStringLiteral("Show or hide renderer diagnostics."),
																	 {},
																	 true,
															 });

	registry.register_action(ActionId::ResetWorkspaceLayout, {
																	 QStringLiteral("Reset Layout"),
																	 QStringLiteral("Reset the saved UI workspace layout."),
															 });
}

} // namespace quader::ui
