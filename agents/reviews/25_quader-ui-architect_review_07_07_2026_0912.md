# Task 25 Rework UI Architect Review

Decision: changes-requested

## Scope Reviewed

- `src/ui/viewport/viewport_controller.cpp`
- `src/ui/viewport/crimson_viewport_render_host.cpp`
- `src/crimson/overlays/source_wire_visibility.hpp`
- `src/crimson/overlays/source_wire_visibility.cpp`
- `src/crimson/gpu/gpu_device.cpp`
- `tests/unit/ui/viewport_tests.cpp`
- `tests/unit/crimson/overlay_tests.cpp`
- `CMakeLists.txt`
- `DEV_VERSION`
- `agents/plans/task25_source_wire_stamp_visibility_rework_07_07_2026_0800.md`

Build/test evidence supplied by requester, not rerun here:

- `cmake --build --preset qt-mingw-debug --parallel 20`
- `cmake --build --preset qt-mingw-debug-tests --parallel 20`
- `ctest --preset qt-mingw-debug-runtime -R "crimson_overlay_tests|ui_viewport_tests" --output-on-failure`
- `ctest --preset qt-mingw-debug-runtime -R "crimson_gpu_resource_tests" --output-on-failure`
- deploy and dev archive `0.1.1-dev.40`

Additional non-mutating check run during review:

- `git diff --check` completed with only existing line-ending conversion warnings.

## Findings

### 1. Missing controller-level regression test for the CPU inside-out handle-picking rework

Severity: High

The new CPU picking behavior is implemented in `ViewportController::surface_hit_for_ray()` by building source-wire stamp metadata for `kComponentPickObjects` and passing `kHandleVisibilityFilter` through both vertex and edge handle occlusion checks:

- `src/ui/viewport/viewport_controller.cpp:1197`
- `src/ui/viewport/viewport_controller.cpp:1206`
- `src/ui/viewport/viewport_controller.cpp:1241`
- `src/ui/viewport/viewport_controller.cpp:1314`

The tests added in this rework only exercise the pure Crimson filter:

- `tests/unit/crimson/overlay_tests.cpp:403`
- `tests/unit/crimson/overlay_tests.cpp:427`
- `tests/unit/crimson/overlay_tests.cpp:447`

The UI test change at `tests/unit/ui/viewport_tests.cpp:1377` only proves prepared component vertex handles are depth-tested. It does not drive `ViewportController`, `handle_occluded_by_same_mesh()`, `component_pick_object_order()`, object id encoding, selection mode, or the edge/vertex pick loops. This means a regression where `ViewportController` stops applying the filter, applies it to the wrong object id, or only fixes vertex but not edge handles would still pass the current suite.

Required correction:

- Add focused `tests/unit/ui/viewport_tests.cpp` coverage that drives `ViewportController::handle_mouse_move()` or `handle_mouse_press()` through the real select tool path in component mode.
- The test must construct an inside-out same-mesh volume, for example by creating a box with `add_box_object()`, reversing all face windings with `object->mesh.reverse_face_windings(object->mesh.face_ids())`, setting `ToolManager` active tool to `ToolId::Select`, and setting `SelectionMode::Vertex` and/or `SelectionMode::Edge`.
- Assert that a handle which the old same-mesh face occlusion path would reject is now reported through `tools.selection_hover()` or committed to document component selection.
- Add the paired outside/normal-volume guard: an outward same-mesh hidden handle remains rejected, so the exception does not turn CPU picking into unconditional through-mesh picking.
- The UI regression must fail if the call to `source_wire_visibility_allows_handle_through_inside_out_volume()` is removed from the vertex or edge overloads of `handle_occluded_by_same_mesh()`.

Acceptance criteria:

- `ui_viewport_tests` contains a behavior-level regression for the inside-out CPU picking exception.
- The test covers the controller/tool event path, not only `SourceWireDepthStampVisibilityFilter` directly.
- Vertex and edge behavior are either both tested, or one is tested and the other has an explicit, defensible reason recorded in the test/review notes.

### 2. Missing wireframe viewport parity regression test for no hidden mesh proxies and draw-on-top topology

Severity: High

The wireframe parity fix is in `CrimsonViewportRenderHost`:

- `src/ui/viewport/crimson_viewport_render_host.cpp:933` returns early from `append_document_render_data()` in wireframe mode, so filled document `RenderObject`s are not submitted.
- `src/ui/viewport/crimson_viewport_render_host.cpp:1039` now emits scene wireframe topology overlays with `OverlayDepthMode::AlwaysOnTop`.

There is no UI test that inspects this behavior. The existing viewport shading test only checks base shader mapping:

- `tests/unit/ui/viewport_tests.cpp:616`

That test would pass if wireframe mode regressed back to submitting filled mesh render objects as depth proxies, or if scene topology returned to `DepthTested`. Both are the exact user-visible parity problems this rework is supposed to lock.

Required correction:

- Add focused `tests/unit/ui/viewport_tests.cpp` coverage for wireframe frame assembly.
- The test should use a document with at least one mesh object and `ViewportShadingMode::Wireframe`.
- Assert that the assembled Crimson frame input contains no document mesh `RenderObject`s in wireframe mode.
- Assert that scene topology overlay commands are emitted for the active view count and use `crimson::OverlayDepthMode::AlwaysOnTop`.
- If the current `CrimsonViewportRenderHost` private structure prevents this without a test seam, extract a narrow pure helper for frame assembly or wireframe overlay payload assembly in the UI viewport module. Keep it UI-side; do not move document truth into Crimson and do not expose a broad public API only for tests.

Acceptance criteria:

- The new test fails against the pre-rework behavior where wireframe submitted mesh objects and depth-tested scene wire overlays.
- The test does not require GPU readback or pixel snapshots; it should assert semantic frame/overlay payloads.
- `ui_viewport_tests` remains deterministic and does not depend on a live native surface.

## Non-Blocking Notes

- I did not find a concrete production-code defect in the inspected CPU filter integration or wireframe implementation. The shared `SourceWireDepthStampVisibilityFilter` is consumed by GPU point routing and by UI CPU picking without mutating prepared overlay lists.
- The `ViewportController` source-stamp construction duplicates the same triangle fan and object-id policy used by the document selection overlay adapter. That is acceptable for this rework, but the new controller-level tests should make object-id and winding parity visible if either path drifts.
- The wireframe implementation intentionally removes filled render objects, which can affect any future GPU picking path that depends on `RenderObject::pickable`. Current tool selection uses CPU document picking, so I am not blocking on that.

## Qt resources checked:

- https://doc.qt.io/qt-6/qmouseevent.html - `QMouseEvent::position()` is widget-local, supporting the current review focus on viewport-local to pane-local conversion in `ViewportController` tests.
- https://doc.qt.io/qt-6/qwidget.html - QWidget mouse event handlers and coordinate mapping APIs confirm that mouse input should stay at the widget/UI boundary before being translated into viewport controller coordinates.
- https://doc.qt.io/qt-6/qobject.html - QObject thread affinity rules confirm no new cross-thread widget or signal/slot ownership concern was introduced by the reviewed UI changes.
