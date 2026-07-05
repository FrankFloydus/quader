# UI Architecture Guide for a Generic Qt Widgets Modeling App

> **Audience:** humans and coding agents building the Qt Widgets UI layer for the polygon modeling app.  
> **Goal:** keep a large desktop UI scalable, testable, and aligned with the command-driven app architecture.

> **Overall architecture:** see [architecture.md](architecture.md).

This document owns the detailed Qt Widgets guidance. App-level module boundaries, mesh/document/command architecture, and general engineering policy remain in the overall architecture guide.

---

## 1. Generic Qt Widgets UI architecture for a large application

This section describes how to build a scalable Qt Widgets UI without locking the product into a premature layout, theme, or panel taxonomy.

The goal is not to say which panels the application must have. The goal is to define how any future window, panel, view, editor, toolbar, dialog, or viewport integration should be structured so the UI can grow without turning into a pile of slots, global state, and widget-specific business logic.

The UI stance is:

> **Qt owns widgets, events, actions, layouts, dialogs, and presentation adapters. The document owns editable state. Commands mutate the document. Tools interpret high-frequency viewport input. Renderers draw snapshots.**

### 1.1 V1 UI decision: use Fusion, do not design a custom theme yet

V1 should use Qt's Fusion style as the default application style.

Recommended startup shape:

```cpp
#include <QApplication>
#include <QStyleFactory>

int main(int argc, char** argv) {
    QApplication app(argc, argv);

    if (auto* fusion = QStyleFactory::create("Fusion")) {
        QApplication::setStyle(fusion);
    }

    // Set organization/application names before using QSettings.
    // Construct services, then MainWindow.

    return app.exec();
}
```

V1 Fusion policy:

- Use Fusion's standard controls, metrics, focus handling, menus, docks, item views, scrollbars, splitters, and dialogs.
- Do not create a custom `QStyle`, global dark theme, or broad application-wide stylesheet in V1.
- Do not hand-paint ordinary controls just to make them look custom.
- Do not scatter `setStyleSheet()` calls across widgets.
- Do not encode product colors in random panels.
- Prefer `QStyle`, `QPalette`, layout spacing, and standard widget states over custom drawing.
- Use small UI metrics and icon rules for consistency, but let Fusion provide the actual look and feel.
- Only add targeted styling when Fusion cannot express a required state clearly, and keep those exceptions centralized.

This gives the team a stable baseline: if the UI looks inconsistent in V1, the fix is usually bad layout, bad component boundaries, or inconsistent control choice, not a missing custom theme.

### 1.2 What the UI layer is allowed to do

The UI layer is an adapter layer. It translates between Qt and editor concepts.

Allowed in UI:

- create and own widgets
- create and own `QAction` objects
- build menus, toolbars, context menus, docks, dialogs, and status UI
- translate Qt events into editor input events
- adapt document state into `QAbstractItemModel` implementations
- adapt command history into action enabled/checked/text state
- show validation, import, export, render, and tool diagnostics
- persist user workspace state through a settings service
- coordinate panel creation and destruction

Not allowed in UI:

- implement topology algorithms
- directly mutate meshes for user-visible edits
- become the source of truth for selection, transforms, object state, materials, or document data
- call into concrete renderer internals except through a viewport/render-host boundary
- make lower layers depend on Qt Widgets
- use signals and slots as an untyped global event bus
- store long-lived raw pointers to mesh elements, scene records, or model indexes as application truth

### 1.3 Dependency direction for Qt Widgets

The dependency direction should remain boring:

```text
app
  └── ui
        ├── actions
        ├── models
        ├── delegates
        ├── panels
        ├── dialogs
        ├── style
        ├── viewport bridge
        └── services
              ↓
          commands / tools / document / render interfaces
```

Forbidden direction:

```text
mesh/document/commands/tools/render core -> QWidget/QMainWindow/QAction/QAbstractItemModel
```

The easiest enforcement is CMake target boundaries:

```cmake
add_library(modeling_ui ...)
target_link_libraries(modeling_ui
    PRIVATE
        Qt6::Widgets
        modeling_document
        modeling_commands
        modeling_tools
        modeling_render
)

# modeling_mesh_core, modeling_document, and modeling_commands do not link Qt6::Widgets.
```

Qt types may appear in UI headers. They should not appear in core mesh, document, command, tool, or renderer-core public headers.

### 1.4 UI module map

Organize the UI by responsibility, not by dumping every class into `widgets/`.

Recommended generic layout:

```text
src/ui/
  qt_app/
    qt_application_bootstrap.*     # Qt startup, app metadata, Fusion style setup
    main_window.*                  # shell only

  actions/
    action_id.hpp                  # stable action identifiers
    action_registry.*              # creates and exposes QActions
    action_state_updater.*         # derives enabled/checked/text from app state

  models/
    item_model_ids.hpp             # QModelIndex payload helpers and stable IDs
    document_item_model.*          # generic adapter for hierarchical document-like data
    property_item_model.*          # generic adapter for editable property rows
    command_history_item_model.*   # adapter for undo/redo display if needed

  delegates/
    standard_item_delegate.*       # common display/editor behavior
    numeric_item_delegate.*        # numeric validation/editing behavior

  panels/
    panel_id.hpp                   # stable panel IDs, not product layout assumptions
    panel_contract.hpp             # lifecycle contract for complex panels
    panel_context.hpp              # explicit non-owning access to services
    panel_host.*                   # QDockWidget/QStackedWidget/QSplitter hosting
    panel_factory.*                # static registration of available panel types

  dialogs/
    dialog_service.*               # file dialogs, confirmations, error details

  style/
    fusion_style_policy.*          # Fusion setup and style guardrails
    ui_metrics.*                   # spacing/icon/density constants using Fusion baseline
    icon_registry.*                # named icon lookup and size policy

  viewport/
    viewport_widget.*              # QWidget/QOpenGLWidget host
    viewport_controller.*          # event translation and interaction routing
    viewport_render_host.*         # render snapshot handoff

  services/
    settings_service.*             # wrapper around QSettings
    notification_service.*         # status/error/diagnostic presentation API
```

