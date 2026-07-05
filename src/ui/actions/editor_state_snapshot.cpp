/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
#include "ui/actions/editor_state_snapshot.hpp"

namespace quader::ui {

EditorStateSnapshot NullEditorStateProvider::editor_state_snapshot() const {
	return {};
}

} // namespace quader::ui
