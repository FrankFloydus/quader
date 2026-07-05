#include "tools/tool_preview.hpp"

namespace quader::tools {

bool ToolPreview::empty() const noexcept {
	return !active && status_text.empty() && points.empty() && segments.empty();
}

void ToolPreview::clear() {
	active = false;
	status_text.clear();
	points.clear();
	segments.clear();
}

} // namespace quader::tools
