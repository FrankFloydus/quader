# Frozen Software Architecture Audit: 20260704_full_architecture

Audit run: 20260704_full_architecture
Baseline date/time: 2026-07-04 18:44:19 +02:00
Repo root: C:\Users\Drako\Desktop\_quader-qt-base
Report owner: software architecture
Report status: individual audit source material only

This report audits the frozen baseline only. It does not consolidate the UI or renderer audit reports, and it is not implementation authority until merged into the master audit.

## Frozen Baseline Status

- The repository is a prototype Qt Widgets + bgfx app, not a Git repository for this run.
- `agents/plans` was empty when checked during this audit.
- No implementation files, architecture docs, source files, CMake files, README files, shaders, or build outputs were modified.
- The only file created by this audit is this report.
- No stale sections were identified during this software audit. If the workspace changes before consolidation, the master audit should mark affected sections stale or request affected audit reruns.

## Architecture Sources Read

- `agents/architecture/architecture.md`
- `agents/architecture/ui.md`
- `agents/architecture/renderer.md`
- Baseline source/build/docs listed in the user request and present under the frozen layout.

Key architecture anchors:

- Overall architecture states that core owns geometry/topology, commands mutate documents, tools create commands, and renderers/UI are views of state (`agents/architecture/architecture.md:14`).
- Lower layers must not include higher layers, and dependency direction is part of the architecture (`agents/architecture/architecture.md:149`, `agents/architecture/architecture.md:167`).
- Target CMake design uses one library target per module or module group, not one broad executable target (`agents/architecture/architecture.md:768`, `agents/architecture/architecture.md:799`).
- Document owns editable scene state; commands are undoable, UI-free user actions (`agents/architecture/architecture.md:565`, `agents/architecture/architecture.md:585`).
- Renderer consumes snapshots, not mutable editor truth (`agents/architecture/architecture.md:655`, `agents/architecture/architecture.md:673`).
- The recommended sequence starts with CMake targets, foundation, IDs, result/error type, and basic tests before mesh, document, UI, and renderer growth (`agents/architecture/architecture.md:2701`, `agents/architecture/architecture.md:2769`).

## Severity Rubric

- P0 Critical: blocks safe implementation of the target architecture or creates strong dependency direction risk.
- P1 High: should be addressed before major feature work depends on the current shape.
- P2 Medium: important drift or missing guardrail that can follow the core restructuring.
- P3 Low: cleanup, consistency, or documentation follow-up.

## Findings

### SW-01: Single executable target prevents dependency enforcement

Severity: P0 Critical
Owner: software architecture

The current build has one executable target that directly compiles every source file and links both Qt Widgets and bgfx/bx. This makes it impossible for CMake to enforce the target dependency graph required by the architecture.

Evidence:

- `CMakeLists.txt:34` creates only `add_executable(${PROJECT_NAME}` for the app.
- `CMakeLists.txt:35` through `CMakeLists.txt:39` list `src/main.cpp`, `MainWindow.*`, and `BgfxWidget.*` directly in that executable.
- `CMakeLists.txt:44` through `CMakeLists.txt:49` link the same target to `Qt6::Widgets`, `bgfx`, and `bx`.
- Architecture requires module targets such as `modeling_foundation`, `modeling_document`, `modeling_commands`, `modeling_render`, and `modeling_ui` (`agents/architecture/architecture.md:772` through `agents/architecture/architecture.md:783`).

Risk:

- Any future mesh, document, command, UI, and renderer code added to this target can include each other accidentally.
- The project will not get compile-time protection against forbidden dependencies such as commands including UI or document including a concrete renderer backend.
- Later extraction becomes harder as each feature increases transitive include coupling.

Recommended direction:

- First implementation batch should introduce module CMake targets even if many targets begin as thin skeletons.
- Keep the current executable as the composition target, but stop using it as the only architectural boundary.
- Add dependency-boundary tests or scripts once targets exist.

### SW-02: Core modeling architecture is absent, not merely incomplete

Severity: P0 Critical
Owner: software architecture

The frozen baseline has no foundation, math, geometry, mesh, document, commands, tools, or I/O modules. This is expected for the prototype, but it is the largest architecture gap and should block adding real modeling operations into the UI or renderer.

Evidence:

- `AGENTS.md:9` through `AGENTS.md:23` explicitly describe the current app as a minimal flat prototype and warn that the final architecture is not implemented.
- The source layout contains only `src/main.cpp`, `src/MainWindow.*`, and `src/BgfxWidget.*`.
- `CMakeLists.txt:34` through `CMakeLists.txt:40` compile only those prototype files.
- Searches for domain types in `src` find only UI menu labels such as `MainWindow.cpp:73` and hardcoded inspector text such as `MainWindow.cpp:121`; there are no `Document`, `ICommand`, `Polyhedron`, `Selection`, `ToolManager`, or `RenderSnapshot` definitions.
- Architecture defines `Document` as the editable scene boundary (`agents/architecture/architecture.md:565` through `agents/architecture/architecture.md:583`) and commands as the undo boundary (`agents/architecture/architecture.md:1218` through `agents/architecture/architecture.md:1241`).

Risk:

- Real user-facing edits would have no safe mutation path, no undo/redo boundary, no selection remapping, and no validation point.
- UI or renderer code is likely to become the first owner of scene truth if feature work starts from the current files.
- Import/export, tools, and picking would have nowhere correct to apply results.

Recommended direction:

- Do not add topology edits, object creation, material edits, selection truth, or import/export behavior to `MainWindow` or `BgfxWidget`.
- Introduce minimal core skeletons in architecture order: foundation IDs/result, mesh records/validation, document object store, selection, command interface/history, then tools.

### SW-03: No command-driven mutation path or undo/redo implementation

Severity: P1 High
Owner: software architecture

The UI displays command-like actions but they are placeholders, not connected to an application command system. The only user-facing toggle observed directly mutates viewport-local renderer state.

Evidence:

- `MainWindow.cpp:67` through `MainWindow.cpp:69` add Undo and Redo actions, but no command history exists.
- `MainWindow.cpp:73` through `MainWindow.cpp:82` add tool/create actions, but no tool manager or command dispatch exists.
- `MainWindow.cpp:147` through `MainWindow.cpp:150` connects the "Rotate cube" checkbox directly to `BgfxWidget::setAnimationEnabled`.
- Architecture says commands represent user-visible operations and the command boundary is the undo boundary (`agents/architecture/architecture.md:1218` through `agents/architecture/architecture.md:1230`).

Risk:

- Placeholder UI can mislead implementing agents into adding behavior directly in menu slots or widgets.
- Undo/redo behavior cannot be added consistently after mutations are scattered.
- Document events and renderer dirty flags cannot be derived from direct widget calls.

Recommended direction:

- Introduce `ICommand`, `CommandResult`, and `CommandHistory` before wiring any user-visible document mutation.
- Treat viewport animation toggles as viewport settings, not document mutations, and route future document-affecting toggles through commands.

### SW-04: Application composition root and service ownership are missing

Severity: P1 High
Owner: software architecture

`main.cpp` directly constructs `QApplication` and `MainWindow` without an application object, explicit service ownership, or `UiContext`/composition root. This is acceptable for the prototype shell, but it conflicts with the target ownership model once services exist.

Evidence:

- `src/main.cpp:8` through `src/main.cpp:16` creates `QApplication`, sets style/app names, constructs `MainWindow window`, and enters the event loop.
- There is no `src/app/` module, `Application`, document provider, command history, tool manager, settings service, notification service, or renderer service.
- UI architecture expects core services, Qt-facing UI services, explicit context, workspace restore, then `MainWindow` construction (`agents/architecture/ui.md:182` through `agents/architecture/ui.md:225`).
- Overall architecture assigns lifetime, service construction, dependency injection, settings, main loop, and shutdown to `app` (`agents/architecture/architecture.md:707` through `agents/architecture/architecture.md:720`).

Risk:

- Services may be created by widgets ad hoc instead of by a single owner.
- Shutdown ordering for document, renderer, and background work will be fragile.
- Testing application services without constructing widgets will be difficult.

Recommended direction:

- Add a minimal `src/app/application.*` or equivalent composition root in the first restructuring batch.
- Keep Qt startup in a UI/app bootstrap boundary and pass an explicit non-owning context into `MainWindow`.

### SW-05: bgfx is exposed through the Qt widget public header

Severity: P0 Critical
Owner: renderer architecture
Cross-domain concern: software dependency direction and renderer boundary

