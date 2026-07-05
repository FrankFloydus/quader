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
    invalid_id,
    rejected,
};

TEST(Foundation, StrongIdValidityEqualityAndHashing)
{
    using quader::foundation::Id;
    using quader::foundation::kInvalidIdIndex;

    static_assert(!std::is_convertible_v<Id<ObjectTag>, quader::foundation::IdIndex>);
    static_assert(!std::is_same_v<Id<ObjectTag>, Id<OtherTag>>);

    const Id<ObjectTag> invalid;
    const Id<ObjectTag> also_invalid = Id<ObjectTag>::invalid();
    const Id<ObjectTag> first{7};
    const Id<ObjectTag> first_copy{7};
    const Id<ObjectTag> second{8};
    const Id<ObjectTag> sentinel{kInvalidIdIndex};

    EXPECT_FALSE(invalid.is_valid());
    EXPECT_FALSE(also_invalid.is_valid());
    EXPECT_FALSE(sentinel.is_valid());
    EXPECT_TRUE(first.is_valid());
    EXPECT_EQ(first.index(), 7U);
    EXPECT_EQ(first, first_copy);
    EXPECT_NE(first, second);

    std::unordered_set<Id<ObjectTag>> ids;
    ids.insert(first);
    EXPECT_EQ(ids.count(first_copy), 1U);
    EXPECT_EQ(ids.count(second), 0U);
}

TEST(Foundation, GenerationalIdDistinguishesReusedSlots)
{
    using quader::foundation::GenerationalId;

    const GenerationalId<ObjectTag> invalid;
    const GenerationalId<ObjectTag> first_generation{3, 1};
    const GenerationalId<ObjectTag> first_generation_copy{3, 1};
    const GenerationalId<ObjectTag> reused_slot{3, 2};
    const GenerationalId<ObjectTag> different_slot{4, 1};

    EXPECT_FALSE(invalid.is_valid());
    EXPECT_TRUE(first_generation.is_valid());
    EXPECT_EQ(first_generation.index(), 3U);
    EXPECT_EQ(first_generation.generation(), 1U);
    EXPECT_EQ(first_generation, first_generation_copy);
    EXPECT_NE(first_generation, reused_slot);
    EXPECT_NE(first_generation, different_slot);

    std::unordered_set<GenerationalId<ObjectTag>> ids;
    ids.insert(first_generation);
    EXPECT_EQ(ids.count(first_generation_copy), 1U);
    EXPECT_EQ(ids.count(reused_slot), 0U);
}

TEST(Foundation, ResultSuccessAndErrorPaths)
{
    using quader::foundation::Result;

    auto success = Result<int, TestError>::success(42);
    EXPECT_TRUE(success.has_value());
    EXPECT_TRUE(static_cast<bool>(success));
    EXPECT_EQ(success.value(), 42);

    auto failure = Result<int, TestError>::failure(TestError::invalid_id);
    EXPECT_FALSE(failure.has_value());
    EXPECT_FALSE(static_cast<bool>(failure));
    EXPECT_EQ(failure.error(), TestError::invalid_id);

    auto void_success = Result<void, TestError>::success();
    EXPECT_TRUE(void_success.has_value());
    void_success.value();

    auto void_failure = Result<void, TestError>::failure(TestError::rejected);
    EXPECT_FALSE(void_failure.has_value());
    EXPECT_EQ(void_failure.error(), TestError::rejected);
}

TEST(Foundation, ResultSupportsMoveOnlySuccessValues)
{
    using quader::foundation::Result;

    auto result = Result<std::unique_ptr<int>, TestError>::success(std::make_unique<int>(17));
    EXPECT_TRUE(result.has_value());
    EXPECT_TRUE(result.value() != nullptr);
    EXPECT_EQ(*result.value(), 17);
}

