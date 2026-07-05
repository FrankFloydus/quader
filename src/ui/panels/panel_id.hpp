#pragma once

#include <cstdint>

#include <QString>

namespace quader::ui {

enum class PanelId : std::uint16_t {
	Properties,
	Scene,
	Diagnostics,
};

[[nodiscard]] QString panel_id_name(PanelId id);
[[nodiscard]] QString panel_object_name(PanelId id);

} // namespace quader::ui
