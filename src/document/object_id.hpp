#pragma once

#include "foundation/id.hpp"

namespace quader::document {

struct ObjectTag;

using ObjectId = quader::foundation::GenerationalId<ObjectTag>;

} // namespace quader::document
