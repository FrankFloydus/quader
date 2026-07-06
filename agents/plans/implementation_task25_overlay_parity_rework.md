# Task 25 Overlay Parity Rework

## Scope

Fix the overlay handling regressions identified by the original Quader Windows helper for task #25. Source of truth is `C:\Users\Drako\Desktop\quader-windows\quader-app`; preserve user-visible overlay substance while adapting Filament/reversed-depth policy to Crimson/bgfx normal-depth rendering.

## Reference Mapping

- Reference overlay constants and policy: `docs/overlay-handling.md`, `src/render/viewport_overlay_policy.hpp:74-151`, `src/render/viewport_overlay_policy.cpp:146-263`.
- Reference line/point construction: `src/render/overlay/overlay_camera_projector.cpp:60-147`, `overlay_line_mesh_builder.cpp:35-94`, `overlay_point_mesh_builder.cpp:36-69`.
- Reference object/component overlay ownership: `src/editor/modeling/runtime/session_selection_overlay.cpp:401-439,663-846`, `tests/modeling_domain/qdr_modeling_runtime_overlay_tests.cpp:330-414,527-694`.
- Current targets: `src/ui/viewport/tool_preview_overlay_adapter.cpp`, `src/crimson/gpu/gpu_overlay_renderer.hpp/.cpp`, `src/crimson/overlays/overlay_system.cpp`, `tests/unit/ui/viewport_tests.cpp`, `tests/unit/crimson/overlay_tests.cpp`, `tests/unit/crimson/gpu_resource_tests.cpp`.

## Ordered Edits

1. Update selection overlay constants in `tool_preview_overlay_adapter.cpp`:
   - selected/source/hover wire and vertex colors exactly match decoded reference RGBA defaults;
   - selected face alpha is `8/255`, hover face alpha is `22/255`;
   - selected object topology width uses selected-edge width `2.5 px`, not `3.0 px`.
2. Add object-mode selected face-fill payloads for selected meshes:
   - keep selected topology wires;
   - add selected face-fill triangles for all live selected object faces using `SelectedFaceFill` and `ObjectSelection`;
   - rely on `OverlaySystem` face-fill splitting for depth-stamp plus equal-depth color.
3. Fix source-wire ownership:
   - current-mode selected components choose the single source-wire owner;
   - hover does not add a second full source wire when selected current-mode components already own one;
   - hover remains source-wire owner only when no selected current-mode component owns a source wire.
4. Rework GPU overlay line/point expansion:
   - project overlay endpoints/centers to viewport pixels/device space;
   - clip lines against the near plane;
   - expand caps/widths in screen pixels with the reference 1 px feather included in the quad footprint;
   - apply post-projection normal-depth bias by subtracting `bias_units * near * 0.0025` from device depth for depth-tested lines/points;
   - unproject expanded corners back to world positions so the existing overlay shader path remains usable.
5. Tighten bgfx state:
   - depth-tested overlays use `LEQUAL`;
   - grid depth-tested state also uses `LEQUAL`;
   - face-fill two-sided/no-cull remains explicit by omitting cull bits.
6. Replace and add tests:
   - object-mode selection emits selected face-fill triangles;
   - component selected lines use selected yellow, face alpha values match reference, and selected object width is `2.5`;
   - selected component owner prevents a different hover object from emitting a second source wire;
   - CPU-exposed overlay quad helpers prove screen-space line caps and point depth bias.
7. Apply helper review follow-up for the screenshot regression:
   - keep component selected/hover handles in source-wire layer order so cyan source wire submits before selected/hover highlights;
   - preserve component handle depth-tested render state even when the draw bucket is `AlwaysOnTop`;
   - split selected face-edge commands from selected edge commands and hover face-edge commands from hover edge commands;
   - suppress selected face/edge/vertex base overlays under a matching selected hover target;
   - prioritize current source-wire owner objects in edge/vertex hover picking before shaded-surface fallback.
