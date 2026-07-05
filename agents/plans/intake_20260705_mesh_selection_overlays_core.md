# Task Intake Domain Findings - Core

## Scope

Core scout coverage for adding mesh selection, component selection modes, selection overlays, and the complementary flip mesh normals operation with 1:1 parity to `C:\Users\Drako\Desktop\quader-windows\quader-app`.

Covered:

- Document/modeling selection state contracts for mesh objects and vertex/edge/face components.
- Command, validation, undo, and redo expectations for selection mutation and flip normals.
- Mesh/topology requirements for face-winding reversal and selection liveness.
- Picking IDs as durable domain references rather than renderer-only element indexes.
- Core-side semantic overlay ownership and sticky source-wire policy.
- Core tests and verification needed before UI/renderer parity approval.

Not covered as owner work:

- Exact UI controls, toolbar presentation, and keyboard/mouse routing details beyond required contracts.
- Exact Crimson drawing implementation, GPU state, render extraction, and pixel/color verification, except as core dependencies.
- Board mutation. This report is an intake artifact only.

Required docs read:

- `AGENTS.md`
- `agents/workflow.md`
- `agents/task_board.md`
- `agents/tests-policy.md`
- `agents/documentation-policy.md`
- `agents/hindsight.md`
- `agents/code-style.md`
- `agents/architecture/architecture.md`

Nested current-repo `AGENTS.md`: none found under `C:\Users\Drako\Desktop\_quader-qt-base` beyond the root file.

Relevant reference nested docs read:

- `C:\Users\Drako\Desktop\quader-windows\quader-app\AGENTS.md`
- `src/AGENTS.md`
- `src/editor/AGENTS.md`
- `src/mesh/AGENTS.md`
- `src/render/AGENTS.md`
- `src/scene/AGENTS.md`
- `src/api/AGENTS.md`
- `tests/AGENTS.md`
- `tests/modeling_domain/AGENTS.md`

Relevant hindsight applied:

- `H-20260705-001`: tool rays must share Crimson camera projection. Selection picking must use the same projection path as rendered views.
- `H-20260705-003`: generated box faces must stay outward-oriented. Flip normals tests must validate true winding reversal and must not hide bad winding through renderer workarounds.

## Current-Code Findings

Current repo refs: C:\Users\Drako\Desktop\_quader-qt-base | src/document/selection.hpp:23-100; src/document/selection.cpp:60-152; src/document/document.cpp:100-120,229-257,356-372; src/commands/selection_commands.cpp:23-84; src/commands/document_commands.cpp:87-124; src/mesh/core/mesh_ids.hpp:16-36; src/mesh/core/polyhedron.hpp:66-115,117-140,166-210; src/crimson/picking/picking.hpp:25-113; src/crimson/picking/picking.cpp:77-84; src/ui/viewport/viewport_types.hpp:142-165; src/tools/editor_input_event.hpp:52-98; src/tools/tool_id.hpp:14-26; src/ui/viewport/viewport_controller.cpp:86-126,401-439,441-492; src/ui/viewport/crimson_viewport_render_host.cpp:73-89,240-290,479-500; tests/unit/document/document_tests.cpp:103-123,177-223; tests/unit/commands/command_tests.cpp:208-255

