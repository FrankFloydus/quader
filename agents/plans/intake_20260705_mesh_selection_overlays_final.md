# Final Task Intake Report - Mesh Selection Overlays

Run id: 20260705_mesh_selection_overlays
Report path: agents/plans/intake_20260705_mesh_selection_overlays_final.md
Mode: TASK_INTAKE_FINALIZATION
Date: 2026-07-05

## Required Inputs Read

Policy and architecture inputs:

- AGENTS.md
- agents/workflow.md
- agents/task_board.md
- agents/hindsight.md
- agents/tests-policy.md
- agents/documentation-policy.md
- agents/code-style.md
- agents/architecture/architecture.md
- agents/architecture/ui.md
- agents/architecture/renderer.md

Domain intake reports:

- agents/plans/intake_20260705_mesh_selection_overlays_core.md
- agents/plans/intake_20260705_mesh_selection_overlays_ui.md
- agents/plans/intake_20260705_mesh_selection_overlays_renderer.md

Duplicate inputs checked:

- project_board.md
- project_board_archive.md
- `python tools/project_board.py list`
- `python tools/project_board.py archive-search --query "selection overlay" --full`
- `python tools/project_board.py archive-search --query "flip normals" --full`
- `python tools/project_board.py archive-search --query "component selection" --full`

## Hindsight Gate

Matching accepted hindsight from agents/hindsight.md:

- H-20260705-001 constrains picking/ray/projection work. Selection and component picking must share Crimson camera projection helpers and must not add UI-only divergent ray math.
- H-20260705-003 constrains flip-normal validation. Flip Mesh Normals must reverse actual face winding and must not be faked with Crimson cull-state, double-sided lit materials, lighting hacks, or shader-side normal fixes.

The core report also referenced H-20260705-002. It remains relevant to general reference parity involving signed basis/corner order, but it does not directly change this selection/overlay board brief beyond the broader rule to verify reference conventions instead of assuming Crimson math matches the Windows app.

## Hindsight Candidate Decisions

The UI hindsight candidate is accepted as critical and constrains the board brief. It should be routed to the hindsight register by root or the docs maintainer. Exact accepted text:

```text
### H-20260705-004 - Component source-wire ownership is mode-specific

- Area/tags: UI, renderer, selection, overlays, reference-parity, sticky-selection
- Status: active
- Owner: software
- Applies when: Implementing or reviewing mesh/component selection overlays, sticky active edit mesh behavior, hover replacement, component-mode transitions, or source-wire regressions.
- Hindsight: Component-mode source-wire ownership is mode-specific. A selected face makes source wire sticky only in face mode, a selected edge only in edge mode, and a selected vertex only in vertex mode; object selection alone is only a fallback when there is no mode-matching selected component or hover candidate.
- Evidence: `C:\Users\Drako\Desktop\quader-windows\quader-app\docs\overlay-handling.md:83-129`, `C:\Users\Drako\Desktop\quader-windows\quader-app\docs\overlay-handling.md:267-273`, `C:\Users\Drako\Desktop\quader-windows\quader-app\src\editor\modeling\runtime\session_selection_overlay.cpp:401-424`, `C:\Users\Drako\Desktop\quader-windows\quader-app\src\editor\modeling\runtime\session_selection_overlay.cpp:450-884`, `C:\Users\Drako\Desktop\quader-windows\quader-app\tests\modeling_domain\qdr_modeling_runtime_overlay_tests.cpp:550-694`, `C:\Users\Drako\Desktop\quader-windows\quader-app\tests\modeling_domain\qdr_modeling_runtime_overlay_tests.cpp:894-950`.
- Last checked: 2026-07-05
- Review when: Selection mode ownership, source-wire fallback policy, or the Windows reference parity target changes.
```

The renderer hindsight candidate is accepted as critical and constrains the board brief. It should be routed to the hindsight register by root or the docs maintainer. Exact accepted text:

```text
### H-20260705-005 - Source-wire parity needs semantic depth-stamp metadata

- Area/tags: renderer, selection-overlays, picking, inverted-normals, reference-parity, source-wire
- Status: active
- Owner: renderer
- Applies when: Planning or reviewing Crimson selection overlays, component handles, source wire, inverted-normal validation, or future gizmo/diagnostic overlay features that need layer/source/depth semantics.
- Hindsight: Source-wire parity is not an overlay layer toggle. It requires semantic source kind plus metadata-only depth stamps; `SourceWire` itself stays draw-on-top, component selected/hover handles become depth-tested only when they carry component source, and selected/hover face fills need depth-stamped two-pass, two-sided rendering to remain correct on flipped or inverted meshes.
- Evidence: `C:\Users\Drako\Desktop\quader-windows\quader-app\docs\overlay-handling.md:132-199`, `C:\Users\Drako\Desktop\quader-windows\quader-app\src\render\viewport_overlay_policy.cpp:73-138`, `C:\Users\Drako\Desktop\quader-windows\quader-app\src\render\overlay\overlay_renderable_builder.cpp:270-448`, `C:\Users\Drako\Desktop\quader-windows\quader-app\tests\core_tests.cpp:1806-1845`, `src/crimson/overlays/overlay_command.hpp:20-115`.
- Last checked: 2026-07-05
- Review when: Crimson gains a semantic selection-overlay frame with depth-stamp metadata and parity tests for flipped/inverted mesh overlays, or the Windows reference parity target changes.
```

## Duplicate Search Result

Decision: new task needed.

No active board or archive item duplicates full mesh/object/component selection parity, sticky source-wire overlays, component picking, and Flip Mesh Normals. Direct archive searches for `selection overlay`, `flip normals`, and `component selection` returned no exact archived task.

Relevant overlaps that must be coordinated:

- Active #13 `Add Godot-style transform gizmos to the viewport` overlaps viewport input routing, picking, overlays, and future transform target semantics. Selection contracts should land before or be explicitly coordinated with #13.
- Active #21 `Restore fly navigation mouse capture in the Qt viewport` overlaps viewport mouse/key routing. Selection input must preserve right-button fly/navigation precedence.
- Active #20 diagnostics flicker is not a duplicate, but scene tree/model selection updates must avoid reset/flicker regressions.
- Archived Box tool work established ray/projection and surface-hit lessons, but did not require selection behavior.
- Archived #9 Crimson foundation established overlay/picking foundations, but did not implement semantic selection overlays, component picking, source-wire depth stamps, or Flip Mesh Normals.

## Final Decision

Add one cross-domain enhancement task.

Do not split into separate board tasks at intake. The feature has one parity surface: document selection truth, UI selection mode/action routing, Crimson semantic overlays/picking, and Flip Mesh Normals must agree. Implementation should be split at execution time after freshness revalidation and the execution split gate.

Third-party library decision: no new third-party dependency is recommended. Use existing Qt action/model/view patterns, core mesh/document/command contracts, and Crimson overlay/render graph infrastructure. The Windows reference app and renderer references are parity/research inputs only and must not be copied verbatim.

## Board Entry

Title: Add mesh and component selection parity with overlays and flip mesh normals

Type: enhancement

Priority: critical

Area: modeling

Summary: Add object and vertex/edge/face component selection with Windows reference parity, sticky source-wire overlays, typed picking, and undoable Flip Mesh Normals.

Final owner: multi-domain; software architect finalizes cross-domain selection contracts and final approval, core owns document/commands/mesh selection and flip operation, UI owns Qt actions/input/state, renderer owns Crimson overlays/picking/render tests.

Active plan path: agents/plans/intake_20260705_mesh_selection_overlays_final.md

Freshness: leave default `needs-refresh` from the add command. Root must revalidate before implementation.

Source refs:

```text
C:\Users\Drako\Desktop\quader-windows\quader-app | docs/overlay-handling.md:39-406; src/mesh/polygon/document_selection.hpp:19-121; src/mesh/polygon/quader_poly_document_selection_api.cpp:19-263; src/editor/modeling/runtime/session.cpp:846-964; src/editor/modeling/runtime/selection_policy.cpp:149-183,1028-1120,1200-1345,1774-2015,2501-2589,2649-2778; src/editor/modeling/runtime/session_selection_overlay.cpp:401-439,663-846,887-948; src/render/viewport_overlay_frame.hpp:20-222; src/render/viewport_overlay_policy.hpp:45-151; src/render/viewport_overlay_policy.cpp:23-340; src/render/overlay/viewport_overlay_style_resolver.cpp:35-312; src/editor/modeling/runtime/session_picking.cpp:1713-1785,1977-2148; src/editor/interactions/select_tool.cpp:558-606,705-820,904-919; src/mesh/polygon/operations/flip_face_normals_operation.cpp:37-80; src/editor/modeling/operations/selection_operations.cpp:209-213; tests/modeling_domain/qdr_modeling_runtime_overlay_tests.cpp:330-414,527-694,894-950; tests/modeling_domain/qdr_modeling_runtime_picking_tests.cpp:650-787; tests/render_extraction_tests.cpp:178-229,275-367
```

