# Master Frozen Architecture Audit: 20260704_full_architecture

Run ID: `20260704_full_architecture`
Frozen baseline timestamp: `2026-07-04 18:44:19 +02:00`
Repo root: `C:\Users\Drako\Desktop\_quader-qt-base`
Status: actionable consolidated audit

This master audit consolidates archived source reports:

- `agents/archive/audit_20260704_full_architecture_software.md`
- `agents/archive/audit_20260704_full_architecture_ui.md`
- `agents/archive/audit_20260704_full_architecture_renderer.md`

The individual reports are archived provenance only. This master audit is
self-contained and remains the implementation authority for this frozen audit
run while active board entries still reference it.

## Baseline And Staleness

- All three reports target `AUDIT_RUN_ID` `20260704_full_architecture` and baseline time `2026-07-04 18:44:19 +02:00`.
- Current non-build file listing matches the frozen baseline plus the three individual audit reports. No source, shader, CMake, README, or architecture-doc drift was detected during consolidation.
- No master-audit section is stale at the time this file was written.
- If source, CMake, shader, README, or architecture docs change before implementation starts, rerun or explicitly refresh the affected audit sections before applying these batches.
- The accepted findings describe only the current prototype state at the frozen baseline. The implementation batches are future work derived from current gaps against the architecture guides; they are not treated as existing project state.
- On 2026-07-04, this master audit was refreshed only for the user-supplied renderer naming decision below. That naming refresh does not change the frozen baseline findings.

## Naming Decisions

- The renderer is named `crimson`.
- The renderer implementation root is `src/crimson/`.
- Crimson implementation code must not encode the graphics runtime name in target, folder, or planned class names.
- The internal GPU/runtime implementation layer belongs under `src/crimson/gpu/`.
- Do not use names such as `crimson-renderer` for the renderer component.

## Boundary Decisions

- `app` is the composition root. It owns application lifetime, service construction, dependency injection, settings loading, main loop, and shutdown.
- Domain truth lives below UI and renderer: `Document` owns editable scene state, `commands` mutate documents, and tools produce previews plus commands.
- UI owns Qt Widgets, menus, panels, action objects, settings presentation, notification presentation, viewport widget lifetime, and Qt-to-neutral input translation.
- Renderer owns render-facing data types, frame snapshots, render graph, material/base-shader schemas, render handles, render diagnostics, and Crimson APIs that hide graphics-runtime details.
- Only `src/crimson/gpu/` owns graphics-runtime includes, runtime handles, shader binary loading, device selection, framebuffer creation, render states, and frame submission.
- UI may pass native surface information through a narrow viewport surface adapter, but must not build or expose graphics-runtime platform data.
- Interactive camera/navigation state is UI/controller state. Immutable `RenderCamera` values are renderer snapshot data.
- Document-backed UI models must store stable domain IDs and dispatch commands for document edits. Qt item widgets may remain prototype-only placeholders until real document models exist.

## Accepted Findings

### A-001: Single executable target blocks architecture enforcement

Severity: P0
Sources: SW-01, UI-001, CD-R-003

Accepted. The flat `QuaderQtBgfx` executable links Qt Widgets, bgfx, and bx directly and compiles all current source files. This prevents CMake from enforcing the target dependency graph in `architecture.md`.

Action: introduce module targets before adding real modeling, UI, or renderer features.

### A-002: Core modeling architecture is absent

Severity: P0
Sources: SW-02, SW-03, UI-011

Accepted. The baseline has no foundation, math, geometry, mesh, document, commands, tools, I/O, or command history modules. UI menus currently expose future actions with no command path.

Action: build the modeling core in architecture order. Do not add topology edits, object creation truth, material edits, selection truth, or import/export behavior to `MainWindow` or `BgfxWidget`.

### A-003: Application composition root and service ownership are missing

Severity: P1
Sources: SW-04, UI-002, UI-004

