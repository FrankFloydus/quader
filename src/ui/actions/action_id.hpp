#pragma once

#include <cstdint>
#include <string_view>

namespace quader::ui {

enum class ActionId : std::uint16_t {
	NewScene,
	OpenScene,
	SaveScene,
	SaveSceneAs,
	Exit,

	Undo,
	Redo,
	DuplicateSelection,
	DeleteSelection,

	SelectTool,
	MoveTool,
	RotateTool,
	ScaleTool,

	CreateCube,
	CreateLight,
	CreateCamera,

	ViewPerspective,
	ViewShaded,
	ToggleQuadViewports,
	FocusViewport,

	ShowScenePanel,
	ShowPropertiesPanel,
	ShowDiagnosticsPanel,
	ResetWorkspaceLayout,
};

[[nodiscard]] std::string_view action_id_name(ActionId id) noexcept;

} // namespace quader::ui