Brief:

- Add document-backed mesh object selection plus vertex, edge, and face component selection modes with parity behavior against `C:\Users\Drako\Desktop\quader-windows\quader-app`.
- Root/implementers must read and apply `C:\Users\Drako\Desktop\quader-windows\quader-app\docs\overlay-handling.md` before planning or implementing selection overlays.
- Keep source/reference parity scope strict. Match behavior and structure as closely as practical without unsafe verbatim copying, and adapt Filament/reversed-depth renderer conventions to Crimson/bgfx instead of copying them literally.
- Store durable selection as typed domain references: `ObjectId` plus `VertexId`, `EdgeId`, or `FaceId`. Renderer raw element indexes or packed transient IDs must be resolved and generation-validated before command creation.
- Add command-backed selection verbs for replace/select-only, add, remove/deselect, toggle/multi-select, clear, select all, invert selection, and selection mode changes. Every user-visible mutation must support undo/redo and liveness validation.
- Implement reference mode behavior: object mode acts on whole mesh objects; component modes act on active/edit-target mesh components; mode transitions, active object/edit target, hover candidate, preserve-selection-on-selected-hit, and multi-click expansion must match the reference.
- Implement the sticky source-wire policy exactly: current-mode selected components own source wire; selected components from other modes do not; hover candidate wins when no current-mode selected component owns it; active/first selected object is fallback only.
- Add the complementary `Flip Mesh Normals` workflow as required scope. It must reverse selected face winding through core mesh operations and commands; object mode flips all faces on selected meshes, face mode flips selected faces, empty/no-live-face selections reject without mutation, and undo/redo restores exact winding and selection state.
- Do not fake inverted normals in Crimson. Render upload/flat normals and overlay behavior must reflect actual winding changes.
- UI must expose appropriate Qt actions/modes/shortcuts/action state for selection, select all, clear, invert, component modes, and Flip Mesh Normals while preserving fly navigation capture/precedence and model/view selection synchronization.
- Crimson must receive semantic selection overlay data and own renderer styling and draw behavior: selected object topology, selected/hover face fills, selected/hover edges and vertices, source wire, source-wire depth-stamp metadata, colors, widths, point sizes, depth modes, draw order, two-sided face fills, and inverted-normal parity.
- Keep overlays unlit and outside lighting, shadows, IBL, fog, bloom, SSAO, tone mapping, and normal scene/picking ID paths except for explicitly planned picking passes.
- Use shared Crimson camera/projection helpers for rendered view, CPU picking, GPU picking, hover previews, and overlay projection. H-20260705-001 applies.
- Keep core/document/mesh/commands free of Qt Widgets and Crimson GPU/runtime dependencies; keep UI free of graphics-runtime headers; keep Crimson from owning modeling selection truth.
- If implementation changes any documented C++ symbol, documented Qt-facing API, Crimson schema, or documented behavior/invariant, update that documentation in place in the same edit.

Likely files/modules:

- `src/document/selection.*`
- `src/document/document.*`
- `src/document/scene_object.*`
- `src/commands/selection_commands.*`
- command modules for mesh/component operations
- `src/mesh/core/polyhedron.*`
- `src/mesh/core/mesh_traversal.*`
- new or existing `src/mesh/ops/*flip*`
- `src/tools/*select*`
- `src/tools/editor_input_event.hpp`
- `src/tools/tool_context.*`
- `src/ui/actions/*`
- `src/ui/models/document_selection_adapter.*`
- `src/ui/qt_app/main_window.cpp`
- `src/ui/viewport/*`
- `src/crimson/overlays/*`
- `src/crimson/picking/*`
- `src/crimson/frame/*`
- Crimson GPU overlay/pass/shader files as required
- targeted tests under `tests/unit/document`, `tests/unit/commands`, `tests/unit/mesh`, `tests/unit/tools`, `tests/unit/ui`, and `tests/unit/crimson`

