#pragma once

#include "crimson/frame/frame_snapshot.hpp"
#include "crimson/prototype_viewport.hpp"
#include "crimson/renderer_diagnostics.hpp"
#include "crimson/scene/render_world.hpp"
#include "foundation/result.hpp"

namespace crimson {

class FrameBuilder final {
public:
	[[nodiscard]] quader::foundation::Result<FrameSnapshot, RendererDiagnostic> build_prototype_snapshot(
			const PrototypeViewportFrame &frame) const;
};

} // namespace crimson
