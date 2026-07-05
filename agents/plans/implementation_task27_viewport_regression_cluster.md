# Task #27 - Viewport Placement, Grid Depth, and Default Texture Regression Cluster

## Board Scope

Fix the current user-visible regressions as one task:

- Box creation must work on top of existing meshes.
- Box creation must work on vertical planes.
- Perspective grid must stop flickering/bleeding based on camera/depth precision.
- Viewport/material color handling must remain correct after the default albedo change.
- The copied default albedo must use mip/filtering quality comparable to the reference app.

Reference source of truth: `C:\Users\Drako\Desktop\quader-windows\quader-app`.

## Reference Substance Preserved

- `src/editor/interactions/box_tool.cpp:121-360`: construction planes are normalized from hit position and normal, ray/plane construction supports locked planes, footprint snapping uses plane coordinates, and tiny footprints expand to a grid cell.
- `src/editor/tools/base/two_stage_box_tool.cpp:94-158`: pointer flow keeps one modal box operation and commits through the preview pipeline.
- `src/editor/commands/modeling/commit_box_preview_command.cpp:88-108`: committed boxes become real document objects and select only the new object.
- `src/viewport/viewport_grid_placement_policy.hpp:18-162`: grid remains camera-following for coverage, with a small plane offset and per-plane axes.
- `src/render/viewport_grid_renderer.cpp:50-55,170-228`: viewport setting colors are sent as linear values and grid parameters drive line width/fade.
- `src/render/png_texture_loader.cpp:26-105`: color textures are created as sRGB textures with generated mipmaps when requested.

## Current Findings

- `src/ui/viewport/viewport_controller.cpp` computes CPU document surface hits for active tools. Box tool input depends on this path, so regressions on existing/vertical surfaces need viewport-level tests that exercise real document geometry rather than synthetic `SurfaceHit` only.
- `src/tools/box_tool.cpp` already supports synthetic vertical surface hits, but locked footprint updates reject a ray/plane miss and ignore live surface fallback while dragging. This is brittle when the ray becomes parallel to a vertical construction plane.
- `src/crimson/overlays/grid_overlay.cpp` should keep the reference `0.002m` plane offset. The previous `0.05m` separation is visibly wrong; perspective flicker must be solved by disabling grid depth testing in perspective while keeping orthographic grid depth-tested.
- `src/crimson/gpu/gpu_overlay_renderer.cpp` currently hard-codes `BGFX_STATE_DEPTH_TEST_LESS` for grid draws, so grid commands cannot actually opt out of depth testing.
- `src/ui/viewport/crimson_viewport_render_host.cpp` receives `ViewportShadingMode` but document meshes are always submitted as `OpaquePbr`, so shaded viewport meshes receive the hard-coded PBR shader light instead of the reference app's unlit shaded look.
- `shaders/fs_grid.sc` uses world coordinates and camera-distance fade. Color uniforms are already linearized by `src/crimson/overlays/overlay_system.cpp`, and grid submission uses premultiplied blending in `src/crimson/gpu/gpu_overlay_renderer.cpp`.
- `src/crimson/gpu/gpu_material_cache.cpp` uploads the default albedo as an sRGB texture but creates only one mip level. The reference app generates mipmaps for loaded PNG textures.
- Screenshot rework: drawing the perspective grid as an always-on-top post-tonemap overlay makes meshes look transparent because the grid paints over the scene. The depth-disabled perspective grid must be submitted as a scene underlay before mesh color drawing; orthographic grid remains in the depth-tested overlay path.
- Second screenshot rework: the copied `default_albedo.png` is byte-identical to the reference app, so the remaining blur is from Crimson upload/sampling/UV handling. Reference Quader uses generated UV density `0.5` tiles per world unit, V-flips material UVs in the shader, and samples mesh material textures with anisotropic repeat filtering.
- Second screenshot rework: the stored box mesh winding tests were not enough to catch viewport-visible inversion. The viewport upload path must be testable directly and must enforce front-facing triangle order against the face normal before BGFX culling sees the triangles.
- Third screenshot rework: original Quader uses authored/winding-derived picked face normals for Box construction and lets Flip Faces visibly change normals. The previous ray-facing Box-tool fix masked flipped faces and contradicted the reference. The real viewport inversion was a Crimson front-face/cull mismatch, and the grazing-angle texture blur was BGFX max anisotropy not being enabled at reset time.

