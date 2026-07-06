# Task #13 Implementation Plan: Godot-Style Transform Gizmos

## Board Revalidation

- Task: `#13 Add Godot-style transform gizmos to the viewport`.
- Freshness result: fresh for current workspace after revalidation.
- Stale-board resolution:
  - The brief's sequencing note about waiting for `#9` is outdated. `#9` Crimson overlay/picking foundations are archived and present in current code.
  - Current `#25` selection/overlay work is present in the workspace and supplies document-backed object/component selection, typed surface hits, and semantic Crimson overlays. This task consumes those contracts.
  - Move, Rotate, and Scale actions are currently wired to shell tools in `src/app/application.cpp`; this task replaces those shells with real tools.
- Hindsight applied:
  - `H-20260705-001`: tool rays, GPU picking, and Crimson projection must share the same camera convention. Use `ViewportController::ray_for_point()`, which already delegates to `crimson::render_camera_ray_from_viewport_point()`.
  - `H-20260705-005`: gizmo overlays must stay semantic/editor overlay data, not lit scene geometry.
- Reference refs:
  - Godot source root: `C:\Users\Drako\Desktop\_QUADER_DEV\resources\_SOURCE_CODE\godot-master`.
  - `editor/scene/3d/node_3d_editor_plugin.cpp:117-133`: handle constants.
  - `editor/scene/3d/node_3d_editor_plugin.cpp:1515-1765`: analytic hit selection for translate, rotate, and scale handles.
  - `editor/scene/3d/node_3d_editor_plugin.cpp:1811-1866`: transform computation shape.
  - `editor/scene/3d/node_3d_editor_plugin.cpp:5027-5150`: camera-based gizmo scale and visibility.
  - `editor/scene/3d/node_3d_editor_plugin.cpp:6170-6584`: transform update/apply flow.
  - `editor/scene/3d/node_3d_editor_plugin.h:336-407`: transform mode/edit state.
- External refs:
  - Qt `QMouseEvent`: mouse move requires a pressed button unless tracking is enabled, and Qt grabs the mouse after press until release. Existing viewport routing already follows this for drag tools.
  - Qt `QAction`: shortcuts are action-owned and default to window context; keep W/E/R action activation through existing action wiring and only let viewport navigation override active fly shortcuts.
  - Graphics Compendium camera rays: keep one world-space camera ray convention and do analytic tool hit/drag against that ray.

## Scope

Implement the first production slice of the gizmo task for document object transforms:

- Move, Rotate, and Scale tools become real `TransformGizmoTool` instances.
- Selected document objects are the transform targets. Component-mode selected object targets can provide a pivot, but mesh component deformation is out of scope until component transform commands exist.
- Gizmo visuals are tool preview overlays: axis lines, planar square handles, rotation rings, and the Quader-specific uniform-scale center cube.
- Picking is CPU analytic against the same geometry used for preview.
- Drag sessions provide live preview transforms and commit exactly one undoable batch transform command on release.
- Escape/cancel clears the preview without mutation.

## Files

- Add `src/tools/transform_gizmo_tool.hpp`.
- Add `src/tools/transform_gizmo_tool.cpp`.
- Update `src/commands/document_commands.hpp`.
- Update `src/commands/document_commands.cpp`.
- Update `src/tools/tool_preview.hpp`.
- Update `src/tools/tool_preview.cpp` only if helper behavior needs new payload awareness.
- Update `src/ui/viewport/tool_preview_overlay_adapter.hpp`.
- Update `src/ui/viewport/tool_preview_overlay_adapter.cpp`.
- Update `src/app/application.cpp`.
- Update `CMakeLists.txt`.
- Update tests:
  - `tests/unit/commands/command_tests.cpp`.
  - `tests/unit/tools/tool_manager_tests.cpp`.
  - `tests/unit/ui/viewport_tests.cpp`.

## API/Data Shape

- Add `BatchSetObjectTransformsCommand`:
  - input: vector of `{ ObjectId object, Transform transform }`.
  - validation: reject empty batches, duplicate objects, stale object ids, non-finite transforms, and no-op batches.
  - execute: capture previous transforms once, apply each new transform through `Document::set_transform()`.
  - undo: restore captured transforms.
  - name: `Transform Objects`.
- Extend `ToolPreview` with optional gizmo overlay payload:
  - axis/color-aware world segments.
  - optional center cube as line edges.
  - status text for drag deltas/angles/factors.
  - preview transform overrides for selected object wire previews are allowed in the tool payload but not sent to Crimson as lit objects.
- Add `TransformGizmoTool`:
  - constructor mode: `Move`, `Rotate`, or `Scale` from `ToolId`.
  - target resolution: selected objects from `Document::selection().selected_objects()`.
  - pivot: center of selected objects' transformed mesh bounds; fallback to selected object translations when bounds are unavailable.
  - scale: camera-scaled from ray origin to pivot using a conservative world-unit size; still finite in orthographic and degenerate camera cases.
  - handle ids: X/Y/Z axis handles, XY/XZ/YZ plane handles, rotation rings, and uniform-scale center cube.
  - hit tests: axis/plane/center/ring analytics against `ViewportRay`.
  - drag:
    - Move: ray/drag-plane delta constrained by axis or plane; grid snapping uses `PointerEvent::grid_size`.
    - Scale: axis, plane, or center uniform factor from drag projection; clamp factors to a small positive minimum.
    - Rotate: signed angle around selected axis using pivot-relative ray-plane intersections.
  - commit: batch command on release only.
  - cancel: restore empty preview and no mutation.

