/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
#include "foundation/assert.hpp"
#include "foundation/diagnostic.hpp"
#include "foundation/id.hpp"
#include "foundation/logging.hpp"
#include "foundation/result.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_set>
#include <vector>

namespace {

struct ObjectTag;
struct OtherTag;

enum class TestError {
	InvalidId,
	Rejected,
};

TEST(Foundation, StrongIdValidityEqualityAndHashing) {
	using quader::foundation::Id;
	using quader::foundation::kInvalidIdIndex;

	static_assert(!std::is_convertible_v<Id<ObjectTag>, quader::foundation::IdIndex>);
	static_assert(!std::is_same_v<Id<ObjectTag>, Id<OtherTag>>);

	const Id<ObjectTag> kInvalid;
	const Id<ObjectTag> kAlsoInvalid = Id<ObjectTag>::invalid();
	const Id<ObjectTag> kFirst{ 7 };
	const Id<ObjectTag> kFirstCopy{ 7 };
	const Id<ObjectTag> kSecond{ 8 };
	const Id<ObjectTag> kSentinel{ kInvalidIdIndex };

	EXPECT_FALSE(kInvalid.is_valid());
	EXPECT_FALSE(kAlsoInvalid.is_valid());
	EXPECT_FALSE(kSentinel.is_valid());
	EXPECT_TRUE(kFirst.is_valid());
	EXPECT_EQ(kFirst.index(), 7U);
	EXPECT_EQ(kFirst, kFirstCopy);
	EXPECT_NE(kFirst, kSecond);

	std::unordered_set<Id<ObjectTag>> ids;
	ids.insert(kFirst);
	EXPECT_EQ(ids.count(kFirstCopy), 1U);
	EXPECT_EQ(ids.count(kSecond), 0U);
}

TEST(Foundation, GenerationalIdDistinguishesReusedSlots) {
	using quader::foundation::GenerationalId;

	const GenerationalId<ObjectTag> kInvalid;
	const GenerationalId<ObjectTag> kFirstGeneration{ 3, 1 };
	const GenerationalId<ObjectTag> kFirstGenerationCopy{ 3, 1 };
	const GenerationalId<ObjectTag> kReusedSlot{ 3, 2 };
	const GenerationalId<ObjectTag> kDifferentSlot{ 4, 1 };

	EXPECT_FALSE(kInvalid.is_valid());
	EXPECT_TRUE(kFirstGeneration.is_valid());
	EXPECT_EQ(kFirstGeneration.index(), 3U);
	EXPECT_EQ(kFirstGeneration.generation(), 1U);
	EXPECT_EQ(kFirstGeneration, kFirstGenerationCopy);
	EXPECT_NE(kFirstGeneration, kReusedSlot);
	EXPECT_NE(kFirstGeneration, kDifferentSlot);

	std::unordered_set<GenerationalId<ObjectTag>> ids;
	ids.insert(kFirstGeneration);
	EXPECT_EQ(ids.count(kFirstGenerationCopy), 1U);
	EXPECT_EQ(ids.count(kReusedSlot), 0U);
}

TEST(Foundation, ResultSuccessAndErrorPaths) {
	using quader::foundation::Result;

	auto success = Result<int, TestError>::success(42);
	EXPECT_TRUE(success.has_value());
	EXPECT_TRUE(static_cast<bool>(success));
	EXPECT_EQ(success.value(), 42);

	auto failure = Result<int, TestError>::failure(TestError::InvalidId);
	EXPECT_FALSE(failure.has_value());
	EXPECT_FALSE(static_cast<bool>(failure));
	EXPECT_EQ(failure.error(), TestError::InvalidId);

	auto void_success = Result<void, TestError>::success();
	EXPECT_TRUE(void_success.has_value());
	void_success.value();

	auto void_failure = Result<void, TestError>::failure(TestError::Rejected);
	EXPECT_FALSE(void_failure.has_value());
	EXPECT_EQ(void_failure.error(), TestError::Rejected);
}

TEST(Foundation, ResultSupportsMoveOnlySuccessValues) {
	using quader::foundation::Result;

	auto result = Result<std::unique_ptr<int>, TestError>::success(std::make_unique<int>(17));
	EXPECT_TRUE(result.has_value());
	EXPECT_TRUE(result.value() != nullptr);
	EXPECT_EQ(*result.value(), 17);
}

TEST(Foundation, DiagnosticListReportsOnlyErrorSeverityAsErrors) {
	using quader::foundation::DiagnosticList;
	using quader::foundation::Severity;

	DiagnosticList diagnostics;
	EXPECT_TRUE(diagnostics.empty());
	EXPECT_FALSE(diagnostics.has_errors());

	diagnostics.add(Severity::Info, "loaded");
	diagnostics.add(Severity::Warning, "missing optional attribute");
	EXPECT_FALSE(diagnostics.empty());
	EXPECT_EQ(diagnostics.size(), 2U);
	EXPECT_FALSE(diagnostics.has_errors());
	EXPECT_EQ(diagnostics[1].severity, Severity::Warning);

	diagnostics.add(Severity::Error, "invalid id");
	EXPECT_TRUE(diagnostics.has_errors());
	EXPECT_EQ(diagnostics.diagnostics().size(), 3U);

	std::size_t iterated = 0;
	for (const auto &diagnostic : diagnostics) {
		EXPECT_FALSE(diagnostic.message.empty());
		++iterated;
	}
	EXPECT_EQ(iterated, diagnostics.size());
}

struct CapturedLog {
	quader::foundation::LogLevel level = quader::foundation::LogLevel::Debug;
	std::string message;
};

std::vector<CapturedLog> g_captured_logs;

void capture_log(quader::foundation::LogLevel level, std::string_view message) {
	g_captured_logs.push_back(CapturedLog{ level, std::string(message) });
}

class LoggingTest : public ::testing::Test {
protected:
	void SetUp() override {
		quader::foundation::set_log_sink(nullptr);
		g_captured_logs.clear();
	}

