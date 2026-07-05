#include "ui/style/fusion_style_policy.hpp"

#include <QApplication>
#include <QStyleFactory>

namespace quader::ui {

bool FusionStylePolicy::apply(QApplication&) const
{
    auto* fusion_style = QStyleFactory::create(QStringLiteral("Fusion"));
    if (fusion_style == nullptr) {
        return false;
    }

    QApplication::setStyle(fusion_style);
    return true;
}

} // namespace quader::ui
