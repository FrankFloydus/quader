#include "ui/models/diagnostics_item_model.hpp"

#include "ui/models/item_model_roles.hpp"

#include <QMetaType>
#include <QStringList>
#include <QVariant>

#include <utility>

namespace quader::ui {
namespace {

constexpr int kDiagnosticsItemColumnCount = 3;

[[nodiscard]] bool is_name_column(int column) noexcept {
	return column == static_cast<int>(DiagnosticsItemColumn::Name);
}

[[nodiscard]] bool is_value_column(int column) noexcept {
	return column == static_cast<int>(DiagnosticsItemColumn::Value);
}

[[nodiscard]] bool is_detail_column(int column) noexcept {
	return column == static_cast<int>(DiagnosticsItemColumn::Detail);
}

[[nodiscard]] QString display_or_dash(const QString &value) {
	return value.isEmpty() ? QStringLiteral("-") : value;
}

[[nodiscard]] QString frame_size_value(const ViewportFrameStats &frame) {
	if (frame.width <= 0 || frame.height <= 0) {
		return QStringLiteral("-");
	}

	return QStringLiteral("%1x%2").arg(frame.width).arg(frame.height);
}

[[nodiscard]] QString fps_value(double fps) {
	return QStringLiteral("%1 fps").arg(fps, 0, 'f', 1);
}

[[nodiscard]] QString pass_detail(const ViewportRenderPassStats &pass) {
	return QStringLiteral("draw calls=%1; draw packets=%2; reads=%3; writes=%4; cpu=%5 us")
			.arg(pass.draw_call_count)
			.arg(pass.draw_packet_count)
			.arg(pass.resource_read_count)
			.arg(pass.resource_write_count)
			.arg(pass.cpu_time_us);
}

[[nodiscard]] QString diagnostic_copy_text(const ViewportRendererDiagnosticRow &row) {
	return QStringLiteral(
			"Severity: %1\nCode: %2\nSubsystem: %3\nResource: %4\nMessage: %5\nDetail: %6\nFrame: %7")
			.arg(display_or_dash(row.severity))
			.arg(display_or_dash(row.code))
			.arg(display_or_dash(row.subsystem))
			.arg(display_or_dash(row.resource_name))
			.arg(display_or_dash(row.message))
			.arg(display_or_dash(row.detail))
			.arg(row.frame_index);
}

} // namespace

DiagnosticsItemModel::DiagnosticsItemModel(ViewportDiagnosticsService &diagnostics, QObject *parent) : QAbstractItemModel(parent), diagnostics_(diagnostics) {
	register_ui_model_metatypes();
	rebuild_from_snapshot(diagnostics_.latest_snapshot());

	connect(&diagnostics_, &ViewportDiagnosticsService::diagnostics_changed, this, &DiagnosticsItemModel::reload_from_service);
	connect(
			&diagnostics_, &ViewportDiagnosticsService::diagnostics_unavailable, this, &DiagnosticsItemModel::reload_from_service);
}

QModelIndex DiagnosticsItemModel::index(int row, int column, const QModelIndex &parent_index) const {
	if (row < 0 || column < 0 || column >= columnCount()) {
		return {};
	}
	if (parent_index.isValid() && parent_index.model() != this) {
		return {};
	}

	const Node *parent_node = node_from_index(parent_index);
	const auto &siblings = parent_node == nullptr ? roots_ : parent_node->children;
	const auto kRowIndex = static_cast<std::size_t>(row);
	if (kRowIndex >= siblings.size()) {
		return {};
	}

	return createIndex(row, column, siblings[kRowIndex].get());
}

QModelIndex DiagnosticsItemModel::parent(const QModelIndex &child) const {
	const Node *node = node_from_index(child);
	if (node == nullptr || node->parent == nullptr) {
		return {};
	}

	const Node *parent_node = node->parent;
	if (parent_node->parent == nullptr) {
		return createIndex(parent_node->row, 0, const_cast<Node *>(parent_node));
	}

	return createIndex(parent_node->row, 0, const_cast<Node *>(parent_node));
}

int DiagnosticsItemModel::rowCount(const QModelIndex &parent_index) const {
	if (parent_index.isValid() && parent_index.model() != this) {
		return 0;
	}
	if (parent_index.isValid() && parent_index.column() != 0) {
		return 0;
	}

	const Node *parent_node = node_from_index(parent_index);
	return static_cast<int>((parent_node == nullptr ? roots_ : parent_node->children).size());
}

int DiagnosticsItemModel::columnCount(const QModelIndex &parent_index) const {
	(void)parent_index;
	return kDiagnosticsItemColumnCount;
}

QVariant DiagnosticsItemModel::data(const QModelIndex &model_index, int role) const {
	const Node *node = node_from_index(model_index);
	if (node == nullptr) {
		return {};
	}

	switch (role) {
		case Qt::DisplayRole:
			if (is_name_column(model_index.column())) {
				return display_or_dash(node->name);
			}
			if (is_value_column(model_index.column())) {
				return node->kind == DiagnosticsNodeKind::Section ? QVariant{} : display_or_dash(node->value);
			}
			if (is_detail_column(model_index.column())) {
				return node->kind == DiagnosticsNodeKind::Section ? QVariant{} : display_or_dash(node->detail);
			}
			return {};
		case Qt::ToolTipRole:
			return node->detail.isEmpty() ? copy_text_for_node(*node) : node->detail;
		case diagnostics_item_roles::kNodeKindRole:
			return QVariant::fromValue(node->kind);
		case diagnostics_item_roles::kSeverityRole:
			return node->severity;
		case diagnostics_item_roles::kCategoryRole:
			return node->category;
		case diagnostics_item_roles::kFrameIndexRole:
			return node->frame_index.has_value() ? QVariant::fromValue(*node->frame_index) : QVariant{};
		case diagnostics_item_roles::kCopyTextRole:
			return copy_text_for_node(*node);
		default:
			return {};
	}
}

Qt::ItemFlags DiagnosticsItemModel::flags(const QModelIndex &model_index) const {
	if (!model_index.isValid()) {
		return Qt::NoItemFlags;
	}

	return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant DiagnosticsItemModel::headerData(int section, Qt::Orientation orientation, int role) const {
	if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
		return {};
	}

	if (section == static_cast<int>(DiagnosticsItemColumn::Name)) {
		return QStringLiteral("Name");
	}
	if (section == static_cast<int>(DiagnosticsItemColumn::Value)) {
		return QStringLiteral("Value");
	}
	if (section == static_cast<int>(DiagnosticsItemColumn::Detail)) {
		return QStringLiteral("Detail");
	}

	return {};
}

QString DiagnosticsItemModel::copy_text_for_index(const QModelIndex &model_index) const {
	const Node *node = node_from_index(model_index);
	return node == nullptr ? QString() : copy_text_for_node(*node);
}

QString DiagnosticsItemModel::copy_text_for_all() const {
	QStringList lines;
	for (const auto &root : roots_) {
		append_copy_text_recursive(*root, lines);
	}
	return lines.join(QStringLiteral("\n"));
}

void DiagnosticsItemModel::reload_from_service() {
	beginResetModel();
	rebuild_from_snapshot(diagnostics_.latest_snapshot());
	endResetModel();
}

DiagnosticsItemModel::Node *DiagnosticsItemModel::node_from_index(const QModelIndex &model_index) const noexcept {
	if (!model_index.isValid()) {
		return nullptr;
	}
	if (model_index.model() != this) {
		return nullptr;
	}

	return static_cast<Node *>(model_index.internalPointer());
}

void DiagnosticsItemModel::rebuild_from_snapshot(const std::optional<ViewportDiagnosticsSnapshot> &snapshot) {
	roots_.clear();

	auto summary = make_node(DiagnosticsNodeKind::Section, QStringLiteral("Summary"));
	auto render_passes = make_node(DiagnosticsNodeKind::Section, QStringLiteral("Render Passes"));
	auto counters = make_node(DiagnosticsNodeKind::Section, QStringLiteral("Counters"));
	auto diagnostics = make_node(DiagnosticsNodeKind::Section, QStringLiteral("Diagnostics"));

	add_summary_rows(*summary, snapshot);

	if (snapshot.has_value()) {
		for (const ViewportRenderPassStats &pass : snapshot->passes) {
			auto node = make_node(
					DiagnosticsNodeKind::RenderPass,
					pass.pass_name,
					QStringLiteral("%1 calls / %2 packets").arg(pass.draw_call_count).arg(pass.draw_packet_count),
					pass_detail(pass));
			node->category = QStringLiteral("RenderPass");
			append_node(*render_passes, std::move(node));
		}

		for (const ViewportRendererCounter &counter : snapshot->counters) {
			auto node = make_node(
					DiagnosticsNodeKind::Counter,
					counter.name,
					counter.unit.isEmpty()
							? QString::number(counter.value)
							: QStringLiteral("%1 %2").arg(counter.value).arg(counter.unit),
					counter.domain);
			node->category = counter.domain;
			append_node(*counters, std::move(node));
		}

		for (const ViewportRendererDiagnosticRow &row : snapshot->diagnostics) {
			auto node = make_node(DiagnosticsNodeKind::Diagnostic, row.message, row.code, row.detail);
			node->severity = row.severity;
			node->category = row.subsystem;
			node->frame_index = row.frame_index;
			node->copy_text = diagnostic_copy_text(row);
			if (!row.resource_name.isEmpty()) {
				node->detail = row.detail.isEmpty()
						? QStringLiteral("resource=%1").arg(row.resource_name)
						: QStringLiteral("%1; resource=%2").arg(row.detail, row.resource_name);
			}
			append_node(*diagnostics, std::move(node));
		}
	}

	append_root(std::move(summary));
	append_root(std::move(render_passes));
	append_root(std::move(counters));
	append_root(std::move(diagnostics));
}

void DiagnosticsItemModel::add_summary_rows(
		Node &section,
		const std::optional<ViewportDiagnosticsSnapshot> &snapshot) {
	if (!snapshot.has_value()) {
		append_node(section,
				make_node(
						DiagnosticsNodeKind::SummaryField,
						QStringLiteral("Status"),
						QStringLiteral("Diagnostics unavailable"),
						QStringLiteral("No viewport diagnostics snapshot has been produced.")));
		return;
	}

	int warning_count = 0;
	int error_count = 0;
	for (const ViewportRendererDiagnosticRow &row : snapshot->diagnostics) {
		if (row.severity.compare(QStringLiteral("Warning"), Qt::CaseInsensitive) == 0) {
			++warning_count;
		} else if (row.severity.compare(QStringLiteral("Error"), Qt::CaseInsensitive) == 0) {
			++error_count;
		}
	}

	append_node(section, make_node(DiagnosticsNodeKind::SummaryField, QStringLiteral("Renderer"), snapshot->renderer_name));
	append_node(section, make_node(DiagnosticsNodeKind::SummaryField, QStringLiteral("Frame Size"), frame_size_value(snapshot->frame)));
	append_node(section,
			make_node(
					DiagnosticsNodeKind::SummaryField,
					QStringLiteral("Viewports"),
					QString::number(snapshot->frame.viewport_count)));
	append_node(section, make_node(DiagnosticsNodeKind::SummaryField, QStringLiteral("FPS"), fps_value(snapshot->frame.fps)));
	append_node(
			section, make_node(DiagnosticsNodeKind::SummaryField, QStringLiteral("Passes"), QString::number(snapshot->passes.size())));
	append_node(section,
			make_node(
					DiagnosticsNodeKind::SummaryField,
					QStringLiteral("Counters"),
					QString::number(snapshot->counters.size())));
	append_node(section,
			make_node(
					DiagnosticsNodeKind::SummaryField,
					QStringLiteral("Warnings"),
					QString::number(warning_count)));
	append_node(
			section, make_node(DiagnosticsNodeKind::SummaryField, QStringLiteral("Errors"), QString::number(error_count)));
}

std::unique_ptr<DiagnosticsItemModel::Node> DiagnosticsItemModel::make_node(
		DiagnosticsNodeKind kind,
		QString name,
		QString value,
		QString detail) {
	auto node = std::make_unique<Node>();
	node->kind = kind;
	node->name = std::move(name);
	node->value = std::move(value);
	node->detail = std::move(detail);
	return node;
}

void DiagnosticsItemModel::append_root(std::unique_ptr<Node> node) {
	node->parent = nullptr;
	node->row = static_cast<int>(roots_.size());
	roots_.push_back(std::move(node));
}

void DiagnosticsItemModel::append_node(Node &parent, std::unique_ptr<Node> child) {
	child->parent = &parent;
	child->row = static_cast<int>(parent.children.size());
	parent.children.push_back(std::move(child));
}

QString DiagnosticsItemModel::copy_text_for_node(const Node &node) const {
	if (!node.copy_text.isEmpty()) {
		return node.copy_text;
	}

	QStringList fields;
	fields.push_back(node.name);
	if (!node.value.isEmpty()) {
		fields.push_back(node.value);
	}
	if (!node.detail.isEmpty()) {
		fields.push_back(node.detail);
	}
	return fields.join(QStringLiteral("\t"));
}

void DiagnosticsItemModel::append_copy_text_recursive(const Node &node, QStringList &lines) const {
	lines.push_back(copy_text_for_node(node));
	for (const auto &child : node.children) {
		append_copy_text_recursive(*child, lines);
	}
}

} // namespace quader::ui
