#pragma once

#include "tools/editor_input_event.hpp"
#include "tools/tool.hpp"
#include "tools/tool_context.hpp"
#include "tools/tool_id.hpp"
#include "tools/tool_preview.hpp"

#include <map>
#include <memory>
#include <optional>

namespace quader::tools {

class ToolManager final {
public:
	explicit ToolManager(ToolContext context);

	[[nodiscard]] bool register_tool(std::unique_ptr<ITool> tool);
	[[nodiscard]] bool has_tool(ToolId id) const noexcept;

	[[nodiscard]] std::optional<ToolId> active_tool_id() const noexcept;
	[[nodiscard]] ITool *active_tool() noexcept;
	[[nodiscard]] const ITool *active_tool() const noexcept;

	[[nodiscard]] bool set_active_tool(ToolId id);
	void clear_active_tool();
	[[nodiscard]] bool cancel_active_tool();

	[[nodiscard]] bool dispatch_pointer_event(const PointerEvent &event);
	[[nodiscard]] bool dispatch_key_event(const KeyEvent &event);
	[[nodiscard]] ToolPreview preview() const;

	[[nodiscard]] ToolContext &context() noexcept;
	[[nodiscard]] const ToolContext &context() const noexcept;

private:
	ToolContext context_;
	std::map<ToolId, std::unique_ptr<ITool>> tools_;
	ITool *active_tool_ = nullptr;
};

} // namespace quader::tools
