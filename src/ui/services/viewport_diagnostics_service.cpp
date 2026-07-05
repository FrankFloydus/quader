#include "ui/services/viewport_diagnostics_service.hpp"

#include <QStringList>

#include <utility>

namespace quader::ui {
namespace {

[[nodiscard]] bool is_warning_severity(const QString &severity) {
	return severity.compare(QStringLiteral("Warning"), Qt::CaseInsensitive) == 0;
}

[[nodiscard]] bool is_error_severity(const QString &severity) {
	return severity.compare(QStringLiteral("Error"), Qt::CaseInsensitive) == 0;
}

[[nodiscard]] QString issue_word(int count, QString singular) {
	return count == 1 ? std::move(singular) : singular + QStringLiteral("s");
}

[[nodiscard]] QString frame_size_text(const ViewportFrameStats &frame) {
	if (frame.width <= 0 || frame.height <= 0) {
		return QStringLiteral("unknown");
	}

	return QStringLiteral("%1x%2, %3 view%4, %5 fps")
			.arg(frame.width)
			.arg(frame.height)
			.arg(frame.viewport_count)
			.arg(frame.viewport_count == 1 ? QString() : QStringLiteral("s"))
			.arg(frame.fps, 0, 'f', 1);
}

[[nodiscard]] QString diagnostic_line(const ViewportRendererDiagnosticRow &row) {
	QString line = row.severity;
	if (!row.code.isEmpty()) {
		line += QStringLiteral(" ");
		line += row.code;
	}
	if (!row.message.isEmpty()) {
		line += QStringLiteral(": ");
		line += row.message;
	}
	return line;
}

} // namespace

ViewportDiagnosticsService::ViewportDiagnosticsService(QObject *parent) : QObject(parent) {
}

void ViewportDiagnosticsService::attach_render_host(IViewportRenderHost &render_host) noexcept {
	render_host_ = &render_host;
}

void ViewportDiagnosticsService::detach_render_host(const IViewportRenderHost &render_host) noexcept {
	if (render_host_ == &render_host) {
		clear_render_host();
	}
}

void ViewportDiagnosticsService::clear_render_host() noexcept {
	render_host_ = nullptr;
	clear_snapshot_and_notify(true);
}

void ViewportDiagnosticsService::refresh() {
	if (render_host_ == nullptr) {
		clear_snapshot_and_notify(false);
		return;
	}

	std::optional<ViewportDiagnosticsSnapshot> snapshot = render_host_->latest_diagnostics();
	if (!snapshot.has_value()) {
		clear_snapshot_and_notify(true);
		return;
	}

	latest_snapshot_ = std::move(snapshot);
	Q_EMIT diagnostics_changed();
	Q_EMIT summary_changed();
}

const std::optional<ViewportDiagnosticsSnapshot> &ViewportDiagnosticsService::latest_snapshot() const noexcept {
	return latest_snapshot_;
}

ViewportDiagnosticsSummary ViewportDiagnosticsService::summary() const {
	ViewportDiagnosticsSummary result;
	if (!latest_snapshot_.has_value()) {
		result.status_text = QStringLiteral("Diagnostics: unavailable");
		result.tooltip_text = QStringLiteral("No viewport diagnostics are available.");
		return result;
	}

	const ViewportDiagnosticsSnapshot &snapshot = *latest_snapshot_;
	result.available = true;
	result.renderer_name = snapshot.renderer_name;
	result.frame = snapshot.frame;
	result.pass_count = snapshot.passes.size();
	result.counter_count = snapshot.counters.size();
	result.diagnostic_count = snapshot.diagnostics.size();

	for (const ViewportRendererDiagnosticRow &row : snapshot.diagnostics) {
		if (is_warning_severity(row.severity)) {
			++result.warning_count;
		} else if (is_error_severity(row.severity)) {
			++result.error_count;
		}
	}

	if (result.warning_count == 0 && result.error_count == 0) {
		result.status_text = QStringLiteral("Diagnostics: OK");
	} else {
		result.status_text = QStringLiteral("Diagnostics: %1 %2, %3 %4")
									 .arg(result.warning_count)
									 .arg(issue_word(result.warning_count, QStringLiteral("warning")))
									 .arg(result.error_count)
									 .arg(issue_word(result.error_count, QStringLiteral("error")));
	}

	QStringList tooltip;
	tooltip.push_back(QStringLiteral("Renderer: %1").arg(result.renderer_name.isEmpty() ? QStringLiteral("unknown") : result.renderer_name));
	tooltip.push_back(QStringLiteral("Frame: %1").arg(frame_size_text(result.frame)));
	tooltip.push_back(QStringLiteral("Passes: %1").arg(result.pass_count));
	tooltip.push_back(QStringLiteral("Counters: %1").arg(result.counter_count));
	tooltip.push_back(QStringLiteral("Diagnostics: %1 (%2 warnings, %3 errors)")
					.arg(result.diagnostic_count)
					.arg(result.warning_count)
					.arg(result.error_count));

	int issue_lines = 0;
	for (const ViewportRendererDiagnosticRow &row : snapshot.diagnostics) {
		if (!is_warning_severity(row.severity) && !is_error_severity(row.severity)) {
			continue;
		}
		tooltip.push_back(diagnostic_line(row));
		if (++issue_lines == 3) {
			break;
		}
	}

	result.tooltip_text = tooltip.join(QStringLiteral("\n"));
	return result;
}

bool ViewportDiagnosticsService::has_provider() const noexcept {
	return render_host_ != nullptr;
}

void ViewportDiagnosticsService::clear_snapshot_and_notify(bool emit_unavailable) {
	const bool kHadSnapshot = latest_snapshot_.has_value();
	latest_snapshot_.reset();

	if (kHadSnapshot) {
		if (emit_unavailable) {
			Q_EMIT diagnostics_unavailable();
		}
		Q_EMIT summary_changed();
	} else if (emit_unavailable) {
		Q_EMIT diagnostics_unavailable();
	}
}

} // namespace quader::ui
