# Task Intake Domain Findings: Mesh and Component Selection UI

## Classification

Recommended title: Reference-parity mesh/component selection modes, overlays, and flip mesh normals

Type: enhancement task, not a bug.

Priority: high/critical. The requested behavior depends on exact reference-app parity, especially selection overlays and sticky component source-wire policy.

Area: Qt Widgets UI actions, shortcuts, action state, viewport input routing, scene/properties panel projection, command dispatch, plus cross-domain dependencies on document selection, mesh commands, Crimson picking, and Crimson overlays.

Active plan path: none found for this exact feature. This intake should be reconciled into a cross-domain implementation plan before code starts.

Final-owner recommendation: `quader-software-architect` should own the board entry and cross-domain contract. Required domain owners are UI architect for actions/shortcuts/action state/panels/viewport routing, renderer architect for Crimson picking and overlay parity, and core architect for document selection commands and flip mesh normals.

## Duplicate And Overlap Check

No direct duplicate was found in `project_board.md` or `project_board_archive.md`.

Overlaps:

- Active `#13` transform gizmos: shares viewport input routing, overlays, picking, tool/gizmo precedence, and shortcuts.
- Active `#21` fly navigation mouse capture parity: shares viewport key/mouse capture and shortcut suppression while navigation owns input.
- Archived `#23` Box tool: establishes neutral viewport tool routing and tool preview overlays; this task must build on that foundation without putting selection behavior in `MainWindow`.
- Archived `#9` Crimson V1 rendering correctness: establishes overlay and picking foundation; this task extends it to semantic selection overlays and component picking.
- Archived `#8` UI models/panels: establishes document item/property models and object selection adapter; component selection must not make those models own document truth.
- Archived `#7` document, selection, commands, tool shell: establishes the current selection/command foundation, but the requested behavior extends it materially.

## Applicable Hindsight

- `H-20260705-001`: selection picking must share Crimson camera projection/ray behavior. Do not introduce UI-only picking math or one-off projection flips.
- `H-20260705-003`: flip normals and inverted-normal overlay behavior must not be masked with renderer culling or double-sided hacks. Fix winding/normal semantics and render the requested overlay policy explicitly.

## Source Refs

Source refs: `C:\Users\Drako\Desktop\quader-windows\quader-app` | `docs/overlay-handling.md:56-130`, `166-199`, `237-291`, `325-340`; `src/api/quader_viewport_input_types.hpp:34-39`, `130-157`; `src/editor/metadata/host_contract.cpp:170-176`, `727-736`, `782-820`; `src/editor/viewport/viewport_selection_interaction.cpp:41-49`, `101-127`, `298-509`; `src/editor/modeling/runtime/session_selection_overlay.cpp:401-444`, `662-848`; `src/editor/modeling/runtime/session_face_operations.cpp:43-66`; `src/mesh/polygon/operations/flip_face_normals_operation.cpp:38-79`; `tests/modeling_domain/operation/qdr_modeling_runtime_operation_invert_mesh_tests.cpp:9-17`.

Source refs: `C:\Users\Drako\Desktop\_quader-qt-base` | `src/document/selection.hpp:23-60`, `63-93`; `src/commands/selection_commands.cpp:23-43`, `59-85`; `src/ui/actions/action_id.hpp:17-49`; `src/ui/actions/action_registry.cpp:61-198`; `src/ui/actions/editor_action_controller.cpp:72-108`, `126-142`; `src/ui/actions/editor_state_snapshot.hpp:18-63`; `src/ui/models/document_selection_adapter.cpp:31-112`; `src/ui/viewport/viewport_controller.cpp:284-399`, `401-544`; `src/tools/editor_input_event.hpp:52-98`; `src/ui/viewport/crimson_viewport_render_host.cpp:479-525`, `639-693`; `src/crimson/overlays/overlay_command.hpp:20-54`; `src/crimson/picking/picking.hpp:25-45`; `tests/unit/ui/ui_action_wiring_tests.cpp:181-281`; `tests/unit/ui/ui_model_tests.cpp:340-405`; `tests/unit/ui/viewport_tests.cpp:245-363`.

