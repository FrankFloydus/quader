# Implementation Plan: Wave 1 A3 Foundation, Document, Command, And Tool Preflight

Status: active software architecture handoff
Task board scope: `#3` assignment `A3`, with dependency preflight for `#6` and `#7`
Primary authority: `agents/plans/audit_20260704_full_architecture_master.md`
Required reads satisfied by this plan: `AGENTS.md`, `agents/workflow.md`, `agents/task_board.md`, `agents/tests-policy.md`, `project_board.md`, `agents/architecture/architecture.md`, `agents/architecture/ui.md`

This plan is not production implementation. It defines the software contracts and sequencing that implementation workers must follow after the module/test harness task `#2` lands.

## Scope

This plan covers:

- Task `#3`: foundation primitives, app composition root, and initial UI service contracts.
- Task `#6`: the mesh/math/geometry dependency contracts that must consume `#3` foundation without depending on UI, commands, tools, document, or Crimson.
- Task `#7`: document, selection, command history, undo/redo, and tool shell contracts that depend on `#3` foundation and `#6` mesh validation.

This plan does not cover:

- Implementing code.
- Editing `project_board.md`.
- Replacing the viewport or Crimson boundary work from tasks `#4` and `#5`.
- Adding topology operations beyond mesh-core creation/deletion/validation primitives.
- Adding document-backed Qt item models or property panels from task `#8`.
- Adding I/O from task `#10`.

## Current Baseline

The current source tree is still the prototype layout:

- `src/main.cpp`
- `src/MainWindow.*`
- `src/BgfxWidget.*`

The current `MainWindow` creates menus inline. Document/tool actions exist as enabled placeholders even though no document, command history, or tool manager exists. The current viewport layout action and cube animation checkbox are prototype viewport/UI behavior and are not document mutation.

The only active architecture authority beyond the architecture docs is:

- `agents/plans/audit_20260704_full_architecture_master.md`

No focused A3 plan existed before this file.

## Hard Dependency Gates

### Gate 1: Task #2 Must Land Before Task #3 Implementation

Task `#3` implementation must not start until task `#2` has produced:

- CMake module targets, at minimum:
  - `modeling_foundation`
  - `modeling_math`
  - `modeling_geometry`
  - `modeling_mesh_core`
  - `modeling_mesh_ops`
  - `modeling_document`
  - `modeling_commands`
  - `modeling_tools`
  - `crimson`
  - `modeling_ui`
  - `QuaderQtBgfx`
- CTest enabled through a documented test preset.
- Architecture boundary checks included in the normal test preset.
- Documentation updates for configure, build, and test commands.

Task `#3` can be planned before task `#2`, but the implementation must use the target names and test command established by `#2`. If `#2` changes target names from the list above, return to software architecture before implementing `#3`.

### Gate 2: Task #3 Unlocks Task #6 Foundation Use

Task `#6` mesh implementation can start only after task `#3` provides:

- Stable foundation ID primitives for mesh IDs.
- Stable `Result` and diagnostic/error helpers for validation and mesh edit failures.
- Assertion/logging hooks usable from lower layers without Qt or app-service dependencies.
- Confirmed module dependency direction through CMake and architecture checks.

Task `#6` must not import UI, command, tool, document, app, or Crimson headers.

### Gate 3: Task #7 Requires Tasks #2, #3, And #6

Task `#7` implementation can start only after:

- Task `#2` is approved and its test/boundary harness is available.
- Task `#3` is approved and the foundation/app/UI service contracts are stable.
- Task `#6` is approved and mesh core validation plus fixtures are available.

Task `#7` must not compensate for missing `#6` behavior by creating a second mesh representation inside `document` or `commands`.

## Dependency Direction

Workers must preserve this dependency chain:

```text
modeling_foundation
  <- modeling_math
  <- modeling_geometry
  <- modeling_mesh_core
  <- modeling_mesh_ops
  <- modeling_document
  <- modeling_commands
  <- modeling_tools
```

UI may depend on document, commands, tools, and Crimson-facing interfaces. Lower layers must not include Qt Widgets headers.

Forbidden for these tasks:

- `foundation` depending on anything project-specific.
- `math` depending on mesh, document, commands, tools, UI, app, or Crimson.
- `geometry` depending on mesh, document, commands, tools, UI, app, or Crimson.
- `mesh` depending on document, commands, tools, UI, app, or Crimson.
- `document` depending on commands, tools, UI, app, or Crimson GPU/runtime internals.
- `commands` depending on UI, app, or Crimson.
- `tools` depending on Qt event classes.
- UI making document mutation outside commands once task `#7` exists.

## Task #3 Contracts

### Foundation Module

Target: `modeling_foundation`

Recommended files:

- `src/foundation/id.hpp`
- `src/foundation/result.hpp`
- `src/foundation/diagnostic.hpp`
- `src/foundation/assert.hpp`
- `src/foundation/logging.hpp`

Namespace:

- `quader::foundation`

Required types:

```cpp
using IdIndex = std::uint32_t;
using IdGeneration = std::uint32_t;

constexpr IdIndex kInvalidIdIndex = std::numeric_limits<IdIndex>::max();

template <class Tag>
class Id;

template <class Tag>
class GenerationalId;

template <class T, class E>
class Result;

template <class E>
class Result<void, E>;

enum class Severity {
    info,
    warning,
    error,
};

struct Diagnostic {
    Severity severity;
    std::string message;
};

class DiagnosticList;
```

`Id<Tag>` rules:

- Stores one `IdIndex`.
- Default constructed value is invalid.
- Has `is_valid()`, `index()`, equality, inequality, and ordering if needed for deterministic containers.
- Provides hashing support without exposing raw integer IDs across module APIs.

`GenerationalId<Tag>` rules:

- Stores `IdIndex index` and `IdGeneration generation`.
- Default constructed value is invalid.
- Has `is_valid()`, equality, inequality, and hashing support.
- Is the preferred base for mesh element IDs and document object IDs.

`Result<T, E>` rules:

- Use for expected failures: invalid IDs, validation failure, unsupported boundary case, malformed input.
- Must not pull in Qt.
- Must not use exceptions for normal edit rejection.
- Must provide clear `has_value()`, `value()`, `error()`, and `operator bool()` style access.
- `E` should be a strongly typed error object or enum, not an unstructured string alone.

`DiagnosticList` rules:

- Stores zero or more `Diagnostic` values.
- Has `empty()`, `has_errors()`, and iterator access.
- May be used by validation, import/export later, and command results.

Assertion/logging rules:

- Provide `QUADER_ASSERT(condition)` or equivalent for programmer errors.
- Provide a minimal logging interface that does not depend on Qt.
- Logging must be replaceable later by app services, but foundation must not know about app services.

Do not add a broad `common.hpp` or `utils.hpp`.

### App Composition Root

Target: executable/app-facing code after `#2` split.

Recommended files:

- `src/app/application.hpp`
- `src/app/application.cpp`
- `src/app/application_config.hpp`
- `src/app/app_services.hpp`

Namespace:

- `quader::app`

Required classes:

```cpp
struct ApplicationConfig {
    std::string application_name;
    std::string organization_name;
};

class Application final {
public:
    Application(QApplication& qt_app, ApplicationConfig config);
    ~Application();

    int run();

private:
    QApplication& qt_app_;
    ApplicationConfig config_;
    AppServices services_;
    std::unique_ptr<quader::ui::MainWindow> main_window_;
};

struct AppServices {
    // Owns services with clear lifetime.
};
```

Composition rules:

- `main.cpp` should become thin: construct `QApplication`, construct `Application`, call `run()`.
- `Application` owns long-lived services and the main window.
- `Application` applies startup policy before constructing service users.
- No service may discover dependencies through global top-level widget lookup.
- No singleton service locator.

Qt lifetime rule:

- `QApplication` may still be constructed in `main.cpp`, because Qt expects it early.
- `Application` receives `QApplication&` and must not construct a second Qt application object.

### Qt Bootstrap And Style Policy

Recommended files:

- `src/ui/qt_app/qt_application_bootstrap.hpp`
- `src/ui/qt_app/qt_application_bootstrap.cpp`
- `src/ui/style/fusion_style_policy.hpp`
- `src/ui/style/fusion_style_policy.cpp`
- `src/ui/style/ui_metrics.hpp`

Namespace:

- `quader::ui`

Required functions/classes:

```cpp
struct QtApplicationMetadata {
    QString application_name;
    QString organization_name;
};

void apply_qt_application_metadata(QApplication& app,
                                   const QtApplicationMetadata& metadata);

class FusionStylePolicy final {
public:
    bool apply(QApplication& app) const;
};

struct UiMetrics {
    int margin = 6;
    int spacing = 8;
    int section_spacing = 8;
    int small_icon = 16;
    int toolbar_icon = 24;
};
```

Rules:

- Set organization/application metadata before `QSettings` is created.
- Apply Fusion only if `QStyleFactory::create("Fusion")` returns a non-null style.
- Do not add a global custom stylesheet or custom `QStyle`.
- Keep metrics centralized, even if only a few constants exist in V1.

### UI Context And Editor State

Recommended files:

- `src/ui/services/ui_context.hpp`
- `src/ui/actions/editor_state_snapshot.hpp`

Namespace:

- `quader::ui`

Required types:

```cpp
struct EditorStateSnapshot {
    bool has_document = false;
    bool has_selection = false;
    bool can_undo = false;
    bool can_redo = false;
    bool tools_available = false;
};

class IEditorStateSource {
public:
    virtual ~IEditorStateSource() = default;
    virtual EditorStateSnapshot editor_state() const = 0;
};

class NullEditorStateSource final : public IEditorStateSource {
public:
    EditorStateSnapshot editor_state() const override;
};

struct UiContext {
    ActionRegistry& actions;
    IEditorStateSource& editor_state;
    SettingsService& settings;
    NotificationService& notifications;
};
```

Reasoning:

- Task `#3` does not yet own `Document`, `CommandHistory`, or `ToolManager`.
- `UiContext` must not force fake document/command/tool services into existence.
- `NullEditorStateSource` lets `ActionStateUpdater` disable document/tool actions until task `#7`.
- Task `#7` will replace or extend the editor state source with an adapter over real document, command, and tool services.

The UI architect may add `PanelContext` or refine the location of these files, but must preserve the rule that lower layers do not depend on Qt Widgets and that `#3` does not invent document truth.

### Actions

Recommended files:

- `src/ui/actions/action_id.hpp`
- `src/ui/actions/action_registry.hpp`
- `src/ui/actions/action_registry.cpp`
- `src/ui/actions/action_state_updater.hpp`
- `src/ui/actions/action_state_updater.cpp`

Namespace:

- `quader::ui`

Required types:

```cpp
enum class ActionId {
    NewScene,
    Open,
    Save,
    SaveAs,
    Exit,
    Undo,
    Redo,
    DuplicateSelection,
    SelectTool,
    MoveTool,
    RotateTool,
    ScaleTool,
    CreateCube,
    CreateLight,
    CreateCamera,
    ToggleFourViewports,
    ResetLayout,
    FocusViewport,
};

struct ActionSpec {
    QString text;
    QKeySequence shortcut;
    bool checkable = false;
    bool initially_enabled = true;
};

class ActionRegistry final : public QObject {
    Q_OBJECT

public:
    explicit ActionRegistry(QObject* parent = nullptr);

    QAction& register_action(ActionId id, ActionSpec spec);
    QAction& action(ActionId id);
    const QAction& action(ActionId id) const;
};

class ActionStateUpdater final {
public:
    explicit ActionStateUpdater(ActionRegistry& actions,
                                IEditorStateSource& state_source);

    void refresh();
};
```

Initial action state in task `#3`:

- Enabled:
  - `Exit`
  - `ToggleFourViewports`
  - `FocusViewport`, if implemented as viewport focus only
  - `ResetLayout`, only if `PanelHost` supports default layout reset in `#3`
- Disabled until task `#7` or later:
  - `NewScene`
  - `Open`
  - `Save`
  - `SaveAs`
  - `Undo`
  - `Redo`
  - `DuplicateSelection`
  - `SelectTool`
  - `MoveTool`
  - `RotateTool`
  - `ScaleTool`
  - `CreateCube`
  - `CreateLight`
  - `CreateCamera`

`Open`, `Save`, and `SaveAs` may remain disabled until I/O task `#10` unless task `#7` implements only in-memory new-document behavior.

Rules:

