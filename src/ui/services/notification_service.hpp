#pragma once

#include <QObject>
#include <QString>

namespace quader::ui {

enum class NotificationSeverity {
    Status,
    Info,
    Warning,
    Error,
};

struct NotificationMessage {
    NotificationSeverity severity = NotificationSeverity::Status;
    QString summary;
    QString detail;
    int timeout_ms = 3000;
};

class NotificationService final : public QObject {
    Q_OBJECT

public:
    explicit NotificationService(QObject* parent = nullptr);

    void post(NotificationMessage message);
    void show_status(QString summary, int timeout_ms = 3000);
    void show_info(NotificationMessage message);
    void show_warning(NotificationMessage message);
    void show_error(NotificationMessage message);

Q_SIGNALS:
    void notification_posted(const quader::ui::NotificationMessage& message);
};

} // namespace quader::ui
