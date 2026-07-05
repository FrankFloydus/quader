#include "ui/services/file_dialog_service.hpp"

#include <QFileDialog>

namespace quader::ui {
namespace {

[[nodiscard]] QString name_filter_from_filter(const FileDialogFilter& filter)
{
    if (filter.patterns.empty()) {
        return filter.label;
    }

    return QStringLiteral("%1 (%2)").arg(filter.label, filter.patterns.join(QStringLiteral(" ")));
}

[[nodiscard]] QStringList name_filters_from_filters(const QList<FileDialogFilter>& filters)
{
    QStringList name_filters;
    name_filters.reserve(filters.size());
    for (const auto& filter : filters) {
        name_filters.push_back(name_filter_from_filter(filter));
    }

    return name_filters;
}

} // namespace

std::optional<QString> QtFileDialogService::choose_open_file(
    QWidget* parent,
    const OpenFileDialogRequest& request)
{
    QFileDialog dialog(parent, request.title, request.initial_directory);
    dialog.setAcceptMode(QFileDialog::AcceptOpen);
    dialog.setFileMode(QFileDialog::ExistingFile);

    const QStringList name_filters = name_filters_from_filters(request.filters);
    if (!name_filters.empty()) {
        dialog.setNameFilters(name_filters);
    }

    if (dialog.exec() != QDialog::Accepted) {
        return std::nullopt;
    }

    const QStringList files = dialog.selectedFiles();
    if (files.empty()) {
        return std::nullopt;
    }

    return files.front();
}

std::optional<QString> QtFileDialogService::choose_save_file(
    QWidget* parent,
    const SaveFileDialogRequest& request)
{
    QFileDialog dialog(parent, request.title, request.initial_directory);
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setFileMode(QFileDialog::AnyFile);
    if (!request.default_suffix.isEmpty()) {
        dialog.setDefaultSuffix(request.default_suffix);
    }

    const QStringList name_filters = name_filters_from_filters(request.filters);
    if (!name_filters.empty()) {
        dialog.setNameFilters(name_filters);
    }

    if (dialog.exec() != QDialog::Accepted) {
        return std::nullopt;
    }

    const QStringList files = dialog.selectedFiles();
    if (files.empty()) {
        return std::nullopt;
    }

    return files.front();
}

} // namespace quader::ui