TEST(Foundation, DiagnosticListReportsOnlyErrorSeverityAsErrors)
{
    using quader::foundation::DiagnosticList;
    using quader::foundation::Severity;

    DiagnosticList diagnostics;
    EXPECT_TRUE(diagnostics.empty());
    EXPECT_FALSE(diagnostics.has_errors());

    diagnostics.add(Severity::info, "loaded");
    diagnostics.add(Severity::warning, "missing optional attribute");
    EXPECT_FALSE(diagnostics.empty());
    EXPECT_EQ(diagnostics.size(), 2U);
    EXPECT_FALSE(diagnostics.has_errors());
    EXPECT_EQ(diagnostics[1].severity, Severity::warning);

    diagnostics.add(Severity::error, "invalid id");
    EXPECT_TRUE(diagnostics.has_errors());
    EXPECT_EQ(diagnostics.diagnostics().size(), 3U);

    std::size_t iterated = 0;
    for (const auto& diagnostic : diagnostics) {
        EXPECT_FALSE(diagnostic.message.empty());
        ++iterated;
    }
    EXPECT_EQ(iterated, diagnostics.size());
}

struct CapturedLog {
    quader::foundation::LogLevel level = quader::foundation::LogLevel::debug;
    std::string message;
};

std::vector<CapturedLog> g_captured_logs;

void capture_log(quader::foundation::LogLevel level, std::string_view message)
{
    g_captured_logs.push_back(CapturedLog{level, std::string(message)});
}

class LoggingTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        quader::foundation::set_log_sink(nullptr);
        g_captured_logs.clear();
    }

    void TearDown() override
    {
        quader::foundation::set_log_sink(nullptr);
        g_captured_logs.clear();
    }
};

TEST_F(LoggingTest, LogLevelNameCoversAllPublicLevelsAndUnknown)
{
    using quader::foundation::LogLevel;
    using quader::foundation::log_level_name;

    EXPECT_EQ(log_level_name(LogLevel::debug), std::string_view("debug"));
    EXPECT_EQ(log_level_name(LogLevel::info), std::string_view("info"));
    EXPECT_EQ(log_level_name(LogLevel::warning), std::string_view("warning"));
    EXPECT_EQ(log_level_name(LogLevel::error), std::string_view("error"));
    EXPECT_EQ(log_level_name(static_cast<LogLevel>(999)), std::string_view("unknown"));
}

TEST_F(LoggingTest, LoggingSinkOverrideReceivesStructuredLevelAndMessageOnce)
{
    using quader::foundation::LogLevel;
    using quader::foundation::log_sink;
    using quader::foundation::log_message;
    using quader::foundation::set_log_sink;

    set_log_sink(capture_log);
    EXPECT_EQ(log_sink(), capture_log);

    log_message(LogLevel::warning, "foundation warning");

    ASSERT_EQ(g_captured_logs.size(), 1U);
    EXPECT_EQ(g_captured_logs[0].level, LogLevel::warning);
    EXPECT_EQ(g_captured_logs[0].message, "foundation warning");
}

TEST_F(LoggingTest, ResetSinkUsesInternalFallbackWithoutCrashing)
{
    using quader::foundation::LogLevel;
    using quader::foundation::log_message;
    using quader::foundation::log_sink;
    using quader::foundation::set_log_sink;

    set_log_sink(capture_log);
    set_log_sink(nullptr);

    EXPECT_EQ(log_sink(), nullptr);
    EXPECT_NO_THROW(log_message(LogLevel::info, "fallback after sink reset"));
    EXPECT_TRUE(g_captured_logs.empty());
}

TEST_F(LoggingTest, LoggingMacrosDispatchPublicLevelsAndMessages)
{
    using quader::foundation::LogLevel;
    using quader::foundation::set_log_sink;

    set_log_sink(capture_log);

    QUADER_LOG_DEBUG("debug message");
    QUADER_LOG_INFO("info message");
    QUADER_LOG_WARNING("warning message");
    QUADER_LOG_ERROR("error message");

    ASSERT_EQ(g_captured_logs.size(), 4U);
    EXPECT_EQ(g_captured_logs[0].level, LogLevel::debug);
    EXPECT_EQ(g_captured_logs[0].message, "debug message");
    EXPECT_EQ(g_captured_logs[1].level, LogLevel::info);
    EXPECT_EQ(g_captured_logs[1].message, "info message");
    EXPECT_EQ(g_captured_logs[2].level, LogLevel::warning);
    EXPECT_EQ(g_captured_logs[2].message, "warning message");
    EXPECT_EQ(g_captured_logs[3].level, LogLevel::error);
    EXPECT_EQ(g_captured_logs[3].message, "error message");
}

} // namespace
