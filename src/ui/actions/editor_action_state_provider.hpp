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

class EditorActionStateProvider final : public IEditorStateProvider {
public:
	EditorActionStateProvider(const quader::document::Document &document,
			const quader::commands::CommandHistory &command_history,
			const quader::tools::ToolManager &tool_manager,
			const quader::io::ImportExportRegistry &io_registry) noexcept;

	[[nodiscard]] EditorStateSnapshot editor_state_snapshot() const override;

private:
	const quader::document::Document &document_;
	const quader::commands::CommandHistory &command_history_;
	const quader::tools::ToolManager &tool_manager_;
	const quader::io::ImportExportRegistry &io_registry_;
};

} // namespace quader::ui
