# Implementation Plan: Task #11 UI Diagnostics Panel And Status Presentation

Owner: UI architect A56
Task: project-board #11, "Mature diagnostics, performance hooks, and golden images"
Assignment: A56
Status: active UI implementation plan
Authority: `agents/plans/audit_20260704_full_architecture_master.md`, Batch 10
Renderer handoff authority: `agents/plans/implementation_task11_diagnostics_performance_golden_renderer_doc.md`
Primary UI authority: `agents/architecture/ui.md`
Supporting authorities: `agents/architecture/architecture.md`, `agents/architecture/renderer.md` for DTO boundary context only, and `agents/tests-policy.md`
Workspace baseline used for this plan: current files under `C:\Users\Drako\Desktop\_quader-qt-base` on 2026-07-05

This is a planning deliverable only. Do not implement code from this file until the PM assigns implementation work for #11. When implementation begins, the executor must set a Codex `/goal` for task #11 or the PM assignment before editing files. The goal is complete only after the assigned UI scope is implemented, verified, documented when required, and handed back through PM and UI authority review.

When implementation is complete, return to the UI architect with this plan path, changed files, implementation notes, verification results, and known deviations. If UI authority approves, the main/root agent should move this plan from `agents/plans/` to `agents/archive/`. Do not delete the plan automatically.

## Goal

Add the UI half of task #11 diagnostics maturity:

- a dockable Diagnostics panel that presents renderer diagnostics, pass stats, counters, and frame graph text from the A53 viewport diagnostics DTO handoff
- a compact status-bar diagnostics summary that complements the existing renderer and frame-stat labels
- canonical action, panel-host, menu, and workspace integration
- item-model contracts and tests for the diagnostics presentation
- no Crimson internals, no GPU/runtime handles, no golden-image implementation, no benchmark UI, and no renderer implementation

The panel is a UI presentation surface for already-produced public DTOs. It must not become renderer truth, document truth, a benchmark runner, or a golden-image control surface.

## Current Workspace Findings

The current workspace already includes the relevant UI shell and A53 DTO handoff:

- `src/ui/viewport/viewport_types.hpp` defines `ViewportDiagnosticsSnapshot`, `ViewportRenderPassStats`, `ViewportRendererCounter`, and `ViewportRendererDiagnosticRow`.
- `src/ui/viewport/viewport_render_host.hpp` exposes `IViewportRenderHost::latest_diagnostics()`.
- `src/ui/viewport/crimson_viewport_render_host.*` maps public Crimson diagnostics snapshots into UI-owned DTOs with Qt strings/containers.
- `tests/unit/ui/viewport_tests.cpp` already verifies that the DTO handoff and Crimson-to-UI diagnostics mapping preserve structured fields.
- `src/ui/panels/panel_id.*` already contains `PanelId::Diagnostics`; do not add a duplicate enum value.
- `src/ui/actions/action_id.*` and `src/ui/actions/action_registry.*` do not yet contain `ActionId::ShowDiagnosticsPanel`.
- `src/ui/panels/panel_host.*` supports stable dock object names, visibility actions, workspace save/restore, and a hard-coded default layout for Scene and Properties panels.
- `src/ui/qt_app/main_window.*` owns viewport host/controller construction, menu placement, panel-host composition, and the existing renderer/status labels.
- `src/ui/services/settings_service.*` persists workspace state under `ui/workspace/v2`, including per-panel visibility by `PanelId`.
- Existing UI tests use `QAbstractItemModelTester`, shell/action tests, panel smoke construction, settings-key checks, and fake viewport hosts without requiring Crimson GPU initialization.

The implementation should extend these contracts. Do not create a parallel diagnostics source, scrape widgets, or query Crimson directly from the panel.

## Strict Non-Goals

Do not implement these in this UI slice:

