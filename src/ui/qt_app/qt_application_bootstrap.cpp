#include "ui/qt_app/qt_application_bootstrap.hpp"

#include <QApplication>

namespace quader::ui {

void apply_qt_application_metadata(QApplication& app, const QtApplicationMetadata& metadata)
{
    app.setApplicationName(metadata.application_name);
    app.setOrganizationName(metadata.organization_name);
}

} // namespace quader::ui
