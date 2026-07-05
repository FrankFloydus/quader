# Task Intake Domain Findings - UI

## Scope

UI scout scope covered Qt Widgets actions, menus, shortcuts, action state, tool-state presentation, selection-mode UI, model/view selection synchronization, visible sticky-selection behavior, viewport input routing expectations, and UI-side integration seams for selection overlays. No implementation was performed and `project_board.md` was not mutated.

The requested feature is not UI-only. Mesh and component selection truth must live in document/modeling commands and selection policy, while Crimson must own overlay rendering, picking semantics, colors, line widths, depth behavior, and inverted-normal overlay correctness. UI should expose actions and modes, dispatch commands, host the viewport, reflect current state, and test the widget/action behavior around those boundaries.

## Required Inputs Read

- `AGENTS.md`
- `agents/workflow.md`
- `agents/task_board.md`
- `agents/tests-policy.md`
- `agents/documentation-policy.md`
- `agents/hindsight.md`
- `agents/code-style.md`
- `agents/architecture/architecture.md`
- `agents/architecture/ui.md`
- Current repo nested `AGENTS.md` scan found only the root `AGENTS.md`.
- Reference-app nested docs read in inspected areas: `C:\Users\Drako\Desktop\quader-windows\quader-app\src\AGENTS.md`, `src\editor\AGENTS.md`, `src\ui\AGENTS.md`, `src\viewport\AGENTS.md`, `src\render\AGENTS.md`, and `tests\modeling_domain\AGENTS.md`.

Relevant accepted hindsight affecting this intake:

- `H-20260705-001`: selection/picking ray construction must stay aligned with Crimson camera projection. Do not add UI-only hand-rolled pick-ray math for selection.
- `H-20260705-003`: inverted normals should be validated through real topology/normal operations, not renderer-side masking. The complementary flip mesh operation is important for this reason.

## Current-Code Findings

