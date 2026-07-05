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

#include <cstdint>
#include <string_view>

namespace quader::ui {

/// Stable identifiers for canonical user-visible Qt actions.
enum class ActionId : std::uint16_t {
	NewScene,   ///< Create a new scene.
	OpenScene, ///< Open an existing scene.
	SaveScene, ///< Save the current scene.
	SaveSceneAs, ///< Save the current scene to a new path.
	Exit, ///< Quit the application.

	Undo, ///< Undo the last command.
	Redo, ///< Redo the next command.
	DuplicateSelection, ///< Duplicate the current selection.
	DeleteSelection, ///< Delete the current selection.

	SelectTool, ///< Activate the selection tool.
	MoveTool, ///< Activate the move tool.
	RotateTool, ///< Activate the rotate tool.
	ScaleTool, ///< Activate the scale tool.
	BoxTool, ///< Activate the box creation tool.

	CreateCube, ///< Create a cube primitive.
	CreateLight, ///< Create a light object.
	CreateCamera, ///< Create a camera object.

	ViewPerspective, ///< Switch the active view to perspective mode.
	ViewShaded, ///< Switch the viewport to shaded mode.
	ToggleQuadViewports, ///< Toggle single/quad viewport layout.
	FocusViewport, ///< Focus the viewport on the current selection.

	ShowScenePanel, ///< Show or hide the scene panel.
	ShowPropertiesPanel, ///< Show or hide the properties panel.
	ShowDiagnosticsPanel, ///< Show or hide the diagnostics panel.
	ResetWorkspaceLayout, ///< Restore the default workspace layout.
};

/// Return the stable debug name for an action id.
[[nodiscard]] std::string_view action_id_name(ActionId id) noexcept;

} // namespace quader::ui