Accepted. `main.cpp` constructs `QApplication` and `MainWindow` directly. There is no app-owned service graph, `UiContext`, settings service, notification service, document provider, command history, tool manager, or renderer service.

Action: add an app composition root and explicit non-owning contexts before expanding shell behavior.

### A-004: Canonical UI actions, state updates, settings, and panel hosting are missing

Severity: P1
Sources: UI-003, UI-004, UI-008

Accepted. Actions are created inline, inert document actions are enabled, workspace state is not persisted, and notifications are raw status-bar strings.

Action: add `ActionId`, `ActionRegistry`, `ActionStateUpdater`, `SettingsService`, `NotificationService`, `PanelId`, and `PanelHost`. Document actions stay disabled until command services exist.

### A-005: Graphics-runtime details leak through the UI boundary

Severity: P0
Sources: SW-05, UI-007, R-001

Accepted. `BgfxWidget.h` includes `<bgfx/bgfx.h>` and stores graphics-runtime handles directly. This is the clearest Crimson boundary violation.

Action: move all graphics-runtime includes, handles, shader loading, render state construction, platform-data conversion, and submission into `src/crimson/gpu/`. UI and Crimson core use project-owned handles and diagnostics only.

### A-006: `BgfxWidget` is a cross-layer object

Severity: P0
Sources: SW-06, UI-006, R-002, CD-R-001

Accepted. The widget owns Qt event handling, camera/navigation state, splitters, bgfx lifetime, shader loading, GPU resources, hardcoded scene data, render submission, grid drawing, and formatted stats.

Action: split it into UI viewport host/controller responsibilities and Crimson GPU/runtime responsibilities before adding picking, tools, materials, PBR, or document-driven rendering.

### A-007: Renderer snapshot, render world, render graph, and pass boundaries are absent

Severity: P1
Sources: SW-07, R-002, R-003

Accepted. Rendering currently submits hardcoded cube/grid data directly from `renderFrame()` with no immutable `FrameSnapshot`, no `RenderWorld`, no render graph, no named resources, and no pass dependency validation.

Action: introduce `FrameSnapshot`, render scene types, `FrameBuilder`, `RenderGraph`, named resources, and explicit pass order before renderer feature growth.

### A-008: Renderer color, shader, material, overlay, picking, and device policies are prototype-only

Severity: P1
Sources: SW-10, R-004, R-005, R-006, R-007, R-008, R-009

Accepted, but deferred behind boundary work. The prototype forces Vulkan, compiles only SPIR-V shaders, writes simple diffuse colors directly to the backbuffer, draws the grid inside the scene path, has no material/base-shader system, and has no picking path.

Action: after the renderer boundary and graph exist, implement graphics device selection/capabilities, shader manifest, linear HDR/tone mapping, base shader materials, overlay domain, and picking in that order.

### A-009: Inspector and prototype UI duplicate future document/render truth

Severity: P2
Sources: SW-09, UI-005, CD-R-002

Accepted. The inspector uses hardcoded `QTreeWidgetItem` rows and property-like widgets for selection, renderer, cube, light, and animation data.

Action: keep this prototype-only. Replace with document-backed models and renderer diagnostics/settings once document and renderer services exist.

### A-010: Test and executable architecture guardrails are absent

Severity: P0
Sources: SW-08, UI-010, R-011

Accepted. There is no CTest setup, no test preset, no module-boundary check, no UI service/model tests, and no renderer graph/shader/material tests.

Action: add test infrastructure with the module split. Boundary checks are a first-class deliverable, not polish.

### A-011: Renderer diagnostics, resource lifetime, and performance hooks are too ad hoc

Severity: P2
Sources: UI-008, R-010, R-011, R-012

Accepted. Renderer status is string-based, resource lifetime is manual, and frame stats are limited to size/view count/FPS.

Action: add structured diagnostics, RAII/resource caches in `src/crimson/gpu/`, `FrameStats`, and Crimson core tests as features land.

