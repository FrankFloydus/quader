# Overlay, Grid Material, and Camera Clip Parity Implementation

Date: 2026-07-06

Source of truth: `C:\Users\Drako\Desktop\quader-windows\quader-app`

Target: `C:\Users\Drako\Desktop\_quader-qt-base`

Board status: paused. This work must not mutate the project-board database or legacy markdown board files.

## Scope

Implement the current Windows reference overlay and grid material behavior in Crimson with necessary Qt/bgfx adaptations:

- Keep source-wire topology draw-on-top and near-plane clipped, without CPU visibility filtering through `SourceWireDepthStamp`.
- Keep `SourceWireDepthStamp` as semantic metadata with no visible draw batch.
- Keep component source/edge/vertex overlays depth-tested while non-component source wire and source vertices draw on top.
- Thread viewport settings and camera clip values through frame snapshots, projection, picking, overlay projection, ground-grid uniforms, and mesh material uniforms.
- Add grid/settings UI state for `Show Grid`, `Show Overlays`, and `Show Mesh Grid`, persisted through `SettingsService`.
- Add mesh surface grid projection in the material shader path, using world position and geometric/world normal rather than UVs or overlay geometry.
- Add perspective-only ground-grid far fade based on the active camera far clip.

## Reference Files

- `src/render/viewport_overlay_policy.cpp`
- `tests/render_extraction_tests.cpp`
- `resources/materials/ground_grid.mat`
- `resources/materials/quader_mesh.mat`
- `resources/materials/quader_mesh_lit.mat`
- `src/viewport/viewport_settings.*`
- `src/viewport/viewport_camera_clip_planes.hpp`
- `src/viewport/viewport_camera_controller.*`
- `src/render/viewport_renderer.*`
- `src/ui/viewport_panel.cpp`
- `src/render/overlay/render_overlay_snapshot.*`

## Target Files

- `src/crimson/overlays/overlay_command.hpp`
- `src/crimson/overlays/overlay_system.*`
- `src/crimson/gpu/gpu_overlay_renderer.*`
- `src/crimson/gpu/gpu_device.cpp`
- `src/crimson/gpu/gpu_material_cache.*`
- `src/crimson/frame/frame_builder.cpp`
- `src/crimson/viewport_frame.hpp`
- `src/crimson/scene/viewport_settings.hpp`
- `src/crimson/scene/render_camera_projection.*`
- `src/crimson/overlays/grid_overlay.*`
- `src/ui/viewport/viewport_types.hpp`
- `src/ui/viewport/viewport_controller.*`
- `src/ui/viewport/crimson_viewport_render_host.cpp`
- `src/ui/services/settings_service.*`
- `src/ui/actions/*`
- `src/ui/qt_app/main_window.*`
- `shaders/overlay_unlit.fs.sc`
- `shaders/pbr/opaque_pbr.fs.sc`
- `shaders/pbr/unlit_surface.fs.sc`
- Relevant Crimson/UI unit tests.

## Invariants

- Do not delete source-wire topology because of depth stamps.
- Component-mode `SourceWire`, selected-edge, and hover-edge line payloads are edit wires: their source kind is `ComponentSelection` or `ComponentHover`, and their bucket depth mode and render-state depth test must both be depth-tested.
- Non-component `SourceWire` lines and source vertices remain draw-on-top; component-sourced source/selected/hover vertex handles are depth-tested.
- Overlay lines use device-space expanded quads with line-distance feathering. Do not unproject expanded line quads back into world space and reproject them in the shader.
- Do not make all mesh materials double-sided or disable culling globally to hide flipped normals.
- Do not reuse `SourceWire` semantics for scene wireframe or mesh surface grid.
- Mesh grid alpha affects only surface color composition, not depth, picking, overlay depth stamps, or topology.
- Mesh surface grid settings must use reference spacing and line sizes (`major = spacing * 4`, minor `0.325`, major `0.250`) and default textured fallback. Do not hide selection overlays to mask material grid artifacts.
- `ShowOverlays=false` must not hide the ground grid when `ShowGrid=true`.
- Per-view camera near/far values must come from the active camera state; no private hard-coded near/far constants for overlays or grid fade.

## Review Rework 2026-07-06

Coordinator/user screenshots corrected the policy wording:

- Cyan component source/edge lines must not x-ray. They belong to the depth-tested edit-wire path.
- Flicker/missing segments come from the current world-unproject/reproject line quad path. Port the reference device-domain line mesh approach.
- The poor line quality is from flat hard-rectangle overlay lines. Add line-distance feathering.
- Yellow/orange false edge lines are mesh material grid inputs being too strong/thick, not selected overlay geometry.
- If ground-grid far fade appears absent after source changes, force shader/runtime copy for the launched configuration.
- Component source/selected/hover vertices must not x-ray through occluding mesh faces; keep them component-sourced and depth-tested.
- Depth-tested component edit wires need the reference edit-wire depth bias so coplanar cyan segments remain stable without being moved back to the always-on-top bucket.

## Verification

Run focused tests first where practical, then:

```powershell
cmake --preset qt-mingw-debug
cmake --build --preset qt-mingw-debug-tests --parallel 20
ctest --preset qt-mingw-debug-runtime
cmake --build --preset qt-mingw-debug --parallel 20
cmake --build --preset qt-mingw-debug-deploy --parallel 20
python tools/dev_builds.py current
python tools/dev_builds.py archive --preset qt-mingw-debug
```

Do not run clang-format, clang-tidy, or style/static-analysis presets without explicit permission.
