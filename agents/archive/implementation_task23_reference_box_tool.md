# Implementation Plan: Task #23 Reference-Style Box Tool

Status: active implementation plan for project-board task #23
Created: 2026-07-05
Board task: #23 "Implement reference-style interactive box creation tool and remove prototype cube"
Parent plan: agents/plans/audit_20260704_full_architecture_master.md
Source refs: C:\Users\Drako\Desktop\quader-windows\quader-app | Tool metadata/shortcut: src/editor/tools/base/viewport_tool_descriptor_catalog.cpp:83-90; src/editor/metadata/host_contract.cpp:782-783. Box state/math: src/editor/interactions/box_tool.hpp:20-54,79-124; src/editor/interactions/box_tool.cpp:130-149,161-188,190-228,248-279,281-323,325-398,400-446,493-608. Viewport tool event flow: src/editor/tools/base/two_stage_box_tool.cpp:41-158; src/editor/tools/interactive/box_tool.hpp:16-18; src/editor/tools/viewport_tool.hpp:94-104,172; src/editor/tools/viewport_tool_ports.hpp:29,52-56,99; src/editor/tools/viewport_tool_ports.cpp:53-107,208-209. Runtime/session commands: src/editor/interactions/runtime_command.cpp:232-354; src/editor/modeling/services/modeling_session_service.cpp:2101-2180,2475-2512; src/editor/modeling/services/runtime_tool_command_service.cpp:134-149,342-390. Preview/overlay/render refs: src/editor/projection/tool_preview_mesh_projection.cpp:19-52; src/render/overlay/viewport_box_overlay_builder.hpp:16-22; src/render/overlay/viewport_box_overlay_builder.cpp:28-41,90-119; src/render/tool_preview_renderable_cache.cpp:36-57,73-90. Commit/material refs: src/editor/commands/modeling/commit_box_preview_command.cpp:37-168; src/assets/material_document.hpp:39-41; src/assets/material_document.cpp:133-139; src/editor/session/project_asset_bootstrap_service.cpp:21-43. Test refs: tests/viewport_tool_catalog_tests.cpp:67-75,245-367; tests/viewport_interaction_controller_tests.cpp:1210-1262; tests/modeling_domain/tool/qdr_modeling_runtime_tool_box_tests.cpp:15-71,157-188,200-240; tests/modeling_domain/operation/qdr_modeling_runtime_operation_create_box_tests.cpp:9-69.

## Current Workspace Facts

- App, UI, tools, document, command, mesh, and Crimson module targets now exist, so #23 can build on real foundations instead of the frozen audit's prototype-only baseline.
- `AppServices` owns `Document`, `CommandHistory`, and `ToolManager`; current registered tools are shell-only `Select`, `Move`, `Rotate`, and `Scale`.
- `ActionRegistry` and `ActionStateUpdater` have tool actions but no `BoxTool` action; `CreateCube` is still an inert creation action and should not become the Box tool contract.
- `ViewportWidget` and `ViewportController` currently route mouse input to splitters and camera navigation only. They do not dispatch neutral pointer events to `ToolManager`.
- `Document` can create undoable mesh objects through `CreateMeshObjectCommand`, but `MeshObject` has no material assignment contract.
- `Polyhedron` has low-level vertex/face creation and validation, but there is no `src/mesh/ops` box builder.
- Crimson has `FrameSnapshot`, render graph, material/base-shader, overlay, and picking foundations. The frame builder still injects `make_prototype_cube()`, and GPU runtime still keeps a prototype cube fallback mesh.
- Existing tests still assert that the prototype cube exists and that `CreateCube` is an action. These tests must be revised to protect the new behavior.

## Decision

Task #23 is fresh if started from this plan. The brief remains valid, but implementation must not begin from the master audit alone because the current workspace has advanced past the audit batches. This plan is the task-specific authority for sequencing and boundaries.

## Reference Clarifications From Implementation