### A-012: Style metrics and UI consistency are not centralized

Severity: P3
Sources: UI-009

Accepted as non-blocking. Fusion is applied and no broad stylesheet exists, which is good. Metrics and style policy should become centralized during UI foundation work.

Action: add `fusion_style_policy` and `ui_metrics` when the UI module is introduced.

## Superseded Or Rejected Findings

- No individual finding is rejected as incorrect.
- SW-01, UI-001, and CD-R-003 are superseded by A-001.
- SW-05, UI-007, and R-001 are superseded by A-005.
- SW-06, UI-006, R-002, R-003, and CD-R-001 are split across A-006 and A-007.
- SW-07 and renderer snapshot/render graph findings are superseded by A-007.
- Renderer PBR, material, overlay, picking, color-space, shader-manifest, and golden-image recommendations are accepted in principle but are not first implementation work. They are sequenced after boundaries, tests, snapshots, and graph foundations.
- UI model/view, property editor, and selection-sync recommendations are accepted, but real document-backed models are deferred until `Document`, `Selection`, and command services exist.

## Conflicts Resolved

### C-001: Renderer graph "from first commit" versus app-wide target/core sequence

There is no substantive conflict. The first renderer-specific work must include a graph boundary, but app-wide implementation must first create CMake targets and guardrails so renderer/UI/core boundaries can be enforced.

Resolution: Batch 1 creates targets and tests. Batch 4 is the first renderer-growth batch and includes `FrameSnapshot` plus a minimal `RenderGraph`.

### C-002: UI viewport host versus Crimson GPU ownership

UI and renderer reports both identify `BgfxWidget` as a boundary problem. Ownership is resolved as follows:

- UI owns `ViewportWidget`, `ViewportController`, `ViewportLayoutState`, and Qt event translation.
- Renderer owns `Renderer`, `FrameSnapshot`, `RenderCamera`, `ViewportSettings`, render graph, handles, diagnostics, and frame submission.
- `src/crimson/gpu/` owns platform data conversion, device selection, GPU resources, shader binaries, and frame submission.
- `app` wires UI surface callbacks to Crimson GPU construction.

### C-003: Camera state ownership

Interactive camera controls belong to UI/controller state because they are user input behavior. Renderer receives immutable camera values in a snapshot and must not depend on Qt events or widget state.

### C-004: Actions before commands

UI needs `ActionRegistry` early, but domain mutation must not be invented in UI. Resolution: create canonical actions early, disable document/tool actions until command/tool services exist, then wire them through `CommandHistory` and `ToolManager`.

### C-005: Material inspector ownership

The UI owns presentation models and editors. The document owns editable material assignments and document truth. Crimson owns base shader schemas, render material handles, texture color-space policy, and GPU parameter layout. UI must consume schemas without depending on graphics-runtime internals.

## Ordered Implementation Batches

### Batch 0: Freeze prototype growth

Priority: P0
Owners: all

Rules:

- Do not add real document mutation, topology editing, selection truth, import/export, material editing, picking-driven selection, PBR, render graph features, or debug panels directly to `MainWindow` or `BgfxWidget`.
- Bug fixes may preserve the current prototype, but new architecture-sensitive work must target the batches below.

Acceptance:

- Any proposed feature work that touches UI/renderer/core boundaries references this master audit or a follow-up implementation plan.

### Batch 1: Module targets and test harness

Priority: P0
Owner: software, with UI/renderer input

Implement:

- Split `CMakeLists.txt` into module targets. Initial targets may be thin, but dependency direction must be real.
- Minimum target families: `modeling_foundation`, `modeling_math`, `modeling_geometry`, `modeling_mesh_core`, `modeling_mesh_ops`, `modeling_document`, `modeling_commands`, `modeling_tools`, `crimson`, `modeling_ui`, and the app executable.
- Keep `QuaderQtBgfx` as the executable/composition target, not the owner of all implementation.
- Add CTest support and a test preset.
- Add architecture guardrails that reject Qt Widgets includes outside UI/app-facing targets and reject graphics-runtime includes outside `src/crimson/gpu/`.
- Update `README.md` and `AGENTS.md` with the test command.