- `document::Selection` already stores either selected `ObjectId`s or selected `ComponentRef`s and uses typed generational mesh IDs for vertices, edges, and faces. This is a good base for domain-safe selection.
- The current selection state is too small for parity: it has no selection mode, active object/edit target, active component, mode-specific component buckets, sticky source-wire owner, selection edit operation, hover candidate, or select-all/invert/toggle helpers.
- Object and component selections are currently mutually exclusive. Reference parity requires object-level selected mesh state and per-object component selections to coexist in component modes: a mesh can be selected because it is the active edit target or because it owns selected components.
- `Document::set_selection()` validates live object and component IDs and marks the selection dirty, but topology-changing mesh operations do not yet return selection remap/prune results because there are no such operations in scope today.
- Commands only cover wholesale set and clear selection. Select all, invert, add, remove, toggle/multi-select, component mode changes, sticky target changes, and flip normals need command-backed mutations with apply/undo/redo snapshots.
- `DeleteObjectCommand` captures selection and restores it on undo. Any new flip normals command should follow the same pattern: snapshot prior mesh content and selection where needed, mutate through `Document`, and restore exactly on undo.
- `Polyhedron` exposes stable generational IDs and face traversal but no public operation to reverse a face winding or flip selected/all normals. This task needs a narrow mesh operation rather than renderer-side normal inversion.
- Crimson picking and viewport pick payloads currently expose object/render IDs plus raw `element_index`. Core/UI selection must not persist raw renderer element indexes. The hit contract should resolve to `document::ObjectId` plus typed `ComponentRef` before command creation.
- `ViewportController::surface_hit_for_ray()` currently computes a face hit and returns a packed object id and `FaceId.index()`. This can seed tool placement, but it is not durable enough for component selection because generation and component kind/id are lost.
- The local ray/triangle intersection is two-sided because it rejects only near-zero determinant, not negative determinant. That matches the inverted-normal pick direction but still needs explicit tests and a typed hit result.
- `ToolId::Select` and the UI action exist, but no concrete Select tool or component mode input contract exists in `src/tools`.
- There is no document-driven selection overlay emission path. The current Crimson host builds mesh uploads and tool preview overlays, but selection/component overlay semantics are absent.

## Reference-App Findings

Source refs: C:\Users\Drako\Desktop\quader-windows\quader-app | src/mesh/polygon/document_selection.hpp:19-121; src/mesh/polygon/quader_poly_document_selection_api.cpp:19-105,107-173,175-205,222-263

- Reference selection has explicit component `SelectionMode` values and stores component vectors by mode (`vertices`, `edges`, `faces`) plus active component fields.
- `select_only`, `add_selection`, and `remove_selection` are mode-aware. Adding a component of a different kind switches mode and clears incompatible component selection.
- Mode conversion exists in the polygon domain, and selected vertices can be derived from selected vertices, edges, or faces.
- Edge-loop selection is a domain operation, not a UI-only behavior.

Source refs: C:\Users\Drako\Desktop\quader-windows\quader-app | src/editor/modeling/runtime/session.cpp:846-960; src/editor/modeling/runtime/session_operations.cpp:311-340,1015-1019; src/editor/interactions/runtime_command.cpp:119-184,1292-1301; src/api/quader_command_ids.hpp:33-35

- `select_all` and `invert_selection` are runtime/session operations and are mode-aware.
- Object mode selects or inverts whole objects and clears component selections.
- Component modes target selected/active component edit objects and select or invert all vertices, edges, or faces on those objects.
- Runtime selection input maps modifiers to replace/add/remove. Paint strokes in component mode force add behavior, and multi-click expands by mode: all faces, edge loop, or all vertices.
- The reference command IDs for selection are `edit.clear_selection`, `edit.select_all`, and `edit.invert_selection`.

Source refs: C:\Users\Drako\Desktop\quader-windows\quader-app | src/editor/modeling/runtime/session_picking.cpp:520-523,848-856,1651-1710,1713-1784,1793-1887,1889-1973,2023-2148; src/editor/modeling/runtime/session.hpp:264-285

- Selection picking explicitly enables backface hits and disables front-face preference so inverted meshes remain pickable by nearest visible surface.
- Component picking uses mode-specific options and can pick the active source wire for edge/vertex selection.
- Replace misses may clear selection; add/remove preserve current selections according to operation.
- Reference has hover state with object id, component kind/id, selected-modifier state, and `candidate_object_id`. Core parity needs a similar semantic hover/candidate contract even if final UI owns event routing.

Source refs: C:\Users\Drako\Desktop\quader-windows\quader-app | src/editor/modeling/runtime/session_selection_overlay.cpp:401-438,662-700,718-785,887-940; tests/modeling_domain/qdr_modeling_runtime_overlay_tests.cpp:607-695; docs/overlay-handling.md:20-52,74-93; memory.md:1232-1265