- `src/ui/actions/action_id.hpp` lacks action IDs for `Select All`, `Clear/Deselect Selection`, `Invert Selection`, component selection modes (`Object`, `Vertex`, `Edge`, `Face`), and flip mesh normals. Existing IDs cover file actions, undo/redo, duplicate/delete, tool activation, creation, viewport actions, and panel toggles.
- `src/ui/actions/action_registry.cpp` registers current actions and shortcuts, but selection editing is limited to duplicate/delete. There is no selection menu/action family, no component-mode action group, and no shortcut parity for the reference selection modes or flip mesh normals.
- `src/ui/actions/editor_action_controller.cpp` routes only undo, redo, delete, and tool QAction toggles for the relevant area. Delete is currently object-only and requires exactly one selected object with no components. There is no routing for select all, deselect, invert, object/component mode changes, multi-select commands, or flip mesh normals.
- `src/ui/actions/editor_state_snapshot.hpp` and `src/ui/actions/editor_action_state_provider.cpp` expose `has_selection`, duplicate/delete enablement, and active tool state, but no current selection mode, component-selection availability, select-all/invert availability, sticky-policy state, or flip-mesh enablement.
- `src/ui/qt_app/main_window.cpp` has `File`, `Edit`, `Tools`, `Create`, `View`, and `Layout` menus. `Edit` contains undo/redo/duplicate/delete only. `Tools` contains tool actions only. The UI needs a parity location for selection commands and selection-mode actions without bloating `MainWindow`; keep action construction in the registry/controller pattern.
- `src/document/selection.hpp` and `src/document/selection.cpp` already define `ComponentKind`, typed component references, and a `Selection` value that can hold object refs or component refs. Object and component selection are currently mutually exclusive. It sorts/deduplicates and validates structural IDs, which is a useful base but is not enough for reference sticky selection policy, active object ownership, mode-aware component conversion, or multi-select operations.
- `src/document/document.hpp` and `src/document/document.cpp` own selection state and emit `SelectionChanged`. This is the correct source-of-truth seam. UI must not duplicate selection truth in widgets or viewport host state.
- `src/commands/selection_commands.hpp` and `src/commands/selection_commands.cpp` only provide replace selection and clear selection commands. The feature needs command-level support for select all, invert, add/remove/toggle multi-select, and mode-aware component selection so undo/redo remains coherent.
- `src/ui/models/document_selection_adapter.cpp` synchronizes document object selection to `QItemSelectionModel` and uses `QSignalBlocker`. Component selection currently clears the scene tree selection. This adapter should remain object-row focused unless a component model/view is introduced; do not make `QTreeView` become component truth.
- `src/ui/models/document_item_model.cpp` is object-row oriented. `SelectedRole` currently reflects object references only.
- `src/ui/viewport/viewport_types.hpp` already has typed pick payload kinds for object, submesh, face, edge, and vertex. This is a good UI-side contract seam for reference parity, but selection workflows are not yet using it end to end.
- `src/ui/viewport/viewport_widget.cpp` forwards mouse events to the controller and currently only passes Shift as snap state. It does not preserve the full modifier set needed for additive/toggle selection and sticky selection workflows.
- `src/ui/viewport/viewport_controller.cpp` routes pointer events to the active tool and has CPU face ray hit support, but no object/edge/vertex selection workflow and no mode-aware selection command dispatch. Navigation correctly consumes input before tools; selection work must preserve that precedence.
- `src/tools/editor_input_event.hpp` lacks modifier fields for selection edit operations and only reports grid/object/face surface hits. Component selection parity will need a richer editor input contract or a pick-result based path that keeps document and renderer responsibilities separated.
- `src/tools/tool_id.hpp` has `Select`, `Move`, `Rotate`, `Scale`, and `Box`, but `src/app/application.cpp` registers `Select`, `Move`, `Rotate`, and `Scale` as shell placeholders. A real Select tool or equivalent selection interaction service is required.
- `src/ui/viewport/tool_preview_overlay_adapter.cpp` handles only box preview overlays. It must not become the owner of selection overlay truth; selection overlay adaptation should consume semantic selection-overlay data from the appropriate core/renderer boundary.
- `src/ui/viewport/crimson_viewport_render_host.cpp` builds document render data and tool previews for Crimson. It is the likely UI-side integration point for passing selection-overlay semantic state into Crimson, while Crimson remains responsible for final overlay style and draw behavior.
- Existing UI tests provide useful seams: `tests/unit/ui/ui_action_wiring_tests.cpp`, `tests/unit/ui/ui_shell_tests.cpp`, `tests/unit/ui/ui_model_tests.cpp`, and `tests/unit/ui/viewport_tests.cpp`. They do not yet cover selection mode actions, selection command action state, multi-select modifier routing, or sticky-policy UI-visible behavior.

## Reference-App Findings

Source refs: C:\Users\Drako\Desktop\quader-windows\quader-app | src/ui/main_menu_bar.cpp:156-190; src/api/quader_command_ids.hpp:131-149,263-272; src/editor/metadata/host_contract.cpp:170-176,352-360,776-805; src/viewport/viewport_input_router.cpp:278-280,350-362; src/viewport/host_viewport_contract_adapter.cpp:485-543,579-588,625-643; src/ui/viewport_panel.cpp:1769-1785,1808-1823,1979-2022,2105-2118; src/editor/modeling/runtime/session.cpp:60-88,256-296,383-424,846-964; src/editor/modeling/runtime/selection_policy.cpp:149-183,1028-1120,1200-1345,1774-2015,2501-2589,2649-2778; src/editor/modeling/runtime/session_selection_overlay.cpp:401-424,450-884; src/editor/projection/selection_overlay_projection.cpp:30-61,99-127,219-226,264-273,377-430; src/viewport/viewport_settings.cpp:83-100; docs/overlay-handling.md:62-79,83-129,134-198,205-213,267-273,365-391; agents/rendering-overlays.md:57-68,74-87; tests/modeling_domain/editor_metadata/qdr_editor_metadata_shortcut_catalog_tests.cpp:217-225; tests/modeling_domain/qdr_modeling_runtime_overlay_tests.cpp:330-414,440-468,494-507,527-694,793-853,894-950,1460-1467; tests/modeling_domain/qdr_modeling_runtime_picking_tests.cpp:318-394

