#pragma once

#include "math/vec2.hpp"

#include <string>
#include <vector>

namespace quader::tools {

struct ToolPreviewSegment {
	quader::math::Vec2 start;
	quader::math::Vec2 end;
};

struct ToolPreview {
	bool active = false;
	std::string status_text;
	std::vector<quader::math::Vec2> points;
	std::vector<ToolPreviewSegment> segments;

	[[nodiscard]] bool empty() const noexcept;
	void clear();
};

} // namespace quader::tools