- Source-wire ownership is a modeling-runtime decision. It is not just a renderer style rule.
- In object mode, selected objects draw selected mesh topology, not component-mode `SourceWire`.
- In component modes, the active edit mesh emits `SourceWire` and `SourceWireDepthStamp` metadata. Component highlights are added over source wire and must not remove source-wire topology.
- Sticky source-wire ownership is mode-specific: selected faces stick only in face mode, selected edges only in edge mode, and selected vertices only in vertex mode.
- When no selected components exist for the current mode, source wire falls back to hover candidate, then active selected object.
- `SourceWireDepthStamp` is metadata for visibility/picking support, not a rendered fill layer.
- Inverted mesh overlays require source wire to remain visible, selected/hover face fills to be two-sided, and picking to stay separate from visualization.

Source refs: C:\Users\Drako\Desktop\quader-windows\quader-app | src/render/viewport_overlay_frame.hpp:20-49,61-110; src/render/viewport_overlay_policy.hpp:45-152,153-192; src/render/viewport_overlay_policy.cpp:23-45,47-59,73-129,169-235,238-260; src/editor/projection/selection_overlay_projection.cpp:45-59,64-115,257-292,390-462; src/render/overlay/viewport_overlay_style_resolver.cpp:35-88,90-166,187-277,279-312

- Reference overlay pipeline is split: modeling builds semantic overlay primitives, projection maps them to viewport layers/style roles, and render policy/style resolver owns draw order, colors, widths, point sizes, depth behavior, and two-sided triangle state.
- Render policy has distinct layers for selected visible face, selected face mask, selected face edge, selected edge, selected/hover vertices, source wire, and source-wire depth stamp.
- Source-wire lines normally draw on top, while component selection/hover sources can use component depth behavior. Face-fill overlay triangles are two-sided for selected/hover face layers.
- Exact colors/thickness must be copied by behavior and constants through the renderer plan; core should emit semantic layers and refs, not hardcode render styling.

Source refs: C:\Users\Drako\Desktop\quader-windows\quader-app | src/mesh/polygon/document_operations.hpp:220-223; src/mesh/polygon/operations/flip_face_normals_operation.cpp:38-80; src/editor/modeling/runtime/session_face_operations.cpp:43-65; src/editor/modeling/runtime/session_operations.cpp:1015-1019; src/editor/modeling/operations/selection_operations.cpp:209-213; src/editor/modeling/services/modeling_instant_operation_catalog.cpp:176-179; src/editor/metadata/host_contract.cpp:173-176,788-789; src/api/quader_command_ids.hpp:131-132,149; tests/modeling_domain/operation/qdr_document_operation_flip_normals_tests.cpp:11-20; tests/modeling_domain/operation/qdr_modeling_runtime_operation_invert_mesh_tests.cpp:9-18

- Complementary operation is exposed as action `"invert_mesh"` / label `"Flip Mesh Normals"` and operation `"flip_face_normals"`.
- Object mode flips all faces on selected objects by building an all-face selection. Face mode flips only selected faces.
- The core polygon operation reverses selected face winding and returns no-change errors when selection is empty or selected faces cannot be found.
- Tests assert selected face winding reversal, triangulation still works, and selected cube/object flipping inverts all faces.

## Duplicate / Overlap Risks From Current Board

Current board refs: C:\Users\Drako\Desktop\_quader-qt-base | project_board.md:6-15,20-34

- No active board item appears to duplicate mesh/component selection and parity overlays.
- Active #13 transform gizmos overlaps viewport input routing, selection pivot/multi-selection semantics, picking IDs, and overlays. Sequence or split so selection contracts land before gizmo code depends on selected objects/components.
- Active #21 fly navigation overlaps viewport mouse/key routing. Selection input must not break right-button fly capture, W/A/S/D consumption during fly mode, or navigation-active event suppression.
- Active #18 project/session work is not a duplicate, but if it lands first, selection ownership must be revalidated per active scene/document and per-scene command history.
- Archived Box tool work established camera ray and surface-hit groundwork. Build on its ray-projection lesson, but do not treat its packed face index as sufficient for component selection.

