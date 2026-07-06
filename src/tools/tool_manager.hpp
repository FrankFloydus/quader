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

#include <functional>
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

	/// Return the active selection granularity used by selection actions/tools.
	[[nodiscard]] SelectionMode selection_mode() const noexcept;
	/// Return the transient selection hover hit, when the viewport has one.
	[[nodiscard]] std::optional<SurfaceHit> selection_hover() const;
	/// Return true when the current hover is a selected-component removal preview.
	[[nodiscard]] bool selection_hover_suppresses_selected() const noexcept;
	/// Clear the transient selection hover hit.
	[[nodiscard]] bool clear_selection_hover();
	/**
	 * Set the active selection granularity.
	 *
	 * @param mode Selection mode to store.
	 * @return True when the mode changed.
	 */
	bool set_selection_mode(SelectionMode mode) noexcept;

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
	/// Set a callback invoked after the active tool changes internally or through actions.
	void set_after_active_tool_changed(std::function<void()> callback);

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
	[[nodiscard]] bool handle_select_pointer_event(const PointerEvent &event);
	[[nodiscard]] bool clear_selection_hover_state() noexcept;
	[[nodiscard]] bool clear_visible_selection_hover() noexcept;
	[[nodiscard]] bool suppress_selection_hover_after_click(const SurfaceHit &hit);
	[[nodiscard]] bool selected_modifier_hover_for_hit(const SurfaceHit &hit,
			const KeyboardModifiers &modifiers) const;
	[[nodiscard]] bool set_selection_hover(SurfaceHit hit, bool suppresses_selected);
	void apply_tool_completion_request(ToolId handled_tool);
	void notify_active_tool_changed();

	ToolContext context_;
	std::map<ToolId, std::unique_ptr<ITool>> tools_;
	ITool *active_tool_ = nullptr;
	SelectionMode selection_mode_ = SelectionMode::Object;
	std::optional<SurfaceHit> selection_hover_;
	bool selection_hover_suppresses_selected_ = false;
	std::optional<SurfaceHit> suppressed_selection_hover_;
	std::function<void()> after_active_tool_changed_;
};

} // namespace quader::tools