- no Crimson renderer internals, `src/crimson/gpu/**` includes, runtime handles, shader changes, render graph changes, readback capture, golden baselines, or benchmark execution
- no use of `crimson::RendererDiagnosticsSnapshot` inside UI models or panels
- no document mutation, selection mutation, topology operations, or command dispatch from diagnostics UI
- no modal dialogs for routine renderer warnings
- no global stylesheet, custom `QStyle`, custom-painted table controls, or one-off theme work
- no pixel-perfect screenshot tests for the panel
- no timing-threshold tests for frame or pass durations
- no status-bar redesign beyond adding one compact diagnostics summary label

## UI Architecture Decision

Add one UI service and one panel/model pair:

```text
IViewportRenderHost::latest_diagnostics()
        |
        v
ViewportDiagnosticsService
        |
        +--> MainWindow compact status label
        |
        +--> DiagnosticsPanel
                |
                v
        DiagnosticsItemModel
```

Key rules:

- `ViewportDiagnosticsService` is the only UI service allowed to call `IViewportRenderHost::latest_diagnostics()`.
- `DiagnosticsPanel` and `DiagnosticsItemModel` consume only `ViewportDiagnosticsService` and UI-owned DTOs.
- The service stores copies of `ViewportDiagnosticsSnapshot`; it never stores Crimson DTOs or GPU/runtime objects.
- `MainWindow` remains a shell. It attaches the viewport render host to the service, creates the dock, places the action in menus, and updates a short status label. It does not parse pass/counter/detail rows.
- The diagnostics panel is a developer-oriented dock. It is hidden by default and can be shown through `View > Panels > Diagnostics Panel`.

## Files And Classes

### New UI service

Files:

```text
src/ui/services/viewport_diagnostics_service.hpp
src/ui/services/viewport_diagnostics_service.cpp
```

Public API:

```cpp
struct ViewportDiagnosticsSummary {
    bool available = false;
    QString renderer_name;
    ViewportFrameStats frame;
    int pass_count = 0;
    int counter_count = 0;
    int diagnostic_count = 0;
    int warning_count = 0;
    int error_count = 0;
    QString status_text;
    QString tooltip_text;
};

class ViewportDiagnosticsService final : public QObject {
    Q_OBJECT

public:
    explicit ViewportDiagnosticsService(QObject* parent = nullptr);

    void attach_render_host(IViewportRenderHost& render_host) noexcept;
    void detach_render_host(const IViewportRenderHost& render_host) noexcept;
    void clear_render_host() noexcept;

    void refresh();

    [[nodiscard]] const std::optional<ViewportDiagnosticsSnapshot>& latest_snapshot() const noexcept;
    [[nodiscard]] ViewportDiagnosticsSummary summary() const;
    [[nodiscard]] bool has_provider() const noexcept;

Q_SIGNALS:
    void diagnostics_changed();
    void diagnostics_unavailable();
    void summary_changed();
};
```

Ownership and lifetime:

- `AppServices` owns `ViewportDiagnosticsService` as an app-owned UI service, alongside `SettingsService` and `NotificationService`.
- `UiContext` gains `ViewportDiagnosticsService& viewport_diagnostics`.
- `MainWindow` owns `std::unique_ptr<IViewportRenderHost> viewport_render_host_`.
- `MainWindow::build_central_area()` attaches `*viewport_render_host_` to `context_.viewport_diagnostics` after creating the render host.
- `MainWindow::~MainWindow()` or a private shutdown helper must call `context_.viewport_diagnostics.clear_render_host()` before `viewport_render_host_` is destroyed, preventing the app-owned service from retaining a dangling non-owning pointer.
- `ViewportDiagnosticsService` must stay GUI-thread only. No worker thread updates, no queued cross-thread payloads, and no widget access from lower layers.

Refresh behavior:

- `refresh()` calls `latest_diagnostics()` only when a render host is attached.
- If the host returns `std::nullopt`, clear the cached snapshot, emit `diagnostics_unavailable()` and `summary_changed()` only when availability changed.
- If a snapshot is present, store the UI DTO copy and emit `diagnostics_changed()` plus `summary_changed()`.
- Avoid hidden global state. Do not find the viewport through `QApplication::topLevelWidgets()`.
- Do not call `latest_diagnostics()` from mouse-move/input handlers. Refresh after successful render frames, renderer startup, and render errors.

### New diagnostics model

Files:

```text
src/ui/models/diagnostics_item_model.hpp
src/ui/models/diagnostics_item_model.cpp
```

Public API:

```cpp
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
    struct Node;
    [[nodiscard]] Node* node_from_index(const QModelIndex& index) const noexcept;
    void rebuild_from_snapshot(const std::optional<ViewportDiagnosticsSnapshot>& snapshot);
};
```

Model contract:

- This is a presentation adapter over `ViewportDiagnosticsSnapshot`, not renderer truth.
- The model stores model-owned nodes only. `QModelIndex::internalPointer()` points to model-owned `Node` objects, never to DTO vectors, renderer data, mesh data, or Qt widgets.
- It is read-only. `setData()` is not implemented and `flags()` never returns `Qt::ItemIsEditable`.
- It has three columns: `Name`, `Value`, `Detail`.
- Root rows are stable sections in this order:
  - `Summary`
  - `Render Passes`
  - `Counters`
  - `Diagnostics`
- Summary children include renderer, frame size, viewport count, FPS, pass count, counter count, warning count, and error count.
- Render-pass children use `ViewportRenderPassStats` and expose pass name, draw calls/packets, resource reads/writes, and CPU microseconds.
- Counter children use `ViewportRendererCounter` and expose domain/name/value/unit.
- Diagnostic children use `ViewportRendererDiagnosticRow` and expose severity, code, subsystem, resource, message, detail, and frame index.
- `Qt::ToolTipRole` and `DiagnosticsItemRoles::CopyTextRole` must provide copyable detail text for rows with diagnostics or long detail fields.
- `DiagnosticsItemRoles::SeverityRole` returns the DTO severity string for diagnostic rows. UI decoration may use standard icons or text, but must not rely on color alone.
- `DiagnosticsItemRoles::FrameIndexRole` returns `qulonglong` for diagnostic rows and an invalid `QVariant` otherwise.
- `register_ui_model_metatypes()` should register `DiagnosticsNodeKind` if the model exposes it through QVariant.

Refresh contract:

- `reload_from_service()` rebuilds from the service's latest snapshot.
- Full model reset is acceptable when the row topology changes. When the row topology is unchanged, prefer updating row values and emitting `dataChanged()` to preserve expansion/selection.
- The panel is hidden by default; if hidden updates become noisy, `DiagnosticsPanel` may skip model reload while hidden and reload in `showEvent()`. The service summary still updates for the status bar.

### New diagnostics panel

Files:

```text
src/ui/panels/diagnostics_panel.hpp
src/ui/panels/diagnostics_panel.cpp
```

Public API:

```cpp
class DiagnosticsPanel final : public QWidget {
    Q_OBJECT

public:
    explicit DiagnosticsPanel(PanelContext context, QWidget* parent = nullptr);

    [[nodiscard]] DiagnosticsItemModel& model() noexcept;
    [[nodiscard]] QTreeView& tree_view() noexcept;

protected:
    void showEvent(QShowEvent* event) override;

private Q_SLOTS:
    void refresh_from_service();
    void copy_selected_rows();
    void copy_all_rows();

private:
    PanelContext context_;
    DiagnosticsItemModel* model_ = nullptr;
    QTreeView* tree_view_ = nullptr;
    QLabel* summary_label_ = nullptr;
    QPlainTextEdit* frame_graph_view_ = nullptr;
    QPushButton* refresh_button_ = nullptr;
    QPushButton* copy_selected_button_ = nullptr;
    QPushButton* copy_all_button_ = nullptr;
};
```

Panel UX:

- Use standard Qt Widgets and Fusion styling.
- The panel should be dense and operational, not a landing page.
- Top row: a compact summary label from `ViewportDiagnosticsService::summary()`.
- Main area: a `QTabWidget` with:
  - `Items`: `QTreeView` backed by `DiagnosticsItemModel`
  - `Frame Graph`: read-only `QPlainTextEdit` showing `ViewportDiagnosticsSnapshot::frame_graph_dump`
- Bottom row: `Refresh`, `Copy Selected`, and `Copy All` buttons.
- `Refresh` calls `context_.ui.viewport_diagnostics.refresh()`.
- Copy actions use `QApplication::clipboard()` from the UI layer only. They copy model-provided text, not raw Crimson structures.
- The tree view should use alternating row colors, no custom painting, stretch the Detail column, and expand the Summary section by default.
- The panel should show an empty but valid state when diagnostics are unavailable: summary text such as `Diagnostics unavailable`, an empty model except the Summary section, and an empty frame graph view.
- No modal dialogs for routine diagnostics. If copying fails or no row is selected, a transient status notification through `NotificationService` is enough.

Panel boundary:

- `DiagnosticsPanel` may call `ViewportDiagnosticsService::refresh()` and read UI DTO summaries.
- It must not call `IViewportRenderHost` directly.
- It must not include Crimson headers.
- It must not mutate documents, commands, tools, settings except through existing workspace visibility handled by `PanelHost`.
- It must not call Scene or Properties panels directly.

## Action Registration And Menus

Update files:

```text
src/ui/actions/action_id.hpp
src/ui/actions/action_id.cpp
src/ui/actions/action_registry.cpp
src/ui/actions/action_state_updater.cpp
tests/unit/ui/ui_shell_tests.cpp
```

Add:

```cpp
enum class ActionId : std::uint16_t {
    ...
    ShowScenePanel,
    ShowPropertiesPanel,
    ShowDiagnosticsPanel,
    ResetWorkspaceLayout,
};
```

Register:

```cpp
registry.register_action(ActionId::ShowDiagnosticsPanel, {
    QStringLiteral("Diagnostics Panel"),
    QStringLiteral("Show or hide renderer diagnostics."),
    {},
    true,
});
```

Action-state rules:

- `ShowDiagnosticsPanel` is a canonical checkable panel visibility action.
- It is enabled whenever the shell exists, matching Scene and Properties panel visibility actions.
- `ActionStateUpdater::refresh_from_snapshot()` should explicitly keep all panel visibility actions enabled. If existing Scene/Properties actions rely only on initial enabled state, this slice may either preserve that behavior or make all three explicit together.
- No shortcut in V1 unless product requirements request one.
- Add the action to `View > Panels` after Properties.

## Panel Host And Workspace

Update files:

```text
src/ui/panels/panel_host.hpp
src/ui/panels/panel_host.cpp
src/ui/qt_app/main_window.cpp
src/ui/services/settings_service.* only if tests expose missing behavior
tests/unit/ui/ui_shell_tests.cpp
```

Use the existing `PanelId::Diagnostics` and `panel_object_name(PanelId::Diagnostics) == "quader.panel.diagnostics"`.

Recommended small `PanelHost` API improvement:

```cpp
struct PanelSpec {
    PanelId id = PanelId::Scene;
    QString title;
    Qt::DockWidgetArea default_area = Qt::RightDockWidgetArea;
    Qt::DockWidgetAreas allowed_areas = Qt::AllDockWidgetAreas;
    bool default_visible = true;
};
```

Rules:

- Scene and Properties remain visible by default.
- Diagnostics is added as a bottom dock and hidden by default:

```cpp
PanelSpec{
    PanelId::Diagnostics,
    QStringLiteral("Diagnostics"),
    Qt::BottomDockWidgetArea,
    Qt::BottomDockWidgetArea | Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea,
    false,
}
```

