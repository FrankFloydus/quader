# Task #8 UI Models, Panels, Workspace Implementation Plan

Plan type: focused UI architecture implementation plan for project-board task `#8`.

Owning architect: UI architect.

Output path: `agents/plans/implementation_task8_ui_models_panels_ui_doc.md`

Active authority:

- `agents/plans/audit_20260704_full_architecture_master.md`, Batch 7.
- `agents/architecture/ui.md`.
- `agents/architecture/architecture.md`.
- `agents/tests-policy.md`.

Prior UI preflight:

- `agents/plans/implementation_wave1_a4_ui_preflight_ui_doc.md`.

This plan is implementation guidance only. It does not implement production code and does not authorize edits outside the task #8 scope. Return to the UI architect after implementation with changed files, build/test output, and deviations. If approved, the main/root agent should move this plan from `agents/plans/` to `agents/archive/`; do not delete it automatically.

## 1. Scope

Task #8 replaces the prototype inspector dock with document-backed UI models, panels, workspace persistence, and tests.

Implement only:

- UI-owned document interaction service that adapts the approved #7 `Document`, `CommandHistory`, and command APIs into Qt-facing notifications.
- `DocumentItemModel` over current document mesh objects.
- `PropertyItemModel` over selected object descriptors.
- Named model roles and model-local UI node IDs.
- Property descriptors for current #7 object name, kind, counts, and transform fields.
- Selection adapter between `QItemSelectionModel` and document selection.
- `ScenePanel` and `PropertiesPanel` dock widgets.
- Workspace save/restore/reset through `SettingsService`, `PanelHost`, and stable panel object names.
- Canonical panel visibility actions through `ActionRegistry`.
- Focused UI model, selection-sync, panel, workspace, and settings tests, including `QAbstractItemModelTester`.

Do not implement:

- Renderer or Crimson #9 work.
- Viewport picking or picking-driven selection.
- Import/export or file dialogs.
- Topology tools, component property editing, or mesh operations.
- Material editing.
- Visibility/locked document state unless software APIs are added by a separate approved task.
- A custom style, broad stylesheet, plugin panel system, universal reflection system, or dynamic property framework.

## 2. Current #7 API Findings

Approved #7 APIs are enough for a focused first document-backed UI:

- `quader::document::Document`
  - `object_ids()`, `find_mesh_object(ObjectId)`, `object_count()`.
  - `selection()`, `set_selection(Selection)`, `clear_selection()`.
  - `is_dirty()`, `dirty_flags()`, `take_pending_changes()`.
  - typed `DocumentChangeKind` values for object creation/deletion/rename, transform, mesh changes, selection, and dirty state.
- `quader::commands::CommandHistory`
  - `execute(std::unique_ptr<ICommand>, Document&)`, `undo`, `redo`, state/name queries.
- Current commands
  - `RenameObjectCommand`.
  - `SetObjectTransformCommand`.
  - `DeleteObjectCommand`.
  - `SetSelectionCommand`.
  - `ClearSelectionCommand`.
  - `CreateMeshObjectCommand`, but no UI create action is in scope here.
- Existing UI action wiring
  - `EditorActionStateProvider`.
  - `ActionStateUpdater`.
  - `EditorActionController`.

Blockers and limits from #7:

- There is no QObject document event source. `Document` exposes pull-style `take_pending_changes()`, so task #8 must add a UI-side event bridge/service.
- There is no visibility, locked, hierarchy, layer, material, object type beyond mesh, or duplicate command API. Do not expose those as editable document properties.
- There are no multi-object transform/name commands. Multi-selection property rows must be read-only summary rows for task #8.
- There are no component property edit commands. Component selection can be summarized read-only only.
- There is no document ordering/move event. `DocumentItemModel` can use precise `dataChanged` for object rename/transform/selection, but object create/delete may use a full model reset in this first slice.
- There is no command merge implementation beyond default hooks. Numeric property editors must commit on edit finish, not on every intermediate keystroke or slider tick.
- `QAbstractItemModelTester` is not currently available in the build. Add a test-only `Qt6::Test` dependency.

None of these are hard blockers for task #8 if the implementation stays within the supported object-list, selection, name, and transform surface.

## 3. Architecture Decisions

### 3.1 UI Does Not Own Document Truth

All models and panels are presentation adapters. They store:

- stable document IDs, such as `quader::document::ObjectId`;
- model-local `UiNodeId` values;
- descriptor rows derived from document state.

They must not store mutable `MeshObject*`, mesh record pointers, or `QModelIndex` values as application truth.

### 3.2 One UI Command Path

Document-backed edits use:

```text
delegate/view commit
  -> model setData()
  -> UI validates QVariant and descriptor/path
  -> DocumentUiController::execute_command(...)
  -> CommandHistory executes ICommand
  -> Document mutates and queues DocumentChange entries
  -> DocumentUiController drains changes and emits Qt signals
  -> models/panels refresh from Document
  -> ActionStateUpdater refreshes undo/redo/delete state
```

Widgets and models must not call `Document::rename_object`, `Document::set_transform`, or `Document::set_selection` directly.

