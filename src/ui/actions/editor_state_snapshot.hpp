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

#include <QString>

namespace quader::ui {

/// Snapshot of editor state used to update canonical actions.
struct EditorStateSnapshot {
	/// True when document services are available.
	bool document_services_available = false;
	/// True when a new-scene action can run.
	bool new_scene_available = false;
	/// True when file I/O services are available.
	bool file_io_available = false;
	/// True when at least one importer is registered.
	bool import_available = false;
	/// True when export is available.
	bool export_available = false;
	/// True when the editor has an active document.
	bool has_active_document = false;
	/// True when the current document is dirty.
	bool document_dirty = false;
	/// True when the document selection is non-empty.
	bool has_selection = false;
	/// True when duplicate-selection can run.
	bool can_duplicate_selection = false;
	/// True when delete-selection can run.
	bool can_delete_selection = false;

	/// True when undo is available.
	bool can_undo = false;
	/// User-visible undo action text.
	QString undo_text;
	/// True when redo is available.
	bool can_redo = false;
	/// User-visible redo action text.
	QString redo_text;

	/// True when tool services are available.
	bool tools_available = false;
	/// True when primitive creation actions can run.
	bool creation_available = false;
	/// Action id corresponding to the active tool.
	ActionId active_tool = ActionId::SelectTool;

	/// True when viewport services are available.
	bool viewport_available = true;
	/// True when viewport layout state is known.
	bool viewport_state_known = false;
	/// True when quad viewport layout is active.
	bool quad_viewports_enabled = false;
};

/// Interface for components that can provide an `EditorStateSnapshot`.
class IEditorStateProvider {
public:
	/// Destroy the provider.
	virtual ~IEditorStateProvider() = default;

	/// Return the current editor state snapshot.
	[[nodiscard]] virtual EditorStateSnapshot editor_state_snapshot() const = 0;
};

/// Null provider used when no editor state source is available.
class NullEditorStateProvider final : public IEditorStateProvider {
public:
	/// Return the default disabled editor state.
	[[nodiscard]] EditorStateSnapshot editor_state_snapshot() const override;
};

} // namespace quader::ui
