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

#include <string>

namespace quader::io {

/// Options that affect deterministic document export.
struct ExportOptions {
	/// Prefer stable output suitable for tests and diffs.
	bool deterministic = true;
	/// Line ending written by text-based exporters.
	std::string line_ending = "\n";
};

} // namespace quader::io