### 3.3 Pull Document Events Need A UI Bridge

Add a small UI service named `DocumentUiController`. It is a QObject adapter, not domain truth. It centralizes command execution, document event draining, notification of rejected commands, and action-state refreshes.

Existing `EditorActionController` should be refactored to use `DocumentUiController` for undo/redo/delete selection. This avoids a split where menu actions mutate the document without notifying task #8 models.

### 3.4 Workspace Versioning

Adding and replacing dock panels changes the saved `QMainWindow` state contract. Bump the current workspace state version from `1` to `2` and use a versioned workspace root for new panel visibility keys:

```text
ui/workspace/v2/main_window/geometry
ui/workspace/v2/main_window/state
ui/workspace/v2/main_window/state_version
ui/workspace/v2/panels/scene/visible
ui/workspace/v2/panels/properties/visible
ui/workspace/v2/viewport/layout_mode
ui/workspace/v2/viewport/quad_split_vertical
ui/workspace/v2/viewport/quad_split_horizontal
```

Do not persist document contents, document selection, meshes, command history, renderer resources, or tool preview state in `SettingsService`.

## 4. New And Changed Files

### 4.1 New UI Services

Add:

```text
src/ui/services/document_ui_controller.hpp
src/ui/services/document_ui_controller.cpp
```

Purpose: app-owned Qt-facing document command and event bridge.

Public API:

```cpp
namespace quader::ui {

enum class CommandFeedback {
    Silent,
    NotifyOnReject,
};

class DocumentUiController final : public QObject {
    Q_OBJECT

public:
    DocumentUiController(quader::document::Document& document,
                         quader::commands::CommandHistory& command_history,
                         ActionStateUpdater& action_state_updater,
                         NotificationService& notifications,
                         QObject* parent = nullptr);

    [[nodiscard]] const quader::document::Document& document() const noexcept;
    [[nodiscard]] const quader::commands::CommandHistory& command_history() const noexcept;

    [[nodiscard]] quader::commands::CommandResult execute_command(
        std::unique_ptr<quader::commands::ICommand> command,
        CommandFeedback feedback = CommandFeedback::NotifyOnReject);
    [[nodiscard]] quader::commands::CommandResult undo(CommandFeedback feedback = CommandFeedback::NotifyOnReject);
    [[nodiscard]] quader::commands::CommandResult redo(CommandFeedback feedback = CommandFeedback::NotifyOnReject);

    // Use after app-owned code mutates the document through an existing approved path.
    void refresh_from_document();

Q_SIGNALS:
    void object_list_changed();
    void object_changed(quader::document::ObjectId object);
    void selection_changed();
    void dirty_state_changed(quader::document::DocumentDirtyFlags flags);
    void command_history_changed();
    void document_events_drained();

private:
    void drain_pending_changes();
    void emit_change(const quader::document::DocumentChange& change);
    void report_rejected_command(const quader::commands::CommandResult& result);
    void refresh_actions();

    quader::document::Document& document_;
    quader::commands::CommandHistory& command_history_;
    ActionStateUpdater& action_state_updater_;
    NotificationService& notifications_;
};

} // namespace quader::ui
```

Signal mapping:

- `ObjectCreated` and `ObjectDeleted` -> `object_list_changed()`.
- `ObjectRenamed`, `TransformChanged`, `MeshTopologyChanged`, `MeshGeometryChanged` -> `object_changed(object)`.
- `SelectionChanged` -> `selection_changed()`.
- `DirtyStateChanged` -> `dirty_state_changed(flags)`.
- every applied execute/undo/redo -> `command_history_changed()` after event draining.
- after all drained changes -> `document_events_drained()`.

Ownership:

- `AppServices` owns `DocumentUiController` as a value member.
- It holds non-owning references to app-owned `Document`, `CommandHistory`, `ActionStateUpdater`, and `NotificationService`.
- It is destroyed after `MainWindow` because `AppServices` outlives `MainWindow`.

Rules:

- This service does not expose non-const `Document&` to panels or models.
- It does not invent domain errors. It forwards command diagnostics to `NotificationService`.
- It stays in `src/ui/services`, not `modeling_document` or `modeling_commands`.

### 4.2 UiContext And AppServices

Change:

```text
src/ui/qt_app/ui_context.hpp
src/app/app_services.hpp
src/app/application.cpp
src/ui/actions/editor_action_controller.hpp
src/ui/actions/editor_action_controller.cpp
```

Add `DocumentUiController& documents;` to `UiContext`:

```cpp
struct UiContext {
    ActionRegistry& actions;
    ActionStateUpdater& action_state_updater;
    IEditorStateProvider& editor_state;
    SettingsService& settings;
    NotificationService& notifications;
    DocumentUiController& documents;
};
```

`AppServices` member order:

```cpp
QSettings settings_store;
document::Document document;
commands::CommandHistory command_history;
tools::ToolManager tool_manager;
ui::ActionRegistry actions;
ui::EditorActionStateProvider editor_state;
ui::ActionStateUpdater action_state_updater;
ui::SettingsService settings;
ui::NotificationService notifications;
ui::DocumentUiController document_ui;
ui::EditorActionController editor_actions;
ui::UiContext ui_context;
```

