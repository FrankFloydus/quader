#include "ui/services/notification_service.hpp"

#include <utility>

namespace quader::ui {

NotificationService::NotificationService(QObject* parent)
    : QObject(parent)
{
}

void NotificationService::post(NotificationMessage message)
{
    Q_EMIT notification_posted(message);
}

void NotificationService::show_status(QString summary, int timeout_ms)
{
    post(NotificationMessage{
        NotificationSeverity::Status,
        std::move(summary),
        {},
        timeout_ms,
    });
}

void NotificationService::show_info(NotificationMessage message)
{
    message.severity = NotificationSeverity::Info;
    if (message.timeout_ms < 0) {
        message.timeout_ms = 3000;
    }
    post(std::move(message));
}

void NotificationService::show_warning(NotificationMessage message)
{
    message.severity = NotificationSeverity::Warning;
    if (message.timeout_ms < 0) {
        message.timeout_ms = 5000;
    }
    post(std::move(message));
}

void NotificationService::show_error(NotificationMessage message)
{
    message.severity = NotificationSeverity::Error;
    if (message.timeout_ms < 0) {
        message.timeout_ms = 0;
    }
    post(std::move(message));
}

} // namespace quader::ui