## Recommended Board Brief Bullets

Recommended title: Add mesh and component selection modes with parity overlays

Task/bug: enhancement

Priority: high

Area: modeling/core with UI and renderer dependencies

Summary:

Add Quader mesh object selection, vertex/edge/face component selection modes, parity selection commands, sticky source-wire/component overlay semantics, and flip mesh normals operation to match `C:\Users\Drako\Desktop\quader-windows\quader-app` 1:1 for selection behavior and inverted-normal overlay validation.

Brief bullets:

- Extend document/editor selection contracts with object mode plus vertex, edge, and face component modes.
- Preserve typed `ObjectId`, `VertexId`, `EdgeId`, and `FaceId` references in durable state; do not persist renderer raw element indexes.
- Track active object/edit target and mode-specific component selections strongly enough to reproduce sticky source-wire ownership.
- Add command-backed selection verbs: replace/select only, add, remove/deselect, clear, select all, invert, toggle/multi-select, and set component mode.
- Ensure every selection mutation records undo/redo snapshots and validates object/component liveness before applying.
- Add a narrow mesh operation/command to flip selected face normals by reversing face winding; object mode flips all faces on selected meshes, face mode flips selected faces.
- Keep selection and flip normals in document/mesh/commands. Tools and UI request commands; Crimson renders semantic overlay data and does not own selection truth.
- Add typed picking conversion from renderer/viewport hit to `ObjectId` plus `ComponentRef`, including generation validation and stale-hit rejection.
- Emit semantic overlay data for object selection, component source wire, selected/hover vertices/edges/faces, and source-wire depth stamp metadata. Renderer owns exact colors, widths, depth state, two-sided fills, and draw order.
- Match reference sticky selection policy exactly: current-mode selected components beat hover; hover beats active object fallback; object selection alone does not make source wire sticky in a component mode when selected components for that mode are absent.

Likely files/modules:

- `src/document/selection.*`
- `src/document/document.*`
- `src/document/scene_object.*`
- `src/commands/selection_commands.*`
- `src/commands/document_commands.*` or new mesh operation command module
- `src/mesh/core/polyhedron.*`
- `src/mesh/core/mesh_traversal.*`
- new `src/mesh/ops/*flip*` or equivalent local mesh operation
- `src/tools/editor_input_event.hpp`
- `src/tools/tool_context.*`
- new concrete selection tool under `src/tools/`
- `src/ui/viewport/viewport_controller.*` for typed hit conversion contract
- `src/ui/viewport/crimson_viewport_render_host.*` for object/component mapping dependency
- `src/crimson/picking/*` and overlay modules as renderer-owned dependencies
- `tests/unit/document/*`
- `tests/unit/commands/*`
- `tests/unit/mesh/*`
- `tests/unit/tools/*`
- `tests/unit/crimson/*` or renderer-specific tests for overlay/picking parity

Final owner:

Multi-domain. Software architect should finalize the board entry and split. Core owns document, command, mesh, selection, typed hit, and flip-operation contracts. UI owns actions, mode controls, shortcuts, viewport input, and outliner/property model interaction. Renderer owns Crimson overlay extraction/rendering, picking payload generation, colors, widths, depth behavior, two-sided face fills, and visual parity checks.

Active plan path:

None for this new task yet. Current overlapping active tasks cite `agents/plans/audit_20260704_full_architecture_master.md`, but this intake does not establish an implementation plan.

Tests-policy applicability:

Applies. This is executable-affecting C++ core/modeling/UI/renderer work with command, document, mesh, picking, and overlay behavior.

## Acceptance Criteria

