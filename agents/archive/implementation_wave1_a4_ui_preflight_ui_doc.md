# Wave 1 A4 UI Preflight Plan

Plan type: UI architecture preflight for project-board tasks `#3` and `#4`, with forward constraints for future `#8`.

Owning architect: UI architect.

Source authorities read:

- `AGENTS.md`
- `agents/workflow.md`
- `agents/task_board.md`
- `agents/tests-policy.md`
- `project_board.md`
- `agents/plans/audit_20260704_full_architecture_master.md`
- `agents/architecture/architecture.md`
- `agents/architecture/ui.md`
- `agents/architecture/renderer.md`
- Current prototype source: `src/main.cpp`, `src/MainWindow.*`, `src/BgfxWidget.*`, `CMakeLists.txt`, `CMakePresets.json`

This plan is implementation guidance only. It does not implement production code. Return to the UI architect after implementation with the changed files, build/test output, and deviations. If approved, the main agent should move this plan from `agents/plans/` to `agents/archive/`; do not delete it automatically.

## 1. Scope And Sequencing

This A4 preflight covers the UI side of:

- Task `#3`: `UiContext`, `ActionRegistry`, `ActionStateUpdater`, UI services, `PanelHost`, and `MainWindow` shell contracts.
- Task `#4`: `ViewportWidget`, `ViewportController`, `ViewportLayoutState`, `ViewportCameraController`, and the UI-facing viewport render-host boundary.
- Future task `#8`: constraints for document-backed panels, item models, property models, delegates, and selection adapters.

Implementation order must stay aligned with the master audit:

1. Task `#2` must land first enough to provide module targets, CTest/test preset, and dependency-boundary checks.
2. Task `#3` may then add UI shell services and refactor `MainWindow`.
3. Task `#4` may then split Qt viewport hosting from Crimson GPU/runtime ownership.
4. Task `#7` must define document, command, command history, selection, and tool contracts before task `#8` implements document-backed panels/models.

Do not add real document mutation, topology editing, selection truth, import/export behavior, picking-driven selection, material editing, PBR, render graph features, or renderer debug panels directly to `MainWindow`, `ViewportWidget`, or the prototype inspector while completing this work.

## 2. Current Prototype Baseline

The current UI is a flat prototype:

- `src/main.cpp` directly constructs `QApplication`, applies Fusion, and constructs `MainWindow`.
- `src/MainWindow.*` directly creates menus, status labels, an inspector dock, a `QTreeWidget` placeholder, property-like widgets, and a `BgfxWidget`.
- `src/BgfxWidget.h` includes `<bgfx/bgfx.h>` and stores bgfx resource handles in a public QWidget header.
- `src/BgfxWidget.cpp` owns Qt event handling, viewport splitters, camera/navigation state, bgfx initialization/shutdown/reset, shader loading, GPU resources, grid/cube submission, and formatted stats.
- `CMakeLists.txt` builds one executable target linking both `Qt6::Widgets` and bgfx/bx, with no CTest support yet.

This baseline is allowed only as prototype behavior. New UI architecture must move toward the guide without adding unrelated feature work.

## 3. Blockers

Hard blockers for production implementation:

- Task `#2` must define the module targets before this plan can be fully enforced. At minimum, `modeling_ui` must link Qt Widgets, and non-UI targets must not link Qt Widgets.
- Task `#2` must add a test preset and architecture guardrails. Until then, UI workers can only use manual configure/build and `rg` checks as substitutes.
- Task `#4` depends on a renderer-side implementation of the render-host boundary. UI can define its side of the contract, but must not move bgfx runtime ownership into another UI class.
- Future task `#8` is blocked by task `#7`. Without `Document`, `CommandHistory`, document events, and selection commands, UI workers must keep document actions disabled and keep panels prototype-only.

Soft blockers and coordination risks:

- `ActionStateUpdater` needs a state source. Before document/command/tool services exist, use a small UI-owned `EditorStateSnapshot` with false document capabilities.
- The renderer architect may choose exact Crimson public type names. This plan uses UI-facing interface names and assumptions; the implementing agent should adapt only names, not ownership boundaries, if the renderer plan settles different public DTO names.

## 4. UI Worker Scopes

Recommended worker split after task `#2`:

- UI worker A: Shell/services/actions for task `#3`.
- UI worker B: Viewport widget/controller/layout/camera for task `#4`.
- Renderer worker: Crimson render-host implementation and GPU boundary for task `#4`.
- Later UI worker C: Document-backed panels/models for task `#8`, only after task `#7`.

UI worker A must not edit Crimson internals. UI worker B may define only the UI-facing render-host interface and call it through explicit injection. Renderer worker must not add Qt Widgets dependencies to Crimson core or GPU code.

## 5. Module Layout

Use the UI guide's module map. After task `#2`, production UI files should move under these paths:

