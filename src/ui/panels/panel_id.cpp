#include "ui/panels/panel_id.hpp"

namespace quader::ui {

QString panel_id_name(PanelId id) {
	switch (id) {
		case PanelId::Properties:
			return QStringLiteral("properties");
		case PanelId::Scene:
			return QStringLiteral("scene");
		case PanelId::Diagnostics:
			return QStringLiteral("diagnostics");
	}

	return QStringLiteral("unknown");
}

QString panel_object_name(PanelId id) {
	return QStringLiteral("quader.panel.%1").arg(panel_id_name(id));
}

} // namespace quader::ui
