# Current Code Architecture And Beautify Audit: 20260706_current_code

Repository: `C:\Users\Drako\Desktop\_quader-qt-base`
Date: 2026-07-06
Mode: Audit and beautify
Status: Current-code audit artifact

## Scope

This audit reviewed the current Quader codebase across the main C++20 Qt Widgets app, document/model code, command stack, Crimson renderer path, tests, CMake wiring, and active project-board context.

The beautify pass focuses on API shape, naming, ownership, call-site clarity, and semantic consistency. It does not cover mechanical formatting.

## Sources Read

- `AGENTS.md`
- `agents/workflow.md`
- `agents/task_board.md`
- `agents/hindsight.md`
- `agents/tests-policy.md`
- `agents/documentation-policy.md`
- `agents/code-style.md`
- `agents/architecture/architecture.md`
- `agents/architecture/ui.md`
- `agents/architecture/renderer.md`
- Selected full architecture references under `agents/architecture/reference/`
- Active board and implementation plans relevant to current renderer, diagnostics, and selection work
- Representative source files under `src/document`, `src/commands`, `src/mesh`, `src/ui`, and `src/crimson`
- Qt 6 model/view documentation for model reset and QModelIndex guidance

## Validation Performed

- `python tools\project_board.py validate`
  - Passed: validated 6 active board entries and 21 archived entries.
- `python tools\check_architecture.py --root .`
  - Passed: architecture boundary checks passed.
- Header/source policy scan:
  - C++ source/header copyright banners are present in the scanned source set.
  - Old Doxygen delimiters such as `/*!`, `//!`, `\param`, and `\return` were not found.
- Dependency include scan:
  - Qt includes are confined to `src/app` and `src/ui`.
  - `bgfx`/`bx` includes are confined to the Crimson GPU implementation path.

No build, clang-format, clang-tidy, static-analysis target, or CTest preset was run for this audit artifact. The default listed CTest preset currently includes style/static-analysis tests, which require immediate explicit permission under the repository guardrails.

## Executive Summary

Quader's architecture is in a materially stronger state than the older full architecture audit: module targets exist, the automated boundary check passes, core/Qt/Crimson dependencies are mostly contained, documentation style scans are clean, and recent work has established real render graph, material, overlay, diagnostics, and selection infrastructure.

The largest remaining issues are not broad dependency violations. They are seams where old prototype-era contracts still carry production viewport data, where the live renderer upload path appears to rebuild and reupload unchanged document mesh buffers every frame, and where a few public APIs promise semantics the implementation does not yet honor.

The highest-value next work is:

1. Replace public `Prototype*` viewport/frame contracts with production-named frame submission DTOs.
2. Stop rebuilding and reuploading unchanged document mesh buffers on each frame.
3. Fix diagnostics model updates so frame stats changes do not reset the tree model.
4. Split style/static-analysis CTest gates from the agent-safe validation path.
5. Either implement command merging in `CommandHistory` or remove the unused merge contract.

## Strengths

- The automated architecture boundary check passes.
- Qt remains isolated to app/UI code in the scanned include set.
- Crimson GPU backend dependencies remain isolated to renderer implementation code.
- Public mesh and document APIs generally use typed IDs, generation-aware handles, `Result`-style failures, and explicit dirty/change tracking.
- The renderer now has real graph, material, pass, overlay, frame snapshot, and diagnostics concepts rather than a single monolithic demo path.
- The documentation style scan did not find legacy Doxygen delimiter forms.
- The active project board already tracks several user-visible issues found during this audit, including diagnostics expansion flicker and fly navigation capture.

## Architecture Audit Findings

### ACB-001 - Public prototype viewport contracts still carry production frame flow

Severity: P1
Area: Renderer/UI architecture

Evidence:

- `src/crimson/prototype_viewport.hpp` defines public `PrototypeCamera`, `PrototypeViewportFrame`, and related DTOs.
- `src/crimson/frame/frame_builder.hpp:29` exposes `build_prototype_snapshot(const PrototypeViewportFrame&)`.
- `src/ui/viewport/crimson_viewport_render_host.cpp` builds a `crimson::PrototypeViewportFrame` and passes it through the real frame builder path.
- `src/ui/viewport/viewport_types.hpp` still exposes `prototype_animation_enabled`.
- `src/crimson/graph/render_graph.cpp` contains `make_minimal_prototype_render_graph`.
- `src/crimson/renderer_types.hpp` still contains prototype-specific shader/pass/queue names such as `PrototypeGridOverlay`, `PrototypeOpaque`, and `PrototypeGrid`.

Risk:

The names no longer describe the role of these types. They are now the production viewport submission path for document meshes, overlays, diagnostics, picking support, selection state, and default material data. This makes new renderer work harder to reason about and keeps old demo semantics in public APIs.

Recommendation:

Introduce production-named frame submission contracts, for example `ViewportFrameInput`, `ViewportCamera`, `ViewportOverlayInput`, and `FrameBuildInput`. Migrate `CrimsonViewportRenderHost` and `FrameBuilder` to those names, then remove or isolate the old `Prototype*` compatibility types. Rename render graph/pass/shader identifiers when the API changes so test names and diagnostics match the real renderer path.

Validation:

- Focused renderer frame-builder tests.
- Existing render graph tests updated to production pass names.
- Viewport smoke test that opens the document viewport and confirms grid, document mesh, selection overlay, and diagnostics still render.

### ACB-002 - Live document mesh rendering rebuilds and reuploads unchanged mesh buffers

Severity: P1
Area: Renderer performance and resource lifetime

Evidence:

- `src/ui/viewport/crimson_viewport_render_host.cpp` computes a mesh revision by hashing vertices/faces and builds a fresh render upload from document mesh data on the render path.
- `append_document_render_data` calls the mesh upload conversion for document objects while assembling the frame.
- `src/crimson/gpu/gpu_device.cpp` iterates `snapshot.mesh_uploads()` and calls `GpuMeshCache::upload_mesh(...)` each frame.
- `src/crimson/gpu/gpu_mesh_cache.cpp` creates vertex and index buffers during `upload_mesh(...)` before upserting the resource.
- `src/crimson/mesh/render_mesh_upload.hpp` and `.cpp` already contain a `RenderMeshUploadTracker` with skipped-clean-revision semantics, but the live GPU submission path does not appear to use it.

Risk:

Unchanged meshes can still pay CPU traversal, triangulation, upload-vector construction, revision hashing, and GPU buffer allocation/upload cost during normal viewport frames. This becomes expensive as object count and polygon count increase. It also obscures the meaning of renderer diagnostics because "upload" activity may reflect frame redraw rather than document mesh edits.

Recommendation:

Make mesh upload lifetime revision-aware in the live path. Two viable approaches:

- Submit mesh upload descriptors only when document mesh revisions are dirty, and let frame snapshots reference cached renderer resource IDs for unchanged meshes.
- Or move revision checks into `GpuMeshCache::upload_mesh(...)` so unchanged `resource_id` plus `revision` pairs skip bgfx buffer recreation.

The first approach better separates document-to-render preparation from draw submission. The second is smaller and protects the GPU cache from accidental redundant uploads.

Validation:

- Add a focused test proving a repeated frame with unchanged mesh revision does not recreate GPU mesh resources.
- Extend existing upload tracker tests if the tracker becomes part of the live path.
- Inspect renderer diagnostics counters such as skipped clean resource count, uploaded bytes, and uploaded mesh count across two unchanged frames.
- Run relevant renderer upload and viewport tests. Do not claim a performance win without measurement.

### ACB-003 - Diagnostics model reset happens on each frame stats refresh

Severity: P2
Area: UI model/view correctness
Board overlap: Active bug #20

Evidence:

- `src/ui/qt_app/main_window.cpp` connects viewport frame stats changes to diagnostics refresh.
- `src/ui/viewport/viewport_controller.cpp` emits `frame_stats_changed` after renderer frame stats update.
- `src/ui/services/viewport_diagnostics_service.cpp` emits `diagnostics_changed` during refresh.
- `src/ui/models/diagnostics_item_model.cpp` connects `diagnostics_changed` to `reload_from_service`.
- `reload_from_service` uses `beginResetModel()`/`endResetModel()` for refreshes.