- Menus, toolbars, and future context menus must reuse the same `QAction` objects.
- Domain code must not include `QAction`.
- `ActionStateUpdater` owns enabled/checked/text refresh logic.
- Trigger handlers in `MainWindow` must be thin wiring, not business logic.

### Settings And Notifications

Recommended files:

- `src/ui/services/settings_service.hpp`
- `src/ui/services/settings_service.cpp`
- `src/ui/services/notification_service.hpp`
- `src/ui/services/notification_service.cpp`

Namespace:

- `quader::ui`

Required types:

```cpp
enum class SettingsKey {
    MainWindowGeometry,
    MainWindowState,
};

class SettingsService final {
public:
    explicit SettingsService(QSettings& settings);

    QByteArray main_window_geometry() const;
    void set_main_window_geometry(QByteArray value);

    QByteArray main_window_state(int version) const;
    void set_main_window_state(int version, QByteArray value);

    QVariant value(SettingsKey key, QVariant fallback = {}) const;
    void set_value(SettingsKey key, QVariant value);
};

enum class NotificationLevel {
    status,
    warning,
    error,
};

struct Notification {
    NotificationLevel level;
    QString message;
    std::optional<int> timeout_ms;
};

class NotificationService final : public QObject {
    Q_OBJECT

public:
    explicit NotificationService(QObject* parent = nullptr);
    void post(Notification notification);

signals:
    void notification_posted(const Notification& notification);
};
```

Rules:

- `SettingsService` wraps `QSettings`; widgets must not invent ad hoc keys.
- Workspace state must be versioned.
- Do not store document data in UI settings.
- `NotificationService` is presentation-neutral enough for tests; `MainWindow` decides status bar display.
- Errors from future commands/importers should be translated to notifications at the UI boundary, not inside low-level code.

### Panel Hosting And Main Window Shell

Recommended files:

- `src/ui/qt_app/main_window.hpp`
- `src/ui/qt_app/main_window.cpp`
- `src/ui/panels/panel_id.hpp`
- `src/ui/panels/panel_contract.hpp`
- `src/ui/panels/panel_host.hpp`
- `src/ui/panels/panel_host.cpp`

Namespace:

- `quader::ui`

Required types:

```cpp
enum class PanelId {
    Inspector,
};

class IPanelWidget {
public:
    virtual ~IPanelWidget() = default;
    virtual PanelId panel_id() const noexcept = 0;
    virtual QString title() const = 0;
    virtual QWidget* widget() = 0;
    virtual void on_editor_state_changed(const EditorStateSnapshot& state) = 0;
};

class PanelHost final : public QObject {
    Q_OBJECT

public:
    PanelHost(QMainWindow& main_window, SettingsService& settings);

    QDockWidget& add_dock_panel(PanelId id,
                                QString object_name,
                                QString title,
                                QWidget* widget,
                                Qt::DockWidgetArea default_area);

    void restore_workspace();
    void save_workspace();
    void reset_workspace();
};

class MainWindow final : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(UiContext& context, QWidget* parent = nullptr);

private:
    void build_menus();
    void build_central_area();
    void build_dock_hosts();
    void connect_lifecycle();
    void restore_workspace();
    void save_workspace();

    UiContext& context_;
};
```

Rules:

- `MainWindow` is a shell.
- `MainWindow` receives `UiContext&`; it must not construct app services.
- Existing inspector placeholder may remain prototype-only, but must not become document truth.
- Saved dock/widget state must use stable `objectName` values.
- Panels must communicate through actions, services, document events, or models later, not direct panel-to-panel pointers.

## Task #6 Mesh Core Contracts

Target dependencies:

```text
modeling_mesh_core -> modeling_foundation, modeling_math, modeling_geometry
modeling_mesh_ops  -> modeling_mesh_core
```

Task `#6` should consume `#3` foundation contracts but must not depend on `modeling_document`, `modeling_commands`, `modeling_tools`, `modeling_ui`, `app`, or `crimson`.

Recommended ID aliases:

```cpp
namespace quader::mesh {
struct VertexTag;
struct HalfedgeTag;
struct EdgeTag;
struct FaceTag;

using VertexId = foundation::GenerationalId<VertexTag>;
using HalfedgeId = foundation::GenerationalId<HalfedgeTag>;
using EdgeId = foundation::GenerationalId<EdgeTag>;
using FaceId = foundation::GenerationalId<FaceTag>;
}
```