- The reference Edit menu exposes selection operations directly: `Select All`, `Clear Selection`, and `Invert Selection`. Flip operations appear near selection/editing commands, including horizontal/vertical flip and mesh-normal flip metadata.
- The reference action/shortcut catalog exposes `invert_mesh` as "Flip Mesh Normals", maps it to operation `flip_face_normals`, enables it in object mode for at least one selected object, and binds viewport shortcut `F`.
- Selection modes are first-class viewport/action state. The reference maps mode IDs for vertex, edge, face, object, and group through the host contract and input router. This task's requested modes are object/mesh, vertex, edge, and face; group mode should not be added unless the software architect explicitly scopes it.
- Selection-mode changes resynchronize editor selection state, input router mode, hover state, interaction controller state, modal drags, and overlay cache invalidation. UI parity is not just checked QAction state.
- Reference selection edits distinguish replace/add/remove/toggle, and selection commands map keyboard/mouse modifiers into those operations. Multi-select parity requires explicit modifier handling rather than only exposing menu commands.
- Reference object/component selection is mode-aware. Entering object mode clears component selection. Entering a component mode converts selected objects into that component selection mode. Component selection is aligned to the active mode and active object.
- Select all and invert selection are mode-specific. Object mode acts across selectable objects. Component modes act over mode-appropriate components, primarily on the active edit mesh/object.
- Sticky selection policy is mode-specific. A selected face makes source wire sticky only in face mode, an edge only in edge mode, and a vertex only in vertex mode. Object selection alone is not sticky ownership in component mode; it is only fallback when no mode-matching selected component or hover candidate owns source wire.
- Overlay policy is semantically layered: object-mode selected meshes emit selected topology lines and optional x-ray fill, while object-mode selected meshes do not emit `SourceWire`. Component mode uses `SourceWire` and `SourceWireDepthStamp` only for the active edit mesh/candidate, with selected/hover component handles depth-tested.
- Inverted-normal parity is explicitly tested in the reference. Face fills and source-wire depth behavior must remain correct for inside-out/open meshes. Overlay checks should validate that backface culling or nearest-hit mistakes do not hide or misclassify inverted normals.
- Reference overlay style is centralized. Colors, widths, depth-stamp behavior, selected/hover role mapping, and source-wire behavior are not local UI constants.

## Duplicate / Overlap Risks From Current Board

- No active board item appears to already cover full mesh/component selection parity.
- Backlog item `#13 Add Godot-style transform gizmos for move/rotate/scale tools` overlaps on viewport input routing, picking, active tool state, and overlay rendering. It is not a duplicate, but it should not define incompatible picking/modifier contracts before this selection work lands.
- Bug `#21 Viewport fly navigation loses mouse capture after reclicking viewport` overlaps on viewport input capture and routing. Selection work must preserve navigation precedence and mouse-capture fixes.
- Bug `#20 Scene tree diagnostics flicker after selection changes` is not duplicate, but selection model changes should preserve stable Qt model/view updates and avoid noisy reset/selection feedback loops.

## Recommended Board Entry

- Recommended title: `Add mesh and component selection parity with overlays and flip mesh normals`
- Type: feature task, with a required supporting operation slice for flip mesh normals.
- Priority: high. This is a foundational modeling workflow and blocks reliable transform, component-editing, overlay, and inverted-normal validation.
- Area: multi-domain feature: core/document commands and selection policy, UI actions/tool state, Crimson overlays/picking, and tests.
- Final owner recommendation: `quader-software-architect` should create and split the task. UI owns action/menu/shortcut/state and widget tests; core owns selection truth, commands, sticky policy, and flip operation; renderer owns overlay style, draw behavior, depth, and picking parity.
- Active plan path: none found. This intake artifact is `agents/plans/intake_20260705_mesh_selection_overlays_ui.md`.
- Summary: implement object mesh selection and vertex/edge/face component selection with 1:1 reference parity for selection edits, multi-select, mode changes, sticky active edit mesh behavior, overlays, and inverted-normal validation through a flip mesh normals operation.

Recommended board brief bullets:

- Add first-class selection mode state for object/mesh, vertex, edge, and face modes, exposed through Qt actions, menus/toolbars as appropriate, shortcut routing, and action-state snapshots.
- Add UI actions and command routing for select, deselect/clear, select all, inverse selection, additive multi-select, removal, and toggle selection for meshes and components.
- Match reference sticky selection policy exactly, including mode-specific source-wire ownership and fallback behavior.
- Add `Flip Mesh Normals` / flip mesh operation parity as a required supporting workflow so inverted-normal overlay checks can be exercised from the app.
- Ensure UI never owns document selection truth, topology truth, or Crimson overlay styling. UI dispatches commands and reflects document/renderer state.
- Integrate selection overlay semantic state into the viewport host without moving colors, line widths, depth policy, or inverted-normal rules into Qt widgets.
- Preserve navigation precedence, viewport focus/capture behavior, and existing scene tree selection synchronization.
- Update existing documentation in place if implementation changes documented C++ symbols, Qt-facing APIs, behavior, or invariants.

## Acceptance Criteria

- User can select, deselect, select all, invert selection, and multi-select multiple mesh objects through viewport interactions and relevant Qt actions/shortcuts.
- User can select, deselect, select all, invert selection, and multi-select vertices, edges, and faces in mode-correct component selection workflows.
- Object/mesh, vertex, edge, and face selection modes are visible in UI state, mutually exclusive where appropriate, shortcut-addressable, and synchronized with the active editor/viewport state.
- QAction enabled/checked state, menu entries, shortcuts, toolbar or mode controls, status behavior, and scene model selection remain consistent after document selection changes, undo/redo, tool changes, mode changes, and active document changes.
- Sticky selection policy matches the reference exactly: mode-matching selected components own sticky source wire; object selection alone does not own component-mode source wire except as fallback; hover candidate replacement and fallback behavior match the reference.
- Selection overlays match reference colors, thickness, draw order, depth behavior, object-mode selected wire/x-ray behavior, component selected/hover behavior, source-wire behavior, and `SourceWireDepthStamp` behavior.
- Inverted-normal selections behave like the reference for object and component overlays, including selected/hover face fills, edge/vertex handles, source wire, and occluded interior picking rejection.
- `Flip Mesh Normals` exists with reference-equivalent UI exposure, shortcut availability, action state, undoable command behavior, and verification coverage sufficient to create inverted-normal test scenes.
- Existing viewport navigation, mouse capture, and current tools continue to work after selection interactions are added.

## Tests / Checks

UI-specific tests to require:

- Extend `tests/unit/ui/ui_shell_tests.cpp` for canonical action registration, menus/toolbars, selection mode actions, shortcut metadata, and exclusive checked state.
- Extend `tests/unit/ui/ui_action_wiring_tests.cpp` for select all, clear/deselect, invert, flip mesh normals, selection mode changes, enabled/disabled state, undo/redo state, and command dispatch.
- Extend `tests/unit/ui/ui_model_tests.cpp` for object selection synchronization when document selection changes through new commands, component selection clearing or non-projecting into the scene tree, and no feedback loops.
- Extend `tests/unit/ui/viewport_tests.cpp` for full modifier propagation, selection mode routing, navigation precedence, typed pick payload dispatch, and viewport host overlay invalidation hooks.
- Add UI-side integration tests or fakes that prove action state reflects document selection mode and that QActionGroup/exclusive mode state stays synchronized after model changes.

Cross-domain tests to require from owning domains:

- Core/modeling tests for object/component select, deselect, select all, invert, add/remove/toggle, mode conversion, active object, undo/redo, and flip mesh normals.
- Renderer/viewport tests for selected mesh overlays, component overlays, source wire, source-wire depth stamps, inverted-normal face fills, depth-tested component handles, and picking on flipped/inverted meshes.
- Reference-parity tests should encode sticky source-wire precedence and inverted-normal regressions from the reference docs/tests.

Build and verification checkpoint notes:

- Targeted unit test binaries are appropriate for development verification.
- `cmake --build --preset qt-mingw-debug` is an ordinary build checkpoint for implementation work.
- `ctest --preset qt-mingw-debug` may be used only if it does not include style/static-analysis gates; inspect before running if uncertain.
- `cmake --build --preset qt-mingw-debug-tests`, clang-format, clang-tidy, style/static-analysis targets, or CTest presets that include them are gated checks requiring immediate explicit user permission unless the prompt says `run now without asking`.
- Because this changes runtime UI/viewport behavior, final review should require either an archived dev build path/version or a valid `Dev build checkpoint deferred:` blocker per workflow policy.