```text
src/ui/
  qt_app/
    main_window.hpp
    main_window.cpp
    qt_application_bootstrap.hpp
    qt_application_bootstrap.cpp
    ui_context.hpp
    editor_state_snapshot.hpp

  actions/
    action_id.hpp
    action_registry.hpp
    action_registry.cpp
    action_state_updater.hpp
    action_state_updater.cpp

  panels/
    panel_id.hpp
    panel_context.hpp
    panel_contract.hpp
    panel_host.hpp
    panel_host.cpp
    prototype_inspector_panel.hpp
    prototype_inspector_panel.cpp

  services/
    settings_service.hpp
    settings_service.cpp
    notification_service.hpp
    notification_service.cpp

  style/
    fusion_style_policy.hpp
    fusion_style_policy.cpp
    ui_metrics.hpp
    ui_metrics.cpp

  viewport/
    viewport_types.hpp
    viewport_widget.hpp
    viewport_widget.cpp
    viewport_controller.hpp
    viewport_controller.cpp
    viewport_layout_state.hpp
    viewport_layout_state.cpp
    viewport_camera_controller.hpp
    viewport_camera_controller.cpp
    viewport_render_host.hpp
```

Future task `#8` adds:

```text
src/ui/models/
  item_model_ids.hpp
  document_item_model.hpp
  document_item_model.cpp
  property_item_model.hpp
  property_item_model.cpp
  property_descriptor.hpp
  selection_adapter.hpp
  selection_adapter.cpp

src/ui/delegates/
  standard_item_delegate.hpp
  standard_item_delegate.cpp
  numeric_item_delegate.hpp
  numeric_item_delegate.cpp
```

The executable remains the composition target. It may own `src/app/application.*` as part of task `#3`, but UI implementation belongs in `modeling_ui`.

## 6. Composition Root And UiContext

Task `#3` should introduce an app composition root owned by the software side, with UI contracts defined here.

Recommended app lifetime:

```cpp
namespace quader::app {

class Application final {
public:
    Application(int& argc, char** argv);
    int run();

private:
    QApplication qt_app_;
    ui::SettingsService settings_;
    ui::NotificationService notifications_;
    ui::ActionRegistry actions_;
    ui::DefaultEditorStateProvider editor_state_;
    std::unique_ptr<ui::MainWindow> main_window_;
};

}
```

Declare services before `main_window_` so `MainWindow` and child widgets are destroyed before services. If Crimson render services are added in task `#4`, declare them before `main_window_` for the same reason.

`UiContext` is non-owning and explicit:

```cpp
namespace quader::ui {

struct UiContext {
    ActionRegistry& actions;
    SettingsService& settings;
    NotificationService& notifications;
    IEditorStateProvider& editor_state;

    // Optional until task #7.
    IDocumentContext* documents = nullptr;
    ICommandDispatcher* commands = nullptr;
    ICommandHistoryView* command_history = nullptr;
    IToolController* tools = nullptr;

    // Optional until task #4 wires a renderer host.
    IViewportRenderHost* viewport_render_host = nullptr;
};

}
```

The optional interfaces above may be defined as narrow abstract adapters in UI-facing headers or replaced by software-approved interfaces when task `#7` lands. Do not use global service locators, `QApplication::topLevelWidgets()` scraping, or hidden singletons.

## 7. Qt Bootstrap And Style

Move Qt startup policy out of `main.cpp` into `src/ui/qt_app/qt_application_bootstrap.*` or `src/ui/style/fusion_style_policy.*`.

Public API:

```cpp
namespace quader::ui {

void configure_qt_application_metadata(QApplication& app);
void apply_fusion_style(QApplication& app);

}
```

Rules:

- Set organization and application names before `QSettings` is used.
- Apply Fusion through `QStyleFactory::create("Fusion")` with a null check.
- Do not introduce a custom `QStyle`, broad global stylesheet, or theme system.
- Add `UiMetrics` with centralized margins, spacing, section spacing, and icon sizes. Use Fusion metrics where practical.

## 8. MainWindow Shell

Replace the current root `MainWindow` class with `src/ui/qt_app/main_window.*`.

Public API:

```cpp
namespace quader::ui {

class MainWindow final : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(UiContext& context, QWidget* parent = nullptr);
    ~MainWindow() override = default;

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    void build_menus();
    void build_toolbars();
    void build_central_area();
    void build_dock_hosts();
    void build_status_ui();
    void connect_actions();
    void connect_notifications();
    void restore_workspace();
    void save_workspace();
    void reset_workspace();

    UiContext& context_;
    PanelHost* panel_host_ = nullptr;
    ViewportWidget* viewport_ = nullptr;
    QLabel* renderer_label_ = nullptr;
    QLabel* stats_label_ = nullptr;
};

}
```

Object ownership:

- `MainWindow` is owned by `app::Application`.
- `PanelHost` is a `QObject` child of `MainWindow`.
- `ViewportWidget` is a QWidget child through `setCentralWidget()`.
- Docks are `QDockWidget` children of `MainWindow`, created through `PanelHost`.
- Status labels are `QLabel` children of `QStatusBar` or `MainWindow`.
- `MainWindow` stores non-owning service references only through `UiContext&`.

`MainWindow` responsibilities:

- Build menu bar, optional initial toolbar, central viewport, docks, status bar, lifecycle, workspace save/restore.
- Reuse canonical actions from `ActionRegistry`.
- Present notifications through status UI.
- Persist only UI workspace state.

`MainWindow` must not:

- Create command implementations.
- Own document truth, selection truth, mesh data, renderer GPU resources, or shader paths.
- Directly mutate future documents.
- Build hardcoded document property editors inline.
- Call other panels directly.

## 9. Action System

### 9.1 Action IDs

Create `src/ui/actions/action_id.hpp`:

```cpp
namespace quader::ui {

enum class ActionId : std::uint16_t {
    NewScene,
    OpenScene,
    SaveScene,
    SaveSceneAs,
    Exit,

    Undo,
    Redo,
    DuplicateSelection,
    DeleteSelection,

    SelectTool,
    MoveTool,
    RotateTool,
    ScaleTool,

    CreateCube,
    CreateLight,
    CreateCamera,

    ViewPerspective,
    ViewShaded,
    ToggleQuadViewports,
    FocusViewport,

    ResetWorkspaceLayout,
};

std::string_view action_id_name(ActionId id);

}
```

Use names close to current menu text but do not imply document behavior exists yet. `SaveScene` and `SaveSceneAs` may be registered disabled or deferred if the current menu does not expose them.

### 9.2 ActionRegistry

Create `src/ui/actions/action_registry.*`.

Public API:

```cpp
namespace quader::ui {

struct ActionSpec {
    QString text;
    QString status_tip;
    QKeySequence shortcut;
    bool checkable = false;
    QAction::MenuRole menu_role = QAction::NoRole;
};

class ActionRegistry final : public QObject {
    Q_OBJECT

public:
    explicit ActionRegistry(QObject* parent = nullptr);

    void register_action(ActionId id, ActionSpec spec);
    QAction& action(ActionId id) const;
    bool contains(ActionId id) const;

Q_SIGNALS:
    void action_triggered(ActionId id);
    void action_toggled(ActionId id, bool checked);

private:
    QHash<ActionId, QAction*> actions_;
};

void register_standard_actions(ActionRegistry& registry);

}
```

Ownership:

- `ActionRegistry` is app-owned.
- Each `QAction` is parented to `ActionRegistry`.
- Menus, toolbars, context menus, and shortcuts never create their own duplicate actions for the same user-visible command.

Implementation notes:

- Connecting a QAction to the registry signal is allowed.
- Shortcuts live in `ActionSpec`.
- `Exit` should have `QAction::QuitRole` where appropriate and connect to `MainWindow::close()`.
- `ToggleQuadViewports` is checkable and keeps the `F1` shortcut from the prototype.
- Future tool actions are checkable but disabled until a tool manager exists.

### 9.3 ActionStateUpdater

Create `src/ui/actions/action_state_updater.*`.

Public API:

```cpp
namespace quader::ui {

struct EditorStateSnapshot {
    bool has_active_document = false;
    bool document_dirty = false;
    bool has_selection = false;

    bool can_undo = false;
    QString undo_text;
    bool can_redo = false;
    QString redo_text;

    bool tools_available = false;
    ActionId active_tool = ActionId::SelectTool;

    bool viewport_available = true;
    bool quad_viewports_enabled = false;
};

class IEditorStateProvider {
public:
    virtual ~IEditorStateProvider() = default;
    virtual EditorStateSnapshot editor_state_snapshot() const = 0;
};

class ActionStateUpdater final : public QObject {
    Q_OBJECT

public:
    ActionStateUpdater(ActionRegistry& actions, IEditorStateProvider& state_provider, QObject* parent = nullptr);

public Q_SLOTS:
    void refresh();
    void refresh_from_snapshot(const EditorStateSnapshot& snapshot);

private:
    ActionRegistry& actions_;
    IEditorStateProvider& state_provider_;
};

}
```

Initial state rules:

- `NewScene`, `OpenScene`, `SaveScene`, `SaveSceneAs`, `Undo`, `Redo`, `DuplicateSelection`, `DeleteSelection`, `SelectTool`, `MoveTool`, `RotateTool`, `ScaleTool`, `CreateCube`, `CreateLight`, and `CreateCamera` remain disabled until task `#7` supplies document/command/tool services.
- `Exit`, `ToggleQuadViewports`, `FocusViewport`, and `ResetWorkspaceLayout` may be enabled when their UI services exist.
- Undo/redo labels must eventually derive from command history, for example `Undo Move Selection`, not hardcoded text.
- Checked tool action state eventually derives from the active tool service, not from the button that was clicked.

Signal flow:

```text
document/command/tool/viewport state changes
  -> IEditorStateProvider snapshot changes
  -> ActionStateUpdater::refresh()
  -> QAction enabled/checked/text/status updates
  -> menus, toolbar, shortcuts, and context menus reflect one state
```

Use `QSignalBlocker` when setting checked state to avoid action loops.

## 10. Settings Service And Workspace

Create `src/ui/services/settings_service.*`.

Public API:

```cpp
namespace quader::ui {

class SettingsService final {
public:
    SettingsService();

    QVariant value(QStringView key, const QVariant& fallback = {}) const;
    void set_value(QStringView key, const QVariant& value);
    void remove_group(QStringView group);
    void sync();

    QByteArray main_window_geometry() const;
    void set_main_window_geometry(const QByteArray& geometry);

    QByteArray main_window_state() const;
    void set_main_window_state(const QByteArray& state);

    QVariant viewport_value(QStringView key, const QVariant& fallback = {}) const;
    void set_viewport_value(QStringView key, const QVariant& value);

    void reset_workspace_v1();
};

}
```

Stable keys:

```text
ui/workspace/v1/main_window/geometry
ui/workspace/v1/main_window/state
ui/workspace/v1/main_window/state_version
ui/workspace/v1/panels/<panel_id>/visible
ui/workspace/v1/viewport/layout_mode
ui/workspace/v1/viewport/quad_split_vertical
ui/workspace/v1/viewport/quad_split_horizontal
```

Rules:

- Treat settings as UI state, not document state.
- `MainWindow::restore_workspace()` runs after all stable docks/panels/actions are created.
- `MainWindow::save_workspace()` runs on close.
- `ResetWorkspaceLayout` clears only workspace keys and then reapplies default panel placement and viewport layout.
- Use stable `objectName()` values for `MainWindow` and every dock.
- Do not persist user document contents, selection truth, mesh state, or renderer GPU resources in `SettingsService`.

Testing should use a temporary INI-backed `QSettings` path or a test constructor overload that accepts organization/application/test scope, so tests do not read or write real user config.

## 11. Notification Service

Create `src/ui/services/notification_service.*`.

Public API:

```cpp
namespace quader::ui {

enum class NotificationSeverity {
    Status,
    Info,
    Warning,
    Error,
};

struct NotificationMessage {
    NotificationSeverity severity = NotificationSeverity::Status;
    QString summary;
    QString detail;
    int timeout_ms = 3000;
};

class NotificationService final : public QObject {
    Q_OBJECT

public:
    explicit NotificationService(QObject* parent = nullptr);

    void show_status(QString summary, int timeout_ms = 3000);
    void show_info(NotificationMessage message);
    void show_warning(NotificationMessage message);
    void show_error(NotificationMessage message);

Q_SIGNALS:
    void notification_posted(const NotificationMessage& message);
};

}
```

MainWindow owns the status-bar presenter:

- `Status` and `Info` may use `QStatusBar::showMessage`.
- `Warning` and `Error` must be visible enough not to be overwritten silently by routine frame stats.
- Renderer diagnostics should enter this service as typed data, not preformatted status-only strings, when task `#4` lands.

## 12. PanelHost And Panels

### 12.1 Panel IDs

Create `src/ui/panels/panel_id.hpp`:

```cpp
namespace quader::ui {

enum class PanelId : std::uint16_t {
    Inspector,
    Properties,
    Scene,
    Diagnostics,
};

QString panel_id_name(PanelId id);
QString panel_object_name(PanelId id);

}
```

Only `Inspector` is needed for the current prototype. Keep the others reserved for near-term task `#8` and diagnostics planning only; do not create empty docks without product need.

Stable object names:

```text
quader.panel.inspector
quader.panel.properties
quader.panel.scene
quader.panel.diagnostics
```

### 12.2 PanelContext

Create `src/ui/panels/panel_context.hpp`:

```cpp
namespace quader::ui {

struct PanelContext {
    UiContext& ui;
};

}
```

Extend this later only with explicit non-owning references. Panels must not discover services globally.

### 12.3 PanelHost

Create `src/ui/panels/panel_host.*`.

Public API:

```cpp
namespace quader::ui {

struct PanelSpec {
    PanelId id;
    QString title;
    Qt::DockWidgetArea default_area = Qt::RightDockWidgetArea;
    Qt::DockWidgetAreas allowed_areas = Qt::AllDockWidgetAreas;
};

class PanelHost final : public QObject {
    Q_OBJECT

public:
    explicit PanelHost(QMainWindow& main_window, PanelContext context, QObject* parent = nullptr);

    QDockWidget& add_panel(PanelSpec spec, QWidget* panel_widget);
    QDockWidget* dock(PanelId id) const;

    void show_panel(PanelId id);
    void hide_panel(PanelId id);
    void reset_to_default_layout();

private:
    QMainWindow& main_window_;
    PanelContext context_;
    QHash<PanelId, QDockWidget*> docks_;
};

}
```

Ownership:

- `PanelHost` is a `QObject` child of `MainWindow`.
- `QDockWidget` instances are children of `MainWindow`.
- The panel widget is passed to `QDockWidget::setWidget()` and becomes owned by the dock.
- `PanelHost` stores non-owning dock pointers that remain valid because docks are `MainWindow` children.

Panel rules:

- Panels communicate through actions, services, document events, models, and explicit contexts.
- Panels must not call each other directly.
- Panels do not own document truth.
- Prototype-only data must be labeled in code as prototype UI state and must not become a document contract.

### 12.4 Prototype Inspector

Move the current hardcoded inspector into `src/ui/panels/prototype_inspector_panel.*`.

Public API:

```cpp
namespace quader::ui {

class PrototypeInspectorPanel final : public QWidget {
    Q_OBJECT

public:
    explicit PrototypeInspectorPanel(PanelContext context, QWidget* parent = nullptr);

Q_SIGNALS:
    void prototype_animation_enabled_changed(bool enabled);
};

}
```

Rules:

- It may use static labels for the current prototype.
- It may expose the rotate-cube toggle as UI/render prototype state.
- It must not use `QTreeWidgetItem` as document truth.
- It must not claim to represent real selection, materials, object properties, or renderer diagnostics.
- It must be replaceable in task `#8` without affecting `MainWindow`.

MainWindow or `ViewportController` can connect `prototype_animation_enabled_changed(bool)` to a viewport/controller method while the prototype cube exists. This connection is temporary and must be removed when render snapshots replace the hardcoded cube.

## 13. Viewport UI Boundary

Task `#4` must split the current `BgfxWidget` into UI and renderer responsibilities. UI side owns widgets, input translation, layout state, and camera interaction. Renderer side owns Crimson, GPU/runtime resources, shader loading, frame submission, and structured render diagnostics.

### 13.1 UI Types

Create `src/ui/viewport/viewport_types.hpp`:

```cpp
namespace quader::ui {

struct ViewportPixelSize {
    int width = 1;
    int height = 1;
};

struct ViewportPoint {
    double x = 0.0;
    double y = 0.0;
};

struct NativeViewportSurface {
    enum class Platform {
        WindowsHwnd,
        Unknown,
    };

    Platform platform = Platform::Unknown;
    void* native_window = nullptr;
};

enum class ViewportLayoutMode {
    Single,
    Quad,
};

struct ViewportPane {
    int x = 0;
    int y = 0;
    int width = 1;
    int height = 1;
    int camera_index = 0;
    QString name;
};

}
```

`NativeViewportSurface` is not graphics-runtime platform data. It is the narrow native surface payload the renderer host needs. The UI must not include `<bgfx/...>`, `<bx/...>`, or graphics-runtime platform headers to build it. On Windows, the UI may use `reinterpret_cast<void*>(winId())` without including `<windows.h>`.

### 13.2 Render Host Interface

Create `src/ui/viewport/viewport_render_host.hpp`:

```cpp
namespace quader::ui {

struct ViewportCameraSnapshot {
    int camera_index = 0;
    // Use project math types after task #2. Until then, keep this in UI
    // source only or use a small value type approved by the software plan.
};

struct ViewportRenderRequest {
    ViewportPixelSize surface_size;
    double device_pixel_ratio = 1.0;
    ViewportLayoutMode layout_mode = ViewportLayoutMode::Single;
    std::span<const ViewportPane> panes;
    std::span<const ViewportCameraSnapshot> cameras;
    bool prototype_animation_enabled = true;
};

struct ViewportFrameStats {
    int width = 0;
    int height = 0;
    int viewport_count = 1;
    double fps = 0.0;
};

class IViewportRenderHost {
public:
    virtual ~IViewportRenderHost() = default;

    virtual void initialize_surface(const NativeViewportSurface& surface,
                                    ViewportPixelSize size,
                                    double device_pixel_ratio) = 0;
    virtual void resize_surface(ViewportPixelSize size, double device_pixel_ratio) = 0;
    virtual void render_frame(const ViewportRenderRequest& request) = 0;
    virtual void shutdown_surface() = 0;

    virtual std::optional<ViewportFrameStats> latest_frame_stats() const = 0;
};

}
```

Renderer handoff assumptions:

- The renderer worker may implement this interface using Crimson public APIs or provide a renderer-owned adapter with equivalent semantics.
- The Crimson GPU layer converts `NativeViewportSurface` into graphics-runtime platform data.
- Crimson owns project render handles, shader loading, runtime resources, frame submission, and diagnostics.
- UI never exposes graphics-runtime handle types in public headers.
- UI does not build `bgfx::PlatformData`, call `bgfx::init`, load shader binaries, or submit draw calls.
- Renderer frame stats and diagnostics should be structured and then presented by `NotificationService` or status UI.

### 13.3 ViewportLayoutState

Create `src/ui/viewport/viewport_layout_state.*`.

Public API:

```cpp
namespace quader::ui {

class ViewportLayoutState final {
public:
    ViewportLayoutMode mode() const;
    void set_mode(ViewportLayoutMode mode);
    bool quad_enabled() const;
    void set_quad_enabled(bool enabled);

    float vertical_split() const;
    float horizontal_split() const;
    void set_vertical_split(float value);
    void set_horizontal_split(float value);

    std::array<ViewportPane, 4> panes_for(ViewportPixelSize size) const;
    int pane_count() const;
    int pane_at(ViewportPixelSize size, ViewportPoint point) const;

private:
    ViewportLayoutMode mode_ = ViewportLayoutMode::Single;
    float vertical_split_ = 0.5f;
    float horizontal_split_ = 0.5f;
};

}
```

Rules:

- Keep this class independent from GPU construction.
- It may be pure C++ and testable without a visible QWidget.
- It owns split fractions and layout mode only.
- It does not own document state, selection, or render resources.
- Persist layout mode and split fractions through `SettingsService`.

### 13.4 ViewportCameraController

Create `src/ui/viewport/viewport_camera_controller.*`.

Public API should separate camera/navigation state from Qt event classes:

```cpp
namespace quader::ui {

enum class CameraProjection {
    Perspective,
    Orthographic,
};

enum class NavigationMode {
    None,
    Orbit,
    Pan,
    Fly,
};

struct ViewportCameraState {
    CameraProjection projection = CameraProjection::Perspective;
    // Use project math types after task #2.
    float fov_degrees = 60.0f;
    float orthographic_size = 24.0f;
    bool locked_orthographic_view = false;
};

class ViewportCameraController final {
public:
    ViewportCameraController();

    void reset_default_cameras();
    const ViewportCameraState& camera(int camera_index) const;

    void begin_navigation(int camera_index, NavigationMode mode, ViewportPoint position);
    void update_navigation(ViewportPoint position, ViewportPixelSize viewport_size);
    void end_navigation();
    void apply_wheel_zoom(int camera_index, float wheel_steps, ViewportPixelSize viewport_size);
    void tick_fly_navigation(float delta_seconds);

    std::array<ViewportCameraSnapshot, 4> camera_snapshots() const;
};

}
```

Rules:

- This class owns interactive camera/navigation UI state.
- It must not consume `QMouseEvent`, `QWheelEvent`, `QKeyEvent`, or `QCursor`.
- It must not execute document commands.
- It must not call renderer GPU APIs.
- It can be tested with synthetic input and a fake clock.
- Future tools consume toolkit-neutral editor input events. The camera controller remains the navigation handler, not a general tool manager.

### 13.5 ViewportController

Create `src/ui/viewport/viewport_controller.*`.

Public API:

```cpp
namespace quader::ui {

class ViewportController final : public QObject {
    Q_OBJECT

public:
    ViewportController(UiContext& context, QObject* parent = nullptr);

    void attach_widget(ViewportWidget& widget);
    void initialize_surface(const NativeViewportSurface& surface,
                            ViewportPixelSize size,
                            double device_pixel_ratio);
    void resize_surface(ViewportPixelSize size, double device_pixel_ratio);
    void shutdown_surface();

    void set_quad_viewports_enabled(bool enabled);
    bool quad_viewports_enabled() const;

    void set_prototype_animation_enabled(bool enabled);
    void request_frame();

    ViewportLayoutState& layout_state();
    const ViewportLayoutState& layout_state() const;

Q_SIGNALS:
    void viewport_layout_changed(bool quad_enabled, QString layout_name);
    void frame_stats_changed(quader::ui::ViewportFrameStats stats);
    void render_error(quader::ui::NotificationMessage message);
    void frame_requested();

private:
    UiContext& context_;
    ViewportWidget* widget_ = nullptr;
    ViewportLayoutState layout_;
    ViewportCameraController cameras_;
    bool prototype_animation_enabled_ = true;
    bool surface_initialized_ = false;
};

}
```

Responsibilities:

- Coordinate viewport layout state, camera controller, and render-host calls.
- Convert UI state into `ViewportRenderRequest`.
- Coalesce frame requests where practical.
- Emit layout and stats signals for action state and status UI.
- Keep high-frequency input paths allocation-light and free of document mutation.

The controller may have methods called by `ViewportWidget` with translated event payloads, but it should not expose Qt event classes in its public API.

### 13.6 ViewportWidget

Create `src/ui/viewport/viewport_widget.*`.

Public API:

```cpp
namespace quader::ui {

class ViewportWidget final : public QWidget {
    Q_OBJECT

public:
    explicit ViewportWidget(ViewportController& controller, QWidget* parent = nullptr);
    ~ViewportWidget() override;

    QSize minimumSizeHint() const override;
    QPaintEngine* paintEngine() const override;

    NativeViewportSurface native_surface() const;
    ViewportPixelSize pixel_size() const;

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

private:
    ViewportController& controller_;
    QWidget* vertical_splitter_handle_ = nullptr;
    QWidget* horizontal_splitter_handle_ = nullptr;
};

}
```

Ownership:

- `ViewportWidget` is a child of `MainWindow` through `setCentralWidget()`.
- `ViewportController` is a `QObject` child of `ViewportWidget` or `MainWindow`, but it holds only non-owning references to app services and render host.
- Splitter handle widgets are children of `ViewportWidget`.
- `IViewportRenderHost` is app/renderer-owned and outlives `ViewportWidget`.

Widget rules:

- Set `WA_NativeWindow`, `WA_PaintOnScreen`, `WA_NoSystemBackground`, and `WA_OpaquePaintEvent` if the renderer host still needs a native child surface.
- Keep `paintEngine()` returning `nullptr` only if the renderer host draws directly to the native surface.
- Convert Qt event details into controller calls or toolkit-neutral input structs.
- Do not store bgfx/bx/graphics-runtime handles.
- Do not load shader files.
- Do not submit draw calls.

## 14. Action And Viewport Signal Flow

Initial `ToggleQuadViewports` flow:

```text
QAction ToggleQuadViewports toggled
  -> ActionRegistry emits action_toggled(ActionId::ToggleQuadViewports, checked)
  -> MainWindow or ViewportController slot calls set_quad_viewports_enabled(checked)
  -> ViewportLayoutState changes
  -> SettingsService stores workspace viewport layout
  -> ViewportController emits viewport_layout_changed(...)
  -> NotificationService shows transient status
  -> ActionStateUpdater refreshes checked state from EditorStateSnapshot
  -> ViewportController requests a frame
```

Renderer-ready flow:

```text
ViewportWidget showEvent
  -> native_surface(), pixel_size(), dpr
  -> ViewportController::initialize_surface(...)
  -> IViewportRenderHost::initialize_surface(...)
  -> renderer diagnostics/status returned through typed callback or signal
  -> NotificationService / status presenter updates UI
```

Resize flow:

```text
ViewportWidget resizeEvent
  -> ViewportController::resize_surface(pixel_size, dpr)
  -> IViewportRenderHost::resize_surface(...)
  -> request_frame()
```

Frame flow:

```text
timer, paint, input, layout, or camera change
  -> ViewportController::request_frame()
  -> ViewportRenderRequest built from layout/cameras/prototype state
  -> IViewportRenderHost::render_frame(request)
  -> structured stats/diagnostics update status UI
```

## 15. Future Task #8 Constraints

Do not implement these models until task `#7` supplies document, selection, command history, document events, and command dispatcher contracts.

### 15.1 DocumentItemModel

Future file: `src/ui/models/document_item_model.*`.

Expected contract:

```cpp
namespace quader::ui {

namespace DocumentItemRoles {
constexpr int DomainIdRole = Qt::UserRole + 1;
constexpr int KindRole = Qt::UserRole + 2;
constexpr int VisibilityRole = Qt::UserRole + 3;
constexpr int LockedRole = Qt::UserRole + 4;
constexpr int DirtyStateRole = Qt::UserRole + 5;
}

enum class DocumentItemColumn {
    Name = 0,
    Visibility = 1,
    Locked = 2,
};

class DocumentItemModel final : public QAbstractItemModel {
    Q_OBJECT

public:
    explicit DocumentItemModel(DocumentModelContext context, QObject* parent = nullptr);

    QModelIndex index(int row, int column, const QModelIndex& parent = {}) const override;
    QModelIndex parent(const QModelIndex& child) const override;
    int rowCount(const QModelIndex& parent = {}) const override;
    int columnCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role) override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

private:
    struct Node;
    std::vector<std::unique_ptr<Node>> roots_;
};

}
```

Rules:

- `internalPointer()` points only to model-owned nodes.
- Nodes store stable domain IDs or component refs, not raw mesh/document pointers.
- `setData()` validates UI-level values and dispatches commands.
- Document events drive precise row/data signals where practical.
- Use model reset only for full rebuilds.

### 15.2 PropertyItemModel

Future files: `src/ui/models/property_descriptor.hpp`, `src/ui/models/property_item_model.*`.

Expected contract:

```cpp
namespace quader::ui {

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

namespace PropertyItemRoles {
constexpr int PropertyPathRole = Qt::UserRole + 1;
constexpr int ValueKindRole = Qt::UserRole + 2;
constexpr int DescriptorRole = Qt::UserRole + 3;
}

class PropertyItemModel final : public QAbstractTableModel {
    Q_OBJECT

public:
    explicit PropertyItemModel(PropertyModelContext context, QObject* parent = nullptr);
    void set_descriptors(std::vector<PropertyDescriptor> descriptors);

    int rowCount(const QModelIndex& parent = {}) const override;
    int columnCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role) override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
};

}
```

Rules:

- The model owns descriptors as presentation rows, not document truth.
- Descriptor builders read document/selection state through approved adapters.
- Edits dispatch commands and wait for document events to update row values.
- Do not create a universal reflection system for task `#8`.

### 15.3 SelectionAdapter

Future file: `src/ui/models/selection_adapter.*`.

Expected flow:

```text
Qt view selection changed
  -> map indexes to source model
  -> read stable domain IDs from named roles
  -> dispatch selection command
  -> document emits selection_changed
  -> adapter updates QItemSelectionModel with QSignalBlocker/re-entry guard
```

Rules:

- Document selection is the source of truth.
- View selection is a projection.
- Hover and focus remain UI/tool state, not document selection.
- Do not let panels directly set each other's selections.

### 15.4 Delegates

Use delegates for editing and formatting only:

- `StandardItemDelegate` for common display/editor behavior.
- `NumericItemDelegate` for numeric validation, range handling, and commit-on-finish.

Delegates must not mutate documents directly or know mesh topology rules.

## 16. UI State Versus Document State

UI state:

- Main window geometry and dock state.
- Panel visibility and local UI expansion/filter state.
- Viewport single/quad layout and split fractions.
- Viewport camera/navigation state.
- Prototype animation toggle until snapshots replace the hardcoded cube.
- Active focus widget and transient hover state.

Document state, future:

- Scene objects.
- Meshes and topology.
- Object transforms.
- Materials and material assignments.
- Selection truth.
- Dirty state.
- Undoable user-visible edits.

UI state persists through `SettingsService`. Document state persists through document serialization when that exists. Do not cross these storage paths.

## 17. Dependency Boundary Rules

After task `#2`, these rules must be executable guardrails:

- Public UI headers may include Qt Widgets headers.
- Lower layers must not include Qt Widgets headers.
- Public UI headers must not include `<bgfx/...>` or `<bx/...>`.
- Only `src/crimson/gpu/` may include graphics-runtime headers and own runtime handles.
- `modeling_ui` may depend on approved command/document/tool/Crimson public interfaces but not Crimson GPU internals.
- `crimson` must not depend on `QWidget`, `QAction`, `QMainWindow`, `QAbstractItemModel`, `QDockWidget`, or dialogs.

Temporary manual checks until guardrails exist:

```powershell
rg -n "<bgfx/|<bx/" src/ui
rg -n "QWidget|QAction|QMainWindow|QAbstractItemModel|QDockWidget|QDialog" src/foundation src/math src/geometry src/mesh src/document src/commands src/tools src/crimson
```

Expected results: no matches outside approved UI/app-facing files and no graphics-runtime matches in public UI headers.

## 18. Test Strategy

Follow `agents/tests-policy.md`: tests must protect real behavior and architecture boundaries, not cosmetic layout details.

Required tests once task `#2` provides CTest:

- `ActionRegistry` tests:
  - Standard actions are registered once.
  - `ActionRegistry::action(ActionId)` returns canonical actions.
  - Shortcuts and checkable flags for current actions match the registry specs.
- `ActionStateUpdater` tests:
  - No active document disables document/tool/create/undo/redo actions.
  - Viewport snapshot checks `ToggleQuadViewports` without signal loops.
  - Future command-history snapshots update undo/redo enabled state and text.
- `SettingsService` tests:
  - Stable key names and workspace version group are used.
  - Reset clears only workspace keys.
  - Temporary test settings do not touch user config.