## Invariants

- Do not implement gizmo logic in `MainWindow`, `ViewportWidget`, or Crimson GPU code.
- Do not add graphics-runtime names to renderer folders, targets, or public class names.
- Gizmo overlays remain unlit `OverlayUnlit` editor geometry and stay out of lit/PBR render queues.
- Do not implement component mesh deformation in this slice.
- Do not use GPU picking for gizmo handles in this slice; CPU hit tests must match preview geometry.
- Do not run clang-format, clang-tidy, style/static-analysis targets, or CTest presets containing them without explicit permission.

## Ordered Edits

1. Add batch transform command and tests for multi-object apply/undo/redo/reject paths.
2. Add transform gizmo data/model helpers and unit tests for target pivot, hit selection, snapping, center uniform scale, drag preview, commit, and cancel.
3. Register real Move/Rotate/Scale gizmo tools in `AppServices` instead of shell tools.
4. Extend tool preview overlay adapter so gizmo payload emits colored semantic `ToolPreview` line overlays to all views or the source view.
5. Add viewport/adapter tests proving gizmo preview uses overlay commands and does not create lit objects.
6. Build and run focused tests; deploy and dev-build archive if the build is coherent.

## Verification

- `cmake --build --preset qt-mingw-debug --parallel 20`
- Targeted tests:
  - `ctest --preset qt-mingw-debug-runtime -R "command_tests|tool_manager_tests|ui_viewport_tests|crimson_overlay_tests" --output-on-failure`
- Deploy:
  - `cmake --build --preset qt-mingw-debug-deploy --parallel 20`
- Dev build:
  - `python tools/dev_builds.py current`
  - `python tools/dev_builds.py archive --preset qt-mingw-debug`
- Board:
  - `python tools/project_board.py validate`

If CMake generation/build is already broken by unrelated dirty workspace changes, record a `Dev build checkpoint deferred:` blocker and keep the task in review only after focused code/tests compile as far as possible.

## Acceptance Criteria

- W/E/R activate real Select/Move/Rotate/Scale action paths with gizmo tools behind Move/Rotate/Scale.
- A selected object displays a constant-screen-size gizmo preview in the viewport.
- Axis/plane/center/ring handles are analytically pickable from the current Crimson camera ray convention.
- Move, rotate, and scale drags show live preview overlays, commit on release through undoable commands, and cancel cleanly.
- Uniform scale uses the Quader-specific center cube.
- Multi-object selection transforms all selected object transforms around the shared pivot.
- Gizmo overlays are Crimson overlay payloads, not lit scene objects.

## Review Rework

User rejection: the first implementation narrowed the task to a Quader-specific object-transform slice and did not match the cited Windows reference gizmo. The source of truth for this rework is now `C:\Users\Drako\Desktop\quader-windows\quader-app`; Godot references are historical context only.

Reference mapping:

- `src/render/viewport_overlay_policy.hpp:92-107`, `142-147`: exact RGBA8 axis, plane, mixed, highlight, trackball, rotate-pie colors plus gizmo size/pick/handle/line-width pixel constants.
- `src/editor/modeling/runtime/tool_constants.hpp:13-17`, `43-45`: exact transform math constants: tool epsilon, screen epsilon, 48 px/grid move step, 96 px/scale factor, 0.01 minimum scale, tau, and axis fade thresholds.
- `src/editor/modeling/runtime/transform_gizmo.cpp`: exact frame geometry: 80 px constant-screen frame, `kGizmoCircleSize = 1.1`, plane size/distance `0.2/0.3`, arrow offset/tip `1.4/1.75`, scale offset/tip `1.4/1.554`, rotate view ring scale `1.14`, 64 rotate segments, plane centers at 0.4 screen-size units, and scale clamping `0.5..2.0`.
- `src/render/overlay/viewport_transform_gizmo_overlay_builder.cpp`: exact emitted visual topology: filled plane quads plus 1.2 px outlines, arrow cone triangles, scale square-prism triangles, hover-only scale center cube with visible-face shading, front-facing-only axis rotate rings, trackball ring, and active rotate pie fill/lines.
- `src/editor/viewport/transform_gizmo_interaction.cpp`: hover/pending/drag lifecycle, including pick-priority and commit/cancel semantics.
- `src/editor/interactions/move_tool.cpp`, `scale_tool.cpp`, `rotate_tool.cpp`, `tool_transform_math.cpp`, `tool_drag_math.cpp`, and `tool_snapping.cpp`: exact screen-axis drag, ray fallback, grid snap, scale factor, edge-on rotate, 15 degree rotate snap, and transform-commit behavior.

Rework edits:

1. Extend `PointerEvent` with toolkit-neutral camera/pane metadata so transform tools can build the same screen-sized frame as the reference while preserving the shared Crimson camera/ray convention.
2. Extend `ToolPreview` with styled world triangles and update the Crimson overlay adapter/render host to emit those triangles as semantic `ToolPreview` `SolidTriangles`.
3. Replace the placeholder gizmo preview with reference-colored planes, arrows, scale prisms, center cube, trackball/rings, and rotate pie indicator.
4. Replace ray-width picking with reference screen-space picking: center scale radius, axis tip/segment priority, convex plane polygons, and front-facing rotate ring segment distance.
5. Rework Move/Scale/Rotate drag math to use reference screen-axis and ray-plane behavior, including 48 px/grid move stepping, 96 px/scale factor, 0.01 minimum scale, and 15 degree rotate snapping.
6. Add/update tests that assert the reference RGBA colors, pixel thicknesses, triangle counts for plane/scale handles, front-facing rotate ring culling, camera metadata dispatch, and unchanged undo/cancel behavior.

Follow-up fixes from user review:

- Tool switching by keybind now refreshes transform gizmo hover/camera state during the next viewport render, so the gizmo does not disappear until the pointer moves.
- Camera zoom/orbit/fly render frames refresh the transform gizmo from the latest camera metadata, removing stale-camera size jumps caused by waiting for the next pointer hover.
- Transform gizmo drags no longer emit the old cyan transformed-bounds wire; rotate previews keep only the reference ring/trackball/pie indicator overlays.

## Review Rework: Live Mesh Preview And Selection Fallback

User rejection: Move/Rotate/Scale drags updated only the gizmo overlay and committed the mesh transform on mouse release; the selected mesh itself did not move in realtime. Empty viewport clicks also failed to deselect while Move/Rotate/Scale were active.

Reference behavior checked:

- `C:\Users\Drako\Desktop\quader-windows\quader-app\src\editor\viewport\transform_gizmo_interaction.cpp`: transform gizmo interaction consumes only real gizmo hits/drags. Missed clicks fall through as normal selection clicks.
- The same file calls `editor.modeling().update_transform_drag(...)` during active drags and updates the active preview before commit.
- `C:\Users\Drako\Desktop\quader-windows\quader-app\src\editor\history\scene_transform_preview_transaction.hpp`: scene transform previews apply live transforms to the scene, cancel restores the captured originals, and commit records one history command from the original transforms to the final transforms.

Rework edits:

1. Add `Document::set_preview_transform()` for transient live object transforms that emit transform-change events but do not mark the document dirty.
2. Add `ToolContext::preview_object_transforms()` plus a preview-mutation callback so viewport/render/UI state refreshes during drag without creating undo history.
3. Change Move/Rotate/Scale drag updates to apply live preview transforms to the document every mouse move, matching the reference transaction behavior.
4. On release, restore captured originals through the preview path, then execute one `BatchSetObjectTransformsCommand` to the final transforms so undo uses the true pre-drag state.
5. On Escape, navigation, cancel, or deactivation, restore captured originals and clear drag state without dirtying the document or pushing history.
6. Remove the intermediate render-only transform override path so lit meshes and selection/source-wire overlays share the same document transform source during preview.
7. Let Move/Rotate/Scale fall back to normal selection handling when their active tool does not consume the pointer event, matching reference pass-through behavior for missed gizmo clicks.
8. Correct the local rotate angle sign so the mesh, rotate pie, and committed transform follow the drag direction in Quader's camera/screen convention.

Regression coverage:

- `tool_manager_tests.TransformGizmoTool.MoveAxisDragCommitsBatchTransformAndUndo`: object transform changes before release, no undo entry or dirty flag appears during preview, release creates one undoable batch command, and undo restores original transform.
- `tool_manager_tests.TransformGizmoTool.RotateZRingCommitsSnappedAngle`: rotate drag now previews and commits the corrected signed angle.
- `tool_manager_tests.TransformGizmoTool.EscapeCancelsDragWithoutMutatingDocument`: live preview is rolled back on Escape with no history and no dirty flag.
- `tool_manager_tests.TransformGizmoTool.EmptyClickFallsBackToSelectionClearWhileMoveToolIsActive`: empty click deselects while Move is active.
- `ui_viewport_tests.Viewport.DocumentSelectionOverlayAdapterUsesPreviewObjectTransform`: selected object overlay lines are emitted from the live preview transform, preventing stale outlines/source wires during drag.

Verification:

- `cmake --build --preset qt-mingw-debug --parallel 20` passed.
- `cmake --build --preset qt-mingw-debug-tests --parallel 20` passed.
- `ctest --preset qt-mingw-debug-runtime -R "tool_manager_tests|ui_viewport_tests|crimson_shader_manifest_tests|crimson_material_system_tests|crimson_render_upload_tests|command_tests|crimson_overlay_tests|document_tests" --output-on-failure` passed, 121/121 tests.
- `cmake --build --preset qt-mingw-debug-deploy --parallel 20` passed.
- `python tools/dev_builds.py archive --preset qt-mingw-debug` created `0.1.1-dev.18`.