- `PanelHost::reset_to_default_layout()` should restore Scene and Properties to the right side and keep Diagnostics in the bottom area hidden by default.
- `PanelHost::save_workspace()` and `restore_workspace()` already iterate all registered docks. They should persist Diagnostics visibility automatically through `SettingsService::panel_visible(PanelId::Diagnostics, fallback)`.
- Do not bump `SettingsService::kWorkspaceStateVersion` unless Qt dock-state compatibility actually breaks. Adding a dock with a stable object name should be compatible with the current versioned state.

## MainWindow Integration

Update files:

```text
src/ui/qt_app/main_window.hpp
src/ui/qt_app/main_window.cpp
```

Add includes for:

```text
ui/panels/diagnostics_panel.hpp
ui/services/viewport_diagnostics_service.hpp
```

New members:

```cpp
QLabel* diagnostics_label_ = nullptr;
```

MainWindow responsibilities:

- Attach the viewport render host to `context_.viewport_diagnostics` in `build_central_area()` after `viewport_render_host_` is created.
- Clear the render host from `context_.viewport_diagnostics` before `viewport_render_host_` destruction.
- Create `DiagnosticsPanel` in `build_dock_hosts()`, add it through `PanelHost`, and bind it to `ActionId::ShowDiagnosticsPanel`.
- Add `ShowDiagnosticsPanel` to `View > Panels`.
- Add a compact diagnostics label to the status bar:
  - default text: `Diagnostics: unavailable`
  - no warnings/errors: `Diagnostics: OK`
  - warnings/errors: `Diagnostics: N warning(s), M error(s)`
  - tooltip: latest issue summary and counts from `ViewportDiagnosticsSummary::tooltip_text`
- Connect `ViewportDiagnosticsService::summary_changed` to a private `update_diagnostics_label()` slot/helper.
- In `connect_viewport()`, call `context_.viewport_diagnostics.refresh()` after successful frames via the existing `frame_stats_changed` connection and after `renderer_ready`.
- On render errors, keep the existing `NotificationService::show_error()` path and also refresh diagnostics so the panel/status can show any structured renderer diagnostic now available.

MainWindow must not:

- inspect `ViewportDiagnosticsSnapshot::passes`, counters, or diagnostic rows directly
- copy details to clipboard
- know about `DiagnosticsItemModel` internals
- include Crimson headers for diagnostics

## UiContext And AppServices

Update files:

```text
src/ui/qt_app/ui_context.hpp
src/app/app_services.hpp
src/app/application.cpp
```

Add:

```cpp
class ViewportDiagnosticsService;

struct UiContext {
    ...
    ViewportDiagnosticsService& viewport_diagnostics;
};
```

`AppServices` adds:

```cpp
ui::ViewportDiagnosticsService viewport_diagnostics;
```

and wires `ui_context` with the new service reference.

No lower layer depends on this service. It is UI-only.

## Status And Notification Policy

- Routine diagnostics live in the panel and status label.
- Renderer startup success can continue to show the existing transient status message.
- Render failures remain `NotificationSeverity::Error`.
- Do not post a notification every time a warning diagnostic appears; that would spam users during rendering.
- The status label tooltip and Diagnostics panel are the detailed surfaces.

## Model/View And Data Formatting

Formatting helpers may live in:

```text
src/ui/models/diagnostics_item_model.cpp
```

or a small private namespace in the service/model implementation. Do not add a broad formatting framework.

Formatting rules:

- FPS uses zero or one decimal place consistently with the existing status label.
- CPU timing displays microseconds as a number and `us`; do not use it for pass/fail logic.
- Bytes counters may stay as raw bytes in V1; humanized units are optional but must remain deterministic in tests.
- Empty strings should display as `-` in `DisplayRole` only. Keep `CopyTextRole` precise and include field names.
- Long diagnostic details belong in `ToolTipRole`, Detail column, and copy text. Do not truncate the stored value.

## High-Frequency And Performance Rules