Risk:

Qt model reset is a radical change notification. Using it for frequent frame stat updates invalidates view state and can collapse expanded diagnostics categories. This matches active board bug #20.

Recommendation:

Preserve stable diagnostics nodes and update existing rows with `dataChanged` where possible. Use insert/remove row signals only when groups or metrics actually appear or disappear. Reserve full model reset for schema-level changes.

Validation:

- Reproduce with expanded diagnostics categories while frame stats are updating.
- Add a UI/model test around persistent group expansion or at least a model signal test proving a simple stats value update emits `dataChanged`, not reset.

### ACB-004 - The documented default CTest preset includes forbidden style/static-analysis gates

Severity: P1
Area: Workflow and validation

Evidence:

- `AGENTS.md` lists `ctest --preset qt-mingw-debug` among standard commands.
- Repository guardrails prohibit agents from running clang-format, clang-tidy, style/static-analysis targets, or CTest presets that include them without immediate explicit permission.
- `CMakeLists.txt` registers CTest entries for `clang_format` and `clang_tidy` when testing is enabled.

Risk:

The most obvious validation command for agents is also a command agents are not allowed to run by default. This creates avoidable uncertainty during implementation and review, and it can cause validation gaps when contributors correctly avoid the preset.

Recommendation:

Add an agent-safe test preset or label filter that excludes style/static-analysis gates, for example a `qt-mingw-debug-runtime-tests` preset or documented `ctest` command with `-LE style`. Keep clang-format and clang-tidy available as explicit permission-gated checks.

Validation:

- Confirm the new preset runs unit/render/runtime tests but excludes clang-format and clang-tidy.
- Update `AGENTS.md` and `agents/tests-policy.md` together so they do not contradict each other.

### ACB-005 - `CrimsonViewportRenderHost` owns too many conversion policies

Severity: P2
Area: UI/renderer ownership

Evidence:

- `src/ui/viewport/crimson_viewport_render_host.cpp` handles document mesh traversal, triangulation, generated UV fallback, mesh revision hashing, world bounds, renderer diagnostics conversion, selection overlay assembly, tool overlay assembly, and frame submission.
- The file pulls together document, mesh traversal, geometry normals, tools, frame builder, render mesh upload, and viewport controller concepts.

Risk:

The class is becoming an integration hub for unrelated policies. Even if the dependency direction is technically allowed, the call sites make it hard to tell which layer owns document-to-render conversion, upload dirtiness, overlay policy, and frame scheduling.

Recommendation:

Extract pure conversion units behind names that describe ownership:

- `DocumentRenderAdapter` for object/mesh to render mesh input conversion.
- `ViewportOverlayAdapter` for selection and tool overlays.
- A renderer-side or UI-side upload revision service, depending on where the final dirty-checking design lands.

Keep `CrimsonViewportRenderHost` responsible for orchestration: collect viewport state, call adapters, submit a frame, publish diagnostics.

Validation:

- Existing viewport and renderer tests should pass with no behavior change.
- The extracted adapters should get focused tests for triangulation, generated UVs, world bounds, and overlay payloads.

## Beautify Findings

### BF-001 - Command merge API is public but not used by command history

Severity: P2
Area: Command API elegance and correctness

Evidence:

- `src/commands/command.hpp` exposes `can_merge_with(...)` and `merge_with(...)`.
- `src/commands/command.cpp` defaults them to "not mergeable".
- `src/commands/command_history.cpp` executes commands, pushes them to undo, and clears redo without attempting to merge with the previous command.
- Search did not find concrete command overrides that rely on merge behavior.

Risk:

The API promises undo-stack coalescing, but the history implementation cannot honor it. Future commands may implement merge methods and appear correct in isolation while never merging at runtime.

Recommendation:

Choose one clear shape:

- Implement merge handling in `CommandHistory::execute(...)`, with tests for a mergeable command preserving undo/redo semantics.
- Or remove the merge methods until a concrete mergeable command lands.

The first option is preferable if upcoming transform, text, or drag-style tools need command coalescing.

### BF-002 - `MeshAttributes` collapses invalid attribute IDs into type mismatches

Severity: P2
Area: Mesh API semantics

Evidence:

- `src/mesh/core/mesh_error.hpp` defines `MeshErrorCode::InvalidAttribute`.
- `src/mesh/core/mesh_attributes.hpp` has `is_valid_attribute_id(...)`.
- `MeshAttributes::value<T>(...)` calls `typed_storage<T>(id)` and returns `AttributeTypeMismatch` when the storage lookup returns null.
- `typed_storage<T>(...)` returns null both for invalid IDs and for valid IDs with the wrong type.

Risk:

Callers cannot distinguish "this attribute does not exist" from "this attribute exists but was requested as the wrong type." The public error enum already expresses the distinction, so the current behavior is surprising and weakens diagnostics.

Recommendation:

Check `is_valid_attribute_id(id)` before type-specific storage lookup in value/descriptor paths that report errors. Return `InvalidAttribute` for bad IDs and `AttributeTypeMismatch` only for valid attributes with incompatible types.

Validation:

- Add focused mesh attribute tests for invalid ID, type mismatch, missing element, and successful typed value read.

### BF-003 - Prototype-era naming remains in tests, render queues, and options

Severity: P3
Area: Naming and future readability

Evidence:

- Render graph tests still assert names like `PrototypeOpaquePass`.
- Renderer enum values include `PrototypeOpaque`, `PrototypeGrid`, and `PrototypeGridOverlay`.
- Viewport options include `prototype_animation_enabled`.

Risk:

This is mostly readability debt, but it compounds ACB-001. New code must decide whether "prototype" means legacy, temporary, or simply "viewport."

Recommendation:

Fold this cleanup into the ACB-001 migration rather than doing one-off renames. Avoid creating new public names with "prototype" unless they truly denote a temporary demo fixture.

### BF-004 - Diagnostics refresh API is event-shaped but model-state-shaped work happens downstream

Severity: P3
Area: UI API shape
Board overlap: Active bug #20

Evidence:

- `ViewportDiagnosticsService::refresh(...)` replaces latest stats and emits one broad `diagnostics_changed` signal.
- `DiagnosticsItemModel` treats that broad signal as permission to rebuild all model rows.

Risk:

The broad event shape encourages heavy model work and hides whether a refresh changed values, structure, or both.

Recommendation:

Expose either a structured diagnostics snapshot diff or narrower service signals such as value update, group inserted, group removed, and schema reset. The model can then map service changes to precise Qt model notifications.

## Known Board-Covered Items Observed

- Bug #20: Diagnostics panel expansion flicker. This audit confirms the likely reset-on-refresh cause.
- Bug #21: Fly navigation needs mouse capture/hide/warp behavior. The board already tracks this as an active high-priority viewport interaction issue.
- Task #25: Selection parity is in review and overlaps current overlay/selection code paths.
- Task #26: Default albedo/PBR material is in review and overlaps current frame/material submission paths.

## Non-Findings

- No current evidence of Qt leaking into core/document/mesh/renderer-independent modules.
- No current evidence of bgfx/bx leaking outside Crimson GPU implementation code.
- No current evidence of legacy graphics runtime names in renderer folders or targets.
- No current evidence of old Doxygen delimiter usage in scanned source.
- No board mutation was needed for this audit.

## Recommended Next Actions

1. Create or update a board item for ACB-002 if no existing renderer upload/performance item covers it.
2. Resolve active bug #20 using precise `QAbstractItemModel` update signals instead of full resets.
3. Plan the prototype-to-production frame contract migration as one renderer/API cleanup task.
4. Add an agent-safe runtime-test preset before expecting agents to run broad CTest validation by default.
5. Decide whether command merging is a near-term feature. If yes, implement it in `CommandHistory`; if no, remove the public merge hooks until needed.