## Implementation Plan

1. Box placement robustness
   - Add viewport regression coverage for creating a box on top of a real existing mesh and on a real vertical face through `ViewportController`, not just direct synthetic `BoxTool` events.
   - Preserve existing command-history and return-to-select behavior.
   - Make locked construction-plane updates robust when the live ray cannot produce a forward ray-plane hit by projecting the current surface hit back to the locked plane as a fallback.
   - Keep selection/tool API boundaries intact: `ViewportController` translates input; `BoxTool` owns construction-plane state and command commit.

2. Grid depth/color stability
   - Restore the reference app's small grid plane offset.
   - Make perspective grid commands `AlwaysOnTop` so they do not z-fight against mesh floors.
   - Submit perspective grid commands through a scene-underlay pass before mesh color drawing, not through the post-tonemap always-on-top overlay pass.
   - Keep orthographic grid commands depth-tested so orthographic views still respect mesh depth.
   - Make `GpuOverlayRenderer::submit_grid` honor the command depth mode instead of hard-coding depth testing.
   - Preserve the existing sRGB-to-linear overlay color path and premultiplied grid blend contract.
   - Add/adjust overlay tests so projection-dependent grid depth mode and the reference offset are explicit.

3. Shaded viewport material path
   - Route `ViewportShadingMode::Shaded` document meshes to `UnlitSurface`; keep `Rendered` on `OpaquePbr`.
   - Add the base-color texture slot to `UnlitSurface` so the default albedo remains visible in shaded mode.
   - Update the unlit shader to sample the sRGB base-color texture and use it without the hard-coded PBR light.
   - Keep the default material helper schema-aware so unlit materials do not receive undeclared PBR parameters.

4. Default albedo quality and color path
   - Generate a complete mip chain for decoded RGBA8 default albedo data.
   - For sRGB albedo mipmaps, downsample RGB in linear light and encode the mip texels back to sRGB bytes before upload.
   - Create the BGFX texture with mip data and linear mip filtering. Keep fallback 1x1 textures cheap and deterministic.
   - Track upload bytes/count consistently.
   - Add material-system tests for mip count/linear-sRGB downsampling helpers without requiring a live GPU context.

5. Box placement rework
   - Superseded: the earlier ray-facing surface-normal orientation hid Flip Faces and does not match the reference app.
   - Use authored/winding-derived surface-hit normals when locking the construction plane.
   - Keep committed boxes outward-oriented through `make_box_from_corners`.
   - Fix viewport-visible inversion through the Crimson cull/front-face convention instead of changing authored mesh normals.

6. Verification
   - Run focused unit tests: `tool_manager_tests`, `ui_viewport_tests`, `crimson_overlay_tests`, `crimson_material_system_tests`, and any touched renderer upload/material tests.
   - Run shader manifest validation after shader changes, if any.
   - Build `qt-mingw-debug` and `qt-mingw-debug-tests`.
   - Deploy and create a dev-build archive because this changes user-visible runtime behavior.
   - Validate the project board and move #27 to review.

7. Second review rework
   - Expose the viewport document-mesh upload builder as a narrow test seam used by the render host.
   - Generate document upload UVs with the reference Quader axis basis and `0.5` tiles per world unit.
   - Correct each uploaded triangle against the computed face normal before writing Crimson vertex/index data.
   - Flip PBR/unlit material texture V in shaders to match the reference material shader convention.
   - Create default/material textures with anisotropic repeat sampler flags while leaving solid fallback textures point/clamp.
   - Add upload-level regressions that assert front-facing triangles and reference UV density for created boxes.

8. Third review rework
   - Restore Box tool authored-normal construction semantics for Flip Faces parity.
   - Preserve flipped mesh winding in the viewport upload path while keeping uploaded triangles consistent with their authored face normals.
   - Switch Crimson single-sided PBR submission to the cull side matching the current bgfx-facing camera/projection convention, fixing normal outward boxes that looked inverted in the viewport.
   - Default viewport tone mapping to linear and make the tone-map shader honor the selected mapper so shaded default material color matches the reference app's post-processing-disabled view.
   - Enable `BGFX_RESET_MAXANISOTROPY` on initialization/reset so anisotropic material sampler flags actually sharpen default albedo filtering at grazing angles.

