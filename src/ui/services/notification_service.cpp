/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
#include "ui/services/notification_service.hpp"

#include <utility>

namespace quader::ui {

NotificationService::NotificationService(QObject *parent) : QObject(parent) {
}

void NotificationService::post(const NotificationMessage &message) {
	Q_EMIT notification_posted(message);
}

void NotificationService::show_status(QString summary, int timeout_ms) {
	post(NotificationMessage{
			NotificationSeverity::Status,
			std::move(summary),
			{},
			timeout_ms,
	});
}

void NotificationService::show_info(NotificationMessage message) {
	message.severity = NotificationSeverity::Info;
	if (message.timeout_ms < 0) {
		message.timeout_ms = 3000;
	}
	post(message);
}

void NotificationService::show_warning(NotificationMessage message) {
	message.severity = NotificationSeverity::Warning;
	if (message.timeout_ms < 0) {
		message.timeout_ms = 5000;
	}
	post(message);
}

void NotificationService::show_error(NotificationMessage message) {
	message.severity = NotificationSeverity::Error;
	if (message.timeout_ms < 0) {
		message.timeout_ms = 0;
	}
	post(message);
}

} // namespace quader::ui