- Object mode supports select, deselect, clear/deselect all, select all, invert selection, and multi-select/toggle across different mesh objects.
- Component modes support vertex, edge, and face selection with select, deselect, clear, select all, invert selection, and multi-select/toggle across components on different mesh objects.
- Selection mode, active object/edit target, and active component behavior match the reference under replace/add/remove and multi-click expansion.
- Sticky source-wire policy is 100% replicated: current-mode selected components stick; other-mode components do not stick; hover candidate wins when there is no current-mode sticky object; active object fallback appears only when no hover candidate and no current-mode selected components exist.
- Object mode selected meshes do not emit component-mode `SourceWire`/`SourceWireDepthStamp`; component modes do not emit whole-object xray selected fill as a substitute for source wire.
- Semantic overlay output distinguishes source wire, source-wire depth stamp metadata, selected visible faces, selected face mask, selected face edges, selected edges, unselected/selected/hover vertices, and hover/remove-selection states.
- Renderer-visible colors, line widths, point sizes, depth behavior, draw order, and two-sided selected/hover face fills match the reference for normal and inverted-normal meshes.
- Picking hits used for commands resolve to typed document references and reject stale/mismatched generation IDs.
- Inverted-normal meshes remain pickable by nearest visible two-sided surface behavior, without making render extraction filter source-wire topology.
- Flip Mesh Normals exists with reference semantics: selected objects in object mode flip all faces; selected faces in face mode flip only those faces; empty/no-live-face selections reject with no mutation.
- Flip operation reverses face winding, keeps mesh validation/triangulation valid, marks document mesh dirty, updates render revisions, and preserves/remaps selection according to documented command semantics.
- All new or changed selection and flip commands support apply, undo, and redo with exact state restoration.
- Existing documentation for changed documented symbols or documented behavior/invariants is updated in place in the same implementation edit.

## Tests / Checks

Core/document tests:

- Selection mode defaults, mode changes, clear semantics, active object/edit target, active component, sorting/deduping, and liveness validation.
- Object select/add/remove/toggle/clear/select-all/invert across at least two mesh objects.
- Vertex, edge, and face component select/add/remove/toggle/clear/select-all/invert across at least two mesh objects.
- Mode-specific sticky source-wire owner selection matrix using semantic overlay output, including hover candidate and active object fallback.
- Selection pruning/remap on object deletion and topology mutation; stale component refs rejected.

Command tests:

- Every new selection command executes, rejects no-op/invalid inputs correctly, and undo/redo restores captured document state.
- Flip normals command executes, rejects invalid/empty selection, and undo/redo restores exact face winding, selection, dirty flags/events as appropriate.

Mesh tests:

- Face-winding reversal on one face, multiple selected faces, and all faces in an object.
- Mesh validation and triangulation remain valid after flip.
- Normal direction/sign changes are observable from face winding or render-upload normal generation.
- Degenerate faces or missing IDs produce explicit errors without partial mutation.

Picking tests:

- Renderer/viewport payload maps to typed `ObjectId` plus `VertexId`/`EdgeId`/`FaceId` and rejects stale generations.
- Two-sided face hit works after flipping normals and chooses nearest visible surface.
- Component hit conversion never stores raw renderer `element_index` in document selection state.

Overlay semantic tests:

- Object-mode selected mesh overlay does not emit component-mode source wire.
- Component-mode active edit mesh emits source wire and source-wire depth stamp metadata.
- Selected component overlays are additive over source wire.
- Sticky policy parity for face, edge, and vertex modes, including selected components in a different mode.
- Inverted-normal selected and hover face semantic layers are still emitted for renderer two-sided fill.

UI/renderer-owned checks needed before final acceptance:

- UI action/mode tests for `edit.select_all`, `edit.invert_selection`, clear/delete behavior, component mode switches, selection tool activation, and modifier routing.
- Renderer extraction/render tests for exact layer mapping, colors, widths, point sizes, depth behavior, draw order, two-sided selected/hover fills, and source-wire visibility on inverted meshes.
- Manual parity run against the reference app for object selection, component modes, sticky policy, source-wire behavior, and flip mesh overlay validation.

