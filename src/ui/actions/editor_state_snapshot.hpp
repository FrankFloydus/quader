#pragma once

#include "ui/actions/action_id.hpp"

#include <QString>

namespace quader::ui {

struct EditorStateSnapshot {
	bool document_services_available = false;
	bool new_scene_available = false;
	bool file_io_available = false;
	bool import_available = false;
	bool export_available = false;
	bool has_active_document = false;
	bool document_dirty = false;
	bool has_selection = false;
	bool can_duplicate_selection = false;
	bool can_delete_selection = false;

	bool can_undo = false;
	QString undo_text;
	bool can_redo = false;
	QString redo_text;

	bool tools_available = false;
	bool creation_available = false;
	ActionId active_tool = ActionId::SelectTool;

	bool viewport_available = true;
	bool viewport_state_known = false;
	bool quad_viewports_enabled = false;
};

class IEditorStateProvider {
public:
	virtual ~IEditorStateProvider() = default;

	[[nodiscard]] virtual EditorStateSnapshot editor_state_snapshot() const = 0;
};

class NullEditorStateProvider final : public IEditorStateProvider {
public:
	[[nodiscard]] EditorStateSnapshot editor_state_snapshot() const override;
};

} // namespace quader::ui