Tests-policy applicability: applies. This is executable C++ behavior spanning document state, mesh operations, commands, UI actions, viewport input, picking, and renderer-visible overlay output. Manual viewport parity checks are necessary but not sufficient.

Expected verification:

- `cmake --build --preset qt-mingw-debug` after coherent implementation slices.
- Targeted unit/render/UI tests for changed behavior.
- `cmake --build --preset qt-mingw-debug-deploy` before review because this changes user-visible runtime behavior.
- Dev-build archive before review: `python tools/dev_builds.py archive --preset qt-mingw-debug`, unless a valid `Dev build checkpoint deferred:` blocker is reported.
- Before any broad `ctest --preset qt-mingw-debug`, inspect whether the preset includes clang-format, clang-tidy, or style/static-analysis gates. If it does, immediate explicit user permission is required.
- Gated checks requiring immediate explicit user permission unless the prompt says `run now without asking`: clang-format, clang-tidy, `check_format`, `check_clang_tidy`, `check_static_analysis`, raw style/static-analysis tools, and any CTest preset that includes them.

## Exact Board Command

Run this block exactly from the repository root. The `$brief` value is joined into one line so `project_board.py` preserves board entry shape.

```powershell
$brief = @(
  'Source: Final Task Intake Report agents/plans/intake_20260705_mesh_selection_overlays_final.md. Source refs: C:\Users\Drako\Desktop\quader-windows\quader-app | docs/overlay-handling.md:39-406; src/mesh/polygon/document_selection.hpp:19-121; src/mesh/polygon/quader_poly_document_selection_api.cpp:19-263; src/editor/modeling/runtime/session.cpp:846-964; src/editor/modeling/runtime/selection_policy.cpp:149-183,1028-1120,1200-1345,1774-2015,2501-2589,2649-2778; src/editor/modeling/runtime/session_selection_overlay.cpp:401-439,663-846,887-948; src/render/viewport_overlay_frame.hpp:20-222; src/render/viewport_overlay_policy.hpp:45-151; src/render/viewport_overlay_policy.cpp:23-340; src/render/overlay/viewport_overlay_style_resolver.cpp:35-312; src/editor/modeling/runtime/session_picking.cpp:1713-1785,1977-2148; src/editor/interactions/select_tool.cpp:558-606,705-820,904-919; src/mesh/polygon/operations/flip_face_normals_operation.cpp:37-80; src/editor/modeling/operations/selection_operations.cpp:209-213; tests/modeling_domain/qdr_modeling_runtime_overlay_tests.cpp:330-414,527-694,894-950; tests/modeling_domain/qdr_modeling_runtime_picking_tests.cpp:650-787; tests/render_extraction_tests.cpp:178-229,275-367.'
  'Scope: add document-backed mesh object selection plus vertex, edge, and face component selection modes with strict parity against C:\Users\Drako\Desktop\quader-windows\quader-app. Implementers must read and apply C:\Users\Drako\Desktop\quader-windows\quader-app\docs\overlay-handling.md before planning or implementing selection overlays. Match behavior and structure as closely as practical without unsafe verbatim copying, and adapt Filament/reversed-depth conventions to Crimson/bgfx instead of copying renderer math literally.'
  'Selection truth: store durable typed domain references as ObjectId plus VertexId, EdgeId, or FaceId. Renderer raw element indexes and packed transient IDs may be transport only and must be resolved and generation-validated before command creation. Add command-backed replace/select-only, add, remove/deselect, toggle/multi-select, clear, select all, invert selection, and selection-mode changes with undo/redo and liveness validation.'
  'Parity behavior: object mode acts on whole mesh objects; component modes act on active/edit-target mesh components; mode transitions, active object/edit target, hover candidate, preserve-selection-on-selected-hit, and multi-click expansion must match the reference. Sticky source-wire policy is exact scope: current-mode selected components own source wire; other-mode selected components do not; hover wins when no current-mode selected component owns it; active/first selected object is fallback only.'
  'Required complementary deliverable: add Flip Mesh Normals. It must reverse selected face winding through core mesh operations and commands; object mode flips all faces on selected meshes, face mode flips selected faces, empty/no-live-face selections reject without mutation, undo/redo restores exact winding and selection state, and Crimson must not fake inverted normals with cull-state, lit material, lighting, or shader-normal hacks.'
  'UI scope: expose Qt actions/modes/shortcuts/action state for selection, select all, clear, invert, object/vertex/edge/face modes, and Flip Mesh Normals while preserving fly navigation capture/precedence and model/view selection synchronization. UI dispatches commands and never owns document selection truth or Crimson overlay styling.'
  'Renderer scope: Crimson receives semantic selection overlay data and owns selected object topology, selected/hover face fills, selected/hover edges and vertices, source wire, source-wire depth-stamp metadata, colors, widths, point sizes, depth modes, draw order, two-sided face fills, and inverted-normal parity. Overlays stay unlit and outside lighting, shadows, IBL, fog, bloom, SSAO, tone mapping, and normal scene/picking ID paths except for explicitly planned picking passes.'
  'Architecture constraints: use shared Crimson camera/projection helpers for rendered view, CPU picking, GPU picking, hover previews, and overlay projection. Keep core/document/mesh/commands free of Qt Widgets and Crimson GPU/runtime dependencies; keep UI free of graphics-runtime headers; keep Crimson from owning modeling selection truth. Update existing documentation in place if documented C++ symbols, Qt-facing APIs, Crimson schemas, or documented behavior/invariants change.'
  'Execution split: parallel-required. Suggested slices are core document/command/mesh selection plus flip normals; UI action/input/tool/state; renderer overlay/picking/pass/shader; integration/verification. Coordinate with active #13 transform gizmos and #21 fly navigation. Tests-policy applies. Verification should include cmake --build --preset qt-mingw-debug, targeted tests, deploy for runtime UI behavior, dev-build archive before review, and board validation. Style/static-analysis and CTest presets containing those gates require immediate explicit user permission before execution.'
) -join ' '
python tools/project_board.py add --architect-authorized --title "Add mesh and component selection parity with overlays and flip mesh normals" --summary "Add object and vertex/edge/face component selection with Windows reference parity, sticky source-wire overlays, typed picking, and undoable Flip Mesh Normals." --brief $brief --priority critical --type enhancement --area modeling --final-owner "multi-domain; software architect finalizes cross-domain selection contracts and final approval, core owns document/commands/mesh selection and flip operation, UI owns Qt actions/input/state, renderer owns Crimson overlays/picking/render tests" --plans agents/plans/intake_20260705_mesh_selection_overlays_final.md
```

