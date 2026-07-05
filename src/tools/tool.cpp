/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
#include "tools/tool.hpp"

namespace quader::tools {

void ITool::activate(ToolContext &context) {
	(void)context;
}

void ITool::deactivate(ToolContext &context) {
	(void)context;
}

void ITool::cancel(ToolContext &context) {
	(void)context;
}

bool ITool::on_pointer_event(const PointerEvent &event, ToolContext &context) {
	(void)event;
	(void)context;
	return false;
}

bool ITool::on_key_event(const KeyEvent &event, ToolContext &context) {
	(void)event;
	(void)context;
	return false;
}

ToolPreview ITool::preview() const {
	return {};
}

ToolCompletionRequest ITool::consume_completion_request() noexcept {
	return ToolCompletionRequest::None;
}

} // namespace quader::tools
