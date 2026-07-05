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

#include <string>

namespace quader::app {

/// Application metadata used during Qt startup.
struct ApplicationConfig {
	/// Name exposed to Qt application metadata and settings.
	std::string application_name = "Quader";
	/// Organization name exposed to Qt settings.
	std::string organization_name = "Quader";
};

} // namespace quader::app
