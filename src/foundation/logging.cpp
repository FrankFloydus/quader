/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
#include "foundation/logging.hpp"

#include <atomic>
#include <memory>

#include <spdlog/logger.h>
#include <spdlog/sinks/stdout_sinks.h>

namespace {

using quader::foundation::LogLevel;
using quader::foundation::LogSink;

std::atomic<LogSink> &log_sink_storage() noexcept {
	static std::atomic<LogSink> sink{ nullptr };
	return sink;
}

spdlog::level::level_enum to_spdlog_level(LogLevel level) noexcept {
	switch (level) {
		case LogLevel::Debug:
			return spdlog::level::debug;
		case LogLevel::Info:
			return spdlog::level::info;
		case LogLevel::Warning:
			return spdlog::level::warn;
		case LogLevel::Error:
			return spdlog::level::err;
	}

	return spdlog::level::info;
}

spdlog::logger &internal_logger() {
	static const auto kSink = std::make_shared<spdlog::sinks::stderr_sink_mt>();
	static spdlog::logger logger{ "quader", kSink };
	static const bool kConfigured = [] {
		logger.set_level(spdlog::level::debug);
		logger.set_pattern("[%l] %v");
		logger.flush_on(spdlog::level::err);
		return true;
	}();

	(void)kConfigured;
	return logger;
}

} // namespace

namespace quader::foundation {

void set_log_sink(LogSink sink) noexcept {
	log_sink_storage().store(sink, std::memory_order_release);
}

LogSink log_sink() noexcept {
	return log_sink_storage().load(std::memory_order_acquire);
}

std::string_view log_level_name(LogLevel level) noexcept {
	switch (level) {
		case LogLevel::Debug:
			return "debug";
		case LogLevel::Info:
			return "info";
		case LogLevel::Warning:
			return "warning";
		case LogLevel::Error:
			return "error";
	}

	return "unknown";
}

void log_message(LogLevel level, std::string_view message) {
	if (const auto kSink = log_sink()) {
		kSink(level, message);
		return;
	}

	internal_logger().log(to_spdlog_level(level), "{}", message);
}

} // namespace quader::foundation
