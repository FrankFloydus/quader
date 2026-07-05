#pragma once

#include "foundation/logging.hpp"

#include <cstdio>
#include <cstdlib>

namespace quader::foundation {

[[noreturn]] inline void assertion_failed(const char *condition,
		const char *file,
		int line,
		const char *function) {
	char message[1024] = {};
	std::snprintf(message,
			sizeof(message),
			"Assertion failed: %s (%s:%d in %s)",
			condition,
			file,
			line,
			function);
	log_message(LogLevel::Error, message);
	std::abort();
}

} // namespace quader::foundation

#define QUADER_ASSERT(condition)                                                              \
	do {                                                                                      \
		if (!(static_cast<bool>(condition))) {                                                \
			::quader::foundation::assertion_failed(#condition, __FILE__, __LINE__, __func__); \
		}                                                                                     \
	} while (false)
