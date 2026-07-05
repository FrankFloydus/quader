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

#include "ui/actions/editor_state_snapshot.hpp"

namespace quader::commands {
class CommandHistory;
}

namespace quader::document {
class Document;
}

namespace quader::tools {
class ToolManager;
}

namespace quader::io {
class ImportExportRegistry;
}

namespace quader::ui {

/// Derives action state from document, command history, tools, and I/O services.
class EditorActionStateProvider final : public IEditorStateProvider {
public:
	/// Construct a provider over non-owning application service references.
	EditorActionStateProvider(const quader::document::Document &document,
			const quader::commands::CommandHistory &command_history,
			const quader::tools::ToolManager &tool_manager,
			const quader::io::ImportExportRegistry &io_registry) noexcept;

	/// Return the current action-state snapshot.
	[[nodiscard]] EditorStateSnapshot editor_state_snapshot() const override;

private:
	const quader::document::Document &document_;
	const quader::commands::CommandHistory &command_history_;
	const quader::tools::ToolManager &tool_manager_;
	const quader::io::ImportExportRegistry &io_registry_;
};

} // namespace quader::ui
