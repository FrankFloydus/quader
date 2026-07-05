# Task Intake Domain Findings: Mesh and Component Selection Parity

## Domain Classification

- Request type: new enhancement task, not a bug.
- Recommended title: Add reference-parity mesh/component selection, sticky overlays, and Flip Mesh Normals.
- Recommended priority: high, with critical parity requirements called out in the brief.
- Recommended area: tools/commands, with document, mesh, app-contract, Crimson, and UI dependencies.
- Scout scope: core/document/commands/tools/mesh/app contracts only.
- Final owner recommendation: multi-domain. Software/core should lead the document, command, tool, mesh, and selection-policy contracts; renderer owns Crimson overlay and picking parity; UI owns modes, actions, shortcuts, and viewport input routing.
- Active plan path: no new implementation plan was created by this scout. Related active architecture baseline is `agents/plans/audit_20260704_full_architecture_master.md`, already referenced by overlapping viewport/tool tasks.

## Duplicate And Overlap Observations

- Not a duplicate of active #13 transform gizmos, but it overlaps viewport picking, input routing, overlay primitives, and selection/pivot assumptions: `project_board.md:8-10`.
- Must coordinate with active #21 fly mouse capture so selection shortcuts/clicks do not fire while navigation capture is active: `project_board.md:29-31`.
- Builds directly on archived #7 document/selection/command shell; #7 explicitly left viewport picking, topology tools, and macro/multi-delete out of scope: `project_board_archive.md:281-288`.
- Builds on archived #9 Crimson overlay/picking foundation; #9 residual says no UI selection or document mutation was added: `project_board_archive.md:239-257`.
- Related archived #23 Box tool established viewport tool, overlay, and surface-hit routing patterns, but explicitly excluded selection behavior: `project_board_archive.md:32-34`.

Source refs: `C:\Users\Drako\Desktop\quader-windows\quader-app` | `docs/overlay-handling.md:18-54,56-84,86-130,186-199,235-291,360-403`; `src/editor/modeling/runtime/session.hpp:34-48,679-695,1221-1225,1321-1342,1479-1525,1846-1851,1881-1899,1989-1991`; `src/mesh/polygon/document_selection.hpp:19-53,102-119`; `src/editor/modeling/runtime/selection_policy.cpp:145-187,189-230,232-388,1181-1350,1595-1705,1788-2023,2025-2113,2418-2485`; `src/editor/modeling/runtime/session_picking.cpp:1535-1649,1713-1935,2086-2140`; `src/editor/modeling/runtime/session_selection_overlay.cpp:401-444,448-575,854-910`; `src/editor/modeling/runtime/session_operations.cpp:83-144,260-346,992-1019`; `src/editor/modeling/runtime/session_face_operations.cpp:43-66`; `src/editor/interactions/runtime_command.cpp:1190-1210,1288-1301`; `src/editor/commands/modeling/select_all_tool.cpp:15-21`; `src/editor/commands/modeling/invert_selection_tool.cpp:14-19`; `src/mesh/polygon/document_operations.hpp:220`; `src/mesh/polygon/poly_operation_descriptor.hpp:46,118-119`; `tests/modeling_domain/operation/qdr_modeling_runtime_operation_select_all_tests.cpp:9-61`; `tests/modeling_domain/operation/qdr_modeling_runtime_operation_invert_selection_tests.cpp:9-63`; `tests/modeling_domain/operation/qdr_modeling_runtime_operation_invert_mesh_tests.cpp:9-16`; `tests/modeling_domain/qdr_modeling_runtime_overlay_tests.cpp:332-389,444-504,615-727,795-833`; `tests/core_tests.cpp:1312-1416,1681-1905,2038-2265,5830-5833,9351-9356,9938-9942`.

Source refs: `C:\Users\Drako\Desktop\_quader-qt-base` | `src/document/selection.hpp:24-96`; `src/document/selection.cpp:60-75,86-124,126-153`; `src/document/document.hpp:20-28,42-70,109-146`; `src/document/document.cpp:41-69,100-125,187-247,249-279,312-313`; `src/commands/selection_commands.hpp:21-82`; `src/commands/selection_commands.cpp:30-85`; `src/commands/document_commands.hpp:31-120`; `src/commands/document_commands.cpp:94-124`; `src/mesh/core/polyhedron.hpp:94-115,123-162,170-203`; `src/mesh/core/polyhedron.cpp:212-425,475-512,667-713`; `src/mesh/core/mesh_traversal.hpp:29-52,61-83,90-111`; `src/tools/editor_input_event.hpp:53-99`; `src/ui/viewport/viewport_controller.cpp:441-497,520-537`; `src/ui/viewport/viewport_types.hpp:142-165`; `src/crimson/picking/picking.hpp:26-45,101-113`; `src/crimson/overlays/overlay_command.hpp:21-52,107-115`; `src/ui/viewport/tool_preview_overlay_adapter.cpp:35-95`; `src/ui/viewport/crimson_viewport_render_host.cpp:646-692`; `src/ui/actions/editor_action_state_provider.cpp:77`; `src/ui/actions/editor_action_controller.cpp:126-143`; `tests/unit/document/document_tests.cpp:177-222`; `tests/unit/commands/command_tests.cpp:208-257`; `tests/unit/ui/viewport_tests.cpp:342-363`.

## Proposed Scope

Implement reference-parity object and component selection for mesh objects:

- Object, vertex, edge, and face modes.
- Select, deselect, select all, invert, add, remove, and toggle multi-selection.
- Multi-mesh object selection and multi-mesh component selection.
- Sticky component edit/source-wire policy matching the reference.
- Typed hit-to-selection contracts from viewport/picking to tools and commands.
- Undoable selection commands and mode-switch behavior.
- Semantic overlay extraction for selected/hover/source-wire state.
- Complementary `invert_mesh` / `flip_face_normals` operation matching the reference action labeled "Flip Mesh Normals".

The requested flip task should be treated as Flip Mesh Normals from the reference app, not the separate horizontal/vertical mirror/view flip path, unless the software architect explicitly broadens the brief.

## Architecture Constraints

- Document remains the selection truth. Renderer and UI must not own selected state.
- User-visible mutations must go through undoable commands: selection replacement, clear, add/remove/toggle, select all, invert, mode switch, and flip normals.
- Current `Selection` supports object refs and component refs, but lacks explicit selection mode, active edit target, sticky owner, and select-all/invert policy. Those need first-class contracts.
- Current picking and surface-hit paths expose raw `uint64_t object_id` and `uint32_t component_index`. Selection commands should receive typed `ObjectId`, `VertexId`, `EdgeId`, and `FaceId` after a resolver boundary.
- Sticky source-wire ownership must be computed in core/tool policy before overlay extraction. Crimson should draw semantic overlay records, not infer selection policy.
- Overlay parity needs semantic source kinds/roles beyond today's generic `OverlayCommand`: object selection, component selection, hover, source wire, selected source wire, face fill, depth stamp, tool preview, and diagnostics.
- Flip normals is a mesh/domain operation, not a renderer workaround. It should preserve IDs/attributes where promised, validate topology after mutation, and mark document mesh topology/render state dirty.
- Documentation synchronization applies: if implementation changes a documented C++ symbol, behavior, or invariant, the existing documentation must be updated in place in the same edit.

## Likely Files And Modules

- `src/document/selection.*`
- `src/document/document.*`
- `src/commands/selection_commands.*`
- `src/commands/document_commands.*` or new mesh operation command files
- `src/mesh/core/polyhedron.*`
- `src/mesh/core/mesh_traversal.*`
- New or existing `src/mesh/ops/*`
- `src/tools/*`
- `src/tools/editor_input_event.hpp`
- `src/ui/viewport/viewport_controller.cpp`
- `src/ui/viewport/viewport_types.hpp`
- `src/ui/viewport/crimson_viewport_render_host.cpp`
- `src/crimson/picking/*`
- `src/crimson/overlays/*`
- `src/ui/actions/*`
- Related document, command, tool, mesh, viewport, and Crimson tests

## Recommended Sequencing

1. Add core selection-mode/state model, active target, sticky policy helpers, typed hit resolution, and pure selection-policy tests.
2. Add undoable selection commands and command-history coverage.
3. Add mesh normal-flip operation plus command, including object-mode all-face behavior and face-mode selected-face behavior.
4. Integrate select tool/app contracts for click, modifier multi-select, select all, invert, deselect, and component modes.
5. Add semantic selection overlay extraction from document state plus hover/sticky source-wire state.
6. Renderer/UI implement visual parity and manual parity validation against the reference app.

## Tests And Verification

`agents/tests-policy.md` applies.

Required core coverage:

- Document selection validation, mode switching, active target, sticky source-wire owner policy, object/component select/deselect/toggle/add/remove, select all/invert, invalid ID rejection, and empty/no-op behavior.
- Command apply/undo/redo for each new selection command, mode-switch command, and flip-normal command.
- Mesh tests for flipping selected face normals, flipping all faces for selected objects, topology validity, ID/attribute preservation expectations, and undo/redo.
- Tool tests for typed pick-to-selection behavior, modifier multi-select, already-selected hit preservation, double-click expansion if included for parity, and navigation-capture exclusion.
- UI action tests for component modes, Select All, Invert Selection, Deselect, Flip Mesh Normals, shortcut/action enablement, and command routing.
- Renderer-owned tests for overlay colors, thickness, layers, object-mode versus component-mode behavior, source-wire sticky policy, inverted-normal/source-wire/depth-stamp behavior, and face-fill parity.

Verification/build checkpoint notes:

- This is executable-affecting. Final implementation review should reject approval unless the report includes an archived dev version/path or a valid `Dev build checkpoint deferred:` blocker.
- Expected ordinary checks include configure/build, targeted CTest selections that do not invoke style/static-analysis, deploy build, manual viewport parity smoke, and dev-build archive.
- `clang-format`, `clang-tidy`, style/static-analysis targets, and any CTest preset proven to include them are gated checks requiring immediate explicit user permission before execution.

## Risks And Dependencies

- Highest risk is overlay parity, especially sticky source wire and inverted-normal behavior. Renderer architect must read `C:\Users\Drako\Desktop\quader-windows\quader-app\docs\overlay-handling.md`.
- Current overlay command shape is too generic for reference parity.
- Raw pick payload IDs are not acceptable as document mutation inputs.
- Multi-mesh component select-all/invert can be large; decide early whether Quader stores explicit component refs only or introduces/reference-matches full-object component compression behavior.
- OpenMesh-backed face winding and ID stability must be proven for flip-normal operations.
- Coordinate with #13 and #21 before viewport input/picking work lands.

## Hindsight Constraints

- `H-20260705-001` applies. Selection picking and tool rays must use the shared Crimson camera projection path; do not add separate UI-only ray math.
- `H-20260705-003` applies narrowly. Flip-normal and inverted-overlay work must keep mesh winding correctness separate from renderer visibility hacks.
