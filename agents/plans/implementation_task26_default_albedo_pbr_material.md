# Task 26: Default Albedo-Textured PBR Material

## Board Entry

- Task: #26 Add default albedo-textured PBR material
- Mode: ad hoc execute
- Source of truth: `C:\Users\Drako\Desktop\quader-windows\quader-app`
- Current repo: `C:\Users\Drako\Desktop\_quader-qt-base`

## Reference Findings

- `C:\Users\Drako\Desktop\quader-windows\quader-app\src\assets\default_material_catalog.hpp` defines the default material texture package paths:
  - `materials/default/default_albedo.png`
  - `materials/default/default_roughness.png`
- `C:\Users\Drako\Desktop\quader-windows\quader-app\resources\modeling_assets\materials\default\default_albedo.png` and `default_roughness.png` are the default material texture assets.
- `C:\Users\Drako\Desktop\quader-windows\quader-app\src\assets\material_document.hpp` and `.cpp` model the default material as white base color, roughness `1`, metallic `0`, with optional base-color and roughness texture slots.
- `C:\Users\Drako\Desktop\quader-windows\quader-app\src\render\material_binding_service.cpp` resolves the base-color texture slot through the texture binding service and falls back to the default albedo texture.
- `C:\Users\Drako\Desktop\quader-windows\quader-app\src\render\texture_binding_service.cpp` treats albedo/base-color texture data as sRGB and non-color maps as linear/data textures.
- `C:\Users\Drako\Desktop\quader-windows\quader-app\src\render\png_texture_loader.cpp` decodes PNG RGBA bytes and uploads them with the texture color-space policy.
- External spec check: glTF 2.0 materials include `baseColorTexture`, and primitives with no material use default material semantics. The albedo/base-color texture represents surface color/base reflectivity and should not bake lighting.

## Current Quader Findings

- `src/document/material.hpp` currently provides a neutral gray, fully rough, non-metallic `default_box_material()`.
- `src/tools/box_tool.cpp` assigns `default_box_material()` to newly committed Box tool meshes.
- `src/crimson/material/base_shader_registry.cpp` already declares a `base_color` PBR texture slot with `TextureColorSpace::Srgb`.
- `src/crimson/gpu/gpu_material_cache.cpp` creates solid default GPU textures and binds only PBR uniforms; it does not bind texture samplers for PBR draws yet.
- `shaders/pbr/*_pbr.fs.sc` only multiply lighting by `u_pbrBaseColor`; they do not sample `base_color` texture data.
- `src/crimson/mesh/render_mesh.hpp`, `src/crimson/mesh/render_mesh_upload.cpp`, and `src/crimson/gpu/gpu_mesh_cache.cpp` currently upload six floats per vertex: position and normal only.
- `src/ui/viewport/crimson_viewport_render_host.cpp` emits document mesh uploads without UV0 data and leaves render objects on the renderer default material handle.
- `CMakeLists.txt` copies compiled shaders to the executable directory, but no modeling asset runtime folder is currently copied.

## Scope

Implement the reference default material substance in current Quader/Crimson without importing the old app architecture wholesale.

In scope:

- Copy the reference default material PNGs into a repo runtime asset folder.
- Deploy the runtime material assets beside `quader.exe`.
- Add a renderer asset-root/default-material path so the UI host points Crimson at deployed assets.
- Use an allowed third-party PNG decoder privately for renderer texture loading. Prefer the existing bgfx/bimg dependency tree's LodePNG source to avoid adding a new package.
- Make PBR draw submission bind a base-color texture sampler.
- Load the copied default albedo as an sRGB texture, expose a renderer texture handle for it, and bind it to the default OpaquePbr material's `base_color` slot.
- Preserve solid fallback textures and structured diagnostics if the default PNG is missing or fails to decode.
- Add UV0 data to uploaded PBR meshes so texture sampling is real.
- Update the document default mesh material intent to white, roughness `1`, metallic `0`, while preserving `default_box_material()` as a compatibility alias if needed.
- Update targeted unit tests and documented symbols changed by this work.

Out of scope:

- Project browser, material editor UI, node graph UI, or broad asset-package import.
- Live user-selected texture import workflow beyond the default albedo asset.
- Normal, metallic/roughness, occlusion, emissive shader sampling beyond maintaining their existing schema/fallbacks.
- Renderer folder, target, or class names that use graphics-runtime names outside existing `src/crimson/gpu`.

## Implementation Steps

1. Assets and deployment:
   - Copy `default_albedo.png` and `default_roughness.png` from the reference app to `resources/modeling_assets/materials/default/`.
   - Add a CMake runtime asset copy target that mirrors `resources/modeling_assets` to `$<TARGET_FILE_DIR:quader_app>/resources/modeling_assets`.
   - Make the deploy target depend on this asset copy.

2. Renderer configuration:
   - Add `RendererConfig::asset_root` or an equivalent documented field.
   - Set it from `CrimsonViewportRenderHost::initialize_surface()` to `QCoreApplication::applicationDirPath() + "/resources/modeling_assets"`.

3. Default material constants/helper:
   - Add a small Crimson material helper for default material name and relative texture paths.
   - Build a default OpaquePbr material instance with white base color, roughness `1`, metallic `0`, and a `base_color` binding to the loaded default albedo texture handle.

4. PNG-backed texture loading:
   - Add private LodePNG source/include wiring to the `crimson` target using the existing `bgfx_cmake_SOURCE_DIR` dependency.
   - Extend `GpuMaterialCache` to load an RGBA PNG from the configured default albedo path, create an sRGB BGFX texture, and track upload counters.
   - Fall back to the solid white base-color texture if load/decode/create fails, and record a `RendererDiagnosticSeverity::Warning` or `Error` only when appropriate for the failure mode.

