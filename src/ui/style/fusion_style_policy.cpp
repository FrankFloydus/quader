/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
#include "ui/style/fusion_style_policy.hpp"

#include <QApplication>
#include <QStyleFactory>

namespace quader::ui {

bool FusionStylePolicy::apply(QApplication &) const {
	auto *fusion_style = QStyleFactory::create(QStringLiteral("Fusion"));
	if (fusion_style == nullptr) {
		return false;
	}

	QApplication::setStyle(fusion_style);
	return true;
}

} // namespace quader::ui