`BgfxWidget.h` includes bgfx directly and exposes bgfx types and handles in the widget class. This violates the target backend boundary and makes UI headers depend on concrete renderer internals.

Evidence:

- `src/BgfxWidget.h:10` includes `<bgfx/bgfx.h>`.
- `src/BgfxWidget.h:57` declares `renderGrid(bgfx::ViewId ...)`.
- `src/BgfxWidget.h:59` returns `bgfx::ShaderHandle`.
- `src/BgfxWidget.h:143` through `src/BgfxWidget.h:164` store bgfx layouts, vertex/index buffers, programs, and uniforms directly on the widget.
- Renderer architecture requires bgfx to be hidden inside `render_bgfx/` and states that no UI code includes bgfx (`agents/architecture/renderer.md:69` through `agents/architecture/renderer.md:83`).

Risk:

- Any UI source including `BgfxWidget.h` inherits the concrete bgfx dependency.
- Future renderer backends cannot be introduced without touching UI headers.
- Architecture checks cannot distinguish viewport host responsibilities from backend responsibilities.

Recommended direction:

- Renderer architect should own the exact backend split.
- Software-level boundary should be: UI owns a viewport host/controller; renderer exposes backend-neutral interfaces and handles; only `render_bgfx` owns bgfx handles.
- Move bgfx includes and handle members out of public UI headers before adding more renderer features.

### SW-06: `BgfxWidget` is a cross-layer god object

Severity: P0 Critical
Owner: shared UI/renderer boundary
Cross-domain concern: UI architecture and renderer architecture

`BgfxWidget` currently combines a native Qt viewport, event translation, camera/navigation state, splitter UI, bgfx initialization/shutdown, shader loading, GPU resources, render submission, grid rendering, hardcoded scene data, and stats. This is the biggest concrete concentration of architectural drift in the baseline.

Evidence:

- Public widget state includes camera/navigation models in `src/BgfxWidget.h:75` through `src/BgfxWidget.h:120`.
- UI event handling and navigation live in `src/BgfxWidget.cpp:798` through `src/BgfxWidget.cpp:980`.
- bgfx initialization and shutdown live in `src/BgfxWidget.cpp:1033` through `src/BgfxWidget.cpp:1080`.
- GPU resource creation and validation live in `src/BgfxWidget.cpp:1084` through `src/BgfxWidget.cpp:1149`.
- Render submission lives in `src/BgfxWidget.cpp:1259` through `src/BgfxWidget.cpp:1348`.
- Grid rendering lives in `src/BgfxWidget.cpp:1371` through `src/BgfxWidget.cpp:1440`.
- UI architecture says `MainWindow` should not own renderer resource management and viewport widgets should translate Qt events to toolkit-neutral editor events (`agents/architecture/ui.md:240` through `agents/architecture/ui.md:248`, `agents/architecture/ui.md:1328`).
- Renderer architecture requires `FrameSnapshot`, render graph, and `BgfxBackend` separation (`agents/architecture/renderer.md:471` through `agents/architecture/renderer.md:488`).

Risk:

- Every viewport feature will add more state to one file.
- Camera/tool behavior cannot be tested without a QWidget and bgfx.
- Renderer lifetime and UI lifetime are tightly coupled.
- Splitting later will be high-risk because behavior is interleaved.

Recommended direction:

- Freeze `BgfxWidget` as a prototype host until a UI/renderer boundary plan is approved.
- Extract in layers: viewport host, viewport controller/input adapter, camera state, renderer interface, bgfx backend, overlay renderer.
- Avoid adding document selection, picking, tools, material previews, or render graph logic directly to `BgfxWidget`.

### SW-07: Rendering path uses hardcoded prototype scene data instead of document snapshots

Severity: P1 High
Owner: renderer architecture
Cross-domain concern: software document-to-render data flow

The renderer currently draws hardcoded cube/grid data directly from the widget. There is no document event, dirty flag, snapshot builder, render world, render mesh cache, or immutable frame data handoff.

Evidence:

- Cube vertex/index data are hardcoded in `src/BgfxWidget.cpp:586` through `src/BgfxWidget.cpp:625`.
- `createResources()` creates vertex/index buffers directly from those arrays at `src/BgfxWidget.cpp:1096` through `src/BgfxWidget.cpp:1099`.
- `renderFrame()` computes a rotating model matrix, hardcoded light direction, hardcoded base color, view transforms, and submits directly at `src/BgfxWidget.cpp:1279` through `src/BgfxWidget.cpp:1341`.
- README describes the app as drawing a rotating red cube with simple directional diffuse lighting (`README.md:3` through `README.md:6`).
- Architecture requires `Document changed -> Dirty flags / document event -> RenderSnapshotBuilder -> RenderSceneSnapshot -> Renderer backend` (`agents/architecture/architecture.md:655` through `agents/architecture/architecture.md:673`).

Risk:

- The first real document model may be forced to match widget/render data rather than the other way around.
- Selection, object IDs, dirty mesh uploads, and picking cannot be introduced cleanly without a snapshot layer.
- Direct render ownership will conflict with command-driven document mutation.

Recommended direction:

- Software plan should define document events and snapshot ownership before renderer-facing document integration.
- Renderer plan should define `FrameSnapshot`, render handles, render mesh cache, and overlay commands.
- Do not connect future mesh/document state directly to bgfx draw calls.

### SW-08: Test and architecture guardrails are absent

Severity: P1 High
Owner: software architecture

There is no test preset, no CTest setup, no dependency-boundary test, and no core validation test infrastructure in the frozen baseline.

Evidence:

- `AGENTS.md:60` states there is no documented test preset yet.
- `CMakePresets.json:35` through `CMakePresets.json:44` define only build presets, no test presets.
- `CMakeLists.txt` has no `enable_testing()` or `add_test()` entries in the frozen baseline.
- Architecture phase 1 calls for basic tests along with CMake targets, foundation, math, IDs, and result/error type (`agents/architecture/architecture.md:2705` through `agents/architecture/architecture.md:2714`).

Risk:

- Boundary regressions will be caught only by manual review.
- Mesh invariants, command undo/redo, and document event behavior will lack executable proof.
- Agents will have no standard test command once implementation begins.

Recommended direction:

- Add `BUILD_TESTING`/CTest support and a documented test preset in the same batch that introduces module targets.
- Start with low-cost tests: foundation IDs/result tests, command history tests, document event tests, and dependency-boundary checks.
- Update `AGENTS.md`, `README.md`, and CMake presets when the first test command lands.

### SW-09: UI placeholders already duplicate future document state

Severity: P2 Medium
Owner: UI architecture
Cross-domain concern: document truth and UI presentation adapters

The inspector panel uses widget-owned placeholder rows and controls for selection, visibility, lock state, scene object, light, and animation. This is harmless as a prototype display, but should not be extended as the real document/property model.

Evidence:

- `MainWindow.cpp:117` creates a `QTreeWidget`.
- `MainWindow.cpp:120` through `MainWindow.cpp:125` hardcode "Selection", "Qt Viewport Box", "Renderer", and "bgfx Vulkan" as widget-owned state.
- `MainWindow.cpp:129` through `MainWindow.cpp:143` hardcode summary and scene controls.
- UI architecture says item models should adapt document state and not become document truth (`agents/architecture/ui.md:366` through `agents/architecture/ui.md:368`, `agents/architecture/ui.md:75` through `agents/architecture/ui.md:83`).

Risk:

- Real object properties could be added to item widgets instead of document-backed models.
- Selection truth could end up in Qt view widgets rather than `Document`.
- Property edits could bypass commands.

Recommended direction:

- UI architect should own the eventual model/view and property panel plan.
- Software boundary should require document-backed models to store stable domain IDs and dispatch commands for document edits.

### SW-10: Platform/backend policy is hardcoded in prototype code

Severity: P2 Medium
Owner: renderer architecture
Cross-domain concern: app startup diagnostics and backend ownership

The prototype initializes only Windows native window handles and forces Vulkan. This matches the current README, but the renderer architecture expects backend selection and capability diagnostics inside the renderer backend boundary.

Evidence:

- `src/BgfxWidget.cpp:1035` through `src/BgfxWidget.cpp:1037` reject non-Windows platforms.
- `src/BgfxWidget.cpp:1047` sets `init.type = bgfx::RendererType::Vulkan`.
- README describes the viewport as a bgfx Vulkan viewport (`README.md:3` through `README.md:5`).
- Renderer architecture defines backend selection and capability ownership under the bgfx boundary (`agents/architecture/renderer.md:69` through `agents/architecture/renderer.md:83`).

Risk:

- Platform decisions live in a UI widget instead of the renderer backend.
- App startup cannot provide structured renderer diagnostics.
- Future Direct3D/Metal fallback work will touch UI code.

Recommended direction:

- Renderer backend should own platform data, backend selection, capability checks, and diagnostics.
- App/UI should request a viewport/render surface and display backend diagnostics returned by renderer services.

## Cross-Domain Concerns for Consolidation

- UI architect should review `MainWindow` shell responsibilities, `QTreeWidget` inspector placeholders, action registry absence, settings/workspace service absence, and viewport event translation.
- Renderer architect should review bgfx header exposure, bgfx lifetime/resource ownership, direct draw submission, shader pipeline, grid overlay pass ownership, backend selection, and absence of render graph/snapshot/cache.
- Software architect should own final dependency direction: domain modules must not depend on Qt Widgets or bgfx; UI may depend on renderer interfaces; only `render_bgfx` may depend on concrete bgfx handles.

## Suggested Implementation Batches

These batches are recommendations for the later master audit. They are not directly actionable until the master audit accepts them.

### Batch 0: Freeze prototype growth

Priority: P0

- Declare `MainWindow` and `BgfxWidget` prototype-only surfaces until boundaries exist.
- Allow bug fixes needed to keep the prototype running, but do not add document mutation, topology operations, selection truth, picking-driven edits, import/export, material editing, or render graph code to them.
- Require architecture plans for any UI/renderer crossing work.

### Batch 1: Build and module skeleton

Priority: P0

- Split CMake into module targets matching architecture direction.
- Add at least: foundation, math, geometry, mesh_core, mesh_ops, document, commands, tools, render, render_bgfx, ui, app/executable.
- Keep source movement conservative; create empty or minimal skeleton targets where needed.
- Make `QuaderQtBgfx` link through app/ui/render interfaces instead of directly owning all code.
- Add a dependency-boundary check or CMake smoke test that fails on forbidden links.

### Batch 2: Foundation and tests

Priority: P0

- Add strong ID primitives, result/error types, assertion/logging hooks, and naming conventions in `foundation`.
- Add CTest/test preset support and document the command in `README.md` and `AGENTS.md`.
- Add initial unit tests for IDs, result handling, and dependency guardrails.

### Batch 3: Mesh core before UI features

Priority: P1

- Add half-edge mesh records, stable IDs, create/delete primitives, validation, traversal helpers, and mesh fixtures.
- Keep mesh free of Qt, renderer, commands, and tools.
- Add validation tests before adding operations.

### Batch 4: Document, selection, commands, and events

Priority: P1

- Add `Document`, object store, selection model, dirty flags, typed document events, and single-threaded mutation policy.
- Add `ICommand`, `CommandHistory`, undo/redo tests, command merge hooks, and initial selection/document commands.
- Wire UI actions only after command history exists.

### Batch 5: Viewport boundary extraction

Priority: P1

- UI architect defines viewport host/controller, action integration, settings, and event translation.
- Renderer architect defines render interfaces, frame snapshot, render world/cache, overlay commands, and `render_bgfx` backend.
- Software boundary rule: UI owns Qt widgets and neutral input events; renderer consumes snapshots; bgfx remains private to `render_bgfx`.

### Batch 6: Renderer V1 foundation

Priority: P2

- Move bgfx initialization, resource handles, shader loading, and draw submission out of `BgfxWidget`.
- Introduce render graph skeleton, render mesh cache, shader manifest validation, color-space policy, and explicit overlay domain.
- Keep prototype cube/grid as test/demo data until document snapshots are ready.

### Batch 7: UI presentation models and services

Priority: P2

- Add `ActionRegistry`, `ActionStateUpdater`, settings service, notification service, and shell context.
- Replace placeholder inspector data with document-backed item/property models when document state exists.
- Ensure model edits dispatch commands.

### Batch 8: I/O and asset boundaries

Priority: P3

- Add importer/exporter interfaces only after document and mesh validation exist.
- Keep file dialogs in UI services and import/export diagnostics in I/O services.
- Start with static registration, not a dynamic plugin system.

## Approval Notes

This software audit should be returned to the software architect during consolidation with the UI and renderer audit reports for the same `AUDIT_RUN_ID`. The master audit should accept, reject, or supersede each finding, resolve ownership conflicts, and produce the only actionable implementation sequence.
