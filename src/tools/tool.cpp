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

} // namespace quader::tools
