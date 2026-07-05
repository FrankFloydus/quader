#pragma once

#include <QString>

class QApplication;

namespace quader::ui {

struct QtApplicationMetadata {
    QString application_name;
    QString organization_name;
};

void apply_qt_application_metadata(QApplication& app, const QtApplicationMetadata& metadata);

} // namespace quader::ui