- The current reference Box tool is not a visible two-step footprint-then-height tool. The active parity target is hover footprint, press/drag footprint, then release or second click commits immediately with the default snapped one-grid height.
- Reference footprint order is `{start, start + v, start + u + v, start + u}`.
- Signed drag corners must not be trusted for mesh winding. The box mesh builder must orient every quad outward against the box center before creating faces so drag direction cannot flip committed normals or lighting.
- Surface hits seed hover/press construction planes only. Once footprint drag starts, the tool must ignore later live surface hits and project the viewport ray onto the locked construction plane, so vertical-plane construction is not clipped to the visible source mesh.

## Non-Goals

- Do not implement transform gizmos from #13.
- Do not add selection behavior beyond what is required to avoid corrupting current selection state.
- Do not solve project/session asset catalog work from #18.
- Do not fold the Box tool into `MainWindow`, `ViewportWidget`, `FrameBuilder::build_prototype_snapshot`, or prototype cube fallback paths.
- Do not weaken style, architecture, or validation rules to keep prototype-cube tests passing.

## Third-Party Library Decision

No new third-party dependency is authorized for this task. Use the existing OpenMesh-backed `Polyhedron` boundary for mesh topology and add Quader-owned ray/plane/grid helpers under `src/geometry` or `src/tools` as appropriate.

Rationale: OpenMesh is already pinned and integrated behind `src/mesh/core/detail`, is mature for polygon mesh storage, and keeps the integration boundary inside the existing mesh core. A new geometry or CSG library would add license, build, and data-conversion risk without a narrow need for simple box construction. Fallback is direct `Polyhedron::create_vertex()` and `create_face()` based construction with validation.

## Implementation Slices

1. Mesh and geometry foundation

- Add a narrow mesh-op API for box construction, for example `src/mesh/ops/box_builder.hpp/.cpp`.
- Support bounds and oriented-corner creation as needed by grid, horizontal surface, and vertical surface placement.
- Reject non-finite and degenerate dimensions before mutating a mesh.
- Produce a closed six-face `Polyhedron` with stable vertex/face topology and validation coverage.
- Add or extend geometry helpers for ray/plane intersection, projection, plane basis, snapping, and box bounds without depending on UI or Crimson.

2. Document and material contract

- Add a document-owned material assignment/value contract that does not depend on Crimson GPU/runtime types.
- Ensure new Box objects receive explicit default PBR values from the board brief: base color `(0.5, 0.5, 0.5)`, roughness `1.0`, metallic `0.0`.
- Extend create/restore/delete command undo data so material assignment round-trips with the mesh object.
- Keep renderer material instance mapping as an adapter from document material values to Crimson `OpaquePbr`; do not let document own Crimson handles.

3. Tool state and modal Box behavior

- Add `ToolId::Box` and a real `BoxTool` under `src/tools`.
- Model the reference state contract: `Idle`, `Footprint`, `Height`, construction plane, raw/snapped start and end points, height distance, hover footprint points, and preview validity.
- Implement the reference math behavior: safe construction plane normalization, ray-to-plane construction point, directional snapping, hover cell, footprint update, height update, negative-height normal flip, preview corners, cancel, and reset.
- The tool must create previews during interaction and commit through `CommandHistory` only. It must not permanently mutate `Document` outside a command.
- Tool events must carry enough neutral data for viewport-space and world-space behavior: pane/view id, pointer phase, button state, world ray, camera snapshot, grid size/snap flag, navigation-active flag, and optional surface placement hit.

4. Viewport and QAction routing

- Add a checkable `BoxTool` action with `B` shortcut through `ActionId`, `ActionRegistry`, `EditorActionController`, `EditorActionStateProvider`, and `ActionStateUpdater`.
- Register the real Box tool in `AppServices`; keep shell tools only for tools that are still placeholders.
- Route hover, left press, move, release, cancel/Escape, and `B` activation through toolkit-neutral tool events. UI translates Qt events; tools own modeling state.
- Do not fire Box activation or tool pointer events while fly/navigation capture is active. This must coordinate with #21 if #21 is implemented in parallel.
- Keep splitters and camera navigation precedence explicit: splitter drag first, navigation capture second, tool routing third.

5. Surface placement and picking

