#include "ui/viewport/viewport_widget.hpp"

#include <QCursor>
#include <QEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QResizeEvent>
#include <QShowEvent>
#include <QStyle>
#include <QStyleOption>
#include <QWheelEvent>
#include <QtGlobal>

#include <algorithm>
#include <cmath>

namespace quader::ui {
namespace {

class StyledSplitterHandle final : public QWidget {
public:
	explicit StyledSplitterHandle(Qt::Orientation orientation, QWidget *parent = nullptr) : QWidget(parent), orientation_(orientation) {
		setCursor(orientation_ == Qt::Vertical ? Qt::SizeHorCursor : Qt::SizeVerCursor);
		setMouseTracking(true);
	}

	[[nodiscard]] QSize sizeHint() const override {
		return orientation_ == Qt::Vertical
				? QSize(kViewportSplitterWidth, kMinimumViewportExtent)
				: QSize(kMinimumViewportExtent, kViewportSplitterWidth);
	}

protected:
	void paintEvent(QPaintEvent *) override {
		QPainter painter(this);
		painter.fillRect(rect(), palette().color(QPalette::Mid));

		QStyleOption option;
		option.initFrom(this);
		option.rect = rect();
		option.state |= QStyle::State_Raised;
		style()->drawControl(QStyle::CE_Splitter, &option, &painter, this);
	}

private:
	Qt::Orientation orientation_ = Qt::Vertical;
};

} // namespace

ViewportWidget::ViewportWidget(ViewportController &controller, QWidget *parent) : QWidget(parent), controller_(controller) {
	setAttribute(Qt::WA_NativeWindow);
	setAttribute(Qt::WA_PaintOnScreen);
	setAttribute(Qt::WA_NoSystemBackground);
	setAttribute(Qt::WA_OpaquePaintEvent);
	setFocusPolicy(Qt::StrongFocus);
	setMouseTracking(true);

	vertical_splitter_handle_ = new StyledSplitterHandle(Qt::Vertical, this);
	vertical_splitter_handle_->installEventFilter(this);
	vertical_splitter_handle_->hide();

	horizontal_splitter_handle_ = new StyledSplitterHandle(Qt::Horizontal, this);
	horizontal_splitter_handle_->installEventFilter(this);
	horizontal_splitter_handle_->hide();

	frame_timer_.setTimerType(Qt::PreciseTimer);
	frame_timer_.setInterval(16);
	connect(&frame_timer_, &QTimer::timeout, this, &ViewportWidget::render_now);
	connect(&controller_, &ViewportController::frame_requested, this, qOverload<>(&ViewportWidget::update));
	connect(&controller_, &ViewportController::viewport_layout_changed, this, [this](bool, const QString &) {
		update_splitter_cursor(mapFromGlobal(QCursor::pos()));
		update_splitter_handles();
	});
}

ViewportWidget::~ViewportWidget() {
	frame_timer_.stop();
	controller_.shutdown_surface();
}

QSize ViewportWidget::minimumSizeHint() const {
	return { 640, 420 };
}

QPaintEngine *ViewportWidget::paintEngine() const {
	return nullptr;
}

NativeViewportSurface ViewportWidget::native_surface() const {
	NativeViewportSurface surface;
#if defined(Q_OS_WIN)
	surface.platform = NativeViewportSurface::Platform::WindowsHwnd;
	surface.native_window = reinterpret_cast<void *>(winId());
#else
	surface.platform = NativeViewportSurface::Platform::Unknown;
	surface.native_window = nullptr;
#endif
	return surface;
}

ViewportPixelSize ViewportWidget::pixel_size() const {
	const double kDpr = std::max(0.01, devicePixelRatioF());
	return ViewportPixelSize{
		.width = std::max(1, static_cast<int>(std::round(width() * kDpr))),
		.height = std::max(1, static_cast<int>(std::round(height() * kDpr))),
	};
}

void ViewportWidget::showEvent(QShowEvent *event) {
	QWidget::showEvent(event);

	if (!controller_.surface_initialized()) {
		controller_.initialize_surface(native_surface(), pixel_size(), devicePixelRatioF());
		elapsed_.start();
		last_frame_seconds_ = 0.0;
		frame_timer_.start();
	}
}

void ViewportWidget::resizeEvent(QResizeEvent *event) {
	QWidget::resizeEvent(event);
	controller_.resize_surface(pixel_size(), devicePixelRatioF());
	update_splitter_handles();
}

void ViewportWidget::paintEvent(QPaintEvent *event) {
	event->ignore();
	render_now();
}

void ViewportWidget::mousePressEvent(QMouseEvent *event) {
	setFocus(Qt::MouseFocusReason);

	if (event->button() == Qt::LeftButton && controller_.quad_viewports_enabled()) {
		active_split_handle_ = controller_.layout_state().splitter_at(
				pixel_size(),
				viewport_point_from(event->position() * devicePixelRatioF()));
		if (active_split_handle_ != ViewportSplitHandle::None) {
			set_split_from_position(event->pos());
			event->accept();
			return;
		}
	}

	controller_.handle_mouse_press(
			mouse_button_from(event->button()),
			viewport_point_from(event->position() * devicePixelRatioF()),
			event->modifiers().testFlag(Qt::ShiftModifier),
			pixel_size());

	if (event->button() == Qt::MiddleButton || event->button() == Qt::RightButton) {
		event->accept();
		return;
	}

	QWidget::mousePressEvent(event);
}

void ViewportWidget::mouseMoveEvent(QMouseEvent *event) {
	if (active_split_handle_ != ViewportSplitHandle::None) {
		set_split_from_position(event->pos());
		event->accept();
		return;
	}

	if (controller_.navigation_mode() != NavigationMode::None) {
		controller_.handle_mouse_move(viewport_point_from(event->position() * devicePixelRatioF()), pixel_size());
		event->accept();
		return;
	}

	update_splitter_cursor(event->pos());
	QWidget::mouseMoveEvent(event);
}

void ViewportWidget::mouseReleaseEvent(QMouseEvent *event) {
	if (event->button() == Qt::LeftButton && active_split_handle_ != ViewportSplitHandle::None) {
		active_split_handle_ = ViewportSplitHandle::None;
		update_splitter_cursor(event->pos());
		event->accept();
		return;
	}

	if (controller_.navigation_mode() != NavigationMode::None) {
		controller_.handle_mouse_release(mouse_button_from(event->button()));
		event->accept();
		return;
	}

	QWidget::mouseReleaseEvent(event);
}

void ViewportWidget::leaveEvent(QEvent *event) {
	if (active_split_handle_ == ViewportSplitHandle::None) {
		unsetCursor();
	}

	QWidget::leaveEvent(event);
}

void ViewportWidget::wheelEvent(QWheelEvent *event) {
	const float kSteps = static_cast<float>(event->angleDelta().y()) / 120.0F;
	if (std::abs(kSteps) > 0.000001F) {
		controller_.handle_wheel(kSteps, viewport_point_from(event->position() * devicePixelRatioF()), pixel_size());
		event->accept();
		return;
	}

	QWidget::wheelEvent(event);
}

void ViewportWidget::keyPressEvent(QKeyEvent *event) {
	controller_.handle_key(key_from(event->key()), true, event->isAutoRepeat());
	QWidget::keyPressEvent(event);
}

void ViewportWidget::keyReleaseEvent(QKeyEvent *event) {
	controller_.handle_key(key_from(event->key()), false, event->isAutoRepeat());
	QWidget::keyReleaseEvent(event);
}

bool ViewportWidget::eventFilter(QObject *watched, QEvent *event) {
	const bool kIsVerticalHandle = watched == vertical_splitter_handle_;
	const bool kIsHorizontalHandle = watched == horizontal_splitter_handle_;

	if (!kIsVerticalHandle && !kIsHorizontalHandle) {
		return QWidget::eventFilter(watched, event);
	}

	const ViewportSplitHandle kHandle = kIsVerticalHandle
			? ViewportSplitHandle::Vertical
			: ViewportSplitHandle::Horizontal;

	switch (event->type()) {
		case QEvent::MouseButtonPress: {
			auto *mouse_event = static_cast<QMouseEvent *>(event);
			if (mouse_event->button() == Qt::LeftButton && controller_.quad_viewports_enabled()) {
				active_split_handle_ = kHandle;
				set_split_from_position(mapFromGlobal(mouse_event->globalPosition().toPoint()));
				return true;
			}
			break;
		}
		case QEvent::MouseMove: {
			auto *mouse_event = static_cast<QMouseEvent *>(event);
			if (active_split_handle_ == kHandle) {
				set_split_from_position(mapFromGlobal(mouse_event->globalPosition().toPoint()));
				return true;
			}
			break;
		}
		case QEvent::MouseButtonRelease: {
			auto *mouse_event = static_cast<QMouseEvent *>(event);
			if (mouse_event->button() == Qt::LeftButton && active_split_handle_ == kHandle) {
				set_split_from_position(mapFromGlobal(mouse_event->globalPosition().toPoint()));
				active_split_handle_ = ViewportSplitHandle::None;
				update_splitter_cursor(mapFromGlobal(QCursor::pos()));
				return true;
			}
			break;
		}
		default:
			break;
	}

	return QWidget::eventFilter(watched, event);
}

void ViewportWidget::render_now() {
	if (!isVisible()) {
		return;
	}

	const double kElapsedSeconds = elapsed_.isValid() ? elapsed_.elapsed() / 1000.0 : 0.0;
	const float kDeltaSeconds = static_cast<float>(std::clamp(kElapsedSeconds - last_frame_seconds_, 0.0, 0.25));
	last_frame_seconds_ = kElapsedSeconds;
	controller_.render_frame(kElapsedSeconds, kDeltaSeconds);
}

ViewportPoint ViewportWidget::viewport_point_from(const QPointF &point) const noexcept {
	return ViewportPoint{ point.x(), point.y() };
}

ViewportMouseButton ViewportWidget::mouse_button_from(Qt::MouseButton button) const noexcept {
	switch (button) {
		case Qt::LeftButton:
			return ViewportMouseButton::Left;
		case Qt::MiddleButton:
			return ViewportMouseButton::Middle;
		case Qt::RightButton:
			return ViewportMouseButton::Right;
		default:
			return ViewportMouseButton::Other;
	}
}

ViewportKey ViewportWidget::key_from(int key) const noexcept {
	switch (key) {
		case Qt::Key_W:
			return ViewportKey::W;
		case Qt::Key_A:
			return ViewportKey::A;
		case Qt::Key_S:
			return ViewportKey::S;
		case Qt::Key_D:
			return ViewportKey::D;
		default:
			return ViewportKey::Other;
	}
}

void ViewportWidget::update_splitter_cursor(const QPoint &position) {
	if (active_split_handle_ == ViewportSplitHandle::Vertical) {
		setCursor(Qt::SizeHorCursor);
		return;
	}

	if (active_split_handle_ == ViewportSplitHandle::Horizontal) {
		setCursor(Qt::SizeVerCursor);
		return;
	}

	switch (controller_.layout_state().splitter_at(pixel_size(), viewport_point_from(position * devicePixelRatioF()))) {
		case ViewportSplitHandle::Vertical:
			setCursor(Qt::SizeHorCursor);
			break;
		case ViewportSplitHandle::Horizontal:
			setCursor(Qt::SizeVerCursor);
			break;
		case ViewportSplitHandle::None:
			unsetCursor();
			break;
	}
}

void ViewportWidget::set_split_from_position(const QPoint &position) {
	controller_.layout_state().set_split_from_position(
			pixel_size(),
			active_split_handle_,
			viewport_point_from(position * devicePixelRatioF()));
	update_splitter_handles();
	controller_.request_frame();
}

void ViewportWidget::update_splitter_handles() {
	if (vertical_splitter_handle_ == nullptr || horizontal_splitter_handle_ == nullptr) {
		return;
	}

	if (!controller_.quad_viewports_enabled()) {
		vertical_splitter_handle_->hide();
		horizontal_splitter_handle_->hide();
		return;
	}

	const double kDpr = std::max(0.01, devicePixelRatioF());
	const ViewportRect kVertical = controller_.layout_state().vertical_splitter_rect(pixel_size());
	const ViewportRect kHorizontal = controller_.layout_state().horizontal_splitter_rect(pixel_size());

	vertical_splitter_handle_->setGeometry(
			static_cast<int>(std::round(kVertical.x / kDpr)),
			static_cast<int>(std::round(kVertical.y / kDpr)),
			std::max(1, static_cast<int>(std::round(kVertical.width / kDpr))),
			std::max(1, static_cast<int>(std::round(kVertical.height / kDpr))));
	horizontal_splitter_handle_->setGeometry(
			static_cast<int>(std::round(kHorizontal.x / kDpr)),
			static_cast<int>(std::round(kHorizontal.y / kDpr)),
			std::max(1, static_cast<int>(std::round(kHorizontal.width / kDpr))),
			std::max(1, static_cast<int>(std::round(kHorizontal.height / kDpr))));

	vertical_splitter_handle_->show();
	horizontal_splitter_handle_->show();
	vertical_splitter_handle_->raise();
	horizontal_splitter_handle_->raise();
}

} // namespace quader::ui
