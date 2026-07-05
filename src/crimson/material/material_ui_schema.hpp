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

#include <cstdint>
#include <string>

namespace crimson {

/// Material schema binding kind represented by a UI field.
enum class MaterialUiFieldKind : std::uint8_t {
	Parameter,   ///< Field edits a material parameter.
	TextureSlot, ///< Field edits a texture slot binding.
};

/// Control kind requested by a material UI schema field.
enum class MaterialUiControl : std::uint8_t {
	Float,    ///< Scalar numeric editor.
	Vec2,     ///< Two-component numeric editor.
	Color,    ///< Color editor.
	Checkbox, ///< Boolean editor.
	Combo,    ///< Enum/list editor.
	Texture,  ///< Texture picker.
};

/// One UI field exposed by a base shader schema.
struct MaterialUiFieldDesc {
	/// User-visible field label.
	std::string label;
	/// Schema binding kind.
	MaterialUiFieldKind kind = MaterialUiFieldKind::Parameter;
	/// Preferred control type.
	MaterialUiControl control = MaterialUiControl::Float;
	/// Parameter or texture slot name edited by this field.
	std::string binding_name;
};

} // namespace crimson