Recommended validation result:

```cpp
namespace quader::mesh {
enum class MeshValidationCode {
    InvalidId,
    BrokenOppositePair,
    BrokenNextPrevCycle,
    FaceCycleMismatch,
    VertexReferenceMismatch,
    DegenerateFace,
};

struct MeshValidationIssue {
    MeshValidationCode code;
    foundation::Diagnostic diagnostic;
};

class MeshValidationReport {
public:
    bool ok() const noexcept;
    std::span<const MeshValidationIssue> issues() const noexcept;
};

MeshValidationReport validate_mesh(const HalfEdgeMesh& mesh);
}
```

Task `#6` acceptance required before `#7`:

- `HalfEdgeMesh` can own mesh records through stable generational IDs.
- Creation/deletion primitives keep ID validity and generation behavior test-covered.
- Validation reports broken topology without Qt or command dependencies.
- Fixtures exist for at least:
  - empty mesh
  - single triangle or quad
  - invalid ID reference
  - boundary placeholder case, if boundary representation is not complete yet
- Mesh tests cover invalid IDs, record deletion/reuse, and basic topology invariants.
- Mesh validation can be run by document/command tests in task `#7`.

## Task #7 Document, Command, And Tool Contracts

Target dependencies:

```text
modeling_document -> modeling_foundation, modeling_math, modeling_geometry, modeling_mesh_core
modeling_commands -> modeling_foundation, modeling_document, modeling_mesh_ops
modeling_tools    -> modeling_foundation, modeling_document, modeling_commands, modeling_math, modeling_geometry
modeling_ui       -> modeling_document, modeling_commands, modeling_tools through UI adapters only
```

### Document

Recommended files:

- `src/document/document.hpp`
- `src/document/document.cpp`
- `src/document/document_error.hpp`
- `src/document/object_id.hpp`
- `src/document/scene_object.hpp`
- `src/document/object_store.hpp`
- `src/document/selection.hpp`
- `src/document/document_events.hpp`
- `src/document/transform.hpp`

Namespace:

- `quader::document`

Required types:

```cpp
struct ObjectTag;
using ObjectId = foundation::GenerationalId<ObjectTag>;

struct Transform {
    math::Vec3 translation;
    math::Vec3 rotation_euler;
    math::Vec3 scale;
};

enum class SceneObjectKind {
    Mesh,
};

struct MeshObject {
    ObjectId id;
    std::string name;
    Transform transform;
    mesh::HalfEdgeMesh mesh;
};

struct ComponentRef {
    ObjectId object;
    std::variant<mesh::VertexId, mesh::EdgeId, mesh::FaceId> component;
};

class Selection {
public:
    bool empty() const noexcept;
    void clear();
    foundation::Result<void, DocumentError> set_objects(std::vector<ObjectId> objects);
    foundation::Result<void, DocumentError> set_components(std::vector<ComponentRef> components);
};

enum class DocumentChangeKind {
    ObjectCreated,
    ObjectDeleted,
    ObjectRenamed,
    TransformChanged,
    MeshTopologyChanged,
    MeshGeometryChanged,
    SelectionChanged,
    DirtyStateChanged,
};

struct DocumentChange {
    DocumentChangeKind kind;
    ObjectId object;
};

class Document {
public:
    foundation::Result<ObjectId, DocumentError> create_mesh_object(std::string name,
                                                                   mesh::HalfEdgeMesh mesh,
                                                                   Transform transform = {});
    foundation::Result<void, DocumentError> delete_object(ObjectId id);
    foundation::Result<void, DocumentError> rename_object(ObjectId id, std::string name);
    foundation::Result<void, DocumentError> set_transform(ObjectId id, Transform transform);

    MeshObject* find_mesh_object(ObjectId id);
    const MeshObject* find_mesh_object(ObjectId id) const;

    const Selection& selection() const noexcept;
    foundation::Result<void, DocumentError> set_selection(Selection selection);
    void clear_selection();

    bool is_dirty() const noexcept;
    void clear_dirty();

    std::vector<DocumentChange> take_pending_changes();
};
```

Document rules:

- `Document` owns editable scene truth.
- Selection is editor state owned by `Document`, not by mesh or UI widgets.
- Every mutating public API validates IDs and emits typed document changes.
- `Document` remains single-threaded.
- `Document` must not know about `CommandHistory`, `QAction`, `QWidget`, `PanelHost`, or Crimson GPU/runtime types.
- Document mutation APIs may be called by commands; UI should not call them directly for user-visible edits once command history exists.

### Commands

Recommended files:

- `src/commands/command.hpp`
- `src/commands/command_result.hpp`
- `src/commands/command_history.hpp`
- `src/commands/command_history.cpp`
- `src/commands/document_commands.hpp`
- `src/commands/document_commands.cpp`
- `src/commands/selection_commands.hpp`
- `src/commands/selection_commands.cpp`

Namespace:

- `quader::commands`

Required types:

```cpp
enum class CommandStatus {
    Applied,
    Rejected,
    NotMergeable,
};

struct CommandResult {
    CommandStatus status;
    foundation::DiagnosticList diagnostics;

    static CommandResult applied();
    static CommandResult rejected(foundation::Diagnostic diagnostic);
    static CommandResult not_mergeable();
};

class ICommand {
public:
    virtual ~ICommand() = default;

    virtual std::string_view name() const noexcept = 0;
    virtual CommandResult execute(document::Document& document) = 0;
    virtual CommandResult undo(document::Document& document) = 0;

    virtual bool can_merge_with(const ICommand& next) const noexcept;
    virtual CommandResult merge_with(std::unique_ptr<ICommand> next);
};

class CommandHistory final {
public:
    CommandResult execute(std::unique_ptr<ICommand> command, document::Document& document);
    CommandResult undo(document::Document& document);
    CommandResult redo(document::Document& document);

    bool can_undo() const noexcept;
    bool can_redo() const noexcept;
    std::string_view undo_name() const noexcept;
    std::string_view redo_name() const noexcept;

    void clear();
};
```

Initial commands for task `#7`:

- `CreateMeshObjectCommand`
- `DeleteObjectCommand`
- `RenameObjectCommand`
- `SetObjectTransformCommand`
- `SetSelectionCommand`
- `ClearSelectionCommand`

Command rules:

- Commands are the undo boundary.
- Commands validate inputs before mutating documents.
- Commands store undo data semantically, not raw pointers.
- For early non-topology commands, before/after snapshots are acceptable.
- Commands do not open dialogs, call renderer APIs, or depend on Qt.
- Command tests must compare semantic document state, not allocation order or pointer identity.

Do not add topology-edit commands until mesh operation contracts exist beyond task `#6` primitives.

### Tools

Recommended files:

- `src/tools/tool_id.hpp`
- `src/tools/tool.hpp`
- `src/tools/tool_context.hpp`
- `src/tools/tool_manager.hpp`
- `src/tools/tool_manager.cpp`
- `src/tools/editor_input_event.hpp`
- `src/tools/tool_preview.hpp`

Namespace:

- `quader::tools`

Required types:

```cpp
enum class ToolId {
    Select,
    Move,
    Rotate,
    Scale,
};

enum class PointerButton {
    Left,
    Middle,
    Right,
};

struct PointerEvent {
    math::Vec2 position;
    PointerButton button;
    bool pressed = false;
};

struct KeyEvent {
    int key_code;
    bool pressed = false;
};

struct ToolPreview {
    // Preview-only data. Not document truth.
};

struct ToolContext {
    document::Document& document;
    commands::CommandHistory& command_history;
};

class ITool {
public:
    virtual ~ITool() = default;
    virtual ToolId id() const noexcept = 0;
    virtual void activate(ToolContext& context) = 0;
    virtual void deactivate(ToolContext& context) = 0;
    virtual void on_pointer_event(const PointerEvent& event, ToolContext& context) = 0;
    virtual void on_key_event(const KeyEvent& event, ToolContext& context) = 0;
    virtual ToolPreview preview() const = 0;
};

class ToolManager final {
public:
    explicit ToolManager(ToolContext context);

    ToolId active_tool() const noexcept;
    void set_active_tool(ToolId id);
    void dispatch_pointer_event(const PointerEvent& event);
    void dispatch_key_event(const KeyEvent& event);
};
```

Tool rules:

- Tools consume toolkit-neutral events, not Qt events.
- Tools may own hover, drag, snapping, and preview-only state.
- Tools must not permanently mutate the document except by executing commands.
- Task `#7` may implement shell tools with minimal behavior; it should not implement real transform/topology workflows until matching commands and picking exist.

### UI Integration In Task #7

Task `#7` may update the task `#3` UI services as follows:

- Add a concrete editor state source that reads `Document`, `CommandHistory`, and `ToolManager`.
- Enable `Undo` and `Redo` from `CommandHistory::can_undo()` and `can_redo()`.
- Set Undo/Redo action text from command names.
- Wire document-safe actions through `CommandHistory`.
- Wire tool actions through `ToolManager`.
- Keep file I/O actions disabled unless a real document creation or persistence flow exists.

Task `#7` must not add document-backed item models; those belong to task `#8` after document events and selection are stable.

## Test Strategy

Tests-policy applies to tasks `#3`, `#6`, and `#7`.

General rules:

- Tests must protect real behavior or architecture invariants.
- Prefer unit tests for foundation, mesh, document, commands, and tools.
- Avoid brittle UI pixel/layout tests.
- No test may depend on sleeps, wall-clock timing, network access, random order, or user-specific paths.
- If a meaningful automated test is not practical, record why and give a concrete manual verification step.

### Task #3 Tests

Required test areas:

- Foundation ID validity, equality, hash use, and generation behavior.
- `Result<T, E>` success/error behavior.
- `DiagnosticList::has_errors()`.
- `ActionRegistry` registers one canonical action per `ActionId`.
- `ActionStateUpdater` disables document/tool actions when using `NullEditorStateSource`.
- `SettingsService` key stability and workspace-state version handling.
- `NotificationService` emits posted notifications.
- `MainWindow` construction smoke test with fake/minimal services if feasible without constructing the Crimson GPU layer.

Do not test exact widget geometry, margins, fonts, or colors.

### Task #6 Tests

Required test areas:

- Mesh ID default invalid values and generation behavior.
- Creation/deletion primitive ID validity.
- Validation catches invalid references.
- Validation passes for the smallest valid fixture.
- Boundary placeholder behavior is explicit and test-covered if full boundary topology is deferred.
- Mesh target has no Qt, UI, command, tool, document, app, or Crimson dependency.

Property/fuzz tests are optional in `#6`; add only if the harness from `#2` makes them practical and deterministic.

### Task #7 Tests

Required test areas:

- Document object creation/deletion emits typed events and dirty-state changes.
- Selection is stored in `Document`, not in mesh.
- Selection rejects invalid object/component IDs.
- Each initial command follows:
  - execute -> expected semantic document state
  - undo -> original semantic document state
  - redo -> expected semantic document state
- Command history clears redo after a new command.
- Command merge hooks default to not mergeable.
- Undo/redo action state reflects `CommandHistory`.
- Tools dispatch only toolkit-neutral events and do not mutate documents outside commands.

Use normalized document state helpers for undo/redo comparisons. Do not compare pointer addresses.

## Required Worker Scopes

### Worker Scope For Task #3

Recommended assignment split:

1. Foundation worker
   - Implements `modeling_foundation` primitives and unit tests.
   - Owns `id.hpp`, `result.hpp`, `diagnostic.hpp`, `assert.hpp`, `logging.hpp`.
2. App/UI service worker
   - Implements `Application`, Qt bootstrap/style policy, `UiContext`, action registry, settings, notifications, panel host, and `MainWindow` shell refactor.
   - Must incorporate the UI architect's A4 contract before implementation.
3. Verification worker or same implementation worker
   - Runs configure, build, CTest, and board validation.
   - Confirms document/tool actions are disabled and visible prototype behavior is otherwise preserved.

### Worker Scope For Task #6

Recommended assignment split:

1. Mesh/math/geometry scout
   - Confirms the minimal math and geometry types needed for mesh validation.
   - Confirms boundary representation choice or placeholder policy.
2. Mesh core worker
   - Implements IDs, records, creation/deletion primitives, traversal helpers, validation, fixtures, and tests.
   - Must use task `#3` foundation contracts.
3. Verification worker or same implementation worker
   - Runs full CTest and focused mesh/foundation tests.
   - Confirms architecture checks reject forbidden dependencies.