	void TearDown() override {
		quader::foundation::set_log_sink(nullptr);
		g_captured_logs.clear();
	}
};

TEST_F(LoggingTest, LogLevelNameCoversAllPublicLevelsAndUnknown) {
	using quader::foundation::log_level_name;
	using quader::foundation::LogLevel;

	EXPECT_EQ(log_level_name(LogLevel::Debug), std::string_view("debug"));
	EXPECT_EQ(log_level_name(LogLevel::Info), std::string_view("info"));
	EXPECT_EQ(log_level_name(LogLevel::Warning), std::string_view("warning"));
	EXPECT_EQ(log_level_name(LogLevel::Error), std::string_view("error"));
	EXPECT_EQ(log_level_name(static_cast<LogLevel>(999)), std::string_view("unknown"));
}

TEST_F(LoggingTest, LoggingSinkOverrideReceivesStructuredLevelAndMessageOnce) {
	using quader::foundation::log_message;
	using quader::foundation::log_sink;
	using quader::foundation::LogLevel;
	using quader::foundation::set_log_sink;

	set_log_sink(capture_log);
	EXPECT_EQ(log_sink(), capture_log);

	log_message(LogLevel::Warning, "foundation warning");

	ASSERT_EQ(g_captured_logs.size(), 1U);
	EXPECT_EQ(g_captured_logs[0].level, LogLevel::Warning);
	EXPECT_EQ(g_captured_logs[0].message, "foundation warning");
}

TEST_F(LoggingTest, ResetSinkUsesInternalFallbackWithoutCrashing) {
	using quader::foundation::log_message;
	using quader::foundation::log_sink;
	using quader::foundation::LogLevel;
	using quader::foundation::set_log_sink;

	set_log_sink(capture_log);
	set_log_sink(nullptr);

	EXPECT_EQ(log_sink(), nullptr);
	EXPECT_NO_THROW(log_message(LogLevel::Info, "fallback after sink reset"));
	EXPECT_TRUE(g_captured_logs.empty());
}

TEST_F(LoggingTest, LoggingMacrosDispatchPublicLevelsAndMessages) {
	using quader::foundation::LogLevel;
	using quader::foundation::set_log_sink;

	set_log_sink(capture_log);

	QUADER_LOG_DEBUG("debug message");
	QUADER_LOG_INFO("info message");
	QUADER_LOG_WARNING("warning message");
	QUADER_LOG_ERROR("error message");

	ASSERT_EQ(g_captured_logs.size(), 4U);
	EXPECT_EQ(g_captured_logs[0].level, LogLevel::Debug);
	EXPECT_EQ(g_captured_logs[0].message, "debug message");
	EXPECT_EQ(g_captured_logs[1].level, LogLevel::Info);
	EXPECT_EQ(g_captured_logs[1].message, "info message");
	EXPECT_EQ(g_captured_logs[2].level, LogLevel::Warning);
	EXPECT_EQ(g_captured_logs[2].message, "warning message");
	EXPECT_EQ(g_captured_logs[3].level, LogLevel::Error);
	EXPECT_EQ(g_captured_logs[3].message, "error message");
}

} // namespace