- Diagnostics refresh is driven by render completion, not by input events.
- Avoid per-mouse-move diagnostics copies.
- Do not rebuild the diagnostics model from hover-only viewport changes unless a frame actually rendered and the service snapshot changed.
- Keep signals small: `ViewportDiagnosticsService` emits no large snapshot payload by default; consumers pull the latest snapshot.
- The diagnostics ring is renderer-owned and bounded by A53/A51. UI must not create unbounded copies of historical diagnostics.
- The panel is hidden by default. If visible updates prove expensive, the panel may skip detailed model reload while hidden and reload on `showEvent()`.

## Tests

Tests must follow `agents/tests-policy.md`: protect user-visible behavior and UI contracts, avoid cosmetic/pixel assertions, avoid timing thresholds, and use the smallest useful test level.

Required automated UI tests:

### `tests/unit/ui/ui_diagnostics_model_tests.cpp` new

Use `QApplication` and `QAbstractItemModelTester`.

Cover:

- empty/unavailable service snapshot produces a valid model with stable Summary section and no invalid indexes
- a synthetic `ViewportDiagnosticsSnapshot` exposes Summary, Render Passes, Counters, and Diagnostics sections
- columns, headers, row counts, parent/index relationships, and flags are correct
- custom roles return expected kind, severity, frame index, and copy text
- model is read-only and does not mutate service or renderer data
- `copy_text_for_index()` and `copy_text_for_all()` include useful diagnostic details without depending on exact widget geometry

### `tests/unit/ui/ui_shell_tests.cpp` updates

Cover:

- `register_standard_actions()` includes `ActionId::ShowDiagnosticsPanel`, `action_id_name()` is not `Unknown`, and registry size is updated
- the diagnostics panel visibility action is checkable and enabled
- `PanelHost` creates a stable `quader.panel.diagnostics` dock, binds it to `ShowDiagnosticsPanel`, saves/restores visibility, and reset layout hides it by default
- `SettingsService` can persist `PanelId::Diagnostics` visibility under the existing workspace root
- `DiagnosticsPanel` constructs without a GPU surface when supplied a fake/empty diagnostics service
- `MainWindow` includes the Diagnostics dock and menu action without showing or initializing a GPU surface in the shell test

### `tests/unit/ui/ui_viewport_diagnostics_service_tests.cpp` new or merged into `ui_shell_tests.cpp`

Use a fake `IViewportRenderHost`.

Cover:

- `ViewportDiagnosticsService::refresh()` returns unavailable when no provider is attached
- attached provider snapshots are cached as UI DTOs
- warnings/errors are counted from `ViewportRendererDiagnosticRow::severity`
- summary text and tooltip are deterministic
- detaching the provider clears or preserves state according to the documented API and cannot leave a dangling pointer

### Existing `tests/unit/ui/viewport_tests.cpp`

Keep the existing A53 mapping tests. Add only focused assertions if the service requires a small helper. Do not duplicate all mapping coverage in the panel tests.

Required verification commands before UI authority review:

```powershell
cmake --build --preset qt-mingw-debug
ctest --preset qt-mingw-debug
cmake --build --preset qt-mingw-debug --target check_architecture
python tools/project_board.py validate
```

Because this changes visible runtime UI, also run:

```powershell
cmake --build --preset qt-mingw-debug --target deploy
```

Manual smoke, if no automated live-GPU UI smoke exists:

1. Launch the deployed app.
2. Confirm the Diagnostics panel is hidden by default.
3. Use `View > Panels > Diagnostics Panel` to show it.
4. Confirm the panel populates after the viewport renders at least one frame.
5. Confirm the status bar shows a compact diagnostics summary.
6. Confirm `Copy Selected`, `Copy All`, and `Refresh` work without modal dialogs.
7. Confirm reset layout hides the Diagnostics panel and restores Scene/Properties.

Manual smoke is a supplement, not a replacement for the model/action/service tests above.

## CMake And Source Registration

Update `CMakeLists.txt`:

- add the new UI service/model/panel source and header files to `modeling_ui`
- add the new UI diagnostics test executable(s)
- link tests to `modeling_ui` and `Qt6::Test` only where `QAbstractItemModelTester` is used
- keep Crimson GPU/runtime dependencies out of diagnostics model and panel tests

The implementation must keep architecture checks passing. Lower layers must not include Qt Widgets, and UI diagnostics files must not include `src/crimson/gpu/**` or graphics-runtime headers.

## Documentation Expectations

Update `dev-changelog.md` for durable development-visible UI diagnostics behavior and verification. Do not update user-facing `changelog.md` during ordinary implementation; release notes are handled by the push workflow.

No README update is required unless new developer commands or workflow expectations are added.

## Implementation Batches

### Batch UI-1: Service, action, and context wiring

Files:

- `src/ui/services/viewport_diagnostics_service.*`
- `src/ui/qt_app/ui_context.hpp`
- `src/app/app_services.hpp`
- `src/app/application.cpp`
- `src/ui/actions/action_id.*`
- `src/ui/actions/action_registry.cpp`
- `src/ui/actions/action_state_updater.cpp`
- focused tests for service and action registration

Verification after this slice:

```powershell
cmake --build --preset qt-mingw-debug
ctest --preset qt-mingw-debug
cmake --build --preset qt-mingw-debug --target check_architecture
```

### Batch UI-2: Diagnostics item model

Files:

- `src/ui/models/diagnostics_item_model.*`
- `src/ui/models/item_model_roles.*` only for metatype registration if needed
- `tests/unit/ui/ui_diagnostics_model_tests.cpp`
- `CMakeLists.txt`

Verification after this slice:

```powershell
cmake --build --preset qt-mingw-debug
ctest --preset qt-mingw-debug
cmake --build --preset qt-mingw-debug --target check_architecture
```

### Batch UI-3: Panel, dock, status label, workspace integration

Files:

- `src/ui/panels/diagnostics_panel.*`
- `src/ui/panels/panel_host.*`
- `src/ui/qt_app/main_window.*`
- `tests/unit/ui/ui_shell_tests.cpp`
- `dev-changelog.md`

Verification after this slice:

```powershell
cmake --build --preset qt-mingw-debug
ctest --preset qt-mingw-debug
cmake --build --preset qt-mingw-debug --target check_architecture
python tools/project_board.py validate
cmake --build --preset qt-mingw-debug --target deploy
```

Then perform the manual smoke listed above if no equivalent automated live-GPU smoke exists.

## Authority Review And Deviation Rules

Return to UI architect before continuing if:

- the panel needs direct access to Crimson types beyond UI DTOs
- the service cannot safely attach/detach the render host without a dangling pointer risk
- the implementation needs a timer, background thread, or cross-thread widget update
- the model requires exact frame-time thresholds or screenshot assertions to pass tests
- `MainWindow` starts accumulating detailed diagnostics parsing or panel business logic
- workspace reset requires a settings version bump
- any workaround weakens panel/action/model tests or skips `check_architecture`

Renderer architect review is required only if the UI handoff DTO contract itself must change. Renderer internals, golden-image capture, benchmark hooks, and runtime performance counters remain outside this UI plan.

Software architect review is required only if the implementation changes app-wide ownership, dependency direction, service lifetime outside UI, or task/workflow rules.

## Residual Risks

- The current diagnostics snapshot may be empty until the renderer has produced a frame. The UI must present this as unavailable/empty, not as an error.
- CPU timing values are useful diagnostics but not deterministic; tests must assert formatting and data flow, not runtime speed.
- Since Diagnostics is hidden by default, manual smoke is useful to catch menu/dock regressions until an interactive GUI smoke harness exists.
- `PanelId::Diagnostics` already exists in the current workspace even though the renderer plan originally listed it as deferred. Treat it as a current UI identifier and wire it; do not remove or duplicate it.