5. PBR texture binding:
   - Add a base-color sampler uniform and bind it at a fixed stage during PBR submission.
   - Resolve the material instance's `base_color` binding to a native texture when valid; otherwise use the solid base-color fallback.
   - Keep missing non-default/user texture handles falling back safely.

6. UV0 upload:
   - Extend render mesh upload payloads from position/normal to position/normal/uv0.
   - Add TexCoord0 to the GPU vertex layout for built-in and document PBR meshes.
   - Generate stable box/document UVs in the viewport upload path. For the current triangulated document geometry, use dominant-normal planar projection per emitted vertex to avoid inventing a wider UV authoring system.
   - Update upload validation, byte accounting, and tests for the new eight-float stride.

7. Shader sampling:
   - Update PBR vertex shaders to input/output `a_texcoord0`/`v_texcoord0`.
   - Update opaque, alpha-cutout, and transparent PBR fragment shaders to sample `s_pbrBaseColorTexture`, apply `u_pbrUvTransform0`, multiply by `u_pbrBaseColor`, and use sampled alpha for cutout/transparent alpha behavior.

8. Document default material:
   - Change the default new mesh material color intent from neutral gray to reference-compatible white while preserving roughness `1` and metallic `0`.
   - Update tests/docs that assert the prior gray default.

## Verification Plan

- `python tools/project_board.py validate`
- Targeted unit tests for:
  - material-system default material helper/texture binding behavior
  - render mesh upload validation and byte accounting with UV0
  - document default mesh material values
- `cmake --build --preset qt-mingw-debug --parallel 20`
- `cmake --build --preset qt-mingw-debug-tests --parallel 20`
- Targeted test executable or CTest filter that does not run style/static-analysis gates.
- `cmake --build --preset qt-mingw-debug-deploy --parallel 20`
- `python tools/dev_builds.py current`

## Acceptance Checklist

- [x] Reference default material assets are present in this repo.
- [x] Runtime asset copy places the assets under the executable directory.
- [x] Default OpaquePbr material binds copied default albedo as sRGB.
- [x] PBR shaders sample and apply base-color texture.
- [x] Uploaded PBR meshes provide UV0 data.
- [x] Missing/failed default albedo load falls back with structured diagnostics.
- [x] New mesh default material matches reference white/rough/metallic intent.
- [x] Tests/build/deploy/dev-build policy completed or concrete blockers recorded.

## Verification Notes

- `python tools/project_board.py validate` passed.
- `cmake --preset qt-mingw-debug` passed.
- `cmake --build --preset qt-mingw-debug --parallel 20` passed.
- `cmake --build --preset qt-mingw-debug-tests --parallel 20` passed.
- Direct focused test executables passed: `crimson_material_system_tests.exe`, `crimson_render_upload_tests.exe`, `crimson_frame_snapshot_tests.exe`, `document_tests.exe`, `command_tests.exe`, and `tool_manager_tests.exe`.
- `python build_tools/validate_shader_manifest.py --manifest shaders/manifest.json --compiled build/qt-mingw-debug/compiled_shaders` passed.
- `cmake --build --preset qt-mingw-debug-deploy --parallel 20` passed.
- `python tools/dev_builds.py archive --preset qt-mingw-debug --json` created `0.1.1-dev.9` with `resources/modeling_assets/materials/default/default_albedo.png` and `default_roughness.png`.
- `ctest --test-dir build/qt-mingw-debug -R ... --output-on-failure` did not reach the filtered tests because GoogleTest discovery attempted to load unrelated `ui_model_tests.exe` without Qt runtime DLL path and exited with `0xc0000135`.
- After user reported `UnlitSurface` runtime program creation failure, `shaders/pbr/unlit_surface.fs.sc` was updated to match the reused PBR vertex shader's `v_texcoord0` varying and the app was rebuilt/redeployed.

## Review Rework: Crimson UV Orientation

User rejection: the default material texture appeared inverted in the viewport; the `QUADER` text in `default_albedo.png` was visibly reversed/upside down.

Reference mapping:

- `C:\Users\Drako\Desktop\quader-windows\quader-app\resources\materials\quader_mesh.mat:23` and `quader_mesh_lit.mat:35` flip V for Filament sampling with `vec2(getUV0().x, 1.0 - getUV0().y)`.
- `C:\Users\Drako\Desktop\quader-windows\quader-app\src\mesh\polygon\quader_poly_document_mesh_internal.cpp:327-356` provides the generated face UV basis mirrored in Quader's current viewport upload path.

Rework edits:

1. Preserve the reference generated UV basis in the mesh upload path.
2. Adapt the material sampling step for Crimson/bgfx PNG upload conventions by removing the extra shader-side V flip from OpaquePbr, AlphaCutoutPbr, TransparentPbr, and UnlitSurface.
3. Add a shader-source regression guard that the PBR shader family samples `v_texcoord0` directly through `u_pbrUvTransform0`, avoiding another hidden texture inversion.

Verification:

- `cmake --build --preset qt-mingw-debug --parallel 20` passed.
- `cmake --build --preset qt-mingw-debug-tests --parallel 20` passed.
- `ctest --preset qt-mingw-debug-runtime -R "tool_manager_tests|ui_viewport_tests|crimson_shader_manifest_tests|crimson_material_system_tests|crimson_render_upload_tests|command_tests|crimson_overlay_tests|document_tests" --output-on-failure` passed, including `ShaderManifest.PbrTextureSamplingPreservesCrimsonPngUvOrientation`.
- `cmake --build --preset qt-mingw-debug-deploy --parallel 20` passed.
- `python tools/dev_builds.py archive --preset qt-mingw-debug` created `0.1.1-dev.18`.