- Add a document-side surface hit contract for tool placement: hit position, hit normal, object id, and component kind/id when available.
- Use grid plane fallback when there is no surface hit.
- Support horizontal and vertical surface planes by seeding the Box construction plane from hit position and normal.
- Crimson picking may provide request/results for viewport hit identity, but document/tool code must derive modeling truth from document geometry and stable ids, not from renderer-owned state.
- Add ray/triangle or ray/face query coverage at the smallest practical level.

6. Preview and render snapshot integration

- Extend `ToolPreview` or replace it with a 3D preview contract that can express footprint lines and mesh preview volume.
- Extend Crimson snapshot/overlay payloads for Box footprint hover, active footprint, and mesh preview as `OverlayUnlit` data.
- Render pre-commit Box preview as overlay/preview data only. It must not enter opaque/lit scene queues, material lighting, fog, bloom, shadow, or document truth before commit.
- Add document-to-render projection for committed boxes so real document objects render through `FrameSnapshot` and Crimson material mapping.
- Do not rely on the prototype cube fallback mesh for committed boxes. If dynamic document mesh upload is not sufficient, stop and return a deviation report instead of rendering every document mesh as the prototype cube.

7. Remove prototype cube paths

- Remove the default rotating cube object from `FrameBuilder::build_prototype_snapshot`.
- Remove or quarantine prototype cube GPU fallback usage once committed document mesh rendering is in place.
- Update shader manifest/tests only as needed after no runtime path depends on `PrototypeLitCube`.
- Empty documents should render the grid/overlays only, with no default object.
- Revise UI and Crimson tests that currently expect the prototype cube or `CreateCube` action. New tests must assert no default cube in an empty document snapshot.

## Tests

Required automated coverage:

- Mesh box topology: bounds/corners creation, reversed bounds normalization, degenerate rejection, finite coordinates, six valid faces, closed edges, validation passes.
- Document command: create Box object, material default values, undo restores original document state, redo restores object and material.
- Tool math/state: construction plane normalization, ray-plane rejection, directional snap, hover footprint, footprint drag/update, height preview, negative height, cancel/reset, commit preconditions.
- Surface placement: grid fallback, horizontal face placement, vertical face placement using hit position and normal.
- UI action/routing: `B` activates Box, action check state follows active tool, viewport events route to tools only when splitters/navigation do not consume them, Box does not activate during fly capture.
- Renderer/snapshot: Box footprint and preview are overlay-only, committed Box creates render object/material data, empty document snapshot has no prototype cube.
- Existing prototype-cube and `CreateCube` tests updated to the new behavior.

Use `agents/tests-policy.md` as the acceptance bar: tests must assert user-visible workflow or data invariants, not internal call order.

## Verification Checkpoints

After each coherent C++/CMake/runtime slice:

```powershell
cmake --build --preset qt-mingw-debug
```

Before review:

```powershell
ctest --preset qt-mingw-debug
cmake --build --preset qt-mingw-debug --target check_architecture
cmake --build --preset qt-mingw-debug --target check_format
cmake --build --preset qt-mingw-debug --target check_clang_tidy
cmake --build --preset qt-mingw-debug --target deploy
python tools/dev_builds.py archive --preset qt-mingw-debug
python tools/project_board.py validate
```

Manual runtime check before review:

- Launch `quader.exe`.
- Confirm empty startup has grid/viewport but no rotating prototype cube.
- Press `B` and confirm Box is active.
- Hover grid and existing surfaces and confirm footprint preview appears.
- Create boxes on grid, horizontal surface, and vertical surface.
- Confirm committed objects use the neutral gray default PBR material and persist through undo/redo.

## Coordination Notes

- #13 transform gizmos overlaps input, snapping, overlay, picking, and ray math. Shared contracts added here must be generic enough for #13, but #13 behavior stays out of this task.
- #21 fly mouse capture overlaps `ViewportWidget`, `ViewportController`, and key/mouse routing. Do not run #21 and #23 workers against those files concurrently without root coordination.
- #20 diagnostics should not block #23, but renderer/frame stats changes should avoid unnecessary diagnostics model resets.
- #18 project/session work can conflict with material/project asset decisions. If #18 starts before #23 completes, root must coordinate document material ownership before either task edits material contracts.