Acceptance:

- Configure/build still works with the documented preset.
- `ctest` has a documented preset and passes.
- Boundary checks run in tests or an explicit architecture-check target.

### Batch 2: Foundation, app composition, and UI shell services

Priority: P0
Owners: software and UI

Implement:

- Foundation primitives: strong IDs, `Result`/error helpers, assertions/logging stubs, and basic tests.
- `src/app/application.*` or equivalent composition root owning service lifetime.
- Qt bootstrap/style policy with Fusion null check and application metadata.
- `UiContext` with explicit non-owning references.
- UI services: `ActionId`, `ActionRegistry`, initial `ActionStateUpdater`, `SettingsService`, `NotificationService`, `PanelId`, and `PanelHost`.
- Refactor `MainWindow` into a shell receiving `UiContext&`.
- Disable inert document/tool actions until services exist.

Acceptance:

- Current visible prototype behavior is preserved except disabled unavailable actions.
- `MainWindow` no longer constructs future app services itself.
- Settings and notification services are testable without Crimson GPU construction.

### Batch 3: Viewport and Crimson GPU boundary extraction

Priority: P0
Owners: UI and renderer, software arbitrates dependency direction

Implement:

- UI side: `ViewportWidget`, `ViewportController`, `ViewportLayoutState`, `ViewportCameraController`, and a narrow viewport render-host interface.
- Renderer side: `Renderer`, renderer config/types, project-owned render handles, structured diagnostics.
- Crimson GPU side: `src/crimson/gpu/GpuDevice`, runtime handle wrappers, shader library shell, framebuffer/resource owner shell.
- Move graphics-runtime includes, handles, platform data conversion, init/shutdown/reset, shader loading, and submit calls out of public UI headers.
- Preserve the current cube/grid prototype through the new boundary.

Acceptance:

- No public UI header includes graphics-runtime headers or exposes runtime handle types.
- Only `crimson` links the graphics runtime.
- Viewport input/controller tests can run without constructing the Crimson GPU layer.
- Prototype viewport still initializes and renders through the new Crimson boundary.

### Batch 4: Renderer snapshot and minimal render graph

Priority: P1
Owner: renderer, with software data-flow review

Implement:

- `FrameSnapshot`, `FrameBuilder`, `RenderWorld`, `RenderObject`, `RenderCamera`, `RenderLight`, `RenderEnvironment`, `ViewportSettings`, and `FrameStats`.
- Convert prototype cube/grid inputs into snapshot or fixture data rather than widget-owned draw state.
- Add minimal `RenderGraph`, `RenderPass`, `RenderResourceDesc`, `RenderResourceRegistry`, named resources, dependency validation, resize behavior, and debug dump.
- Keep this graph minimal: prototype opaque pass, clear/present path, and placeholders only where needed.

Acceptance:

- Renderer can render from immutable frame input.
- Render graph order is explicit and test-covered.
- The Crimson GPU layer receives prepared packets/resources, not editor/widget truth.

### Batch 5: Mesh core and validation

Priority: P1
Owner: software

Implement:

- `mesh/core` IDs, records, creation/deletion primitives, attribute storage hooks, traversal helpers, and validation.
- Basic `math`/`geometry` value types and algorithms needed by mesh validation.
- Mesh fixtures and unit tests.

Acceptance:

- Mesh code has no Qt, UI, command, tool, or renderer dependency.
- Validation tests cover invalid IDs, boundary placeholders, and basic topology invariants.

### Batch 6: Document, selection, commands, and tools

Priority: P1
Owner: software, with UI integration after core tests pass

