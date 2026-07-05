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
#include "tools/tool_context.hpp"
#include "tools/tool_id.hpp"
#include "tools/tool_preview.hpp"

namespace quader::tools {

/// Base interface for toolkit-neutral editor tools.
class ITool {
public:
	/// Destroy the tool.
	virtual ~ITool() = default;

	/// Return the stable tool id.
	[[nodiscard]] virtual ToolId id() const noexcept = 0;

	/// Called when the tool becomes active.
	virtual void activate(ToolContext &context);
	/// Called when the tool is deactivated.
	virtual void deactivate(ToolContext &context);
	/// Cancel any in-progress interaction.
	virtual void cancel(ToolContext &context);

	/// Handle a pointer event.
	[[nodiscard]] virtual bool on_pointer_event(const PointerEvent &event, ToolContext &context);
	/// Handle a key event.
	[[nodiscard]] virtual bool on_key_event(const KeyEvent &event, ToolContext &context);
	/// Return the current preview payload.
	[[nodiscard]] virtual ToolPreview preview() const;
};

} // namespace quader::tools
