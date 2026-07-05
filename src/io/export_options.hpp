#pragma once

#include <string>

namespace quader::io {

struct ExportOptions {
	bool deterministic = true;
	std::string line_ending = "\n";
};

} // namespace quader::io
