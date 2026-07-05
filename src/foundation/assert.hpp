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

#include "foundation/logging.hpp"

#include <cstdio>
#include <cstdlib>

namespace quader::foundation {

/**
 * Log an assertion failure and terminate the process.
 *
 * @param condition Stringized failed condition.
 * @param file Source file where the assertion failed.
 * @param line Source line where the assertion failed.
 * @param function Function where the assertion failed.
 */
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
