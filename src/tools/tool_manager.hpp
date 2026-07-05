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

#include "tools/editor_input_event.hpp"
#include "tools/tool.hpp"
#include "tools/tool_context.hpp"
#include "tools/tool_id.hpp"
#include "tools/tool_preview.hpp"

#include <map>
#include <memory>
#include <optional>

namespace quader::tools {

/// Owns registered editor tools and forwards host input to the active tool.
class ToolManager final {
public:
	/**
	 * Construct a manager with the services shared by all registered tools.
	 *
	 * @param context Document and command-history services retained by the manager.
	 */
	explicit ToolManager(ToolContext context);

	/**
	 * Register a tool by its stable id.
	 *
	 * @param tool Tool instance to take ownership of.
	 * @return True when the tool was non-null and no tool with the same id existed.
	 */
	[[nodiscard]] bool register_tool(std::unique_ptr<ITool> tool);
	/// Return true when a tool with the id is registered.
	[[nodiscard]] bool has_tool(ToolId id) const noexcept;

	/// Return the active tool id, or no value when no tool is active.
	[[nodiscard]] std::optional<ToolId> active_tool_id() const noexcept;
	/// Return the active tool, or null when no tool is active.
	[[nodiscard]] ITool *active_tool() noexcept;
	/// Return the active tool, or null when no tool is active.
	[[nodiscard]] const ITool *active_tool() const noexcept;

	/**
	 * Activate a registered tool and deactivate the previous active tool.
	 *
	 * @param id Tool id to activate.
	 * @return True when the tool was registered.
	 */
	[[nodiscard]] bool set_active_tool(ToolId id);
	/// Deactivate and clear the current active tool.
	void clear_active_tool();
	/// Cancel the active tool interaction without changing the active tool.
	[[nodiscard]] bool cancel_active_tool();

	/// Dispatch a pointer event to the active tool.
	[[nodiscard]] bool dispatch_pointer_event(const PointerEvent &event);
	/// Dispatch a key event to the active tool.
	[[nodiscard]] bool dispatch_key_event(const KeyEvent &event);
	/// Return the active tool preview, or an empty preview when no tool is active.
	[[nodiscard]] ToolPreview preview() const;

	/// Return the shared tool context.
	[[nodiscard]] ToolContext &context() noexcept;
	/// Return the shared tool context.
	[[nodiscard]] const ToolContext &context() const noexcept;

private:
	ToolContext context_;
	std::map<ToolId, std::unique_ptr<ITool>> tools_;
	ITool *active_tool_ = nullptr;
};

} // namespace quader::tools
