#pragma once

#include "tools/editor_input_event.hpp"
#include "tools/tool_context.hpp"
#include "tools/tool_id.hpp"
#include "tools/tool_preview.hpp"

namespace quader::tools {

class ITool {
public:
    virtual ~ITool() = default;

    [[nodiscard]] virtual ToolId id() const noexcept = 0;

    virtual void activate(ToolContext& context);
    virtual void deactivate(ToolContext& context);
    virtual void cancel(ToolContext& context);

    [[nodiscard]] virtual bool on_pointer_event(const PointerEvent& event, ToolContext& context);
    [[nodiscard]] virtual bool on_key_event(const KeyEvent& event, ToolContext& context);
    [[nodiscard]] virtual ToolPreview preview() const;
};

} // namespace quader::tools