### Worker Scope For Task #7

Recommended assignment split:

1. Document/selection worker
   - Implements `Document`, object store, transforms, selection, typed events, dirty flags, and semantic state snapshot helpers for tests.
2. Command-history worker
   - Implements `ICommand`, `CommandResult`, `CommandHistory`, initial non-topology commands, and undo/redo tests.
3. Tool-shell worker
   - Implements toolkit-neutral input events, `ITool`, `ToolContext`, `ToolManager`, and preview-only state contracts.
4. UI action integration worker
   - Wires Undo/Redo and document-safe actions through `ActionRegistry`, `ActionStateUpdater`, and `CommandHistory`.
   - Must not implement document-backed item models or panels from task `#8`.

## Blockers

Current blockers:

- Task `#2` is a hard blocker for `#3`, `#6`, and `#7` implementation because targets, CTest, and boundary checks do not exist yet.
- Task `#3` implementation should wait for UI architect assignment `A4` before finalizing `ActionRegistry`, `PanelHost`, and `MainWindow` shell file placement.
- Task `#6` implementation waits on approved task `#3` foundation contracts.
- Task `#7` implementation waits on approved tasks `#2`, `#3`, and `#6`.

Architectural blockers to avoid:

- Do not create fake document services in task `#3` just to make actions enabled.
- Do not put mesh validation diagnostics in UI types.
- Do not let `Document` own command history.
- Do not let tools consume Qt event classes.
- Do not let UI item widgets become document truth.

## Verification Commands

After task `#2` lands, workers for tasks `#3`, `#6`, and `#7` should use these commands unless task `#2` documents a different test preset:

```powershell
cmake --preset qt-mingw-debug
cmake --build --preset qt-mingw-debug
ctest --preset qt-mingw-debug
python tools/project_board.py validate
```

Focused checks after task `#3`:

```powershell
ctest --preset qt-mingw-debug -R "foundation|ui_actions|settings|notifications|architecture"
```

Focused checks after task `#6`:

```powershell
ctest --preset qt-mingw-debug -R "foundation|math|geometry|mesh|architecture"
```

Focused checks after task `#7`:

```powershell
ctest --preset qt-mingw-debug -R "document|commands|tools|ui_actions|architecture"
```

If the task `#2` harness does not support regex filtering, run the full `ctest --preset qt-mingw-debug` command and report that focused filtering is unavailable.

## Acceptance Criteria

### Task #3 Approval Criteria

- `MainWindow` receives `UiContext&` and no longer constructs future app services itself.
- `Application` or equivalent app composition root owns service lifetime.
- Fusion style setup is null-checked and centralized.
- `ActionRegistry` owns canonical actions.
- `ActionStateUpdater` disables unavailable document/tool actions.
- `SettingsService` and `NotificationService` are testable without Crimson GPU construction.
- Prototype viewport behavior is preserved except disabled unavailable document/tool actions.
- Tests and architecture checks pass.

### Task #6 Approval Criteria

- Mesh code has no Qt, UI, command, tool, document, app, or Crimson dependency.
- Mesh IDs use task `#3` foundation ID contracts.
- Mesh validation produces structured diagnostics.
- Fixtures and tests cover invalid IDs, deletion/reuse, and basic topology invariants.
- Tests and architecture checks pass.

### Task #7 Approval Criteria

- `Document` owns scene objects, transforms, selection, dirty flags, and typed events.
- User-visible document mutations go through commands.
- Undo/redo is test-covered for each initial command.
- UI Undo/Redo state derives from `CommandHistory`.
- Tools produce previews and commands, not direct document mutation.
- UI and renderer-facing future integration can consume events/snapshots, not direct mutation callbacks.
- Tests and architecture checks pass.

## Handoff

Implementation should return to the software architect after each relevant task implementation with:

- Plan path: `agents/plans/implementation_wave1_a3_foundation_document_command_preflight_doc.md`
- Changed files.
- Build and test results.
- Any deviations from this plan.
- Confirmation that `project_board.md` was updated only by the root/main agent through board commands.

If implementation is approved, the main/root agent should move this plan from `agents/plans/` to `agents/archive/` only after all linked work that depends on this plan is approved or explicitly superseded. Do not delete the plan automatically.