## Current UI Findings

The current Qt action registry has file/edit/tool/view/panel actions, but lacks selection mode actions, select all, invert selection, clear/deselect selection behavior, and flip mesh normals. `EditorActionController` currently routes undo, redo, and delete only, and delete is restricted to one selected object with no component selection.

The current `EditorStateSnapshot` only exposes general selection presence and duplicate/delete availability. It does not expose active selection mode, selected object/component counts, select-all/invert/flip availability, or mode-specific checked state.

The current `DocumentSelectionAdapter` correctly keeps scene-tree selection object-only and command-routed through `SetSelectionCommand`, using a re-entry guard and `QSignalBlocker`. Component selection currently clears the object view selection. This is a good boundary and should not become component truth.

The current viewport controller handles navigation and tool pointer routing, but it does not carry primary/alt modifiers, click count, selection edit operation, active selection mode, or vertex/edge hit information. Its CPU surface hit path only reports face hits. This is insufficient for parity component selection.

The current Crimson render host appends document render objects and tool preview overlays only. Selection overlays are not present. Current `OverlayCommand` has primitive/depth/color/thickness, but no semantic source kind, overlay layer, style role, object/component identity, or source-wire/depth-stamp metadata required by the reference overlay policy.

## Proposed UI Scope

Add canonical `ActionId` values and action descriptors for:

- Object selection mode.
- Vertex selection mode.
- Edge selection mode.
- Face selection mode.
- Select all.
- Invert selection.
- Clear/deselect selection if required by reconciled command vocabulary.
- Flip mesh normals.

Mirror reference shortcuts where compatible:

- `1` and `Keypad1`: vertex mode.
- `2` and `Keypad2`: edge mode.
- `3` and `Keypad3`: face mode.
- `4` and `Keypad4`: object mode.
- `Ctrl+A`: select all.
- `Ctrl+I`: invert selection.
- `F`: flip mesh normals in viewport context.

Selection mode actions should be checkable and mutually exclusive. Use the existing action registry/state updater pattern and a `QActionGroup`-compatible setup instead of ad hoc menu or toolbar state.

Add state snapshot fields for active selection mode, selected object/component counts, and enablement for selection operations and flip mesh normals. Action state should come from document/tool snapshots, not direct widget polling.

Extend viewport event translation so Qt mouse/key input can provide primary/shift/alt modifiers, click count, active viewport pane, and selection edit intent to a neutral selection workflow. Preserve navigation capture precedence and avoid firing selection shortcuts while fly navigation or text input owns input.

Keep scene tree selection object-only unless a separate component model is explicitly designed. Property panels may show component summaries, but must remain adapters over document/controller state.

## Architecture Constraints

UI dispatches commands and hosts the viewport; it must not own document selection truth, mesh topology, or Crimson overlay policy.

Document/core must own:

- Active selection mode and mode transitions.
- Sticky component edit mesh policy.
- Select all, deselect, inverse select, and multi-select semantics.
- Undoable selection commands.
- Flip mesh normals operation.

Renderer must own:

- Component picking implementation and payload fidelity.
- Selection overlay projection, batching, layer order, depth modes, source-wire behavior, selected/hover handle rendering, inverted-normal behavior, and parity style constants.

Renderer architect must read `C:\Users\Drako\Desktop\quader-windows\quader-app\docs\overlay-handling.md` before finalizing renderer scope.

If implementation changes documented C++ symbols, Qt-facing APIs, viewport event structs, action IDs, behavior invariants, or overlay contracts, documentation must be updated in place in the same edit under the documentation policy.

## Recommended Sequencing

1. Software architect reconciles core, renderer, and UI findings into one cross-domain implementation plan and board entry.
2. Core/document defines active selection mode, selection edit operations, sticky component edit mesh state, select-all/invert/deselect semantics, multi-mesh component refs, undo/redo, and flip mesh normals.
3. UI adds action IDs, action descriptors, shortcuts, checked state, menu/toolbar placement, action-state snapshot updates, and command dispatch.
4. Viewport input routing adds modifiers, click count, mode-aware selection workflow hooks, and navigation/tool/gizmo precedence.
5. Renderer implements component picking and semantic selection overlay contracts, including reference color/thickness/depth/source-wire behavior.
6. UI model/panel projection is updated only after document truth and selection summaries are stable.
7. Runtime verification and dev-build checkpoint are performed before review approval.

