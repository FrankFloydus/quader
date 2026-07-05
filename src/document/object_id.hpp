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

#include "foundation/id.hpp"

namespace quader::document {

/// Tag type for stable document object ids.
struct ObjectTag;

/// Stable generational id for scene objects stored in a document.
using ObjectId = quader::foundation::GenerationalId<ObjectTag>;

} // namespace quader::document