## Execution Split Guidance

Classification: parallel-required.

Rationale: cross-domain critical task, active plan, runtime C++/UI/renderer/shader/picking behavior, likely more than three files, and separable implementation/test/docs/build slices.

Suggested worker slices:

1. Core/document/command/mesh slice: selection mode model, typed component references, liveness validation, command verbs, undo/redo, and Flip Mesh Normals operation/command/tests.
2. UI/tool/input slice: action IDs/registry/controller/state, selection mode controls, shortcuts, modifier routing, viewport selection tool behavior, scene tree synchronization, and UI tests.
3. Renderer/picking/overlay slice: semantic selection overlay frame, overlay roles/source kinds/depth stamps, point/triangle/screen-space line support, picking boundary, shader/manifest/render graph changes, and Crimson tests.
4. Integration/verification slice: end-to-end selection parity, inverted-normal viewport checks, coordination with #13/#21, targeted test execution, deploy, and dev-build archive.

Dependencies that must serialize:

- Core selection/command API must settle before UI dispatch and renderer overlay extraction can finalize.
- Renderer overlay payload schema must settle before UI viewport host adapters and semantic overlay tests finalize.
- Fly-navigation input precedence from #21 must be preserved or revalidated before selection input is approved.
- Transform gizmo task #13 must consume this selection contract or explicitly coordinate if it starts first.

Expected verification owners:

- Core owns document/command/mesh tests and flip-normal correctness.
- UI owns action/menu/shortcut/state/model/view/viewport routing tests.
- Renderer owns Crimson overlay/picking/render tests and visual parity checks.
- Root/integrator owns final build, deploy, dev-build archive, board validation, and review handoff.

## Blockers

No blocker to adding the board task.

Implementation blockers after add:

- Root must run start-gate freshness revalidation before implementation.
- Root must run the execution split gate and use scoped workers; solo execution is not acceptable for this task.
- Accepted hindsight candidates in this report should be routed to the hindsight register, but that routing does not block board task creation.