## Verification

Second review rework verification:

- `cmake --build --preset qt-mingw-debug --parallel 20` passed after one transient Windows link lock retry.
- `cmake --build --preset qt-mingw-debug-tests --target ui_viewport_tests crimson_material_system_tests crimson_shader_manifest_tests --parallel 20` passed.
- Focused tests passed:
  - `.\build\qt-mingw-debug\ui_viewport_tests.exe --gtest_filter=Viewport.CrimsonMeshUploadUsesReferenceUvScaleAndFrontFacingTriangles:Viewport.BoxToolCreatesMeshOnExistingTopSurfaceThroughController:Viewport.BoxToolCreatesMeshFromVerticalSurfaceThroughController`
  - `.\build\qt-mingw-debug\crimson_material_system_tests.exe --gtest_filter=MaterialSystem.MaterialTextureSamplerFlagsUseReferenceAnisotropicRepeatFiltering:MaterialSystem.DefaultQuaderMaterialBindsReferenceAlbedoTexture:MaterialSystem.Rgba8MipChainUsesSrgbAwareDownsampling`
  - `.\build\qt-mingw-debug\crimson_shader_manifest_tests.exe`
- Broader focused suites passed:
  - `.\build\qt-mingw-debug\ui_viewport_tests.exe`
  - `.\build\qt-mingw-debug\crimson_material_system_tests.exe`
  - `.\build\qt-mingw-debug\tool_manager_tests.exe --gtest_filter=BoxTool.*`
- `cmake --build --preset qt-mingw-debug-deploy --parallel 20` passed.
- `python tools\dev_builds.py archive --preset qt-mingw-debug --json` created `0.1.1-dev.13` from `build\qt-mingw-debug\quader.exe`.

Third review rework verification:

- `cmake --build --preset qt-mingw-debug --parallel 20` passed and relinked `build\qt-mingw-debug\quader.exe`.
- `cmake --build --preset qt-mingw-debug-tests --target ui_viewport_tests tool_manager_tests crimson_material_system_tests crimson_overlay_tests crimson_render_graph_correctness_tests crimson_shader_manifest_tests crimson_frame_snapshot_tests crimson_color_space_tests crimson_render_packet_pipeline_tests --parallel 20` passed.
- Focused and broader tests passed:
  - `.\build\qt-mingw-debug\ui_viewport_tests.exe`
  - `.\build\qt-mingw-debug\tool_manager_tests.exe --gtest_filter=BoxTool.*`
  - `.\build\qt-mingw-debug\crimson_material_system_tests.exe`
  - `.\build\qt-mingw-debug\crimson_overlay_tests.exe`
  - `.\build\qt-mingw-debug\crimson_render_graph_correctness_tests.exe`
  - `.\build\qt-mingw-debug\crimson_shader_manifest_tests.exe`
  - `.\build\qt-mingw-debug\crimson_frame_snapshot_tests.exe`
  - `.\build\qt-mingw-debug\crimson_color_space_tests.exe`
  - `.\build\qt-mingw-debug\crimson_render_packet_pipeline_tests.exe`
- `python build_tools\validate_shader_manifest.py --manifest shaders\manifest.json --compiled build\qt-mingw-debug\compiled_shaders` passed.
- After the grazing-angle texture report, `cmake --build --preset qt-mingw-debug --parallel 20` passed again with `BGFX_RESET_MAXANISOTROPY`, and these focused tests passed:
  - `.\build\qt-mingw-debug\crimson_material_system_tests.exe --gtest_filter=MaterialSystem.MaterialTextureSamplerFlagsUseReferenceAnisotropicRepeatFiltering:MaterialSystem.DefaultQuaderMaterialBindsReferenceAlbedoTexture:MaterialSystem.Rgba8MipChainUsesSrgbAwareDownsampling`
  - `.\build\qt-mingw-debug\ui_viewport_tests.exe --gtest_filter=Viewport.CrimsonMeshUploadUsesReferenceUvScaleAndFrontFacingTriangles:Viewport.CrimsonMeshUploadPreservesFlipFacesWinding:Viewport.BoxToolCreatesMeshOnExistingTopSurfaceThroughController:Viewport.BoxToolCreatesMeshFromVerticalSurfaceThroughController`
  - `.\build\qt-mingw-debug\tool_manager_tests.exe --gtest_filter=BoxTool.TopSurfaceHitUsesAuthoredNormalForFlipFaces:BoxTool.VerticalSurfaceHitUsesAuthoredNormalForFlipFaces:BoxTool.SurfaceHitSeedsVerticalConstructionPlane`
  - `.\build\qt-mingw-debug\crimson_frame_snapshot_tests.exe`
  - `.\build\qt-mingw-debug\crimson_shader_manifest_tests.exe`
