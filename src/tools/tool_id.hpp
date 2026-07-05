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

namespace quader::tools {

/// Stable identifier for an editor tool.
enum class ToolId {
	/// Selection tool.
	Select,
	/// Move transform tool.
	Move,
	/// Rotate transform tool.
	Rotate,
	/// Scale transform tool.
	Scale,
	/// Box creation tool.
	Box,
};

} // namespace quader::tools
