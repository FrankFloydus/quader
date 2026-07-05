#pragma once

#include "ui/viewport/viewport_controller.hpp"

#include <QElapsedTimer>
#include <QTimer>
#include <QWidget>

class QEvent;
class QKeyEvent;
class QMouseEvent;
class QPaintEngine;
class QPaintEvent;
class QResizeEvent;
class QShowEvent;
class QWheelEvent;

namespace quader::ui {

class ViewportWidget final : public QWidget {
    Q_OBJECT

public:
    explicit ViewportWidget(ViewportController& controller, QWidget* parent = nullptr);
    ~ViewportWidget() override;

    [[nodiscard]] QSize minimumSizeHint() const override;
    [[nodiscard]] QPaintEngine* paintEngine() const override;

    [[nodiscard]] NativeViewportSurface native_surface() const;
    [[nodiscard]] ViewportPixelSize pixel_size() const;

protected:
    void showEvent(QShowEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private Q_SLOTS:
    void render_now();

private:
    [[nodiscard]] ViewportPoint viewport_point_from(const QPointF& point) const noexcept;
    [[nodiscard]] ViewportMouseButton mouse_button_from(Qt::MouseButton button) const noexcept;
    [[nodiscard]] ViewportKey key_from(int key) const noexcept;
    void update_splitter_cursor(const QPoint& position);
    void set_split_from_position(const QPoint& position);
    void update_splitter_handles();

    ViewportController& controller_;
    QTimer frame_timer_;
    QElapsedTimer elapsed_;
    QWidget* vertical_splitter_handle_ = nullptr;
    QWidget* horizontal_splitter_handle_ = nullptr;
    ViewportSplitHandle active_split_handle_ = ViewportSplitHandle::None;
    double last_frame_seconds_ = 0.0;
};

} // namespace quader::ui