The constructor should:

1. register standard actions;
2. register shell tools;
3. construct `DocumentUiController` before `EditorActionController`;
4. construct `EditorActionController` with `DocumentUiController&`, not raw `Document&` plus `CommandHistory&`;
5. refresh action state after tools/actions are registered.

`EditorActionController` should keep tool activation behavior, but undo/redo/delete must call `document_ui_.undo()`, `document_ui_.redo()`, and `document_ui_.execute_command(std::make_unique<DeleteObjectCommand>(...))`.

### 4.3 New Model Role And Descriptor Files

Add:

```text
src/ui/models/item_model_roles.hpp
src/ui/models/item_model_roles.cpp
src/ui/models/property_descriptor.hpp
src/ui/models/property_descriptor.cpp
```

`item_model_roles.hpp`:

```cpp
namespace quader::ui {

struct UiNodeId {
    std::uint64_t value = 0U;
    [[nodiscard]] bool is_valid() const noexcept { return value != 0U; }
    friend bool operator==(UiNodeId, UiNodeId) = default;
};

enum class DocumentItemKind : std::uint8_t {
    MeshObject,
};

enum class DocumentItemColumn : int {
    Name = 0,
    Kind = 1,
};

namespace DocumentItemRoles {
constexpr int UiNodeIdRole = Qt::UserRole + 1;
constexpr int ObjectIdRole = Qt::UserRole + 2;
constexpr int KindRole = Qt::UserRole + 3;
constexpr int SelectedRole = Qt::UserRole + 4;
}

void register_ui_model_metatypes();

} // namespace quader::ui
```

Declare metatypes for values used in `QVariant` roles:

```cpp
Q_DECLARE_METATYPE(quader::document::ObjectId)
Q_DECLARE_METATYPE(quader::ui::UiNodeId)
Q_DECLARE_METATYPE(quader::ui::DocumentItemKind)
```

`property_descriptor.hpp`:

```cpp
namespace quader::ui {

enum class PropertyValueKind : std::uint8_t {
    String,
    Float,
    Int,
    Vector3,
    ReadOnlyText,
};

enum class PropertyField : std::uint8_t {
    ObjectName,
    ObjectKind,
    VertexCount,
    EdgeCount,
    FaceCount,
    TranslationX,
    TranslationY,
    TranslationZ,
    RotationX,
    RotationY,
    RotationZ,
    ScaleX,
    ScaleY,
    ScaleZ,
    SelectionSummary,
};

struct PropertyPath {
    quader::document::ObjectId object;
    PropertyField field = PropertyField::SelectionSummary;
    friend bool operator==(const PropertyPath&, const PropertyPath&) = default;
};

struct PropertyDescriptor {
    PropertyPath path;
    QString label;
    PropertyValueKind kind = PropertyValueKind::ReadOnlyText;
    QVariant value;
    QVariant minimum;
    QVariant maximum;
    int decimals = 3;
    bool editable = false;
    QString tooltip;
};

namespace PropertyItemRoles {
constexpr int PropertyPathRole = Qt::UserRole + 1;
constexpr int ValueKindRole = Qt::UserRole + 2;
constexpr int EditableRole = Qt::UserRole + 3;
constexpr int DescriptorRole = Qt::UserRole + 4;
}

std::vector<PropertyDescriptor> build_property_descriptors(
    const quader::document::Document& document);

} // namespace quader::ui
```

Declare metatypes:

```cpp
Q_DECLARE_METATYPE(quader::ui::PropertyPath)
Q_DECLARE_METATYPE(quader::ui::PropertyValueKind)
Q_DECLARE_METATYPE(quader::ui::PropertyDescriptor)
```

Descriptor builder rules:

- No selection: return an empty descriptor vector. The panel may show a UI-only placeholder label.
- One selected mesh object: return editable descriptors for object name and transform components, plus read-only kind and mesh counts.
- Multiple selected objects: return read-only summary descriptors only.
- Component selection: return read-only summary descriptors only.
- Unknown or stale selected object: return read-only summary or empty descriptors and rely on selection sync to clear stale view selection.
- Do not expose visibility, locked, material, hierarchy, file path, or renderer fields in task #8.

### 4.4 DocumentItemModel

Add:

```text
src/ui/models/document_item_model.hpp
src/ui/models/document_item_model.cpp
```

Class:

```cpp
namespace quader::ui {

class DocumentItemModel final : public QAbstractItemModel {
    Q_OBJECT

public:
    explicit DocumentItemModel(DocumentUiController& documents, QObject* parent = nullptr);

    [[nodiscard]] QModelIndex index(int row, int column, const QModelIndex& parent = {}) const override;
    [[nodiscard]] QModelIndex parent(const QModelIndex& child) const override;
    [[nodiscard]] int rowCount(const QModelIndex& parent = {}) const override;
    [[nodiscard]] int columnCount(const QModelIndex& parent = {}) const override;
    [[nodiscard]] QVariant data(const QModelIndex& index, int role) const override;
    [[nodiscard]] bool setData(const QModelIndex& index, const QVariant& value, int role) override;
    [[nodiscard]] Qt::ItemFlags flags(const QModelIndex& index) const override;
    [[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    [[nodiscard]] QModelIndex index_for_object(quader::document::ObjectId object,
                                               int column = static_cast<int>(DocumentItemColumn::Name)) const;

public Q_SLOTS:
    void rebuild_from_document();
    void refresh_object(quader::document::ObjectId object);
    void refresh_selection();

private:
    struct Node {
        UiNodeId ui_node_id;
        quader::document::ObjectId object;
        int row = -1;
    };

    [[nodiscard]] Node* node_from_index(const QModelIndex& index) const noexcept;
    [[nodiscard]] const quader::document::MeshObject* object_for_node(const Node& node) const noexcept;
    [[nodiscard]] int row_for_object(quader::document::ObjectId object) const noexcept;

    DocumentUiController& documents_;
    std::vector<std::unique_ptr<Node>> roots_;
    std::uint64_t next_ui_node_id_ = 1U;
};

} // namespace quader::ui
```

Model contract:

- Flat V1 tree: one root row per `MeshObject`.
- `parent()` returns invalid for root rows.
- `internalPointer()` points only to model-owned `Node`.
- Nodes store `ObjectId`, not `MeshObject*`.
- Column `Name` displays and edits `MeshObject::name`.
- Column `Kind` displays `"Mesh"`, read-only.
- `ObjectIdRole` returns `QVariant::fromValue(object_id)`.
- `KindRole` returns `DocumentItemKind::MeshObject`.
- `SelectedRole` returns whether `Document::selection().references_object(object_id)`.
- `flags()` marks only `Name` column rows editable.
- `setData()` for `Name/EditRole` trims no hidden document behavior; it validates a non-empty changed string and dispatches `RenameObjectCommand` through `DocumentUiController`.
- Rejected commands return `false`; accepted commands return `true`.

Refresh policy:

- Connect `DocumentUiController::object_list_changed` to `rebuild_from_document()`.
- Connect `DocumentUiController::object_changed` to `refresh_object(object)`.
- Connect `DocumentUiController::selection_changed` to `refresh_selection()`.
- `rebuild_from_document()` may use `beginResetModel()` and `endResetModel()` for task #8 because the current document API has no move/order deltas.
- `refresh_object()` emits `dataChanged()` for the row when it still exists.
- `refresh_selection()` emits `dataChanged()` for the `SelectedRole` across all rows.

### 4.5 PropertyItemModel

Add:

```text
src/ui/models/property_item_model.hpp
src/ui/models/property_item_model.cpp
```

Class:

```cpp
namespace quader::ui {

enum class PropertyItemColumn : int {
    Label = 0,
    Value = 1,
};

class PropertyItemModel final : public QAbstractTableModel {
    Q_OBJECT

public:
    explicit PropertyItemModel(DocumentUiController& documents, QObject* parent = nullptr);

    [[nodiscard]] int rowCount(const QModelIndex& parent = {}) const override;
    [[nodiscard]] int columnCount(const QModelIndex& parent = {}) const override;
    [[nodiscard]] QVariant data(const QModelIndex& index, int role) const override;
    [[nodiscard]] bool setData(const QModelIndex& index, const QVariant& value, int role) override;
    [[nodiscard]] Qt::ItemFlags flags(const QModelIndex& index) const override;
    [[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

public Q_SLOTS:
    void rebuild_from_document();
    void refresh_object(quader::document::ObjectId object);

private:
    [[nodiscard]] const PropertyDescriptor* descriptor_for_row(int row) const noexcept;
    [[nodiscard]] bool apply_descriptor_edit(const PropertyDescriptor& descriptor, const QVariant& value);

    DocumentUiController& documents_;
    std::vector<PropertyDescriptor> descriptors_;
};

} // namespace quader::ui
```

Model contract:

- Rows are descriptors derived from current document selection.
- Columns are `Label` and `Value`.
- The model owns descriptor rows as presentation data only.
- `PropertyPathRole`, `ValueKindRole`, `EditableRole`, and `DescriptorRole` are documented named roles.
- `setData()` only accepts `Value` column and `Qt::EditRole`.
- Editable descriptor fields map to current commands:
  - `ObjectName` -> `RenameObjectCommand`.
  - transform component fields -> `SetObjectTransformCommand`.
- Read-only rows return `false` from `setData()`.
- Invalid, non-finite, wrong-type, or unchanged values return `false` and do not execute a command.
- After an accepted command, the model waits for document events to rebuild descriptors. It must not directly mutate `descriptors_` as document truth.

Refresh policy:

- Connect `DocumentUiController::selection_changed` to `rebuild_from_document()`.
- Connect `DocumentUiController::object_changed` to `refresh_object(object)`.
- `refresh_object()` can rebuild all descriptors if the selected object matches the changed object. Use model reset because descriptor order and rows can change as selection changes.
- Connect `DocumentUiController::object_list_changed` to `rebuild_from_document()` to handle selected-object deletion.

### 4.6 Property Delegate

Add:

```text
src/ui/delegates/property_value_delegate.hpp
src/ui/delegates/property_value_delegate.cpp
```

Class:

```cpp
namespace quader::ui {

class PropertyValueDelegate final : public QStyledItemDelegate {
    Q_OBJECT

public:
    explicit PropertyValueDelegate(QObject* parent = nullptr);

    [[nodiscard]] QWidget* createEditor(QWidget* parent,
                                        const QStyleOptionViewItem& option,
                                        const QModelIndex& index) const override;
    void setEditorData(QWidget* editor, const QModelIndex& index) const override;
    void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;
    [[nodiscard]] QString displayText(const QVariant& value, const QLocale& locale) const override;
};

} // namespace quader::ui
```

Delegate rules:

- String rows use `QLineEdit`.
- Float rows use `QDoubleSpinBox` with descriptor min/max/decimals when present.
- Read-only rows create no editor.
- The delegate only formats, validates local editor values, and commits to the model.
- It must not include document or command headers and must not dispatch commands directly.
- Use commit-on-finish behavior. Do not introduce sliders or continuous numeric commits in task #8.

### 4.7 Selection Adapter

Add:

```text
src/ui/models/document_selection_adapter.hpp
src/ui/models/document_selection_adapter.cpp
```

Class:

```cpp
namespace quader::ui {

class DocumentSelectionAdapter final : public QObject {
    Q_OBJECT

public:
    DocumentSelectionAdapter(DocumentItemModel& model,
                             QItemSelectionModel& selection_model,
                             DocumentUiController& documents,
                             QObject* parent = nullptr);

public Q_SLOTS:
    void sync_from_document();

private:
    void on_view_selection_changed(const QItemSelection& selected,
                                   const QItemSelection& deselected);
    [[nodiscard]] std::vector<quader::document::ObjectId> selected_object_ids_from_view() const;

    DocumentItemModel& model_;
    QItemSelectionModel& selection_model_;
    DocumentUiController& documents_;
    bool syncing_ = false;
};

} // namespace quader::ui
```

Selection flow:

```text
User changes ScenePanel tree selection
  -> DocumentSelectionAdapter reads ObjectIdRole from selected rows
  -> builds document::Selection with objects
  -> executes SetSelectionCommand through DocumentUiController
  -> Document emits SelectionChanged into pending changes
  -> DocumentUiController drains and emits selection_changed()
  -> adapter sync_from_document() updates QItemSelectionModel with re-entry guard
  -> PropertyItemModel rebuilds descriptors
```

Rules:

- Document selection is source of truth.
- View selection is projection.
- Use `QSignalBlocker` and `syncing_` to prevent loops.
- Only object rows are selectable in task #8.
- Component selection is not driven by this adapter because viewport picking is out of scope.
- Do not let `ScenePanel` call `PropertiesPanel`.

### 4.8 Panel Classes

Add:

```text
src/ui/panels/scene_panel.hpp
src/ui/panels/scene_panel.cpp
src/ui/panels/properties_panel.hpp
src/ui/panels/properties_panel.cpp
```

`ScenePanel`:

```cpp
namespace quader::ui {

class ScenePanel final : public QWidget {
    Q_OBJECT

public:
    explicit ScenePanel(PanelContext context, QWidget* parent = nullptr);

    [[nodiscard]] DocumentItemModel& model() noexcept;
    [[nodiscard]] QTreeView& tree_view() noexcept;

private:
    PanelContext context_;
    DocumentItemModel* model_ = nullptr;
    QTreeView* tree_view_ = nullptr;
    DocumentSelectionAdapter* selection_adapter_ = nullptr;
};

} // namespace quader::ui
```

Scene panel UI:

- `QTreeView` with `DocumentItemModel`.
- No `QTreeWidget`.
- `setRootIsDecorated(false)` for the flat V1 object list.
- `setSelectionMode(QAbstractItemView::ExtendedSelection)` because `Selection::set_objects` supports multi-object object selection.
- Edit triggers may allow selected edit or F2 for the Name column.
- Object name editing is command-routed through the model.
- The panel owns only widgets, the model, the selection adapter, and view UI state.

`PropertiesPanel`:

```cpp
namespace quader::ui {

class PropertiesPanel final : public QWidget {
    Q_OBJECT

public:
    explicit PropertiesPanel(PanelContext context, QWidget* parent = nullptr);

    [[nodiscard]] PropertyItemModel& model() noexcept;
    [[nodiscard]] QTableView& table_view() noexcept;

private:
    PanelContext context_;
    PropertyItemModel* model_ = nullptr;
    QTableView* table_view_ = nullptr;
    PropertyValueDelegate* delegate_ = nullptr;
    QLabel* empty_label_ = nullptr;
};

} // namespace quader::ui
```

Properties panel UI:

- `QTableView` with `PropertyItemModel`.
- `PropertyValueDelegate` installed for the Value column.
- No custom-painted controls or broad stylesheets.
- A UI-only placeholder label may say no selection; it is not document state.
- The panel must not know about `ScenePanel`.
- All document-backed edits route through the model and `DocumentUiController`.

Prototype inspector replacement:

- Stop creating `PrototypeInspectorPanel` in `MainWindow`.
- Remove it from `modeling_ui` sources if no code still uses it.
- Keep `PanelId::Inspector` only if needed for compatibility, but do not create a live inspector dock.
- Current prototype cube animation remains renderer/viewport default behavior. Do not add or change viewport controls in task #8.

### 4.9 PanelHost, SettingsService, And MainWindow

Change:

```text
src/ui/panels/panel_host.hpp
src/ui/panels/panel_host.cpp
src/ui/services/settings_service.hpp
src/ui/services/settings_service.cpp
src/ui/qt_app/main_window.hpp
src/ui/qt_app/main_window.cpp
src/ui/actions/action_id.hpp
src/ui/actions/action_id.cpp
src/ui/actions/action_registry.cpp
src/ui/actions/action_state_updater.cpp
```

Panel IDs:

- Existing `PanelId::Scene` -> object name `quader.panel.scene`.
- Existing `PanelId::Properties` -> object name `quader.panel.properties`.
- Do not create `PanelId::Diagnostics` in task #8.

Add action IDs:

```cpp
ActionId::ShowScenePanel
ActionId::ShowPropertiesPanel
```

Action specs:

- Text: `"Scene Panel"` and `"Properties Panel"`.
- Checkable: `true`.
- No default shortcut.
- Add to `View -> Panels` submenu.
- `ActionStateUpdater` does not need to derive these from `EditorStateSnapshot`; panel visibility is workspace UI state. `PanelHost` or `MainWindow` updates checked state using `QSignalBlocker`.

`PanelHost` public API additions:

```cpp
void bind_panel_visibility_action(PanelId panel, ActionRegistry& actions, ActionId action);
void save_workspace(SettingsService& settings) const;
void restore_workspace(SettingsService& settings);
void reset_to_default_layout();
```

PanelHost behavior:

- `add_panel()` remains the only dock creation path.
- Every dock gets stable object name from `panel_object_name(panel_id)`.
- `bind_panel_visibility_action()` connects action toggles to `show_panel`/`hide_panel` and dock `visibilityChanged` back to checked state with `QSignalBlocker`.
- `save_workspace()` writes per-panel visibility keys.
- `restore_workspace()` applies per-panel visibility after `QMainWindow::restoreState()`.
- `reset_to_default_layout()` places:
  - `Scene` dock in `Qt::RightDockWidgetArea`.
  - `Properties` dock in `Qt::RightDockWidgetArea`, tabified below or split below Scene using standard `QMainWindow` APIs.
  - both visible and raised deterministically.
- Do not make panels save the whole workspace themselves.

`SettingsService` additions:

```cpp
[[nodiscard]] QVariant workspace_value(QStringView key, const QVariant& fallback = {}) const;
void set_workspace_value(QStringView key, const QVariant& value);

[[nodiscard]] bool panel_visible(PanelId id, bool fallback) const;
void set_panel_visible(PanelId id, bool visible);

void reset_workspace();
```

Keep or remove `reset_workspace_v1()` at implementer discretion, but if kept it should call the new `reset_workspace()` only when compatible. Tests should target the current API name after implementation.

`MainWindow` changes:

- `build_dock_hosts()` creates `ScenePanel` and `PropertiesPanel` through `PanelHost`.
- `build_menus()` adds a View/Panels submenu with visibility actions.
- `restore_workspace()`:
  1. restore geometry from `SettingsService`;
  2. attempt `restoreState(state, SettingsService::kWorkspaceStateVersion)`;
  3. if restore failed or no state exists, call `PanelHost::reset_to_default_layout()`;
  4. call `PanelHost::restore_workspace(settings)`.
- `save_workspace()`:
  1. save geometry;
  2. save main window state with workspace version;
  3. call `PanelHost::save_workspace(settings)`;
  4. `settings.sync()`.
- `reset_workspace()`:
  1. `settings.reset_workspace()`;
  2. `PanelHost::reset_to_default_layout()`;
  3. keep existing viewport layout reset behavior unchanged;
  4. resize to default;
  5. notify status.

## 5. CMake Changes

Change `CMakeLists.txt`:

- `find_package(Qt6 REQUIRED COMPONENTS Widgets Test)`.
- Add new source/header files to `modeling_ui`.
- Link `modeling_ui` explicitly to `modeling_document` and `modeling_commands` in `PRIVATE` dependencies, in addition to current UI dependencies.
- Add `tests/unit/ui/ui_model_tests.cpp` target linked to:
  - `modeling_ui`
  - `modeling_commands`
  - `modeling_document`
  - `Qt6::Widgets`
  - `Qt6::Test`
- Keep `AUTOMOC` enabled on `modeling_ui`.
- Do not add Qt dependencies to `modeling_document`, `modeling_commands`, or `modeling_tools`.

Expected target:

```cmake
add_executable(ui_model_tests
    tests/unit/document/document_test_helpers.hpp
    tests/unit/ui/ui_model_tests.cpp
)

target_link_libraries(ui_model_tests
    PRIVATE
        modeling_commands
        modeling_document
        modeling_ui
        Qt6::Widgets
        Qt6::Test
)

add_test(
    NAME ui_document_models_panels
    COMMAND ui_model_tests
)
```