This is not a list of mandatory product panels. It is a set of places where future UI code has a predictable home.

### 1.5 Composition root: keep Qt construction centralized

Qt object construction should be concentrated in the application composition root and shell-building code.

Recommended construction order:

```text
main()
  ↓
QApplication
  ↓
set organization/application metadata
  ↓
apply Fusion style policy
  ↓
construct core services
  ↓
construct Qt-facing UI services
  ↓
construct MainWindow with explicit context
  ↓
restore workspace settings
  ↓
show window
  ↓
app.exec()
```

Do not let random panels create global services. Do not let services pull widgets from `QApplication::topLevelWidgets()`. Pass dependencies explicitly through a small `UiContext` or `PanelContext`.

Example:

```cpp
struct UiContext {
    DocumentProvider& documents;
    CommandHistory& commands;
    ToolManager& tools;
    ActionRegistry& actions;
    SettingsService& settings;
    NotificationService& notifications;
};
```

The context is non-owning. Ownership lives in the application object or composition root.

### 1.6 `QMainWindow` is a shell, not the application brain

`MainWindow` should coordinate the top-level Qt shell:

- menu bar
- toolbars
- central widget area
- dock hosting
- status bar
- high-level window lifecycle
- workspace save/restore
- top-level action placement

`MainWindow` should not contain:

- mesh topology logic
- file-format parsing
- command implementations
- detailed panel behavior
- large property-editing switch statements
- renderer resource management
- business rules for whether an edit is valid

Good `MainWindow` shape:

```cpp
class MainWindow final : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(UiContext& context, QWidget* parent = nullptr);

private:
    void build_menus();
    void build_toolbars();
    void build_central_area();
    void build_dock_hosts();
    void connect_lifecycle();
    void restore_workspace();
    void save_workspace();

    UiContext& context_;
    PanelHost* panel_host_ = nullptr;
};
```

Bad `MainWindow` smell:

```cpp
void MainWindow::on_split_edge_button_clicked() {
    auto* mesh = current_mesh_from_some_widget();
    mesh->halfedges()[h].next = ...;
}
```

Good path:

```cpp
void MainWindow::on_split_edge_action_triggered() {
    context_.commands.execute(make_split_edge_command(context_.documents.current_selection()));
}
```

### 1.7 Actions are the shared language of menus, toolbars, and shortcuts

A large Qt application should not create separate logic for menus, toolbar buttons, context menus, and keyboard shortcuts.

Use an `ActionRegistry` that owns `QAction` instances by stable action ID:

```cpp
enum class ActionId {
    NewDocument,
    OpenDocument,
    SaveDocument,
    Undo,
    Redo,
    DeleteSelection,
    DuplicateSelection,
    ToggleActiveTool,
    FocusViewport,
};

class ActionRegistry final : public QObject {
    Q_OBJECT

public:
    QAction& action(ActionId id);
    void register_action(ActionId id, QActionSpec spec);
};
```

Action rules:

- Every user-visible command has one canonical `QAction` where practical.
- Menus, toolbars, and context menus reuse the same action object.
- Shortcuts are assigned in one place.
- Enabled/disabled/checked state is derived from current application state, not guessed by individual widgets.
- Action IDs are stable; displayed text can change.
- Domain code never depends on `QAction`.

Action update flow:

```text
Document/tool/selection/command history changes
  ↓
EditorStateSnapshot is produced
  ↓
ActionStateUpdater updates QAction enabled/checked/text/tooltips
  ↓
Menus, toolbars, and shortcuts reflect the same state
```

Do not connect the same shortcut to several unrelated slots. That creates inconsistent behavior depending on focus.

### 1.8 Slots should be thin

Qt slots are wiring points. They should translate an event into one clear application action.

Good:

```cpp
void SomePanel::on_value_committed(double value) {
    context_.commands.execute(make_set_numeric_property_command(current_path_, value));
}
```

Bad:

```cpp
void SomePanel::on_value_changed(double value) {
    // validates document, mutates mesh, updates renderer, changes selection,
    // stores settings, refreshes other widgets, and logs errors
}
```

Slot rule:

> **A slot may validate UI-local input, dispatch a command, update UI-local state, or call a UI service. It should not become a business transaction script.**

### 1.9 Qt model/view is the default for scalable structured UI data

For dynamic data that can grow, change, sort, filter, select, or edit, prefer Qt model/view over item widgets.

Use model/view for:

- hierarchical document-like data
- large lists
- tables of attributes or diagnostics
- command history views
- editable property rows
- asset/resource lists
- search results
- any data shown in more than one view

Accept item widgets only for:

- tiny static lists
- prototypes
- dialogs with very small local choices
- throwaway developer-only tools

For scalable UI, prefer:

```text
QAbstractItemModel / QAbstractListModel / QAbstractTableModel
  +
QTreeView / QListView / QTableView
  +
QStyledItemDelegate when editing or custom display is needed
```

Avoid treating `QTreeWidget`, `QListWidget`, and `QTableWidget` as the main architecture for live document data. They are convenient, but the item-owned model makes it too easy to duplicate application state inside widgets.

### 1.10 Qt model is not the domain model

A Qt item model is a presentation adapter. It exposes data to Qt views and delegates. It should not own the authoritative document state.

Good:

```text
Document owns objects and selection
  ↓
DocumentItemModel adapts visible rows/columns/roles
  ↓
QTreeView displays rows
  ↓
setData dispatches command for edits
```

Bad:

```text
QTreeWidgetItem owns object names, visibility, lock state, selection, and hierarchy
  ↓
Document is reconstructed from widget items when saving
```

Presentation model rules:

- Store stable domain IDs in model nodes, not raw pointers to domain records.
- Keep row order/cache data as derived presentation state.
- Rebuild caches from document state after explicit changes.
- Use precise row insert/remove/move/data signals instead of resetting the whole model for every small change.
- Use model reset only for full rebuilds.
- Do not expose `QModelIndex` outside the model/view boundary as persistent application truth.
- Do not store `QModelIndex::internalPointer()` in commands, tools, or document state.

### 1.11 Model index payload rules

`QModelIndex` is a view/model addressing mechanism. It is not a domain handle.

Good payload:

```cpp
struct ItemModelNode {
    StableUiNodeId ui_node_id;
    std::optional<ObjectId> object_id;
    std::optional<ComponentRef> component_ref;
    int parent_row = -1;
    std::vector<std::unique_ptr<ItemModelNode>> children;
};
```

Rules:

- `internalPointer()` may point to a model-owned node, never to a mutable mesh record.
- Proxy models may create their own indexes; do not assume internal pointers survive proxy mapping.
- For long-lived references, use document IDs or `QPersistentModelIndex` only inside UI code that truly needs persistent view identity.
- Commands must store domain IDs, not `QModelIndex`.

### 1.12 Roles and columns must be documented

Every non-trivial model should document its columns and custom roles.

Example:

```cpp
namespace DocumentItemRoles {
constexpr int DomainIdRole = Qt::UserRole + 1;
constexpr int KindRole = Qt::UserRole + 2;
constexpr int VisibilityRole = Qt::UserRole + 3;
constexpr int DirtyStateRole = Qt::UserRole + 4;
}
```

Model role rules:

- Use built-in roles such as `Qt::DisplayRole`, `Qt::EditRole`, `Qt::DecorationRole`, `Qt::CheckStateRole`, and `Qt::ToolTipRole` where they fit.
- Put custom roles behind named constants.
- Do not overload `DisplayRole` with parseable hidden data.
- Do not return heavyweight objects from `data()` in hot views.
- Keep `data()` cheap; cache expensive derived strings/icons when needed.

### 1.13 Editable models dispatch commands

For document-backed data, `setData()` should not directly mutate the document.

Recommended editable flow:

```text
Delegate/editor commits value
  ↓
QAbstractItemModel::setData(index, value, Qt::EditRole)
  ↓
Model validates UI-level type/range
  ↓
Model builds typed edit request
  ↓
CommandHistory executes command
  ↓
Document emits change
  ↓
Model updates from document event
```

Example:

```cpp
bool PropertyItemModel::setData(const QModelIndex& index,
                                const QVariant& value,
                                int role) {
    if (role != Qt::EditRole || !index.isValid()) {
        return false;
    }

    const auto edit = make_property_edit_request(index, value);
    if (!edit) {
        return false;
    }

    context_.commands.execute(make_property_command(*edit));
    return true;
}
```

For UI-only models, direct mutation is acceptable if the model owns the data and it is not document truth.

### 1.14 Selection synchronization needs one policy

Qt item views have `QItemSelectionModel`. The document has editor selection. These are different concepts and must be synchronized deliberately.

Recommended policy:

```text
User changes Qt view selection
  ↓
Selection adapter converts selected indexes to domain IDs
  ↓
Selection command updates document selection
  ↓
Document emits selection_changed
  ↓
Selection adapter updates Qt selection model while blocking re-entry
```

Rules:

- The document selection is the source of truth.
- View selection is a projection of document selection when the view represents document data.
- Use a re-entry guard when synchronizing both directions.
- Never let two views fight by directly setting each other's selections.
- Hover is not selection. Keep hover in UI/tool state.
- Focus is not selection. Keep focus as UI state.

### 1.15 Proxy models should adapt presentation, not mutate truth

Use `QSortFilterProxyModel` or custom proxy models for:

- sorting
- filtering
- search
- grouping
- hiding rows based on presentation rules
- mapping a full model to a smaller view

Proxy model rules:

- Sort/filter changes should not change document order unless the user explicitly performs a reorder command.
- Always map proxy indexes to source indexes before creating domain edit requests.
- Keep filter criteria in UI state or a small presentation controller.
- Avoid stacking many proxies unless there is a clear reason; debugging index mapping becomes hard.

### 1.16 Delegates are for item painting and editing, not business logic