Implement:

- `Document`, scene object store, object IDs, transforms, active selection, dirty flags, and typed document events.
- `ICommand`, `CommandResult`, `CommandHistory`, undo/redo tests, and initial non-topology commands.
- Tool interfaces and `ToolManager` shell; tools produce previews and commands, not direct document mutation.
- Wire `Undo`, `Redo`, and simple document-safe actions through `ActionRegistry` and `CommandHistory`.

Acceptance:

- User-visible document mutations go through commands.
- Undo/redo state drives action enabled/text state.
- UI and renderer update from document events/snapshots, not direct mutation callbacks.

### Batch 7: UI models, panels, workspace, and selection adapters

Priority: P2
Owner: UI, with software contracts

Implement:

- Replace inline inspector placeholder with panel classes.
- Add workspace save/restore and reset through `SettingsService` and `PanelHost`.
- Add `DocumentItemModel`, `PropertyItemModel`, named roles, property descriptors, and selection adapters once document selection exists.
- Route document-backed edits through commands.

Acceptance:

- Inspector no longer owns document truth.
- Model tests use `QAbstractItemModelTester` where applicable.
- Workspace state persists with versioned keys.

### Batch 8: Renderer V1 correctness foundation

Priority: P2
Owner: renderer

Implement in this order:

1. Backend selection and capability diagnostics.
2. Shader manifest and multi-target shader output layout.
3. Linear HDR scene target, tone-map/final conversion, and color-space tests.
4. Material/base-shader system with `OpaquePbr`, then `AlphaCutoutPbr`, `TransparentPbr`, and `OverlayUnlit`.
5. Overlay system and grid as overlay commands.
6. Picking target, request-scoped readback, and picking ID tests.
7. Structured renderer diagnostics, resource RAII/caches, and frame stats.

Acceptance:

- No runtime shader compilation.
- Overlays do not enter lit/post/tone-map/fog/bloom paths.
- Picking uses non-color-managed IDs and request-scoped readback.
- Renderer tests cover graph, shader manifest, color space, material schema, overlays, and picking as each feature lands.

### Batch 9: I/O and asset boundaries

Priority: P3
Owner: software, with renderer material mapping input

Implement:

- Importer/exporter interfaces after mesh/document validation exists.
- File dialogs stay in UI services.
- Importers produce documents or document fragments plus structured diagnostics.
- glTF material mapping consumes renderer material schemas without exposing graphics-runtime internals.
- Start with static registration only.

Acceptance:

- Import/export code has no UI dependency.
- Imported meshes validate before becoming editable document state.
- Material mapping is deterministic and test-covered.

### Batch 10: Performance, diagnostics, and golden-image maturity

Priority: P3
Owners: all, renderer leads viewport rendering

Implement:

- Render culling, draw packet sorting, instancing batch keys, dirty resource uploads, and expanded `FrameStats`.
- UI diagnostics panel/status presentation fed by typed services.
- Golden image harness only after render graph, color space, and material foundations are deterministic.
- Benchmark hooks for mesh traversal/ops and renderer upload paths.

Acceptance:

- Performance counters exist before optimization work.
- Golden images are stable enough to be useful and not brittle placeholders.

## Required Follow-Up Reviews

- After each batch implementation, return to the owning architect for review with changed files, build/test results, deviations, and this master audit reference.
- Cross-boundary batches 3, 4, 6, 7, and 8 require both owner review and software architecture arbitration if dependency direction is disputed.
- Do not delete individual or batch implementation plan files until the relevant architect explicitly approves the implementation.

## Immediate Non-Goals

- Do not build a plugin system.
- Do not implement PBR, glTF import, picking-driven selection, or golden images before renderer boundaries and graph foundations exist.
- Do not implement topology operations in UI callbacks.
- Do not expose graphics-runtime handles outside `src/crimson/gpu/`.
- Do not make Qt item widgets the source of document truth.
