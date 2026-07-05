#pragma once

#include "math/vec2.hpp"

namespace quader::tools {

enum class PointerButton {
	None,
	Left,
	Middle,
	Right,
};

struct PointerEvent {
	quader::math::Vec2 position;
	PointerButton button = PointerButton::None;
	bool pressed = false;
};

struct KeyEvent {
	int key_code = 0;
	bool pressed = false;
	bool auto_repeat = false;
};

} // namespace quader::tools