- `cmake --build --preset qt-mingw-debug-deploy --parallel 20` passed.

Review rework verification:

- `cmake --build --preset qt-mingw-debug --parallel 20` passed.
- `cmake --build --preset qt-mingw-debug-tests --parallel 20` passed.
- Focused tests passed:
  - `.\build\qt-mingw-debug\tool_manager_tests.exe`
  - `.\build\qt-mingw-debug\ui_viewport_tests.exe`
  - `.\build\qt-mingw-debug\crimson_overlay_tests.exe`
  - `.\build\qt-mingw-debug\crimson_material_system_tests.exe`
  - `.\build\qt-mingw-debug\crimson_render_packet_pipeline_tests.exe`
  - `.\build\qt-mingw-debug\crimson_render_upload_tests.exe`
  - `.\build\qt-mingw-debug\crimson_render_graph_correctness_tests.exe`
  - `.\build\qt-mingw-debug\crimson_shader_manifest_tests.exe`
- `python build_tools\validate_shader_manifest.py --manifest shaders\manifest.json --compiled build\qt-mingw-debug\compiled_shaders` passed.
- `cmake --build --preset qt-mingw-debug-deploy --parallel 20` passed.
- `python tools\dev_builds.py archive --preset qt-mingw-debug --json` created `0.1.1-dev.12`.
- `python tools\project_board.py validate` passed.

Superseded rework archive:

- `0.1.1-dev.11` is superseded by `0.1.1-dev.12`; it still drew the perspective grid through the post-tonemap overlay path and made meshes appear transparent.

Previous verification from the rejected pass is retained below for history only.

- `cmake --build --preset qt-mingw-debug --parallel 20` passed.
- `cmake --build --preset qt-mingw-debug-tests --parallel 20` passed.
- Focused tests passed:
  - `.\build\qt-mingw-debug\tool_manager_tests.exe`
  - `.\build\qt-mingw-debug\ui_viewport_tests.exe`
  - `.\build\qt-mingw-debug\crimson_overlay_tests.exe`
  - `.\build\qt-mingw-debug\crimson_material_system_tests.exe`
  - `.\build\qt-mingw-debug\crimson_render_upload_tests.exe`
- `python build_tools\validate_shader_manifest.py --manifest shaders\manifest.json --compiled build\qt-mingw-debug\compiled_shaders` passed.
- `cmake --build --preset qt-mingw-debug-deploy --parallel 20` passed.
- `python tools\dev_builds.py archive --preset qt-mingw-debug --json` created `0.1.1-dev.10`.
- `python tools\project_board.py validate` passed.

## Checklist

- [x] Box tool creates a new mesh on top of an existing mesh through viewport-dispatched input.
- [x] Box tool creates a new mesh from a vertical face through viewport-dispatched input.
- [x] Locked vertical construction-plane drags survive parallel/near-parallel rays by using safe fallback data.
- [x] Perspective grid no longer z-fights with ground-level meshes by disabling grid depth testing in perspective.
- [x] Perspective grid is submitted before mesh color drawing so disabling grid depth testing does not draw grid lines through meshes.
- [x] Orthographic grid remains depth-tested.
- [x] New boxes created through top-face and vertical-face viewport input keep outward face winding.
- [x] Shaded viewport document meshes use textured unlit shading instead of the hard-coded PBR light.
- [x] Overlay/grid color remains linearized once and blended with the matching premultiplied contract.
- [x] Default albedo uploads with generated mip levels and sRGB-aware downsampling.
- [x] BGFX max anisotropy is enabled so anisotropic sampler flags affect grazing-angle material filtering.
- [x] Flip Faces preserves authored inverted normals instead of being hidden by Box tool or upload repair logic.
- [x] Focused tests pass.
- [x] Debug build, tests build, deploy, dev-build archive, and board validation are complete or blockers recorded.
