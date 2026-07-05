#pragma once

#include "ui/services/viewport_diagnostics_service.hpp"

#include <QAbstractItemModel>
#include <QMetaType>
#include <QModelIndex>
#include <QObject>
#include <QStringList>
#include <Qt>

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

namespace quader::ui {

enum class DiagnosticsNodeKind : std::uint8_t {
    Section,
    SummaryField,
    RenderPass,
    Counter,
    Diagnostic,
};

enum class DiagnosticsItemColumn : int {
    Name = 0,
    Value = 1,
    Detail = 2,
};

namespace DiagnosticsItemRoles {
constexpr int NodeKindRole = Qt::UserRole + 40;
constexpr int SeverityRole = Qt::UserRole + 41;
constexpr int CategoryRole = Qt::UserRole + 42;
constexpr int FrameIndexRole = Qt::UserRole + 43;
constexpr int CopyTextRole = Qt::UserRole + 44;
} // namespace DiagnosticsItemRoles

class DiagnosticsItemModel final : public QAbstractItemModel {
    Q_OBJECT

public:
    explicit DiagnosticsItemModel(ViewportDiagnosticsService& diagnostics, QObject* parent = nullptr);

    [[nodiscard]] QModelIndex index(int row, int column, const QModelIndex& parent = {}) const override;
    [[nodiscard]] QModelIndex parent(const QModelIndex& child) const override;
    [[nodiscard]] int rowCount(const QModelIndex& parent = {}) const override;
    [[nodiscard]] int columnCount(const QModelIndex& parent = {}) const override;
    [[nodiscard]] QVariant data(const QModelIndex& index, int role) const override;
    [[nodiscard]] Qt::ItemFlags flags(const QModelIndex& index) const override;
    [[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    [[nodiscard]] QString copy_text_for_index(const QModelIndex& index) const;
    [[nodiscard]] QString copy_text_for_all() const;

public Q_SLOTS:
    void reload_from_service();

private:
    struct Node {
        DiagnosticsNodeKind kind = DiagnosticsNodeKind::Section;
        QString name;
        QString value;
        QString detail;
        QString severity;
        QString category;
        std::optional<quint64> frame_index;
        QString copy_text;
        Node* parent = nullptr;
        int row = -1;
        std::vector<std::unique_ptr<Node>> children;
    };

    [[nodiscard]] Node* node_from_index(const QModelIndex& index) const noexcept;
    void rebuild_from_snapshot(const std::optional<ViewportDiagnosticsSnapshot>& snapshot);
    void add_summary_rows(Node& section, const std::optional<ViewportDiagnosticsSnapshot>& snapshot);
    [[nodiscard]] static std::unique_ptr<Node> make_node(
        DiagnosticsNodeKind kind,
        QString name,
        QString value = {},
        QString detail = {});
    void append_root(std::unique_ptr<Node> node);
    void append_node(Node& parent, std::unique_ptr<Node> child);
    [[nodiscard]] QString copy_text_for_node(const Node& node) const;
    void append_copy_text_recursive(const Node& node, QStringList& lines) const;

    ViewportDiagnosticsService& diagnostics_;
    std::vector<std::unique_ptr<Node>> roots_;
};

} // namespace quader::ui

Q_DECLARE_METATYPE(quader::ui::DiagnosticsNodeKind)
