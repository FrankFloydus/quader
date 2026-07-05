#include "ui/models/item_model_roles.hpp"

#include "ui/models/diagnostics_item_model.hpp"
#include "ui/models/property_descriptor.hpp"

#include <QMetaType>

namespace quader::ui {

void register_ui_model_metatypes() {
	qRegisterMetaType<quader::document::ObjectId>("quader::document::ObjectId");
	qRegisterMetaType<quader::ui::UiNodeId>("quader::ui::UiNodeId");
	qRegisterMetaType<quader::ui::DocumentItemKind>(
			"quader::ui::DocumentItemKind");
	qRegisterMetaType<quader::ui::PropertyPath>("quader::ui::PropertyPath");
	qRegisterMetaType<quader::ui::PropertyValueKind>(
			"quader::ui::PropertyValueKind");
	qRegisterMetaType<quader::ui::PropertyDescriptor>(
			"quader::ui::PropertyDescriptor");
	qRegisterMetaType<quader::ui::DiagnosticsNodeKind>(
			"quader::ui::DiagnosticsNodeKind");
}

} // namespace quader::ui