Verification/build checkpoint notes:

- Normal implementation verification should include `cmake --build --preset qt-mingw-debug` and `cmake --build --preset qt-mingw-debug-tests`.
- Prefer targeted test execution for new/changed unit suites first. Before broad `ctest --preset qt-mingw-debug`, the implementer must inspect whether that preset includes clang-format, clang-tidy, or style/static-analysis gates; if it does, immediate explicit user permission is required before running it.
- `check_format`, `check_clang_tidy`, `check_static_analysis`, and any preset/target that includes them are gated checks requiring immediate explicit user permission unless the prompt explicitly says `run now without asking`.
- Because this changes executable behavior across document/commands/tools/mesh/picking/overlays, final review must include an archived dev build path or a valid `Dev build checkpoint deferred:` blocker.

## Architecture Constraints

- Selection is document/editor state, not mesh topology state. Mesh core should expose IDs, traversal, and narrow operations; document/commands own selection mutation.
- Commands are the only path for user-visible selection and flip mutations. UI, tools, renderer, and models must not directly mutate document selection.
- Store IDs, not raw pointers, raw vector offsets, or renderer-only indexes, in selection, commands, tools, or UI state.
- Use typed domain references: `ObjectId` plus `VertexId`, `EdgeId`, or `FaceId`. Packed render IDs may be transient transport only and must be resolved before command creation.
- Mesh flip normals must be a real winding/topology/geometry operation. Do not fake inverted normals in Crimson materials or overlay code to satisfy visual checks.
- Keep core/document/mesh/commands free of Qt Widgets and Crimson GPU/runtime dependencies.
- Semantic overlay extraction may live in modeling/editor-facing core, but render styling remains renderer-owned.
- Document dirty flags and change events must distinguish selection-only changes from mesh geometry/topology changes so render caches avoid unnecessary topology rebuilds.
- Expected failures return structured `Result`/diagnostic values rather than exceptions.
- If implementation changes any documented class, function, enum, member, or behavior/invariant, update the existing documentation in place in the same edit per `agents/documentation-policy.md`.

## Dependencies On UI / Renderer

UI dependencies:

- Component mode controls for object/vertex/edge/face, selection tool activation, action state, shortcuts, and menu/toolbar wiring.
- Pointer modifier mapping for replace/add/remove, multi-select/toggle, paint stroke behavior, multi-click expansion, preserve-selection-on-selected-hit, and clear-on-miss parity.
- Outliner/property/action behavior for multi-object selection and component selection summaries.
- Coordination with fly navigation so navigation-active pointer/key paths do not trigger selection commands.
- Coordination with transform gizmo task so gizmo pivot and transform target semantics consume the same selection contract.

Renderer dependencies:

- Build or extend render object/component mapping so rendered/picked triangles, edges, and vertices can resolve to document IDs.
- Emit and render selection overlays from semantic overlay frames, not ad hoc UI flags.
- Match reference overlay layer order, colors, thicknesses, point sizes, depth bias, draw-on-top/depth-tested behavior, source-wire depth stamp handling, and two-sided face-fill state.
- Validate overlays on normal and flipped/inverted-normal meshes.
- Keep source-wire render visibility separate from picking/hover occlusion policy.

## Risks

- Treating renderer `element_index` or `FaceId.index()` as a durable component reference will break undo, deletion, compaction, and stale-hit validation.
- Modeling sticky source-wire policy only in Crimson would miss the mode-specific owner rules and make tests brittle.
- A single global component selection vector cannot express reference parity across multiple selected meshes and per-object component selections without extra active edit target state.
- Flip normals can accidentally become a visual-only normal inversion. Acceptance requires actual face winding reversal.
- Broad UI action names such as "Invert Mesh" can be confused with selection invert. The task should keep `edit.invert_selection` separate from flip mesh normals.
- Full visual parity depends on renderer-owned constants/settings and should not be approved by core tests alone.