## Risks

Overlay parity is the largest risk. The reference distinguishes object selection wire, component source wire, selected component overlays, hover overlays, source-wire depth stamp, x-ray face fill, and inverted-normal face behavior. The current overlay command surface is too generic to express that policy without additional semantic data or renderer-side selection overlay contracts.

Sticky selection policy is subtle and mode-specific. Object selection or active object alone is not enough to make component source wire sticky. Stickiness must depend on mode-specific selected components.

Multi-select across different meshes and different component kinds needs stable document refs and command semantics. The current `Selection` can hold component refs, but object and component selections are mutually exclusive and there is no active mode or sticky edit-target state.

Viewport selection must not regress navigation capture, box/tool routing, transform gizmo precedence, or keyboard shortcut suppression.

Inverted normal overlays and flip mesh normals must preserve real winding/normal semantics. Renderer culling workarounds would fail the acceptance criteria.

## Tests And Verification

Tests-policy applicability: yes. This task requires meaningful behavior tests across command, UI, viewport, and renderer seams.

Required tests:

- Action registry/action-state tests for selection modes, checked exclusivity, `Ctrl+A`, `Ctrl+I`, `F`, and enablement.
- Document/command tests for select, deselect, select all, inverse select, multi-select, undo/redo, stale IDs, and mode transitions for objects, vertices, edges, and faces.
- Sticky selection tests covering mode-specific source-wire object inference and fallback behavior.
- Viewport tests for modifier mapping, click count, navigation capture precedence, selection fallback, paint/box selection behavior if implemented, and shortcut suppression.
- Model tests keeping scene tree selection object-only and verifying component selection clears object view selection unless a component model is explicitly introduced.
- Renderer/adapter tests for semantic overlay layer output, source-wire draw-on-top policy, selected/hover component handle style, face fill/mask behavior, normal and inverted-normal parity, and color/thickness constants.
- Flip mesh normals tests for object mode and face/component mode behavior as approved by the core contract.

Verification/build checkpoint notes:

- Future implementation may run normal build and targeted tests such as `cmake --build --preset qt-mingw-debug` and targeted `ctest` selections.
- Do not run clang-format, clang-tidy, style/static-analysis targets, or any CTest preset containing them without immediate explicit user permission.
- Because this changes runtime UI/renderer behavior, review should reject approval unless the implementation report includes an archived dev build/path or a valid `Dev build checkpoint deferred:` blocker.
- Manual verification must launch `quader.exe` and cover object selection, component selection in vertex/edge/face modes, multi-mesh selection, deselect/select-all/invert, sticky source-wire policy, flip mesh normals, and normal/inverted-normal overlay parity.

## Qt Resources Checked

- https://doc.qt.io/qt-6/qaction.html: `QAction` is the correct shared command abstraction for menus, toolbars, shortcuts, checked state, enabled state, and triggered/toggled signals.
- https://doc.qt.io/qt-6/qactiongroup.html: `QActionGroup` supports mutually exclusive checkable actions, suitable for selection-mode controls.
- https://doc.qt.io/qt-6/qkeysequence.html: `QKeySequence` should be used for portable shortcut representation, including standard and custom shortcuts.
- https://doc.qt.io/qt-6/qitemselectionmodel.html: `QItemSelectionModel` remains appropriate for scene-tree object selection projection.
- https://doc.qt.io/qt-6/qsignalblocker.html: `QSignalBlocker` is the right re-entry guard for document/view selection sync.
- https://doc.qt.io/qt-6/qmouseevent.html: viewport event translation should preserve button, position, and modifier data before converting to neutral input structs.

## Board Command Note

No `project_board.py` command is issued by this UI scout. Board creation and exact authorized commands belong to `quader-software-architect`.
