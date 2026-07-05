#include "ui/actions/action_id.hpp"

namespace quader::ui {

std::string_view action_id_name(ActionId id) noexcept {
	switch (id) {
		case ActionId::NewScene:
			return "NewScene";
		case ActionId::OpenScene:
			return "OpenScene";
		case ActionId::SaveScene:
			return "SaveScene";
		case ActionId::SaveSceneAs:
			return "SaveSceneAs";
		case ActionId::Exit:
			return "Exit";
		case ActionId::Undo:
			return "Undo";
		case ActionId::Redo:
			return "Redo";
		case ActionId::DuplicateSelection:
			return "DuplicateSelection";
		case ActionId::DeleteSelection:
			return "DeleteSelection";
		case ActionId::SelectTool:
			return "SelectTool";
		case ActionId::MoveTool:
			return "MoveTool";
		case ActionId::RotateTool:
			return "RotateTool";
		case ActionId::ScaleTool:
			return "ScaleTool";
		case ActionId::CreateCube:
			return "CreateCube";
		case ActionId::CreateLight:
			return "CreateLight";
		case ActionId::CreateCamera:
			return "CreateCamera";
		case ActionId::ViewPerspective:
			return "ViewPerspective";
		case ActionId::ViewShaded:
			return "ViewShaded";
		case ActionId::ToggleQuadViewports:
			return "ToggleQuadViewports";
		case ActionId::FocusViewport:
			return "FocusViewport";
		case ActionId::ShowScenePanel:
			return "ShowScenePanel";
		case ActionId::ShowPropertiesPanel:
			return "ShowPropertiesPanel";
		case ActionId::ShowDiagnosticsPanel:
			return "ShowDiagnosticsPanel";
		case ActionId::ResetWorkspaceLayout:
			return "ResetWorkspaceLayout";
	}

	return "Unknown";
}

} // namespace quader::ui
