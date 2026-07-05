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

#include <QObject>
#include <QString>

namespace quader::ui {

/// Severity level for UI notifications.
enum class NotificationSeverity {
	Status, ///< Short status-bar style message.
	Info,   ///< Informational message.
	Warning,///< Recoverable warning.
	Error,  ///< Error message.
};

/// Notification payload emitted to UI surfaces.
struct NotificationMessage {
	/// Message severity.
	NotificationSeverity severity = NotificationSeverity::Status;
	/// Short user-visible summary.
	QString summary;
	/// Optional detail text.
	QString detail;
	/// Suggested display timeout in milliseconds.
	int timeout_ms = 3000;
};

/// Broadcasts status, warning, and error messages to Qt UI surfaces.
class NotificationService final : public QObject {
	Q_OBJECT

public:
	/// Construct a notification service parented to `parent`.
	explicit NotificationService(QObject *parent = nullptr);

	/// Post a fully-specified notification.
	void post(const NotificationMessage &message);
	/// Post a transient status notification.
	void show_status(QString summary, int timeout_ms = 3000);
	/// Post an informational notification.
	void show_info(NotificationMessage message);
	/// Post a warning notification.
	void show_warning(NotificationMessage message);
	/// Post an error notification.
	void show_error(NotificationMessage message);

Q_SIGNALS:
	/// Emitted whenever a notification is posted.
	void notification_posted(const quader::ui::NotificationMessage &message);
};

} // namespace quader::ui