- `PanelHost` tests:
  - Stable object names for docks.
  - Panel ownership through Qt parents.
  - Reset creates the default inspector placement.
- `MainWindow` smoke test:
  - Construct with fake services and no Crimson GPU.
  - Menus reuse registry actions.
  - Inert document actions start disabled.
- `ViewportLayoutState` tests:
  - Single and quad pane counts.
  - Split clamping and hit testing.
  - Pixel-size edge cases do not produce zero or negative pane sizes.
- `ViewportController` tests with fake `IViewportRenderHost`:
  - Surface initialize/resize calls are forwarded.
  - Toggle quad updates layout and requests a frame.
  - Camera/navigation synthetic input changes camera state without GPU construction.
  - Prototype animation toggle is UI/render state only.
- Boundary checks:
  - No graphics-runtime includes in UI public headers.
  - No Qt Widgets includes outside UI/app-facing targets.

Future task `#8` tests:

- `QAbstractItemModelTester` for every custom model.
- Role/column/flag/index mapping tests.
- `setData()` edit-dispatch tests that assert command requests and semantic document updates.
- Selection-sync tests with re-entry guards.
- Property descriptor tests for mixed selection and unsupported edits.

Manual verification substitute only if GUI automation is not practical:

- Launch the built app.
- Confirm visible prototype behavior is preserved.
- Confirm unavailable document/tool/create actions are disabled.
- Toggle Four Viewports with the menu and `F1`.
- Resize and close/reopen to confirm workspace and viewport layout persistence.
- Confirm renderer errors use notification/status presentation rather than silent raw strings.

## 19. Verification Commands

Existing commands before task `#2`:

```powershell
cmake --preset qt-mingw-debug
cmake --build --preset qt-mingw-debug
cmake --build --preset qt-mingw-debug --target deploy
.\build\qt-mingw-debug\QuaderQtBgfx.exe
python tools/project_board.py validate
```

Expected commands after task `#2` adds tests:

```powershell
cmake --preset qt-mingw-debug
cmake --build --preset qt-mingw-debug
ctest --preset qt-mingw-debug
python tools/project_board.py validate
```

If task `#2` creates a named architecture-check target, also run it. If the test preset name differs, use the documented preset from task `#2` and update this plan during review.

## 20. Renderer Handoff Assumptions

This plan coordinates with renderer work through explicit assumptions only:

- UI owns `ViewportWidget`, `ViewportController`, `ViewportLayoutState`, `ViewportCameraController`, Qt parent lifetimes, Qt event translation, workspace persistence, and action/status integration.
- Renderer owns Crimson public renderer APIs, `FrameSnapshot`, `RenderCamera`, `ViewportSettings`, render graph, render handles, diagnostics, shader loading, GPU resources, runtime platform data conversion, and frame submission.
- `src/crimson/gpu/` owns all graphics-runtime includes and handles.
- UI may pass `NativeViewportSurface`, pixel size, device pixel ratio, layout panes, camera snapshots, and prototype render options through an injected render-host interface.
- UI must not expose or store `bgfx::VertexBufferHandle`, `bgfx::ProgramHandle`, `bgfx::UniformHandle`, `bgfx::ViewId`, or equivalent runtime handle types.
- Renderer diagnostics returned to UI should be structured, with summary/detail/severity. UI presents them through `NotificationService` and future diagnostics panels.
- Picking is not part of this A4 implementation. When picking arrives, UI sends request-scoped input/pick requests, renderer returns project-owned IDs, and document selection changes still go through commands.

If the renderer implementation cannot satisfy these assumptions, stop and return to the software architect for boundary arbitration rather than weakening the UI boundary.

## 21. Acceptance Criteria

Task `#3` UI acceptance:

- `MainWindow` receives `UiContext&` and is a shell.
- Canonical actions live in `ActionRegistry`; menus use registry actions.
- `ActionStateUpdater` disables unavailable document/tool/create actions.
- `SettingsService`, `NotificationService`, `PanelId`, and `PanelHost` exist with explicit ownership.
- Inspector prototype is isolated in a panel class and does not own document truth.
- Workspace save/restore/reset uses versioned settings keys and stable object names.
- Services are testable without Crimson GPU construction.

Task `#4` UI acceptance:

- Public UI headers do not include graphics-runtime headers.
- `ViewportWidget` is a Qt native surface and event-forwarding widget only.
- `ViewportController` and camera/layout controllers are testable without GPU construction.
- UI uses an injected `IViewportRenderHost` or renderer-equivalent adapter.
- `ViewportLayoutState` persists single/quad layout and split fractions.
- Prototype cube/grid rendering is preserved through the renderer boundary, not through UI-owned runtime handles.

Future task `#8` acceptance constraints:

- Document-backed panels use `QAbstractItemModel` adapters, not item widgets as truth.
- Models store stable domain IDs or UI node IDs.
- Model edits dispatch commands.
- Selection sync treats document selection as truth.
- `QAbstractItemModelTester` and behavior tests cover roles, flags, indexes, edits, and selection adapters.

## 22. Non-Goals

- Do not implement document, command history, real tools, mesh operations, material editing, import/export, picking-driven selection, PBR, render graph, renderer diagnostics panel, or golden-image harness as part of A4.
- Do not add a plugin panel system.
- Do not add a custom theme or broad stylesheet.
- Do not solve renderer internals in UI.
- Do not edit `project_board.md` from this architect assignment.
