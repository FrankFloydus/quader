/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
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
	action->setShortcuts(spec.shortcuts);
	action->setShortcutContext(spec.shortcut_context);
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
														 QList<QKeySequence>{ QKeySequence::New },
												 });
	registry.register_action(ActionId::OpenScene, {
														  QStringLiteral("&Open..."),
														  QStringLiteral("Open a scene when file I/O services are available."),
														  QList<QKeySequence>{ QKeySequence::Open },
												  });
	registry.register_action(ActionId::SaveScene, {
														  QStringLiteral("&Save"),
														  QStringLiteral("Save the active scene when persistence is available."),
														  QList<QKeySequence>{ QKeySequence::Save },
												  });
	registry.register_action(ActionId::SaveSceneAs, {
															QStringLiteral("Save &As..."),
															QStringLiteral("Save the active scene to a new path when persistence is available."),
															QList<QKeySequence>{ QKeySequence::SaveAs },
													});
	registry.register_action(ActionId::Exit, {
													 QStringLiteral("E&xit"),
													 QStringLiteral("Close Quader."),
													 QList<QKeySequence>{ QKeySequence::Quit },
													 Qt::WindowShortcut,
													 false,
													 true,
													 QAction::QuitRole,
											 });

	registry.register_action(ActionId::Undo, {
													 QStringLiteral("&Undo"),
													 QStringLiteral("Undo the last command when command history is available."),
													 QList<QKeySequence>{ QKeySequence::Undo },
											 });
	registry.register_action(ActionId::Redo, {
													 QStringLiteral("&Redo"),
													 QStringLiteral("Redo the last undone command when command history is available."),
													 QList<QKeySequence>{ QKeySequence::Redo },
											 });
	registry.register_action(ActionId::SelectAll, {
														 QStringLiteral("Select &All"),
														 QStringLiteral("Select all items for the active selection mode."),
														 QList<QKeySequence>{ QKeySequence::SelectAll },
												 });
	registry.register_action(ActionId::ClearSelection, {
															 QStringLiteral("Clear Selection"),
															 QStringLiteral("Clear the current document selection."),
															 QList<QKeySequence>{ QKeySequence(QStringLiteral("Ctrl+Shift+A")) },
													 });
	registry.register_action(ActionId::InvertSelection, {
															  QStringLiteral("&Invert Selection"),
															  QStringLiteral("Invert the current selection in the active selection mode."),
															  QList<QKeySequence>{ QKeySequence(QStringLiteral("Ctrl+I")) },
													  });
	registry.register_action(ActionId::DuplicateSelection, {
																   QStringLiteral("Duplicate"),
																   QStringLiteral("Duplicate the current selection when document commands are available."),
																   QList<QKeySequence>{ QKeySequence(QStringLiteral("Ctrl+D")) },
														   });
	registry.register_action(ActionId::DeleteSelection, {
																QStringLiteral("Delete"),
																QStringLiteral("Delete the current selection when document commands are available."),
																QList<QKeySequence>{ QKeySequence::Delete, QKeySequence(Qt::Key_Backspace) },
														});

	registry.register_action(ActionId::SelectTool, {
														   QStringLiteral("Select"),
														   QStringLiteral("Activate the selection tool when tool services are available."),
														   QList<QKeySequence>{ QKeySequence(Qt::Key_Q) },
														   Qt::WidgetWithChildrenShortcut,
														   true,
												   });
	registry.register_action(ActionId::MoveTool, {
														 QStringLiteral("Move"),
														 QStringLiteral("Activate the move tool when tool services are available."),
														 QList<QKeySequence>{ QKeySequence(Qt::Key_W) },
														 Qt::WidgetWithChildrenShortcut,
														 true,
												 });
	registry.register_action(ActionId::RotateTool, {
														   QStringLiteral("Rotate"),
														   QStringLiteral("Activate the rotate tool when tool services are available."),
														   QList<QKeySequence>{ QKeySequence(Qt::Key_R) },
														   Qt::WidgetWithChildrenShortcut,
														   true,
												   });
	registry.register_action(ActionId::ScaleTool, {
														  QStringLiteral("Scale"),
														  QStringLiteral("Activate the scale tool when tool services are available."),
														  QList<QKeySequence>{ QKeySequence(Qt::Key_S) },
														  Qt::WidgetWithChildrenShortcut,
														  true,
												  });
	registry.register_action(ActionId::BoxTool, {
														QStringLiteral("Box"),
														QStringLiteral("Activate the Box creation tool when tool services are available."),
														QList<QKeySequence>{ QKeySequence(Qt::Key_B) },
														Qt::WidgetWithChildrenShortcut,
														true,
												});

	registry.register_action(ActionId::SelectObjectMode, {
																  QStringLiteral("Object"),
																  QStringLiteral("Use whole-object selection mode."),
																  QList<QKeySequence>{ QKeySequence(Qt::Key_4), QKeySequence(Qt::KeypadModifier | Qt::Key_4) },
																  Qt::WidgetWithChildrenShortcut,
																  true,
														  });
	registry.register_action(ActionId::SelectVertexMode, {
																  QStringLiteral("Vertex"),
																  QStringLiteral("Use vertex component selection mode."),
																  QList<QKeySequence>{ QKeySequence(Qt::Key_1), QKeySequence(Qt::KeypadModifier | Qt::Key_1) },
																  Qt::WidgetWithChildrenShortcut,
																  true,
														  });
	registry.register_action(ActionId::SelectEdgeMode, {
																QStringLiteral("Edge"),
																QStringLiteral("Use edge component selection mode."),
																QList<QKeySequence>{ QKeySequence(Qt::Key_2), QKeySequence(Qt::KeypadModifier | Qt::Key_2) },
																Qt::WidgetWithChildrenShortcut,
																true,
														});
	registry.register_action(ActionId::SelectFaceMode, {
																QStringLiteral("Face"),
																QStringLiteral("Use face component selection mode."),
																QList<QKeySequence>{ QKeySequence(Qt::Key_3), QKeySequence(Qt::KeypadModifier | Qt::Key_3) },
																Qt::WidgetWithChildrenShortcut,
																true,
														});

	registry.register_action(ActionId::FlipMeshNormals, {
																 QStringLiteral("Flip Mesh Normals"),
																 QStringLiteral("Reverse face winding for the current mesh selection when the mesh command is available."),
																 QList<QKeySequence>{ QKeySequence(Qt::Key_F) },
																 Qt::WidgetWithChildrenShortcut,
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
																QStringLiteral("Use the perspective viewport."),
														});
	registry.register_action(ActionId::ViewShaded, {
														   QStringLiteral("Shaded"),
														   QStringLiteral("Use the shaded viewport."),
												   });
	registry.register_action(ActionId::ToggleQuadViewports, {
																	QStringLiteral("Four Viewports"),
																	QStringLiteral("Toggle the four-viewport layout."),
																	QList<QKeySequence>{ QKeySequence(Qt::Key_F1) },
																	Qt::WindowShortcut,
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
															   Qt::WindowShortcut,
															   true,
													   });
	registry.register_action(ActionId::ShowPropertiesPanel, {
																	QStringLiteral("Properties Panel"),
																	QStringLiteral("Show or hide the Properties panel."),
																	{},
																	Qt::WindowShortcut,
																	true,
															});
	registry.register_action(ActionId::ShowDiagnosticsPanel, {
																	 QStringLiteral("Diagnostics Panel"),
																	 QStringLiteral("Show or hide renderer diagnostics."),
																	 {},
																	 Qt::WindowShortcut,
																	 true,
															 });

	registry.register_action(ActionId::ResetWorkspaceLayout, {
																	 QStringLiteral("Reset Layout"),
																	 QStringLiteral("Reset the saved UI workspace layout."),
															 });
}

} // namespace quader::ui
