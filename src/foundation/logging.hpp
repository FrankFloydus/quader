#pragma once

#include <string_view>

namespace quader::foundation {

enum class LogLevel {
	Debug,
	Info,
	Warning,
	Error,
};

using LogSink = void (*)(LogLevel level, std::string_view message);

void set_log_sink(LogSink sink) noexcept;

[[nodiscard]] LogSink log_sink() noexcept;

[[nodiscard]] std::string_view log_level_name(LogLevel level) noexcept;

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
