/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
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

/// QWidget host for native Crimson viewport rendering and input translation.
class ViewportWidget final : public QWidget {
	Q_OBJECT

public:
	/// Construct a viewport widget driven by `controller`.
	explicit ViewportWidget(ViewportController &controller, QWidget *parent = nullptr);
	/// Shut down the controller surface before widget destruction.
	~ViewportWidget() override;

	/// Return the minimum size for one viewport pane.
	[[nodiscard]] QSize minimumSizeHint() const override;
	/// Return null because native rendering bypasses Qt painting.
	[[nodiscard]] QPaintEngine *paintEngine() const override;

	/// Return the borrowed native surface handle for the widget.
	[[nodiscard]] NativeViewportSurface native_surface() const;
	/// Return widget size in physical pixels.
	[[nodiscard]] ViewportPixelSize pixel_size() const;

protected:
	/// Initialize the native render surface on first show.
	void showEvent(QShowEvent *event) override;
	/// Resize the native render surface.
	void resizeEvent(QResizeEvent *event) override;
	/// Submit a frame during paint handling.
	void paintEvent(QPaintEvent *event) override;
	/// Translate Qt mouse press events to viewport input.
	void mousePressEvent(QMouseEvent *event) override;
	/// Translate Qt mouse move events to viewport input.
	void mouseMoveEvent(QMouseEvent *event) override;
	/// Translate Qt mouse release events to viewport input.
	void mouseReleaseEvent(QMouseEvent *event) override;
	/// End hover/splitter state when the cursor leaves.
	void leaveEvent(QEvent *event) override;
	/// Translate Qt wheel events to viewport input.
	void wheelEvent(QWheelEvent *event) override;
	/// Translate Qt key press events to viewport input.
	void keyPressEvent(QKeyEvent *event) override;
	/// Translate Qt key release events to viewport input.
	void keyReleaseEvent(QKeyEvent *event) override;
	/// Track child splitter-handle input.
	bool eventFilter(QObject *watched, QEvent *event) override;

private Q_SLOTS:
	/// Render immediately when the frame timer fires.
	void render_now();

private:
	[[nodiscard]] ViewportPoint viewport_point_from(const QPointF &point) const noexcept;
	[[nodiscard]] ViewportMouseButton mouse_button_from(Qt::MouseButton button) const noexcept;
	[[nodiscard]] ViewportKey key_from(int key) const noexcept;
	void update_splitter_cursor(const QPoint &position);
	void set_split_from_position(const QPoint &position);
	void update_splitter_handles();

	ViewportController &controller_;
	QTimer frame_timer_;
	QElapsedTimer elapsed_;
	QWidget *vertical_splitter_handle_ = nullptr;
	QWidget *horizontal_splitter_handle_ = nullptr;
	ViewportSplitHandle active_split_handle_ = ViewportSplitHandle::None;
	double last_frame_seconds_ = 0.0;
};

} // namespace quader::ui
