#include "ui/services/file_dialog_service.hpp"

#include <gtest/gtest.h>

#include <QApplication>
#include <QWidget>

#include <cstdio>
#include <optional>

namespace {

class FakeFileDialogService final : public quader::ui::IFileDialogService {
public:
    std::optional<QString> choose_open_file(
        QWidget* parent,
        const quader::ui::OpenFileDialogRequest& request) override
    {
        last_open_parent = parent;
        last_open_request = request;
        ++open_calls;
        return next_open_result;
    }

    std::optional<QString> choose_save_file(
        QWidget* parent,
        const quader::ui::SaveFileDialogRequest& request) override
    {
        last_save_parent = parent;
        last_save_request = request;
        ++save_calls;
        return next_save_result;
    }

    int open_calls = 0;
    int save_calls = 0;
    QWidget* last_open_parent = nullptr;
    QWidget* last_save_parent = nullptr;
    quader::ui::OpenFileDialogRequest last_open_request;
    quader::ui::SaveFileDialogRequest last_save_request;
    std::optional<QString> next_open_result;
    std::optional<QString> next_save_result;
};

TEST(UiFileDialogService, FakeDialogServicePreservesOpenRequestAndResult)
{
    FakeFileDialogService dialogs;
    QWidget parent;
    dialogs.next_open_result = QStringLiteral("C:/tmp/scene.fake");

    quader::ui::OpenFileDialogRequest request;
    request.title = QStringLiteral("Open Scene");
    request.initial_directory = QStringLiteral("C:/tmp");
    request.filters.push_back(quader::ui::FileDialogFilter{
        QStringLiteral("Fake"),
        {QStringLiteral("*.fake")},
    });

    const auto result = dialogs.choose_open_file(&parent, request);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, QStringLiteral("C:/tmp/scene.fake"));
    EXPECT_EQ(dialogs.open_calls, 1);
    EXPECT_TRUE(dialogs.last_open_parent == &parent);
    EXPECT_EQ(dialogs.last_open_request.title, QStringLiteral("Open Scene"));
    EXPECT_EQ(dialogs.last_open_request.filters.front().patterns.front(), QStringLiteral("*.fake"));
}

TEST(UiFileDialogService, FakeDialogServicePreservesSaveRequestAndCancelResult)
{
    FakeFileDialogService dialogs;
    QWidget parent;

    quader::ui::SaveFileDialogRequest request;
    request.title = QStringLiteral("Save Scene");
    request.initial_directory = QStringLiteral("C:/tmp");
    request.default_suffix = QStringLiteral("fake");
    request.filters.push_back(quader::ui::FileDialogFilter{
        QStringLiteral("Fake"),
        {QStringLiteral("*.fake")},
    });

    const auto result = dialogs.choose_save_file(&parent, request);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(dialogs.save_calls, 1);
    EXPECT_TRUE(dialogs.last_save_parent == &parent);
    EXPECT_EQ(dialogs.last_save_request.default_suffix, QStringLiteral("fake"));
}

} // namespace
