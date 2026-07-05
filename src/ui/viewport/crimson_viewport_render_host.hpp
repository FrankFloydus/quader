#pragma once

#include "crimson/diagnostics/renderer_diagnostics_snapshot.hpp"
#include "ui/viewport/viewport_render_host.hpp"

#include <memory>

#include <QString>

namespace quader::ui {

[[nodiscard]] ViewportDiagnosticsSnapshot viewport_diagnostics_from_crimson(
		const crimson::RendererDiagnosticsSnapshot &snapshot,
		QString renderer_name = {});

std::unique_ptr<IViewportRenderHost> create_crimson_viewport_render_host();

} // namespace quader::ui