8. Apply helper review follow-up for stale hover and component-mode XRay:
   - clear visible selection hover immediately after a valid left-click selection hit and suppress the same hit until pointer/modifier state changes, including no-op selected clicks;
   - carry modifier-aware selected-hover/remove-preview state from `ToolManager` into overlay extraction;
   - suppress matching selected base overlays only for modifier selected-hover, not for normal hover;
   - keep component-mode mesh surfaces in the normal opaque scene queue and keep object-mode whole-mesh XRay fills out of vertex/edge/face component modes;
   - mark selected component face-fill commands as `DepthTested` rather than `XRay` while preserving the two-pass depth-stamped/equal-depth render state.

## Acceptance Criteria

- Reference substance preserved: overlay colors, widths, face-fill behavior, source-wire ownership, and line/point screen-space construction match the original app policy after Crimson/bgfx depth convention translation.
- Source-wire depth stamps remain metadata-only.
- Face fills remain two-pass depth-stamped/equal-depth and two-sided; no cull-state or shader-normal workaround is used for inverted/normal face handling.
- Focused unit tests pass; debug build/deploy/dev-build checkpoint are run unless blocked by existing environment state.

## Review Rework Result

- Updated adapter defaults to the decoded reference RGBA colors and sizes, including selected face fill `8/255`, hover face fill `22/255`, selected/source/hover wire and vertex colors, and selected object topology `2.5 px`.
- Object-mode selected meshes now emit selected face-fill triangle payloads in addition to selected topology wire payloads.
- Component hover no longer emits a second full source-wire owner when current-mode selected components already own source wire.
- Crimson overlay lines and points now build screen-space quads from projected points with near-plane clipping, reference cap/feather growth, and post-projection normal-depth bias before unprojecting back to the existing overlay shader path.
- Depth-tested grid state now uses bgfx `LEQUAL`; XRay generic overlay/grid paths draw without depth test while face fills keep their depth-stamped/equal-depth policy.
- Helper screenshot follow-up: component selected/hover handles now stay in the same draw-order bucket as source wire, with source wire emitted first and handle render state still depth-tested. Selected face-edge, selected edge, hover face-edge, and hover edge commands are split by role so the selected orange/yellow pass is not hidden behind a later cyan source-wire pass.
- Matching selected hover targets now suppress the selected base face/edge/vertex overlay for that same component and let the hover overlay draw on top, matching the reference suppression-mask behavior.
- Component hover picking now prioritizes current-mode selected source-wire owners and current hover candidates before shaded face fallback, and edge handle occlusion uses the existing ray-to-segment distance helper.
- Helper stale-hover/component-mode follow-up: `ToolManager` now clears visible hover and stores same-hit suppression after valid click-selection, including no-op selected clicks, and exposes modifier-aware selected-hover state for remove-preview overlays.
- Normal hover over an already-selected component no longer suppresses selected yellow/orange face/edge/vertex payloads. Suppression is now limited to explicit modifier selected-hover/remove-preview state passed into `append_document_selection_overlays`.
- Selected component face-fill commands are authored as `DepthTested`, not `XRay`; component-mode selected fills remain selected-face-only and object-selection whole-mesh XRay fill is not emitted in vertex/edge/face modes.

## Verification

- `cmake --build --preset qt-mingw-debug-tests --target crimson_overlay_tests crimson_gpu_resource_tests ui_viewport_tests --parallel 20`
- `ctest --preset qt-mingw-debug-runtime -R "(crimson_overlay_tests|crimson_gpu_resource_tests|ui_viewport_tests)" --output-on-failure` passed 55/55 tests.
- `cmake --build --preset qt-mingw-debug --parallel 20`
- `cmake --build --preset qt-mingw-debug-deploy --parallel 20`
- `python tools/dev_builds.py archive --preset qt-mingw-debug` archived `0.1.1-dev.22`.
- `cmake --build --preset qt-mingw-debug-tests --target tool_manager_tests ui_viewport_tests crimson_overlay_tests --parallel 20`
- `ctest --preset qt-mingw-debug-runtime -R "(tool_manager_tests|ui_viewport_tests|crimson_overlay_tests)" --output-on-failure` passed 80/80 tests.
- `cmake --build --preset qt-mingw-debug-tests --target crimson_gpu_resource_tests --parallel 20` reported no work to do.
- `ctest --preset qt-mingw-debug-runtime -R "(tool_manager_tests|ui_viewport_tests|crimson_overlay_tests|crimson_gpu_resource_tests)" --output-on-failure` passed 90/90 tests.
- `cmake --build --preset qt-mingw-debug --parallel 20`
- `cmake --build --preset qt-mingw-debug-deploy --parallel 20`
- `python tools/dev_builds.py archive --preset qt-mingw-debug` archived `0.1.1-dev.23`.

