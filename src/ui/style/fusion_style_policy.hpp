#pragma once

class QApplication;

namespace quader::ui {

class FusionStylePolicy final {
public:
    [[nodiscard]] bool apply(QApplication& app) const;
};

} // namespace quader::ui
