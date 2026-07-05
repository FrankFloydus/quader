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

#include <string_view>

namespace quader::foundation {

/// Logging level used by the foundation logging sink.
enum class LogLevel {
	/// Verbose developer diagnostics.
	Debug,
	/// Informational messages.
	Info,
	/// Recoverable warnings.
	Warning,
	/// Error messages.
	Error,
};

/// Callback type that receives log messages.
using LogSink = void (*)(LogLevel level, std::string_view message);

/**
 * Replace the process-local logging sink.
 *
 * @param sink Sink callback, or `nullptr` to restore the default behavior.
 */
void set_log_sink(LogSink sink) noexcept;

/**
 * Return the active logging sink.
 *
 * @return Current process-local sink callback.
 */
[[nodiscard]] LogSink log_sink() noexcept;

/**
 * Return the stable display name for a log level.
 *
 * @param level Level to name.
 * @return Static string view for the level.
 */
[[nodiscard]] std::string_view log_level_name(LogLevel level) noexcept;

/**
 * Emit a message through the active sink.
 *
 * @param level Severity level for the message.
 * @param message Borrowed message text.
 */
void log_message(LogLevel level, std::string_view message);

} // namespace quader::foundation

#define QUADER_LOG_DEBUG(message) \
	::quader::foundation::log_message(::quader::foundation::LogLevel::Debug, (message))
#define QUADER_LOG_INFO(message) \
	::quader::foundation::log_message(::quader::foundation::LogLevel::Info, (message))
#define QUADER_LOG_WARNING(message) \
	::quader::foundation::log_message(::quader::foundation::LogLevel::Warning, (message))
#define QUADER_LOG_ERROR(message) \
	::quader::foundation::log_message(::quader::foundation::LogLevel::Error, (message))