## Second Helper Rework Result

- Implemented the helper-requested source-wire depth-stamp visibility filter without changing the source-wire GPU state to depth-tested. `SourceWireDepthStamp` records remain metadata-only, now retaining triangle geometry for CPU visibility tests; surviving source-wire line segments still submit through the always-on-top line path.
- The filter keys stamps by view and render object, treats inverted/inside-out stamp sets as visible, samples source-wire segments at `0.2`, `0.5`, and `0.8`, requires at least two visible samples, and uses the reference tolerance `max(0.015, target_distance * 0.0005)`.
- Source-wire line submission now filters only `SourceWire`/`SourceWire` batches before `submit_lines`; selected/hover face-edge, edge, and vertex handle batches keep their existing layer-order bucket and component depth-tested render state.
- Added sticky-wire regressions for same-mesh selected face plus hover face and for no-current-mode component selection where hover replaces the fallback source-wire owner.

## Second Helper Verification

- `cmake --build --preset qt-mingw-debug-tests --target crimson_overlay_tests crimson_gpu_resource_tests ui_viewport_tests --parallel 20`
- `ctest --preset qt-mingw-debug-runtime -R "(crimson_overlay_tests|crimson_gpu_resource_tests|ui_viewport_tests)" --output-on-failure` passed 60/60 tests.
- `cmake --build --preset qt-mingw-debug --parallel 20`
- `cmake --build --preset qt-mingw-debug-deploy --parallel 20`
- `python tools/dev_builds.py archive --preset qt-mingw-debug` archived `0.1.1-dev.24`.

## Third Helper Rework Result

- Replaced the camera-facing inside-out source-wire heuristic with reference-style aggregate stamp winding keyed by `view_index + object_id`. The filter now computes a stamp-set centroid, scores each matching stamp triangle normal against its triangle centroid offset, and treats negative aggregate winding as inside-out.
- Broadened source-wire depth-stamp line filtering from only ordinary `SourceWire` batches to the full reference visibility set for source wire plus selected/hover face-edge and edge handles. Source-wire lines still submit always-on-top after CPU filtering; they were not converted to GPU depth-tested lines.
- Added point visibility filtering for `SourceVertex`, `SelectedVertex`, and `HoverVertex` before `submit_points`. Outward meshes cull hidden point handles using the same camera-to-point ray and `max(0.015, target_distance * 0.0005)` tolerance, while inside-out stamp sets bypass CPU deletion so inverted-room source/component handles remain visible.
- Threaded `source_wire_depth_stamps` through point overlay bucket submission so point commands use the same per-view metadata as line commands.
- Added regressions for hidden `SourceVertex`, hidden selected/hover vertex handles, inside-out preservation for source/component point handles, and the camera-inside outward-box case that proves inside-out is object-winding based rather than camera-facing.

## Third Helper Verification

- `cmake --build --preset qt-mingw-debug-tests --target crimson_gpu_resource_tests --parallel 20`
- `ctest --preset qt-mingw-debug-runtime -R "crimson_gpu_resource_tests" --output-on-failure` passed 16/16 tests.
- `cmake --build --preset qt-mingw-debug-tests --target crimson_overlay_tests crimson_gpu_resource_tests ui_viewport_tests --parallel 20`
- `ctest --preset qt-mingw-debug-runtime -R "(crimson_overlay_tests|crimson_gpu_resource_tests|ui_viewport_tests)" --output-on-failure` passed 64/64 tests.
- `cmake --build --preset qt-mingw-debug --parallel 20`
- `cmake --build --preset qt-mingw-debug-deploy --parallel 20`
- `python tools/dev_builds.py archive --preset qt-mingw-debug` archived `0.1.1-dev.25`.