Tests-policy applicability:

- This is not docs-only. It changes C++ behavior, Qt Widgets action state, selection workflows, and renderer-visible output, so focused unit/integration coverage is required. Do not rely on manual viewport inspection alone.

## Architecture Constraints

- UI actions and widgets must dispatch intent through existing application/controller/command seams. They must not mutate document internals or hold authoritative selection state.
- Document/core/modeling commands must own selection truth, component identity, active object, selection edit operations, sticky policy, undo/redo behavior, and flip mesh normals.
- Crimson must own overlay rendering, overlay style roles, colors, line widths, draw order, depth policy, source-wire depth stamps, and inverted-normal correctness.
- Viewport host UI may adapt document/selection/overlay semantic state into renderer inputs, but it must not become a second renderer policy layer.
- Selection picking must share the Crimson camera/projection contract. Avoid UI-only ray math that can drift from rendered pixels.
- Selection modes should use a Qt action-state pattern consistent with the existing `ActionRegistry`, `EditorActionController`, `EditorActionStateProvider`, and `ActionStateUpdater`; use an exclusive QAction group or equivalent local pattern for mode actions.
- Scene tree selection should remain model/view synchronized through `DocumentSelectionAdapter` and `QSignalBlocker`-style guarded updates. Do not make the scene tree a component-selection authority unless a separate designed component model is introduced.
- If implementation changes documented C++ symbols, documented Qt-facing APIs, or documented behavior/invariants, update the existing documentation in place in the same edit.

## Dependencies On Core / Renderer

- Core/document dependency: selection needs a richer command and policy model for object and component add/remove/toggle/replace, select all, clear, invert, mode transitions, active object, and undo/redo.
- Core/mesh dependency: flip mesh normals must be introduced as a command/operation available from UI and tests. It should be mode/selection aware and equivalent to the reference `Flip Mesh Normals` action.
- Renderer dependency: Crimson needs semantic overlay inputs and policy for selected object wire/x-ray, source wire, source-wire depth stamps, selected/hover face fills, selected/hover edge and vertex handles, and inverted-normal draw/picking behavior.
- Viewport dependency: picking must produce object/face/edge/vertex candidates with depth and occlusion semantics compatible with the rendered scene and reference behavior.
- UI dependency: action state requires document/editor state snapshots that expose current selection mode, available modes, selection counts or availability, and flip-operation availability without leaking core topology into widgets.

## Qt Resources Checked

- https://doc.qt.io/qt-6/qaction.html - `QAction` is the correct reusable command abstraction for menus, toolbars, and shortcuts. Qt documents checkable actions, enabled state, shortcuts, shortcut context, and automatic UI synchronization across surfaces.
- https://doc.qt.io/qt-6/qactiongroup.html - `QActionGroup` supports mutually exclusive checkable actions and exposes `checkedAction()`, matching the selection-mode/tool-mode requirement.
- https://doc.qt.io/qt-6/qitemselectionmodel.html - `QItemSelectionModel` supports `Select`, `Deselect`, `Toggle`, and `ClearAndSelect` flags and two-layer current/committed selection. It is appropriate for scene-tree row projection but should not become authoritative component selection truth.
- https://doc.qt.io/qt-6/qsignalblocker.html - `QSignalBlocker` is the Qt RAII mechanism for avoiding feedback loops during programmatic model/view selection synchronization, matching the existing adapter approach.

## Hindsight Candidate

Area/tags: UI, renderer, selection, overlays, reference parity, sticky selection

Lesson: Component-mode source-wire ownership is mode-specific. A selected face should make source wire sticky only in face mode, a selected edge only in edge mode, and a selected vertex only in vertex mode. Object selection alone should not own component-mode source wire except as fallback when no mode-matching selected component or hover candidate exists.

Evidence: Reference docs and tests encode this in `docs/overlay-handling.md:83-129,267-273`, `src/editor/modeling/runtime/session_selection_overlay.cpp:401-424,450-884`, and `tests/modeling_domain/qdr_modeling_runtime_overlay_tests.cpp:550-694,894-950`.

Applies when: implementing or reviewing mesh/component selection overlays, sticky active edit mesh behavior, hover replacement, component-mode transitions, or source-wire regressions.
