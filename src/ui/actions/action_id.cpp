/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
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
		case ActionId::SelectAll:
			return "SelectAll";
		case ActionId::ClearSelection:
			return "ClearSelection";
		case ActionId::InvertSelection:
			return "InvertSelection";
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
		case ActionId::BoxTool:
			return "BoxTool";
		case ActionId::SelectObjectMode:
			return "SelectObjectMode";
		case ActionId::SelectVertexMode:
			return "SelectVertexMode";
		case ActionId::SelectEdgeMode:
			return "SelectEdgeMode";
		case ActionId::SelectFaceMode:
			return "SelectFaceMode";
		case ActionId::FlipMeshNormals:
			return "FlipMeshNormals";
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
		case ActionId::ShowGrid:
			return "ShowGrid";
		case ActionId::ShowOverlays:
			return "ShowOverlays";
		case ActionId::ShowMeshGrid:
			return "ShowMeshGrid";
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