Use `QStyledItemDelegate` for reusable editing and display behavior.

Good delegate responsibilities:

- create an editor widget for a type
- validate local editor input
- format display text
- paint lightweight visual state
- commit editor data to the model

Bad delegate responsibilities:

- mutate document state directly
- know mesh topology rules
- open unrelated dialogs from paint paths
- run expensive queries during `paint()`
- store persistent business state

A delegate should usually know about value kind and formatting, not the whole application.

### 1.17 Generic property editing without building a giant framework too early

Large modeling applications often need editable property rows. Do not start with a universal reflection system. Start with typed descriptors generated by adapters.

Generic descriptor shape:

```cpp
enum class PropertyValueKind {
    Bool,
    Int,
    Float,
    Vector2,
    Vector3,
    String,
    Enum,
    Color,
    FilePath,
};

struct PropertyDescriptor {
    PropertyPath path;
    QString label;
    PropertyValueKind kind;
    QVariant value;
    QVariant minimum;
    QVariant maximum;
    bool editable = true;
    QString tooltip;
};
```

Flow:

```text
Current editor selection / context
  ↓
PropertyDescriptorBuilder asks domain adapters for descriptors
  ↓
PropertyItemModel exposes rows
  ↓
Delegate edits values
  ↓
Property edit command mutates document
```

Rules:

- Descriptors are presentation metadata, not a replacement for typed domain APIs.
- Property paths must be validated by command code before mutation.
- Do not create one dangerous `SetAnythingCommand` that bypasses invariants.
- For high-frequency numeric edits, support command merging so sliders do not create hundreds of undo entries.
- Custom widgets are allowed for complex editors, but the default should be descriptor-driven for ordinary scalar values.

### 1.18 Panel componentization

A panel is any substantial UI area hosted in a dock, tab, side area, central region, or modal/non-modal tool window. The architecture should not assume a fixed product panel list.

Complex panels should follow a small contract:

```cpp
class IPanelWidget {
public:
    virtual ~IPanelWidget() = default;
    virtual PanelId panel_id() const = 0;
    virtual QString title() const = 0;
    virtual QWidget* widget() = 0;
    virtual void on_editor_state_changed(const EditorStateSnapshot& state) = 0;
};
```

Panel rules:

- A panel may read document state through read APIs or presentation models.
- A panel may dispatch commands.
- A panel may own UI-only state such as collapsed sections, filter text, scroll position, and splitter state.
- A panel must not directly mutate mesh/document state.
- A panel must not call another panel directly.
- A panel should expose behavior through actions, models, or services.
- A panel should be constructible in a smoke test with a fake or minimal context.
- A panel should have a stable ID if its visibility/layout is saved.

### 1.19 Panel communication

Do not wire panels to each other by pointer.

Bad:

```cpp
left_panel_->set_current_object(right_panel_->selected_object());
```

Good:

```text
Panel A dispatches command or changes document/UI service state
  ↓
Document/UI service emits typed change
  ↓
Panel B observes state through model/service and updates itself
```

Allowed communication paths:

- document events
- command execution
- action registry
- selection service or document selection
- notification service
- settings service
- typed UI state service for non-document presentation state

Do not solve panel communication with a stringly typed global event bus.

### 1.20 UI state versus document state

A large application needs a strict state split.

Document state:

- objects and components
- topology and geometry
- transforms
- materials/assets used by the document
- persistent editable selection if selection is saved with the document
- undoable user-visible edits
- dirty state of the document

UI state:

- active/focused widget
- expanded rows
- panel visibility
- filter text
- selected rows in a purely UI list
- splitter positions
- scroll positions
- preview-only hover state
- temporary input buffers
- current workspace layout

Tool state:

- active interaction mode
- drag start
- hover candidate
- snapping candidate
- preview geometry
- numeric input during an operation

Rules:

- Document state is undoable when changed by the user.
- UI state is usually not undoable.
- Tool preview state is usually not persisted.
- Do not save UI state into the document unless it is intentionally part of the file format.

### 1.21 Viewport widgets are high-frequency UI, not ordinary forms

The viewport has special constraints because mouse move, tablet, wheel, key, hover, and redraw events can happen at high frequency.

Recommended viewport structure:

```text
ViewportWidget
  receives Qt paint/input events
  ↓
ViewportController
  translates events and manages camera/tool routing
  ↓
ToolManager
  active tool consumes toolkit-neutral input events
  ↓
CommandHistory
  committed edits become commands
  ↓
RenderHost
  draws snapshots and overlays
```

The viewport widget should not directly own document truth.

### 1.22 Translate Qt events into editor input events

Tools should not depend on `QMouseEvent`, `QWheelEvent`, `QTabletEvent`, or `QKeyEvent`.

Use toolkit-neutral events:

```cpp
struct PointerEvent {
    PointerDevice device;
    PointerPhase phase;
    Vec2 viewport_position;
    Vec2 screen_position;
    KeyboardModifiers modifiers;
    MouseButtons buttons;
    double timestamp_seconds = 0.0;
};

struct KeyEvent {
    Key key;
    KeyboardModifiers modifiers;
    bool auto_repeat = false;
};
```

Benefits:

- tools are testable without Qt
- input mapping is centralized
- future platform/input changes are localized
- viewport code can be profiled separately

### 1.23 Viewport rendering with Qt Widgets

Keep the rendering backend behind a viewport host boundary.

