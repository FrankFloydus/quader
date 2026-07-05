# Frozen UI Architecture Audit: 20260704_full_architecture

Audit scope: Qt Widgets UI architecture only.

Frozen baseline:
- Run ID: `20260704_full_architecture`
- Baseline time: `2026-07-04 18:44:19 +02:00`
- Repo root: `C:\Users\Drako\Desktop\_quader-qt-base`
- Generated `build\` output excluded.
- This individual UI report is source material only. The later master audit is the actionable report.

Architecture authorities read:
- `agents/architecture/ui.md`
- `agents/architecture/architecture.md`

Staleness note:
- No extra UI modules, action/model/panel services, or settings/test infrastructure were found beyond the frozen source layout.
- This report does not incorporate any speculative software or renderer changes outside the frozen baseline.

## Severity Key

- Critical: currently blocks the target architecture or will force rewrites before normal UI features can be added.
- High: likely architectural drift that should be corrected before expanding feature work.
- Medium: important maintainability/testability gap, but can be sequenced after the main shell boundaries.
- Low: local consistency or polish issue that should be folded into nearby work.

## Positive Baseline Compliance

- `src/main.cpp:8-11` creates `QApplication`, sets organization/application names, and applies Fusion.
- `src/MainWindow.cpp:31-35` uses a `QDockWidget` with a stable object name for the inspector dock.
- Current UI code uses Qt parent ownership for widgets owned by `MainWindow` and `BgfxWidget`.
- No broad global stylesheet or custom `QStyle` was found.

These are useful starting points, but the prototype still diverges from the target UI architecture in the findings below.

## Findings

### UI-001: UI module boundaries are not established

Severity: High

Domain: UI architecture, with CMake boundary implications.

Evidence:
- `CMakeLists.txt:34-49` builds one executable from `src/main.cpp`, `src/MainWindow.*`, and `src/BgfxWidget.*`, then links both `Qt6::Widgets` and `bgfx` directly into that executable.
- The frozen source layout has no `src/ui/qt_app`, `src/ui/actions`, `src/ui/models`, `src/ui/panels`, `src/ui/services`, or `src/ui/viewport` modules.

Violated target rules:
- `agents/architecture/ui.md` sections 1.3 and 1.4 require a visible UI module map and boring dependency direction.
- `agents/architecture/architecture.md` section 4.3 recommends CMake targets that enforce dependency boundaries.

Practical risk:
- The current flat target cannot prevent Qt Widgets, bgfx backend details, shell code, panel code, and future document/command code from accumulating in the same compilation unit.
- UI testing and dependency-boundary checks cannot be added cleanly while the shell, viewport, renderer, and application bootstrap remain in one executable target.

Recommended correction:
- Add a dedicated UI module layout before expanding the UI:
  - `src/ui/qt_app/main_window.*`
  - `src/ui/qt_app/qt_application_bootstrap.*`
  - `src/ui/actions/action_id.hpp`
  - `src/ui/actions/action_registry.*`
  - `src/ui/actions/action_state_updater.*`
  - `src/ui/panels/panel_id.hpp`, `panel_host.*`, `panel_context.hpp`
  - `src/ui/services/settings_service.*`, `notification_service.*`
  - `src/ui/viewport/viewport_widget.*`, `viewport_controller.*`
- Introduce a `modeling_ui` CMake target once there are enough files to make the boundary meaningful. The executable should become the composition root and link `modeling_ui`, not own UI implementation directly.

### UI-002: `MainWindow` is already more than a shell

Severity: High

Domain: UI architecture.

Evidence:
- `src/MainWindow.h:16-22` stores direct pointers to the viewport and status labels.
- `src/MainWindow.cpp:20-57` constructs the viewport, menus, inspector dock, status labels, and renderer signal wiring directly in the constructor.
- `src/MainWindow.cpp:108-153` builds the inspector panel inline and wires panel control directly to the viewport.
- `src/main.cpp:13` constructs `MainWindow` without an explicit `UiContext` or application-owned services.

Violated target rules:
- `agents/architecture/ui.md` section 1.5 requires explicit composition root and service injection through `UiContext` or `PanelContext`.
- `agents/architecture/ui.md` section 1.6 says `QMainWindow` is a shell for menus, toolbars, central areas, docks, status UI, workspace save/restore, and lifecycle.
- `agents/architecture/ui.md` sections 1.18 and 1.19 require panels to communicate through actions, services, models, and document events instead of direct panel-to-widget calls.

Practical risk:
- As soon as document state, command history, import/export, tools, or property editing are added, `MainWindow` will become the application brain.
- Inline panel construction makes panel smoke tests and panel reuse difficult.
- Direct `MainWindow` to `BgfxWidget` connections encourage future features to bypass action and service boundaries.

Recommended correction:
- Define a non-owning `UiContext` owned by the app composition root:
  - `ActionRegistry& actions`
  - `SettingsService& settings`
  - `NotificationService& notifications`
  - future `DocumentProvider&`, `CommandHistory&`, `ToolManager&`, and viewport/render interfaces
- Refactor `MainWindow` into shell methods:
  - `build_menus()`
  - `build_toolbars()`
  - `build_central_area()`
  - `build_dock_hosts()`
  - `connect_lifecycle()`
  - `restore_workspace()`
  - `save_workspace()`
- Move inspector construction into a `panels/InspectorPanel` or placeholder panel owned by a `PanelHost`.

### UI-003: Canonical action system is missing

Severity: High

Domain: UI architecture.

Evidence:
- `src/MainWindow.cpp:59-105` creates menu actions inline.
- `src/MainWindow.cpp:62-82` adds user-visible actions for New Scene, Open, Undo, Redo, Duplicate, tools, and creation commands with no canonical IDs, no command dispatch, and no action-state logic.
- `src/MainWindow.cpp:89-94` creates a local `Four Viewports` action, assigns `F1`, and connects it directly to `BgfxWidget::setQuadViewportEnabled`.
- Search found no `ActionRegistry`, `ActionId`, or `ActionStateUpdater` implementation.

Violated target rules:
- `agents/architecture/ui.md` section 1.7 requires `ActionRegistry` to own canonical `QAction` objects by stable action ID.
- `agents/architecture/ui.md` section 1.37 requires undo/redo UI to derive enabled/text state from command history.
- `agents/architecture/ui.md` section 1.39 requires keyboard shortcuts to be centralized.

Practical risk:
- Menus, toolbars, context menus, and shortcuts will drift because each UI surface can create its own action and behavior.
- Inert actions are currently enabled by default, which creates misleading UI state.
- Direct viewport action wiring bypasses future action-state derivation and workspace/settings persistence.

Recommended correction:
- Add `ActionId` and `ActionRegistry` before adding more user-visible commands.
- Register initial actions:
  - `NewScene`, `OpenScene`, `Exit`
  - `Undo`, `Redo`, `DuplicateSelection`
  - `SelectTool`, `MoveTool`, `RotateTool`, `ScaleTool`
  - `CreateCube`, `CreateLight`, `CreateCamera`
  - `ViewPerspective`, `ViewShaded`, `ToggleQuadViewport`
  - `ResetWorkspaceLayout`
- Add `ActionStateUpdater` that consumes a small editor state snapshot. Until document/commands exist, unavailable document actions should be disabled rather than inert.
- Menus, future toolbars, context menus, and shortcuts should all reuse `ActionRegistry::action(ActionId)`.

### UI-004: Workspace, settings, and panel hosting are incomplete

Severity: High

Domain: UI architecture.

Evidence:
- `src/main.cpp:9-10` sets application metadata, but search found no `QSettings`, `SettingsService`, `saveGeometry`, `saveState`, `restoreGeometry`, or `restoreState` usage.
- `src/MainWindow.cpp:31-35` creates one dock directly in `MainWindow`; there is no `PanelHost`.
- `src/MainWindow.cpp:104-105` adds `Reset Layout` as an inert menu action.
- `src/BgfxWidget.h:178-179` stores quad-viewport split fractions only in memory.

Violated target rules:
- `agents/architecture/ui.md` section 1.26 requires one settings service wrapping `QSettings`.
- `agents/architecture/ui.md` section 1.27 requires workspace/dock management behind a small panel host abstraction, stable panel IDs, versioned workspace state, and default workspace reset.
- `agents/architecture/ui.md` section 1.20 separates UI state such as current workspace layout from document state.

Practical risk:
- Users lose dock state, window geometry, viewport layout mode, viewport split positions, and panel-local state on every run.
- The inactive Reset Layout action cannot be implemented cleanly without a central workspace owner.
- Future panels may invent independent settings keys.

Recommended correction:
- Add `SettingsService` with stable, versioned keys:
  - `workspace/v1/main_window/geometry`
  - `workspace/v1/main_window/state`
  - `workspace/v1/panels/<panel_id>/...`
  - `workspace/v1/viewport/layout`
  - `workspace/v1/viewport/quad_split_vertical`
  - `workspace/v1/viewport/quad_split_horizontal`
- Add `PanelId` and `PanelHost`; panel host owns dock creation and stable object names.
- `MainWindow` should call `restore_workspace()` after shell construction and `save_workspace()` during close/lifecycle.
- Wire `ResetWorkspaceLayout` action through the panel/workspace service, not through direct dock lookup.

### UI-005: Inspector uses item widgets and stores presentation truth

Severity: High

Domain: UI model/view and panel architecture.

Evidence:
- `src/MainWindow.cpp:117-127` creates a `QTreeWidget` and hard-coded `QTreeWidgetItem` rows for Selection and Renderer.
- `src/MainWindow.cpp:129-143` creates hard-coded property-like rows using labels and checkboxes.
- `src/MainWindow.cpp:147-150` connects the `Rotate cube` checkbox directly to `BgfxWidget::setAnimationEnabled`.
- Search found no `QAbstractItemModel`, custom roles, delegates, property descriptors, or selection adapter.

Violated target rules:
- `agents/architecture/ui.md` sections 1.9 and 1.10 require Qt model/view for scalable document-backed data and state that Qt item models are presentation adapters, not domain models.
- `agents/architecture/ui.md` section 1.13 requires editable document-backed models to dispatch commands.
- `agents/architecture/ui.md` section 1.14 requires document selection to be the source of truth with explicit selection synchronization.
- `agents/architecture/ui.md` sections 1.18 and 1.19 require panels to be componentized and avoid direct panel-to-panel/widget calls.

Practical risk:
- The inspector will become a duplicate source of truth for selection, object properties, renderer status, and settings.
- Future property edits cannot participate in undo/redo if they are wired directly from widgets.
- `QTreeWidget` will become hard to keep in sync with document events, filtering, sorting, selection, and model tests.

Recommended correction:
- Replace the inline inspector with a panel class:
  - `src/ui/panels/inspector_panel.*` or split into `ScenePanel` and `PropertiesPanel` when real document data exists.
- For document data, use model/view:
  - `DocumentItemModel` for scene/object rows.
  - `PropertyItemModel` with `PropertyDescriptor` rows for editable properties.
  - Named roles for domain IDs, kind, visibility, locked state, dirty state, and tooltips.
- Route edits through commands once command infrastructure exists.
- Treat renderer status and animation toggle as UI/render settings, not document object properties.

### UI-006: Viewport input is not separated from Qt events or tool routing

Severity: High

Domain: UI viewport architecture.

Evidence:
- `src/BgfxWidget.h:34-45` overrides Qt show, resize, paint, mouse, wheel, key, and event-filter methods directly in the viewport widget.
- `src/BgfxWidget.cpp:798-984` handles mouse navigation, fly controls, wheel zoom, and key state directly from Qt event classes.
- `src/BgfxWidget.cpp:987-1030` uses an event filter on splitter child widgets for viewport split manipulation.
- `src/BgfxWidget.cpp:1643-1847` owns navigation state machine behavior directly in the widget.
- Search found no `ViewportController`, toolkit-neutral `EditorInputEvent`, tool manager, or input routing boundary.

Violated target rules:
- `agents/architecture/ui.md` section 1.21 says viewport widgets are high-frequency UI and should be split into widget, controller, tool manager, and renderer host roles.
- `agents/architecture/ui.md` section 1.22 requires Qt events to be translated into toolkit-neutral editor input events.
- `agents/architecture/ui.md` section 1.36 requires high-frequency input to avoid heavy work, broad refresh, and direct document mutation.
- `agents/architecture/architecture.md` section 9 says tools are state machines that convert input into previews and commands.

Practical risk:
- Future tools will either depend on Qt event classes or be bolted into `BgfxWidget`.
- Camera, splitter, navigation, picking, preview, and tool state will compete inside one high-frequency widget.
- Input behavior is difficult to unit test without constructing a QWidget and a bgfx surface.

Recommended correction:
- Split viewport responsibilities:
  - `ViewportWidget`: Qt native child surface, focus, mouse tracking, resize, and minimal event forwarding.
  - `ViewportController`: converts Qt events to `EditorInputEvent`, manages active viewport pane, coalesces repaint requests.
  - `ViewportCameraController`: owns camera/navigation UI state until a broader tool/camera service exists.
  - `ViewportLayoutState`: owns single/quad mode and split fractions; persists through `SettingsService`.
- Keep Qt event types out of tools and non-widget viewport logic.
- Ensure high-frequency mouse move produces preview/navigation updates and coalesced redraws, not direct document mutation.

### UI-007: Cross-domain concern - bgfx backend ownership is inside a QWidget

Severity: High

Domain: Cross-domain concern. Renderer architect owns the backend/snapshot design; UI owns the viewport host contract.

Evidence:
- `src/BgfxWidget.h:10` includes `<bgfx/bgfx.h>` in a QWidget-derived public header.
- `src/BgfxWidget.h:143-164` stores bgfx layouts, buffers, programs, and uniforms as widget members.
- `src/BgfxWidget.cpp:1033-1149` initializes bgfx, creates buffers/programs/uniforms, and loads shaders inside the widget.
- `src/BgfxWidget.cpp:1259-1440` builds render passes and submits cube/grid draw calls inside the widget.
- `src/BgfxWidget.cpp:1443-1464` loads shader files through the widget.

Violated target rules:
- `agents/architecture/ui.md` section 1.23 requires the rendering backend to stay behind a viewport host boundary.
- `agents/architecture/ui.md` section 1.2 says UI must not call concrete renderer internals except through a viewport/render-host boundary.
- `agents/architecture/architecture.md` sections 3.9 and 11.1 require renderers to consume snapshots and not own editor truth.

Practical risk:
- UI headers require bgfx and expose backend details to any code that includes the viewport widget.
- Renderer backend changes will force UI recompilation and UI design changes.
- Render tests and viewport UI tests cannot be separated.

Recommended correction:
- UI-side requirement: define a narrow render-host interface used by `ViewportWidget`/`ViewportController`, such as `IViewportRenderHost` or `ViewportRenderHost`.
- Renderer-side requirement: move bgfx resource ownership, shader loading, render passes, and frame submission into renderer/backend code planned by the renderer architect.
- UI should pass native surface data, pixel size, device pixel ratio, viewport layout/camera state or render-view requests, and receive renderer status/errors through typed callbacks or services.
- Do not include bgfx headers from public UI widget headers after the split.

### UI-008: Notification and diagnostics presentation is ad hoc

Severity: Medium

Domain: UI services.

Evidence:
- `src/MainWindow.cpp:37-55` stores renderer/status labels directly and updates them from `BgfxWidget` signals.
- `src/BgfxWidget.h:28-32` exposes renderer status/error signals as raw strings.
- `src/BgfxWidget.cpp:1035-1056` emits renderer errors that are displayed only in the status bar by `MainWindow`.
- `src/BgfxWidget.cpp:1363-1368` emits formatted stats strings from the rendering widget.

Violated target rules:
- `agents/architecture/ui.md` section 1.31 requires notification levels and a UI service for status, warnings, blocking decisions, diagnostics, and developer-only debug.
- `agents/architecture/ui.md` section 1.34 says signals and slots should remain typed and meaningful, not become broad hidden workflows.
- `agents/architecture/architecture.md` section 21.4 says UI translates errors into user messages and should not invent low-level diagnostics.

Practical risk:
- Important renderer failures can be overwritten by routine status messages.
- Diagnostics cannot be copied, expanded, categorized, or routed to a future diagnostics panel.
- Formatted status strings couple producer and presenter.

Recommended correction:
- Add `NotificationService` with typed APIs:
  - `show_transient_status(QString message, int timeout_ms)`
  - `show_warning(NotificationMessage message)`
  - `show_error(NotificationMessage message)`
  - future `show_diagnostics(DiagnosticReport report)`
- Add a small status presenter owned by `MainWindow` that binds the service to the status bar and labels.
- Prefer typed render status data over preformatted stats strings when the renderer boundary is split.

### UI-009: Fusion is applied, but style policy and metrics are not centralized

Severity: Low

Domain: UI style and layout consistency.

Evidence:
- `src/main.cpp:11` calls `QApplication::setStyle(QStyleFactory::create("Fusion"))` directly.
- `src/MainWindow.cpp:114-115` hard-codes panel margins and spacing.
- `src/BgfxWidget.cpp:36-64` hard-codes splitter and viewport constants in the widget implementation.
- `src/BgfxWidget.cpp:551-580` implements a custom splitter handle widget, though it does use palette and `QStyle::CE_Splitter`.

Violated or incomplete target rules:
- `agents/architecture/ui.md` section 1.1 recommends a startup/bootstrap style policy with a Fusion null check.
- `agents/architecture/ui.md` sections 1.28 through 1.30 call for centralized metrics, icon/text conventions, and conservative custom styling.

Practical risk:
- As panels are added, each panel will likely invent margins, spacing, icon sizes, and text conventions.
- Custom viewport splitters may diverge from standard Qt splitter behavior unless the interaction is kept deliberately viewport-local.

Recommended correction:
- Add `fusion_style_policy.*` to apply Fusion with a null check.
- Add `ui_metrics.*` with baseline margins, spacing, section spacing, small icon size, and toolbar icon size.
- Keep the viewport splitter custom only if standard `QSplitter` cannot express the rendering-pane interaction; otherwise reuse Qt splitters where user-resizable UI areas are ordinary widgets.

### UI-010: UI test and architecture guardrail coverage is absent

Severity: Medium

Domain: UI testing and dependency checks.

Evidence:
- `CMakeLists.txt` contains no `enable_testing`, `Qt6::Test`, `QAbstractItemModelTester`, or test target setup.
- The frozen source layout contains no `tests/` directory.
- Search found no action-state tests, panel smoke tests, settings-key tests, model tests, or dependency-boundary checks.

Violated target rules:
- `agents/architecture/ui.md` section 1.40 requires model tests, action-state tests, settings-key tests, panel smoke tests, selection sync tests, and dependency-boundary checks.
- `agents/architecture/architecture.md` section 16 requires a test pyramid and guardrails.
- `agents/architecture/architecture.md` section 19.7 recommends executable guardrails for architectural rules.

Practical risk:
- Future UI refactors will rely on manual testing.
- The intended lower-layer no-Qt-Widgets rule and UI no-backend-header rule will remain documentation-only.
- Item model and action-state bugs will surface late, after panels become complex.

Recommended correction:
- Add UI tests as soon as the first UI services/models/actions are introduced:
  - `ActionRegistry` canonical action and shortcut tests.
  - `ActionStateUpdater` tests from fake editor-state snapshots.
  - `SettingsService` key/version tests.
  - `MainWindow` smoke test with fake services.
  - Panel construction smoke tests.
  - `QAbstractItemModelTester` coverage for each custom model.
  - Viewport controller event-translation tests without bgfx.
- Add a dependency-boundary check that rejects Qt Widgets includes from non-UI targets and bgfx backend includes from public UI headers once targets exist.

### UI-011: Cross-domain concern - command/document/tool services required by the UI do not exist yet

Severity: High

Domain: Cross-domain concern. Software architect owns document, command, tool, and application service contracts; UI consumes them.

Evidence:
- The frozen source layout has no `src/document`, `src/commands`, `src/tools`, or app-service modules.
- `src/MainWindow.cpp:62-82` exposes future document/tool/create actions without command or tool-manager backing.
- `src/MainWindow.cpp:129-143` displays property-like UI without document-backed descriptors or command dispatch.

Violated target rules:
- `agents/architecture/architecture.md` sections 1.3, 3.6, 3.7, and 3.8 require one mutation path through documents, commands, and tools.
- `agents/architecture/ui.md` sections 1.7, 1.13, 1.14, and 1.37 depend on document, command history, selection, and tool state snapshots.

Practical risk:
- The UI cannot fully satisfy action state, undo/redo, property editing, or selection synchronization until those lower-layer contracts exist.
- Adding UI behavior now without these contracts would encourage direct widget or viewport mutation paths.

Recommended correction:
- Treat document/command/tool contracts as prerequisites for real document-backed UI behavior.
- UI may create placeholder actions and panels, but unavailable actions should be disabled until command services are present.
- Once the software plan defines the service APIs, UI should consume them only through `UiContext` and panel/model contexts.

## Suggested UI Implementation Batches

These batches are UI recommendations for the later master audit. They are not implementation authority until accepted by the consolidated report.

### Batch 1: UI foundation and shell boundaries

- Create `src/ui/qt_app`, `src/ui/actions`, `src/ui/services`, `src/ui/panels`, and `src/ui/viewport` directories.
- Add `qt_application_bootstrap` for app metadata and Fusion style policy.
- Add `UiContext` with explicit non-owning service references.
- Add `SettingsService`, `NotificationService`, `ActionId`, `ActionRegistry`, and initial `ActionStateUpdater`.
- Convert `MainWindow` into a shell that receives `UiContext&`.
- Keep this batch behavior-preserving except for disabling currently inert actions.

### Batch 2: Actions, menus, toolbar, and workspace state

- Register all current menu actions through `ActionRegistry`.
- Build menus and the first toolbar from canonical actions.
- Add `PanelHost` and `PanelId`; move inspector dock creation into panel host.
- Implement versioned `QMainWindow::saveGeometry/saveState` and restore through `SettingsService`.
- Implement `ResetWorkspaceLayout`.
- Persist viewport layout mode and split fractions as UI workspace state.

### Batch 3: Viewport UI bridge

- Rename/split `BgfxWidget` responsibilities into:
  - `ViewportWidget`
  - `ViewportController`
  - `ViewportLayoutState`
  - `ViewportCameraController`
  - UI-facing `ViewportRenderHost` interface
- Translate Qt mouse/wheel/key events into toolkit-neutral editor input events.
- Keep camera/navigation state testable without bgfx.
- Coordinate with the renderer audit before moving bgfx ownership; UI should define only the host contract and widget lifetime rules.

### Batch 4: Panels and model/view adapters

- Replace inline inspector creation with an `InspectorPanel` or `PropertiesPanel`.
- Add `PropertyDescriptor` and `PropertyItemModel` when document-backed properties exist.
- Add `DocumentItemModel` only when real document hierarchy exists.
- Add selection adapters when document selection exists.
- Route document-backed edits through commands; keep purely UI/render settings separate.

### Batch 5: Tests and guardrails

- Add a Qt test target after Batch 1 introduces testable UI services.
- Add action registry/state tests, settings key/version tests, and main-window/panel smoke tests.
- Add model tests with `QAbstractItemModelTester` as soon as custom models exist.
- Add viewport controller event-translation tests.
- Add CMake or script guardrails for forbidden Qt Widgets and bgfx backend includes once module targets exist.

## UI Approval Loop Note

After the master audit is produced and any approved UI implementation plan is implemented, return the implementation to the UI architect for review against:
- `agents/architecture/ui.md`
- `agents/architecture/architecture.md`
- the accepted master audit findings
- any UI implementation plan derived from this audit

If the implementation is correct, the UI architect can approve it and any corresponding UI plan file can be deleted. Otherwise, the UI architect should update the plan/report with the required corrections and repeat the review loop.