## 6. Tests

Follow `agents/tests-policy.md`: test real behavior and architecture boundaries, not layout cosmetics.

### 6.1 UI Model Tests

Add `tests/unit/ui/ui_model_tests.cpp`.

Required fixture:

- `QApplication`.
- temporary INI-backed `QSettings`.
- `Document`.
- `CommandHistory`.
- `ToolManager` if needed by existing `EditorActionStateProvider`.
- `ActionRegistry`, `EditorActionStateProvider`, `ActionStateUpdater`, `SettingsService`, `NotificationService`, `DocumentUiController`, and `UiContext`.
- use `tests/unit/document/document_test_helpers.hpp` for triangle mesh objects.

Required `DocumentItemModel` tests:

- Construct with `QAbstractItemModelTester` in failure-reporting mode.
- Empty document has zero rows and two columns.
- One mesh object exposes:
  - display name,
  - kind text,
  - `ObjectIdRole`,
  - `KindRole`,
  - `SelectedRole == false`.
- Flags mark only name column editable.
- `setData(Name, "Renamed", EditRole)`:
  - returns true,
  - changes document through `RenameObjectCommand`,
  - pushes command history undo entry,
  - updates model data after document event draining,
  - undo through `DocumentUiController` restores the old name and model data.
- No-op rename returns false and does not push history.
- Object deletion through `DocumentUiController` causes model row reset and no stale selected row.

Required `PropertyItemModel` tests:

- Construct with `QAbstractItemModelTester`.
- No selection returns zero editable property rows.
- Single selected object produces descriptors for name, kind, counts, and transform components.
- Name descriptor is editable and dispatches `RenameObjectCommand`.
- Translation X descriptor edits dispatch `SetObjectTransformCommand` and preserves other transform fields.
- Read-only descriptors reject `setData()`.
- Non-finite float values reject and do not mutate the document.
- Multi-object selection produces read-only summary descriptors only.

Required selection adapter tests:

- Selecting a `ScenePanel`/`DocumentItemModel` row updates `Document::selection()` through `SetSelectionCommand`.
- Undoing the selection command updates the `QItemSelectionModel` through the adapter without a re-entry loop.
- Direct document selection change through `DocumentUiController` updates view selection.
- Component selection in document clears object-view selection or leaves no object rows selected, without attempting component edits.

### 6.2 Panel And Workspace Tests

Can live in `ui_model_tests.cpp` or a separate `ui_panel_workspace_tests.cpp` if the file grows too large.

Required tests:

- `ScenePanel` constructs with fake/minimal services and no Crimson GPU construction.
- `PropertiesPanel` constructs with fake/minimal services and no Crimson GPU construction.
- `PanelHost` creates stable dock object names:
  - `quader.panel.scene`
  - `quader.panel.properties`
- Panel visibility actions are canonical `QAction` instances from `ActionRegistry`.
- Toggling the action hides/shows the dock; changing dock visibility updates checked state without signal loops.
- `SettingsService` writes v2 workspace keys and reset removes only `ui/workspace/v2`, preserving unrelated settings.
- `MainWindow` smoke test still constructs without showing the GPU surface and contains Scene/Properties docks, not the prototype inspector dock.

### 6.3 Existing Tests To Update

Update:

- `tests/unit/ui/ui_shell_tests.cpp`
  - standard action count and IDs include the two panel actions.
  - workspace settings expectations use v2/current API.
  - panel host tests cover Scene/Properties default layout.
  - main window smoke expects Scene/Properties docks.
- `tests/unit/ui/ui_action_wiring_tests.cpp`
  - use `DocumentUiController` in fixture if `EditorActionController` constructor changes.
  - verify undo/redo/delete still route through command history and refresh actions.

Do not weaken existing command, document, or tool tests.

### 6.4 Boundary Checks

Existing `architecture_boundaries` test must continue passing:

- No Qt Widgets includes in `src/document`, `src/commands`, or `src/tools`.
- No graphics-runtime includes outside `src/crimson/gpu`.
- UI models may include Qt model/view headers because they live under `src/ui`.

If `QAbstractItemModelTester` headers are not in the current architecture checker allowlist, no change is needed because tests are outside `src/`. Do not add Qt Test includes to lower-layer source.

## 7. Ordered Worker Slices

### Slice 1: UI Document Command/Event Bridge

Owned files:

- `src/ui/services/document_ui_controller.*`
- `src/ui/qt_app/ui_context.hpp`
- `src/app/app_services.hpp`
- `src/app/application.cpp`
- `src/ui/actions/editor_action_controller.*`
- `tests/unit/ui/ui_action_wiring_tests.cpp`
- `CMakeLists.txt`

Work:

- Add `DocumentUiController`.
- Extend `UiContext`.
- Refactor `EditorActionController` to use `DocumentUiController` for undo/redo/delete.
- Keep tool action routing unchanged.
- Add or update tests proving menu actions still mutate only through command history and document events are drained.

Verification after slice:

```powershell
cmake --build --preset qt-mingw-debug
ctest --preset qt-mingw-debug
cmake --build --preset qt-mingw-debug --target check_architecture
python tools/project_board.py validate
```

### Slice 2: Models, Roles, Descriptors, Delegate, Selection Adapter

Owned files:

- `src/ui/models/item_model_roles.*`
- `src/ui/models/property_descriptor.*`
- `src/ui/models/document_item_model.*`
- `src/ui/models/property_item_model.*`
- `src/ui/models/document_selection_adapter.*`
- `src/ui/delegates/property_value_delegate.*`
- `tests/unit/ui/ui_model_tests.cpp`
- `CMakeLists.txt`

Work:

- Add named roles and metatype registration.
- Add `DocumentItemModel`.
- Add `PropertyItemModel`.
- Add `PropertyValueDelegate`.
- Add `DocumentSelectionAdapter`.
- Add `QAbstractItemModelTester` coverage and behavior tests.
- Add `Qt6::Test` test dependency.

Verification after slice:

```powershell
cmake --build --preset qt-mingw-debug
ctest --preset qt-mingw-debug
cmake --build --preset qt-mingw-debug --target check_architecture
python tools/project_board.py validate
```

### Slice 3: Scene/Properties Panels And Workspace Integration

Owned files:

- `src/ui/panels/scene_panel.*`
- `src/ui/panels/properties_panel.*`
- `src/ui/panels/panel_host.*`
- `src/ui/panels/panel_id.*` if helper changes are needed.
- `src/ui/services/settings_service.*`
- `src/ui/qt_app/main_window.*`
- `src/ui/actions/action_id.*`
- `src/ui/actions/action_registry.cpp`
- `src/ui/actions/action_state_updater.cpp` only if required to ignore or initialize new panel actions safely.
- `tests/unit/ui/ui_shell_tests.cpp`
- `tests/unit/ui/ui_model_tests.cpp` or `tests/unit/ui/ui_panel_workspace_tests.cpp`
- `CMakeLists.txt`

Work:

- Add Scene and Properties panels.
- Stop creating `PrototypeInspectorPanel`.
- Add panel visibility actions and menu wiring.
- Add PanelHost save/restore/reset behavior.
- Bump workspace state version and settings keys.
- Update main window workspace save/restore/reset order.
- Update smoke/workspace tests.

Verification after slice:

```powershell
cmake --build --preset qt-mingw-debug
ctest --preset qt-mingw-debug
cmake --build --preset qt-mingw-debug --target check_architecture
python tools/project_board.py validate
```

Because this slice changes user-visible runtime behavior, also run deploy before authority review:

```powershell
cmake --build --preset qt-mingw-debug --target deploy
```

Manual smoke before review:

```powershell
.\build\qt-mingw-debug\QuaderQtBgfx.exe
```

Check:

- app launches;
- Scene and Properties panels are present;
- panel visibility actions show/hide docks;
- selecting an object in Scene updates Properties;
- renaming selected object via Scene or Properties updates the same document state;
- undo/redo updates both panels;
- close/reopen restores dock visibility/layout.

## 8. UI Authority Gates

Return to the UI architect after implementation with:

- this plan path;
- changed files;
- slice order followed;
- any deviations and rationale;
- build output summary;
- `ctest --preset qt-mingw-debug` result;
- `check_architecture` result;
- `python tools/project_board.py validate` result;
- deploy result if runtime behavior changed;
- manual smoke result or reason it could not be run.

Approval criteria:

- Models and panels do not own document truth.
- All document-backed edits route through commands.
- Document events update models through the UI bridge.
- Selection adapter treats document selection as source of truth and prevents re-entry loops.
- Stable named roles are documented and tested.
- `QAbstractItemModelTester` covers custom models.
- Workspace state uses versioned settings keys and stable panel object names.
- MainWindow remains a shell.
- No renderer/viewport picking/#9 Crimson behavior is added.

If approved, the main/root agent should request the PM/authority board update and move this plan to `agents/archive/`. Do not delete the file automatically.

## 9. Residual Risks

- Object create/delete model updates may use full model reset because #7 document events do not expose row ordering deltas. This is acceptable for task #8 and should be revisited when hierarchy/reorder APIs exist.
- Tool-triggered document commands can still bypass `DocumentUiController` if future viewport/tool wiring calls `ToolContext::execute_command()` directly. Task #8 does not add viewport tool dispatch, so this is a recorded follow-up risk, not a blocker.
- Property descriptors are intentionally small and object-focused. Future material, hierarchy, component, and multi-object editing need software-approved commands before UI exposes editable rows.
- Workspace version bump invalidates old v1 panel layout state. This is acceptable because panel set changes from prototype inspector to Scene/Properties.
- `QAbstractItemModelTester` adds a Qt Test module requirement for tests. This should remain test-only and must not pull Qt Test into lower-layer modules.

## 10. Documentation

Implementation should update `dev-changelog.md` after durable source/test changes land. Do not update `changelog.md` or version files for this ordinary task implementation.

No architecture guide update is required unless implementation intentionally changes public architecture beyond this plan. If such a gap appears, return to the UI architect before proceeding.
