#include "ui/actions/editor_state_snapshot.hpp"

namespace quader::ui {

EditorStateSnapshot NullEditorStateProvider::editor_state_snapshot() const
{
    return {};
}

} // namespace quader::ui
