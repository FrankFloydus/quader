# Task 25 Rework UI Architect Review - Fresh Pass

Decision: approved

## Scope Reviewed

- `src/ui/viewport/viewport_controller.cpp`
- `src/ui/viewport/crimson_viewport_render_host.cpp`
- `src/ui/viewport/crimson_viewport_render_host.hpp`
- `src/crimson/overlays/source_wire_visibility.cpp`
- `src/crimson/overlays/source_wire_visibility.hpp`
- `src/crimson/overlays/overlay_system.cpp`
- `src/crimson/gpu/gpu_device.cpp`
- `tests/unit/ui/viewport_tests.cpp`
- `tests/unit/crimson/overlay_tests.cpp`

Per request, `agents/hindsight.md` was not read or used for this review.

## Findings

None. I did not find UI-blocking bugs, regressions, or missing tests in the current uncommitted rework.

## Previous Changes-Requested Coverage

- The CPU inside-out picking gap is addressed through the real UI controller/select-tool path. `Viewport.ControllerPicksInsideOutRoomVertexBehindSameMeshSurface` now constructs an inside-out room fixture and drives `ViewportController::handle_mouse_move` with the active select tool in vertex mode (`tests/unit/ui/viewport_tests.cpp:388`, `tests/unit/ui/viewport_tests.cpp:1726`). The implementation routes component handle same-mesh occlusion through the shared source-wire visibility filter and only bypasses occlusion when that filter promotes the same object to `AlwaysOnTop` for an inside-out volume with the camera inside (`src/ui/viewport/viewport_controller.cpp:547`, `src/ui/viewport/viewport_controller.cpp:566`, `src/ui/viewport/viewport_controller.cpp:586`, `src/ui/viewport/viewport_controller.cpp:1206`, `src/ui/viewport/viewport_controller.cpp:1242`, `src/ui/viewport/viewport_controller.cpp:1315`).
- The wireframe viewport parity gap is addressed. `append_crimson_document_render_data` returns before appending filled scene `RenderObject`s in `Wireframe`, while `append_crimson_scene_wireframe_overlays` emits scene topology with `OverlayDepthMode::AlwaysOnTop` (`src/ui/viewport/crimson_viewport_render_host.cpp:579`, `src/ui/viewport/crimson_viewport_render_host.cpp:631`). `Viewport.WireframeFrameAssemblySkipsFilledObjectsAndUsesAlwaysOnTopTopology` asserts both the empty filled-object path and always-on-top topology for unselected and selected object topology (`tests/unit/ui/viewport_tests.cpp:540`).
- The component line-depth policy correction is covered. `SourceWire` now resolves to `AlwaysOnTop` before component edit-wire routing, and component selected/hover edge handles remain depth-tested (`src/crimson/overlays/overlay_system.cpp:361`, `src/crimson/overlays/overlay_system.cpp:385`, `tests/unit/crimson/overlay_tests.cpp:528`).
- The shared source-wire visibility filter has focused unit coverage for outward culling, inside-out CPU visibility, and inside-camera promotion to always-on-top (`tests/unit/crimson/overlay_tests.cpp:403`, `tests/unit/crimson/overlay_tests.cpp:427`, `tests/unit/crimson/overlay_tests.cpp:447`).

## Residual Risk

- The new controller regression covers the vertex branch. The edge branch shares the same same-mesh exception helper and existing controller edge picking coverage remains present, so I do not consider a separate inside-out edge test blocking for this UI review.
- The wireframe parity test targets the extracted frame-assembly helpers rather than a GPU readback. That is acceptable for this task because the risk was semantic frame construction: no filled render objects in wireframe mode and always-on-top topology commands.

## Verification

I relied on the requester-supplied successful verification:

- `cmake --build --preset qt-mingw-debug-tests --parallel 20`
- `ctest --preset qt-mingw-debug-runtime -R "crimson_overlay_tests|ui_viewport_tests|crimson_gpu_resource_tests" --output-on-failure`
- `cmake --build --preset qt-mingw-debug --parallel 20`
- `cmake --build --preset qt-mingw-debug-deploy --parallel 20`
- `python tools/dev_builds.py archive --preset qt-mingw-debug` -> `0.1.1-dev.41`

Additional non-mutating check run during review:

- `git diff --check` exited 0. It reported only Git LF-to-CRLF working-copy warnings for modified files, with no whitespace errors.

## Qt Resources Checked

- https://doc.qt.io/qt-6/qmouseevent.html - `QMouseEvent::position()` is local to the receiving widget/item, matching the controller tests' pane-local mouse coordinates.
- https://doc.qt.io/qt-6/qwidget.html - QWidget is the Qt Widgets input boundary, and mouse event handling remains on the viewport/controller side rather than moving document truth into widgets.
- https://doc.qt.io/qt-6/threads-qobject.html - QObject thread-affinity guidance was checked; this rework does not introduce new cross-thread UI ownership or queued UI mutation concerns.
