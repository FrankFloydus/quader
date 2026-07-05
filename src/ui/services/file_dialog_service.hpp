#pragma once

#include <QList>
#include <QString>
#include <QStringList>

#include <optional>

class QWidget;

namespace quader::ui {

struct FileDialogFilter {
    QString label;
    QStringList patterns;
};

struct OpenFileDialogRequest {
    QString title;
    QString initial_directory;
    QList<FileDialogFilter> filters;
};

struct SaveFileDialogRequest {
    QString title;
    QString initial_directory;
    QList<FileDialogFilter> filters;
    QString default_suffix;
};

class IFileDialogService {
public:
    virtual ~IFileDialogService() = default;

    [[nodiscard]] virtual std::optional<QString> choose_open_file(
        QWidget* parent,
        const OpenFileDialogRequest& request) = 0;
    [[nodiscard]] virtual std::optional<QString> choose_save_file(
        QWidget* parent,
        const SaveFileDialogRequest& request) = 0;
};

class QtFileDialogService final : public IFileDialogService {
public:
    [[nodiscard]] std::optional<QString> choose_open_file(
        QWidget* parent,
        const OpenFileDialogRequest& request) override;
    [[nodiscard]] std::optional<QString> choose_save_file(
        QWidget* parent,
        const SaveFileDialogRequest& request) override;
};

} // namespace quader::ui