## Fourth Helper Rework Result

- Added the reference endpoint/incident-triangle ownership skip to source-wire depth-stamp visibility. Whole-mesh `SourceWire`, `SelectedEdge`, and `HoverEdge` segments that carry only `object_id + edge_index` can no longer be culled by a stamp triangle that owns both line endpoints, which fixes the angle-dependent disappearing interior edge case on flipped/open room meshes.
- Replaced the boolean-only inside-out test with a stamp-set classification that keeps the original aggregate winding score and adds area-weighted inward/outward evidence. Inward-majority stamp sets are inside-out, mixed near-balanced sets are ambiguous, and both inside-out and ambiguous sets keep source/component topology through CPU filtering while normal outward planar/closed sets still cull hidden topology.
- Kept `SourceWire` draw state always-on-top after CPU filtering and kept selected/hover component edge render state depth-tested; the change is CPU visibility classification/ownership only, not a global XRay/depth-state workaround.
- Added regressions for endpoint-owned source-wire survival at a shallow angle, open inverted room interior edge visibility from multiple cameras, inside-out component edge CPU retention with depth-tested render state, and line depth bias moving toward the camera.

## Fourth Helper Verification

- `cmake --build --preset qt-mingw-debug-tests --target crimson_gpu_resource_tests --parallel 20`
- `ctest --preset qt-mingw-debug-runtime -R "crimson_gpu_resource_tests" --output-on-failure` passed 20/20 tests.
- `cmake --build --preset qt-mingw-debug-tests --target crimson_overlay_tests crimson_gpu_resource_tests ui_viewport_tests --parallel 20`
- `ctest --preset qt-mingw-debug-runtime -R "(crimson_overlay_tests|crimson_gpu_resource_tests|ui_viewport_tests)" --output-on-failure` passed 68/68 tests.
- `cmake --build --preset qt-mingw-debug --parallel 20`
- `cmake --build --preset qt-mingw-debug-deploy --parallel 20`
- `python tools/dev_builds.py archive --preset qt-mingw-debug` archived `0.1.1-dev.26`.

## Fifth Helper Rework Result

- Applied the exact review from communication thread `019f2d49-d913-7292-89da-5cb5fdd53b63`: source-wire depth-stamp CPU visibility now applies only to `SourceWire` lines and `SourceVertex` points with `OverlaySourceKind::SourceWire`. Selected/hover component face edges, edges, and vertices are no longer pre-deleted by source-wire stamps and remain GPU depth-tested through their overlay render state.
- Removed the ambiguous/open mixed-evidence visual bypass. Stamp sets keep source topology only when the aggregate classification is actual inside-out; mixed/open ambiguous stamp sets now fall back to normal source-wire self-visibility filtering.
- Replaced geometric endpoint ownership skipping with semantic ownership. `OverlayElementRef` now carries up to two incident face indices for edge primitives; whole-mesh source wires, selected/hover edges, and face-boundary selected/hover lines emit those incident faces from mesh halfedge topology. The source-wire filter skips a stamp only when the segment explicitly references the stamp face through `face_index` or incident-face metadata.
- Kept `SourceWireDepthStampMetadata` triangle geometry and the normal-depth bgfx bias sign unchanged.
- Updated GPU resource regressions so component selected/hover edges and vertices bypass CPU filtering while retaining depth-tested state; source topology still culls hidden outward handles; inside-out source topology remains visible; mixed/open stamp sets do not bypass; non-incident triangles with matching endpoint coordinates still occlude.

## Fifth Helper Verification

- `cmake --build --preset qt-mingw-debug-tests --target crimson_gpu_resource_tests --parallel 20`
- `ctest --preset qt-mingw-debug-runtime -R "crimson_gpu_resource_tests" --output-on-failure` passed 22/22 tests.
- `cmake --build --preset qt-mingw-debug --parallel 20`
- `cmake --build --preset qt-mingw-debug-deploy --parallel 20`
- `python tools/dev_builds.py archive --preset qt-mingw-debug --json` archived `0.1.1-dev.27`.