For an OpenGL-based V1, `QOpenGLWidget` is a reasonable Qt Widgets host. The widget should own Qt/OpenGL lifecycle hooks and delegate rendering to a renderer-facing host.

Recommended boundary:

```text
QOpenGLWidget subclass
  initializeGL / resizeGL / paintGL
  ↓
ViewportRenderHost
  ↓
Renderer interface
  ↓
RenderSceneSnapshot / overlay snapshot
```

Rules:

- Do not let the renderer query random widgets.
- Do not let widgets own GPU resource lifetime beyond the widget-host boundary.
- Do not draw heavy overlays through ad-hoc widget painting if they belong to the renderer.
- Do not rebuild render buffers from live document state inside `paintGL()`.
- Schedule redraws with explicit dirty/redraw requests, not by forcing synchronous repaint loops.

If the rendering backend is not OpenGL, keep the Qt-specific host equally thin. The point is the boundary, not the specific graphics API.

### 1.24 Overlays and temporary UI affordances

Overlays are visual aids drawn over or near the viewport. They are not general panels and not document truth.

Overlay examples by category:

- selection outlines
- transform handles
- snapping hints
- measurement guides
- brush radius previews
- temporary operation previews
- warning markers

Rules:

- Overlay data should be generated from document/tool state snapshots.
- Overlay rendering should not mutate the document.
- Overlay hit testing should produce typed interaction requests, not raw widget pointers.
- Overlay state should be invalidated precisely on document, tool, camera, or viewport-size changes.

### 1.25 Dialog architecture

Dialogs should be thin UI around a request/response object.

Good dialog flow:

```text
Action triggered
  ↓
DialogService opens dialog
  ↓
Dialog returns typed request or cancellation
  ↓
Command/import/export/service handles work
  ↓
NotificationService reports result
```

Rules:

- Do not let importers/exporters open dialogs.
- Do not let low-level code show `QMessageBox`.
- Do not perform heavy work synchronously inside dialog accept handlers.
- Keep validation close to the dialog for UI-only validation and inside commands/services for domain validation.
- Use modeless progress only when the operation can continue safely while the user interacts.
- Prefer typed result structs over pulling values out of widgets from the outside.

### 1.26 Settings architecture

Use one settings service wrapping `QSettings`. Do not let every widget invent keys.

Recommended:

```cpp
class SettingsService {
public:
    QByteArray main_window_geometry() const;
    void set_main_window_geometry(QByteArray value);

    QByteArray main_window_state(int version) const;
    void set_main_window_state(int version, QByteArray value);

    QVariant value(SettingsKey key, QVariant fallback = {}) const;
    void set_value(SettingsKey key, QVariant value);
};
```

Settings rules:

- Set organization and application names before constructing default `QSettings` objects.
- Use typed helper methods for common settings.
- Use stable keys and document them.
- Version workspace layout state.
- Save `QMainWindow::saveGeometry()` and `QMainWindow::saveState()` through the service.
- Give every dock/widget whose state is saved a stable `objectName`.
- Do not store document data in UI settings.
- Do not read/write settings in hot paint or model `data()` paths.

### 1.27 Workspace and dock management

Use `QMainWindow` docking as the V1 workspace mechanism, but keep it behind a small workspace/panel host abstraction.

Generic workspace responsibilities:

- create panel hosts by stable panel ID
- show/hide/toggle panels
- restore saved dock/toolbar state
- save dock/toolbar state
- expose panel visibility as actions
- support default workspace reset
- avoid product-specific hard-coding inside unrelated panels

Rules:

- Stable `objectName` is required for state restore.
- Saved workspace state should have a version number.
- Failed restore should fall back to a default layout.
- Dock creation should be deterministic.
- Panels should not save the whole workspace themselves.
- A panel can save its own local splitter/expanded state through `SettingsService` using its panel ID.

V1 should not build a dynamic plugin-based workspace system. Use static panel registration until real extension requirements exist.

### 1.28 Design system under Fusion

For V1, the design system is not a custom visual style. It is a consistency contract over Fusion.

It should define:

- spacing names
- icon sizes
- standard control choices
- form layout conventions
- terminology for actions and tooltips
- severity levels for messages
- when to use menu, toolbar, context menu, dialog, dock, or status message
- accessibility and keyboard-navigation expectations
- object-name conventions for saved state and tests

Example metrics API:

```cpp
struct UiMetrics {
    int dialog_margin = 12;
    int panel_margin = 8;
    int section_spacing = 8;
    int form_row_spacing = 4;
    int small_icon = 16;
    int toolbar_icon = 24;
};

class UiMetricsProvider {
public:
    UiMetrics metrics_for(const QWidget& widget) const;
};
```

Rules:

- Metrics may start as constants, but keep them centralized.
- Prefer querying the active style for standard metrics where Qt provides them.
- Do not define a color palette in V1 unless the product has a concrete requirement.
- Prefer semantic icons through an icon registry instead of hard-coded file paths in widgets.
- Use Fusion's enabled, disabled, checked, hover, focus, and selection states instead of custom state styling.
- Keep UI text consistent: the same action should have the same label everywhere.

### 1.29 Style sheets policy

Style sheets are powerful but easy to abuse. With Fusion as the V1 baseline, the default policy should be conservative.

Allowed:

- a small centralized stylesheet for a proven gap
- object-name or dynamic-property selectors for a narrow component state
- temporary developer-only diagnostic styling

Avoid:

- global stylesheets that restyle every widget
- inline stylesheets in constructors
- panel-specific colors that bypass Fusion
- styling used to fix layout problems
- styling used to hide invalid widget composition
- styling that changes metrics unpredictably across platforms

Preferred V1 fix order:

```text
1. Choose the correct standard widget.
2. Fix layout, margins, size policies, and focus policy.
3. Use a delegate for item display/editing.
4. Use icons/text/status messages for semantic state.
5. Add a targeted centralized stylesheet only if necessary.
```

### 1.30 Layout and density rules

Generic layout rules:

- Use Qt layouts; avoid absolute positioning except inside custom canvas/viewport widgets.
- Use `QSplitter` where users need to resize areas.
- Avoid deeply nested layouts that are hard to reason about.
- Avoid magic pixel constants in panels.
- Keep form labels aligned.
- Keep primary actions easy to reach and secondary options less prominent.
- Do not cram advanced controls into the first visible V1 layout.
- Use progressive disclosure for advanced settings.
- Use `QSizePolicy` deliberately; do not fight layouts with fixed sizes everywhere.
- Test with long labels and localized strings early.

A large UI becomes unmaintainable when every panel solves density differently. Centralize simple layout helpers for common forms and sections, but avoid building a full UI framework.

### 1.31 Notifications, status, and diagnostics

Separate notification levels:

| Level                | UI behavior                              |
| -------------------- | ---------------------------------------- |
| transient status     | status bar or small non-blocking message |
| recoverable warning  | notification area or non-modal details   |
| blocking decision    | modal confirmation dialog                |
| detailed diagnostics | diagnostics/log view or details expander |
| developer-only debug | debug panel/log output                   |

Rules:

- Do not use modal dialogs for routine success messages.
- Do not hide important failures in the status bar only.
- Error objects should come from services/commands/importers; UI decides presentation.
- Provide a way to copy detailed diagnostics.
- Keep user-facing messages separate from developer logs.

### 1.32 Threading and UI updates

Qt widgets belong on the GUI thread. Background work must not update widgets directly.

Recommended flow:

```text
UI starts background job with immutable input/snapshot
  ↓
Worker runs without QWidget access
  ↓
Worker emits result/progress through queued connection
  ↓
Main-thread service receives result
  ↓
Command/service applies result or reports diagnostics
  ↓
UI updates through document/service events
```

Rules:

- No direct widget access from workers.
- No live document mutation from workers.
- Workers consume copies, immutable snapshots, or read-only inputs.
- Completed work is applied on the main/editor thread through commands or explicit services.
- Progress reporting should be throttled.
- Cancellation should be explicit and cooperative.
- Avoid blocking the event loop with long operations.

### 1.33 QObject lifetime and ownership

Qt parent ownership is useful, but it must not blur application ownership.

Rules:

- Widgets can usually be parent-owned.
- Services should usually be owned by the application composition root, not by arbitrary widgets.
- Non-owning references in contexts should be explicit references or pointers with documented nullability.
- Do not mix `std::unique_ptr` ownership and Qt parent ownership blindly for the same object.
- Use `deleteLater()` when deleting QObjects across event-loop boundaries.
- Avoid holding raw widget pointers in long-lived non-UI services.
- Use `QPointer<T>` for optional widget references that may be deleted by Qt ownership.

Good:

```cpp
auto* panel = new SomePanel(context, parent_widget); // Qt parent owns widget.
```

Good for service ownership:

```cpp
class Application {
    SettingsService settings_;
    NotificationService notifications_;
    ActionRegistry actions_;
};
```

### 1.34 Signals and slots policy

Signals and slots are excellent for local component wiring. They should not become the application architecture.

Good uses:

- widget value committed
- model rows changed
- action triggered
- dialog accepted/rejected
- worker progress/result to main thread
- panel visibility changed

Bad uses:

- stringly typed global event bus
- hidden business workflows across many disconnected slots
- signal chains that mutate document state in surprising order
- catch-all `somethingChanged()` signals that force every component to refresh

Rules:

- Prefer typed signals.
- Keep signal payloads small and meaningful.
- Avoid sending large domain objects through signals in hot paths.
- Use document events for document changes.
- Use queued connections intentionally for cross-thread communication.
- Use `QSignalBlocker` or re-entry guards for selection/model synchronization.

### 1.35 Refresh strategy

Refreshing everything after every change is the easiest way to make a modeling UI slow.

Recommended event mapping:

| Change                   | UI response                                                                   |
| ------------------------ | ----------------------------------------------------------------------------- |
| object/component created | insert rows in affected presentation models; mark relevant render cache dirty |
| object/component deleted | remove rows; clear invalid UI selections; mark render cache dirty             |
| object/component renamed | emit `dataChanged` for affected rows                                          |
| transform changed        | update visible property rows and viewport snapshot                            |
| mesh topology changed    | rebuild topology-dependent render buffers and affected data views             |
| mesh geometry changed    | update position/normal buffers and dependent property rows                    |
| selection changed        | update selection adapters, action state, and property descriptors             |
| command history changed  | update undo/redo actions and any command-history view                         |
| tool changed             | update checked tool actions and any tool-specific option view                 |
| settings changed         | update only components that consume that setting                              |

Rules:

- Use precise document change events.
- Use dirty flags for render/cache updates.
- Avoid full model reset for small changes.
- Avoid rebuilding property rows on hover-only changes.
- Avoid repainting all viewports for a change visible in one widget only.
- Throttle high-frequency UI updates such as progress, hover text, and mouse tracking.

### 1.36 High-frequency input policy

High-frequency input must not run heavy document queries or allocate excessively.

Rules:

- Mouse move should not execute commands continuously unless the command supports merging and the operation truly requires live commits.
- Prefer preview state during drag, then one command on commit.
- Picking should use cached acceleration structures where possible.
- Avoid rebuilding item models from mouse hover.
- Avoid logging per mouse move in normal builds.
- Reuse temporary buffers for overlays and picking.
- Keep viewport redraw requests coalesced.

### 1.37 Undo/redo UI

Undo/redo UI should be a view of `CommandHistory`, not a separate command stack.

Rules:

- Undo and redo actions read their text/enabled state from command history.
- Any command-history view uses a presentation model over command history.
- UI edits that affect the document go through commands.
- Slider/dragger/text edits that produce many intermediate values should use command merging or commit-on-finish behavior.
- UI-only changes such as panel collapse usually do not belong in document undo.

### 1.38 File dialogs and external UI services

File dialogs, native dialogs, clipboard access, URL opening, and desktop integration belong in UI/platform services.

Rules:

- Importers/exporters receive file paths or streams; they do not ask the user for files.
- Commands receive validated inputs; they do not open dialogs.
- Dialog services return typed results.
- Platform-specific quirks stay inside UI/platform services.
- Keep non-UI code testable without a desktop session.

### 1.39 Accessibility, localization, and keyboard behavior

Even in V1, keep the UI compatible with basic accessibility and localization work.

Rules:

- Use real Qt widgets for ordinary controls instead of custom-painted fake controls.
- Set accessible names/descriptions where icon-only controls are used.
- Do not rely on color alone to convey state.
- Keep keyboard shortcuts centralized.
- Avoid focus traps.
- Ensure dialogs have clear default/cancel buttons.
- Use translatable strings for user-visible text.
- Test layouts with longer strings.
- Keep action text, tooltip, and status text consistent.

Fusion helps here because standard widgets already provide expected desktop behavior. Do not throw that away with custom painting unless there is a clear product reason.

### 1.40 UI testing strategy

UI tests should focus on behavior and architecture boundaries, not pixel-perfect screenshots.

Test layers:

```text
Many:
  model tests, descriptor tests, action-state tests, settings-key tests

Some:
  panel construction smoke tests, dialog request/response tests, selection sync tests

Few:
  full-window integration tests and basic interaction tests
```

Recommended tests:

- `QAbstractItemModelTester` for each custom item model.
- Unit tests for row counts, roles, flags, editability, and index mapping.
- Tests that `setData()` dispatches the expected command for document-backed models.
- Tests for action enabled/checked state from editor state snapshots.
- Tests for settings key stability and workspace-state version handling.
- Smoke tests that construct panels with fake services.
- Tests that lower layers do not include or link Qt Widgets.

Avoid making most tests depend on exact widget geometry or screenshots. Those tests are brittle across platforms and styles.

### 1.41 UI performance traps

| Trap                                      | Why it hurts                                             | Prefer                             |
| ----------------------------------------- | -------------------------------------------------------- | ---------------------------------- |
| full model reset after every change       | loses selection/expansion and causes unnecessary repaint | precise insert/remove/data signals |
| storing document truth in widgets         | duplicates state and breaks undo/save                    | document + presentation models     |
| panel-to-panel pointer calls              | hidden coupling                                          | document/events/services/actions   |
| broad stylesheets                         | unpredictable metrics and maintenance cost               | Fusion + centralized exceptions    |
| synchronous heavy work in slots           | frozen UI                                                | background job + queued result     |
| direct worker-to-widget updates           | undefined/fragile thread behavior                        | main-thread result handling        |
| per-row heap-heavy widgets in large lists | memory and performance cost                              | item views + delegates             |
| using indexes as domain IDs               | stale references and proxy bugs                          | stable domain IDs                  |
| repainting on every mouse event           | jank                                                     | coalesced updates and dirty flags  |
| custom painting standard controls         | accessibility/style regression                           | standard widgets/delegates         |

### 1.42 UI implementation sequence

A safe V1 sequence:

1. Qt application bootstrap with Fusion style policy.
2. Application-owned services and explicit `UiContext`.
3. `MainWindow` shell with menu bar, status bar, central area, and panel host.
4. `ActionRegistry` with canonical actions and shortcuts.
5. `ActionStateUpdater` driven by editor state snapshots.
6. Minimal viewport host and event translation path.
7. First document-backed item model for one structured view.
8. Selection synchronization policy for that model.
9. Generic property item model for ordinary editable values, if needed.
10. Workspace save/restore through `SettingsService`.
11. Dialog service for file/confirmation/error details.
12. Notification/status service.
13. Model tests and panel smoke tests.
14. Additional panels/views only when product features require them.

Do not begin with a custom theme, plugin-based panel system, universal reflection framework, or a large designer-driven UI file that owns business behavior.

### 1.43 UI-specific agent rules

Recommended `src/ui/AGENTS.md`:

```md
# UI module rules

## Boundaries

- UI code may use Qt Widgets.
- Lower layers must not include Qt Widgets headers.
- UI dispatches commands for document mutations.
- UI presentation models adapt document state; they do not own document truth.
- Keep MainWindow as a shell.

## Fusion V1

- Use Fusion as the default style.
- Do not add a custom QStyle, global theme, or broad stylesheet without explicit approval.
- Keep spacing, icons, and text conventions centralized.
- Prefer standard widgets and delegates over custom-painted controls.

## Actions

- Add user-visible actions to ActionRegistry.
- Reuse the same QAction in menus, toolbars, context menus, and shortcuts.
- Derive enabled/checked state from editor state snapshots.

## Model/view

- Use QAbstractItemModel/QAbstractListModel/QAbstractTableModel for scalable document-backed data.
- Store stable domain IDs in model nodes, not raw mesh pointers.
- Do not store QModelIndex in commands or document state.
- Use precise model signals instead of reset when possible.
- Add QAbstractItemModelTester coverage for custom models.

## Panels

- Panels may read state and dispatch commands.
- Panels must not call other panels directly.
- Panels should communicate through document events, actions, services, or models.
- Panels with saved layout must have stable IDs/object names.

## Viewport

- Viewport widgets translate Qt events to toolkit-neutral editor events.
- Tools consume editor input events, not Qt event classes.
- The viewport renders snapshots/overlays and does not own document truth.

## Testing

- Add model tests for roles, flags, index mapping, and edit dispatch.
- Add action-state tests for new actions.
- Add smoke tests for non-trivial panels where practical.
- Do not fix UI issues by changing mesh/document/command dependencies.
```

### 1.44 UI code review checklist

```md
## Qt/UI architecture

- [ ] Does this keep Qt Widgets inside the UI layer?
- [ ] Does this preserve command-driven document mutation?
- [ ] Is MainWindow still a shell?
- [ ] Are services injected explicitly instead of found through globals?
- [ ] Are widgets free of mesh topology logic?

## Fusion V1

- [ ] Does this keep Fusion as the baseline style?
- [ ] Did this avoid broad stylesheets and custom QStyle work?
- [ ] Are spacing/icon/text conventions centralized?
- [ ] Are standard widgets/delegates used where possible?

## Actions

- [ ] Is there one canonical QAction for the user-visible action?
- [ ] Are shortcuts centralized?
- [ ] Is enabled/checked state derived from editor state?

## Model/view

- [ ] Is a Qt item model only an adapter, not document truth?
- [ ] Are stable domain IDs used instead of raw pointers/indexes?
- [ ] Are custom roles documented?
- [ ] Are edits routed through commands?
- [ ] Are model updates precise enough?

## Componentization

- [ ] Does the panel/widget have one clear responsibility?
- [ ] Does it avoid panel-to-panel direct calls?
- [ ] Does it use services/models/actions for communication?
- [ ] Is UI state separated from document state?

## Threading and performance

- [ ] Does heavy work avoid the GUI thread?
- [ ] Are worker results marshaled back to the main thread?
- [ ] Are high-frequency input paths allocation-aware?
- [ ] Are refreshes scoped to the actual change?

## Tests

- [ ] Are custom models tested?
- [ ] Are action states tested?
- [ ] Are non-trivial panels smoke-tested?
- [ ] Are architecture boundaries enforceable by CMake/tests?
```

---

## 2. UI References and external anchors

These Qt references support the UI guidance above.

- Qt `QApplication` documentation: https://doc.qt.io/qt-6/qapplication.html
  - Application lifetime, control flow, GUI style setup, and `QApplication::setStyle`.
- Qt `QStyleFactory` documentation: https://doc.qt.io/qt-6/qstylefactory.html
  - Built-in style lookup, including Fusion.
- Qt Model/View Programming: https://doc.qt.io/qt-6/model-view-programming.html
  - Models, views, delegates, and the separation between data source and presentation adapter.
- Qt `QAbstractItemModel` documentation: https://doc.qt.io/qt-6/qabstractitemmodel.html
  - Standard item model interface, model indexes, roles, and model/view contract.
- Qt `QAbstractItemView` documentation: https://doc.qt.io/qt-6/qabstractitemview.html
  - Standard view behavior for selection, keyboard navigation, editing, and scrolling.
- Qt `QMainWindow` documentation: https://doc.qt.io/qt-6/qmainwindow.html
  - Menu, toolbar, dock, status bar, and `saveState` / `restoreState` workspace state.
- Qt `QWidget` documentation: https://doc.qt.io/qt-6/qwidget.html
  - Base widget behavior and style sheet considerations.
- Qt `QSettings` documentation: https://doc.qt.io/qt-6/qsettings.html
  - Portable application settings and GUI state persistence.
- Qt `QOpenGLWidget` documentation: https://doc.qt.io/qt-6/qopenglwidget.html
  - OpenGL rendering integration with Qt Widgets.
- Qt `QThread` documentation: https://doc.qt.io/qt-6/qthread.html
  - Worker-object threading pattern and queued signal/slot behavior.
- Qt Synchronizing Threads documentation: https://doc.qt.io/qt-6/threads-synchronizing.html
  - Queued invocations and event-loop-based cross-thread communication.
- Qt `QAbstractItemModelTester` documentation: https://doc.qt.io/qt-6/qabstractitemmodeltester.html
  - Non-destructive consistency testing for custom item models.
