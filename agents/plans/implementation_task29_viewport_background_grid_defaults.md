# Task 29: Viewport Background And Grid Defaults

## Board

- Task: #29 Match reference viewport background and grid defaults
- Source of truth: `C:\Users\Drako\Desktop\quader-windows\quader-app`
- Scope: Crimson viewport background clear defaults and grid major/minor/axis default color, alpha, and thickness.

## Reference Findings

- `src\core\scene_environment_catalog.hpp:27` defines the default scene clear channel as `2.0f / 255.0f`.
- `src\viewport\viewport_settings_defaults.hpp:10-18` defines:
  - grid minor color: `150 / 255` RGB, alpha `1.0`
  - grid major color: `210 / 255` RGB, alpha `1.0`
  - grid minor line size: `0.325`
  - grid major line size: `0.250`
  - grid axis line size: `1.0`
- `src\viewport\viewport_settings.cpp:76-82` defines:
  - X axis: `{1.0, 0.239, 0.0, 0.72}`
  - Y axis: `{0.29, 0.58, 0.0, 0.0}`
  - Z axis: `{0.059, 0.612, 1.0, 0.72}`
- `src\render\viewport_grid_renderer.cpp:158-186` sends these settings to the grid shader as minor, major, axis colors, and line thickness values.
- `src\render\viewport_renderer.cpp:39-48,309-310` routes scene environment clear color into the viewport background.

## Current Quader Findings

- `src\crimson\scene\viewport_settings.hpp` already uses `0x020202ff` for the packed viewport clear color.
- `src\crimson\gpu\gpu_device.cpp` already uses `0x020202ff` for bgfx clear color.
- `src\crimson\overlays\overlay_command.hpp` and `src\crimson\overlays\grid_overlay.cpp` already carry the reference grid colors and thickness values through the grid command path.
- `src\crimson\scene\render_environment.hpp` still defaults linear clear color to `{0.02, 0.02, 0.02}`, which is not the reference `2 / 255`.

## Review Rework

- Rejection evidence: user screenshot showed the Crimson grid rendering far brighter than the reference even though the nominal ground-grid setting values matched.
- Screenshot sampling confirmed the visible mismatch: the Crimson grey grid reached about RGB 123, while the reference grid reached about RGB 68 in comparable lower-view grid areas.
- `resources\materials\ground_grid.mat` uses `blending : transparent` and writes `material.baseColor = vec4(color * alpha, alpha)`.
- Crimson's `shaders\fs_grid.sc` correctly ported the reference shader output, but `GpuOverlayRenderer::submit_grid` used premultiplied bgfx blending (`ONE`, `INV_SRC_ALPHA`). That applies alpha once, while the reference transparent material applies source alpha to an already alpha-weighted RGB value.
- Rework fix: use straight transparent blending (`SRC_ALPHA`, `INV_SRC_ALPHA`) for grid submissions so the visible grid luminance matches the reference material's effective alpha treatment.

## Implementation

1. Change `RenderEnvironment::clear_color_linear` default to exact `2.0F / 255.0F` channels.
2. Add focused regression coverage that proves the default snapshot environment and viewport packed clear color match the reference.
3. Add focused grid regression coverage that proves minor/major/axis colors and minor/major/axis thicknesses match the reference for the default XZ grid and orthographic axis-plane variants.
4. Change only the grid GPU submit blend state to the reference straight-alpha transparent material behavior.
5. Add focused GPU resource regression coverage for the grid submit blend state.
6. Keep renderer public naming under Crimson and avoid changing shader math, grid placement, or overlay ordering.

## Verification

- `cmake --build --preset qt-mingw-debug --parallel 20`
- `cmake --build --preset qt-mingw-debug-tests --parallel 20`
- `ctest --preset qt-mingw-debug-runtime -R "crimson_(gpu_resource|overlay|frame_snapshot)_tests" --output-on-failure`
- `cmake --build --preset qt-mingw-debug-deploy --parallel 20`
- `python tools/dev_builds.py archive --preset qt-mingw-debug`
- `python tools/project_board.py validate`

## Verified

- `cmake --build --preset qt-mingw-debug --parallel 20` passed.
- `cmake --build --preset qt-mingw-debug-tests --parallel 20` passed.
- `ctest --preset qt-mingw-debug-runtime -R "crimson_(gpu_resource|overlay|frame_snapshot)_tests" --output-on-failure` passed: 24/24 tests.
- `cmake --build --preset qt-mingw-debug-deploy --parallel 20` passed.
- `python tools/dev_builds.py archive --preset qt-mingw-debug` archived `0.1.1-dev.20`.
