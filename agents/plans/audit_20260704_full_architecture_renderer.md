# Frozen Renderer Architecture Audit: 20260704_full_architecture

Individual renderer audit report for the frozen baseline captured at `2026-07-04 18:44:19 +02:00`.

This report audits only renderer and renderer-integration scope. It is source material for the later master audit and is not implementation authority. No stale sections were detected while auditing; generated `build/` output was ignored.

## Scope And Baseline

- Repo root: `C:\Users\Drako\Desktop\_quader-qt-base`
- Renderer authority: `agents/architecture/renderer.md`
- App-wide authority used for renderer boundaries: `agents/architecture/architecture.md`
- UI authority used only for viewport-hosting and renderer-setting seams: `agents/architecture/ui.md`
- Current renderer implementation files: `src/BgfxWidget.h`, `src/BgfxWidget.cpp`, `shaders/*.sc`, `CMakeLists.txt`
- Current state: flat Qt Widgets prototype that initializes bgfx Vulkan from a `QWidget`, draws a rotating red cube and procedural grid, and reports basic renderer/status text.

## Executive Summary

The current renderer is a useful prototype, but it is not yet aligned with the target architecture. The primary drift is structural: Qt viewport code owns bgfx, GPU handles, shader loading, view submission, camera state, grid drawing, and sample scene data directly. That blocks the intended separation between `src/render` renderer-core APIs, `src/render_bgfx` backend ownership, and UI viewport hosting.

The next renderer work should first split the backend boundary and immutable frame snapshot path before adding PBR features. Adding materials, picking, overlays, glTF import, or debug views on top of the current `BgfxWidget` would deepen the drift.

## Findings

### R-001: bgfx Boundary Is Exposed Through The Qt Widget

- Severity: Critical
- Priority: P0
- Owner: Renderer
- Evidence:
  - `src/BgfxWidget.h:10` includes `<bgfx/bgfx.h>` from a Qt widget header.
  - `src/BgfxWidget.h:143-164` stores `bgfx::VertexLayout`, buffers, programs, and uniforms directly in the widget.
  - `src/BgfxWidget.cpp:24-25` includes `<bgfx/platform.h>` and `<bx/math.h>` in UI-facing code.
  - `CMakeLists.txt:34-49` builds one executable target that links Qt Widgets, `bgfx`, and `bx` together.
- Violated rule:
  - `renderer.md` requires bgfx hidden inside `render_bgfx/`, with no UI code including bgfx and renderer-core using project-owned handles.
  - `architecture.md` requires renderer boundaries to be adapters, not concrete backend leakage into higher layers.
- Practical risk:
  - UI code can accidentally depend on backend-specific handles and render states.
  - Future material inspectors, viewport controls, and importers have no clean project-owned handle API to use.
  - Backend fallback, backend capability checks, resource lifetime, and renderer tests cannot be isolated.
- Visible rendering failure it can cause:
  - Backend or resize bugs appear as Qt widget failures, black viewports, or invalid handle use with little diagnostic separation.
  - Adding Direct3D or Metal shader paths will require editing UI-facing code.
- Recommended correction:
  - Introduce `src/render/` public renderer-core types and `src/render_bgfx/` backend implementation before feature work.
  - Move all `<bgfx/*>` includes, bgfx handles, shader binary loading, render state construction, and `bgfx::submit` calls into `src/render_bgfx/`.
  - Expose only project-owned handles such as `RenderMeshHandle`, `RenderMaterialHandle`, `RenderProgramHandle`, `RenderFrameBufferHandle`, and `RenderEnvironmentHandle`.
  - Make the Qt viewport own only a renderer host/surface adapter and call a narrow API such as `renderer.render(frame_snapshot)`.

### R-002: There Is No Immutable Render Snapshot Or Render World Boundary

- Severity: Critical
- Priority: P0
- Owner: Renderer
- Evidence:
  - `src/BgfxWidget.cpp` defines prototype cube vertices and indices inside the widget implementation.
  - `src/BgfxWidget.cpp:1279-1285` computes the animated cube transform, light direction, and base color directly inside `renderFrame()`.
  - `src/BgfxWidget.cpp:1292-1342` builds camera matrices and submits draw calls directly per viewport.
  - `src/BgfxWidget.h:169` stores four cameras as widget state.
- Violated rule:
  - `renderer.md` requires `FrameSnapshot` containing `RenderCamera`, `RenderEnvironment`, `RenderObject`, `RenderLight`, overlays, debug view mode, and viewport settings.
  - `architecture.md` requires the renderer to consume snapshots/cache data and not become the source of editor truth.
- Practical risk:
  - Scene data, camera data, renderer resources, and UI interaction state are fused.
  - Future document selection, object transforms, materials, and lights have no explicit conversion step into render data.
  - Testing render behavior requires constructing Qt widgets instead of renderer snapshots.
- Visible rendering failure it can cause:
  - The viewport can only show the hardcoded cube/grid scene.
  - Future document changes can become stale or race with viewport interaction because there is no immutable frame input.
- Recommended correction:
  - Add `src/render/frame/frame_snapshot.hpp`, `src/render/frame/frame_builder.hpp`, and scene packet types under `src/render/scene/`.
  - Define `FrameSnapshot`, `RenderWorld`, `RenderObject`, `RenderCamera`, `RenderLight`, `RenderEnvironment`, and `ViewportSettings`.
  - Make the prototype cube enter the renderer through a temporary snapshot fixture rather than hardcoded widget draw state.
  - Keep document-to-render conversion outside `render_bgfx`; the backend should receive prepared draw packets only.

### R-003: No Render Graph Or Renderer Pass Boundary Exists

- Severity: Critical
- Priority: P0
- Owner: Renderer
- Evidence:
  - `src/BgfxWidget.cpp:1269-1271` sets one background view.
  - `src/BgfxWidget.cpp:1292-1342` loops over viewport panes, sets a bgfx view, submits grid, then submits cube in the same view.
  - There are no `src/render/graph/*` files or named render graph resources in the frozen source layout.
- Violated rule:
  - `renderer.md` requires a small render graph from the first renderer work, with named passes, named resources, declared read/write relationships, fixed dependency-validated order, resize-aware framebuffer recreation, and debug dumps.
  - V1 pass order requires separate passes such as `FrameSetupPass`, `ResourceUploadPass`, `DepthPrepass`, `PickingPass`, `OpaquePbrPass`, `ToneMapPass`, overlay passes, and `PresentPass`.
- Practical risk:
  - Pass order is implicit in `renderFrame()` call order.
  - Picking, HDR color, tone mapping, overlays, depth read-only modes, shadow maps, and future debug capture have no safe insertion point.
  - Resize behavior can only reset the swapchain, not graph-owned framebuffers.
- Visible rendering failure it can cause:
  - Grid, overlays, scene color, and picking IDs can be accidentally drawn into the same target.
  - Adding tone mapping or post effects later could affect overlays and editor visualization.
- Recommended correction:
  - Add `RenderGraph`, `RenderPass`, `RenderResourceDesc`, and `RenderResourceRegistry`.
  - Start V1 with explicit resources: `Backbuffer`, `SceneDepth`, `HdrSceneColor`, `ToneMappedColor`, and `PickingIdTarget` when picking lands.
  - Implement dependency validation and a debug dump before adding material or post-processing complexity.
  - Keep bgfx framebuffer creation in `BgfxFramebufferPool` under `src/render_bgfx/`.

### R-004: Linear HDR, sRGB, And Tone Mapping Invariants Are Missing

- Severity: High
- Priority: P0
- Owner: Renderer
- Evidence:
  - `src/BgfxWidget.cpp:1052` initializes the swapchain with `BGFX_RESET_VSYNC` only.
  - `src/BgfxWidget.cpp:1253-1256` resizes with `BGFX_RESET_VSYNC` only.
  - `shaders/fs_cube.sc:10-14` writes a simple diffuse color directly to `gl_FragColor`.
  - `shaders/fs_grid.sc:95` writes grid color directly to `gl_FragColor`.
  - There is no HDR render target, tone-map shader, final composite pass, or texture color-space declaration.
- Violated rule:
  - `renderer.md` requires linear HDR lighting first and display conversion last exactly once.
  - V1 requires `RGBA16F` HDR scene color, `ToneMapPass`, explicit texture color-space flags, and `BGFX_RESET_SRGB_BACKBUFFER` or equivalent manual final conversion.
- Practical risk:
  - Future PBR materials, IBL, texture imports, and overlays will be vulnerable to double gamma, missing gamma, and inconsistent brightness.
  - Renderer bugs will be debugged by changing fake light values instead of fixing exposure/color-space errors.
- Visible rendering failure it can cause:
  - glTF base colors, emissive maps, and UI-authored overlay colors can look too dark, washed out, or inconsistent across backends.
- Recommended correction:
  - Add color-space utilities and tests before material work.
  - Render lit scene into linear `RGBA16F` `HdrSceneColor`.
  - Add `ToneMapPass` writing linear SDR/tone-mapped output, then present with exactly one sRGB conversion.
  - Ensure overlay colors are stored as UI sRGB values, converted to linear SDR, and drawn after tone mapping.

### R-005: Grid/Overlay Rendering Is Not A Separate Domain

- Severity: High
- Priority: P1
- Owner: Renderer
- Evidence:
  - `src/BgfxWidget.cpp:1328` calls `renderGrid()` inside the same scene view before cube submission.
  - `src/BgfxWidget.cpp:1435-1440` submits the grid with ad hoc bgfx state in the same pass family as the cube.
  - There is no `OverlayCommand`, `OverlaySystem`, `OverlayUnlit` material, or separate overlay pass.
- Violated rule:
  - `renderer.md` states overlays, gizmos, grid, selection, debug lines, and editor visualization must never receive lighting, fog, bloom, SSAO, shadowing, or tone mapping.
  - `renderer.md` requires overlays after lit geometry and after post-processing, with depth-tested, X-ray, and always-on-top modes.
- Practical risk:
  - Grid behavior is coupled to scene pass ordering and backbuffer color handling.
  - Future selection outlines, gizmos, normals, debug lines, and measurement guides may be drawn through lit/material paths.
- Visible rendering failure it can cause:
  - Grid and selection colors can be tone-mapped, bloom-eligible, hidden incorrectly, or blended against the wrong color space.
- Recommended correction:
  - Move grid into `src/render/overlays/grid_renderer.*` as overlay commands, not as a world mesh in `BgfxWidget`.
  - Add `OverlayDepthTestedPass`, `OverlayXRayPass`, and `OverlayAlwaysOnTopPass` after `ToneMapPass`.
  - Use scene depth as read-only input for depth-tested overlays.
  - Add overlay exclusion tests proving overlays bypass lighting, fog, bloom, tone mapping, and picking unless explicitly requested.

### R-006: Backend Selection And Capability Diagnostics Are Hardcoded

- Severity: High
- Priority: P1
- Owner: Renderer
- Evidence:
  - `src/BgfxWidget.cpp:1035-1037` rejects non-Windows native handle wiring.
  - `src/BgfxWidget.cpp:1047` hardcodes `bgfx::RendererType::Vulkan`.
  - `src/BgfxWidget.cpp:1054-1056` reports only Vulkan initialization failure.
  - `README.md` documents a bgfx Vulkan viewport as the only runtime path.
- Violated rule:
  - `renderer.md` fixes backend priority as Vulkan on Windows/Linux, Metal on macOS, and Direct3D fallback on Windows.
  - Startup must query supported renderers, select the highest-priority available backend, check V1 capabilities, and emit clear diagnostics.
- Practical risk:
  - Windows machines without Vulkan cannot fall back to Direct3D.
  - macOS and Linux are architecturally excluded by the current widget code.
  - Missing required features are discovered later as black frames or invalid resources.
- Visible rendering failure it can cause:
  - Viewport fails to initialize even when a supported fallback backend exists.
  - Shader/backend mismatches produce missing shaders or blank rendering.
- Recommended correction:
  - Move backend selection into `BgfxBackend`.
  - Add `RendererBackendConfig`, backend priority policy, capability probes, and diagnostic records.
  - Keep platform surface extraction in a narrow viewport host adapter; keep bgfx initialization in `src/render_bgfx/`.
  - Add backend capability tests that can run without constructing the full UI.

### R-007: Shader Pipeline Is Not Manifest-Based Or Multi-Backend

- Severity: High
- Priority: P1
- Owner: Renderer
- Evidence:
  - `CMakeLists.txt:68` uses a single `compiled_shaders/spirv` output directory.
  - `CMakeLists.txt:74-80` invokes `shaderc` with `--profile spirv` only.
  - `CMakeLists.txt:95-98` compiles only `vs_cube`, `fs_cube`, `vs_grid`, and `fs_grid`.
  - `src/BgfxWidget.cpp:1443-1463` manually loads binaries from `shaders/spirv/`.
  - There is no shader manifest or manifest validation tool in the frozen source layout.
- Violated rule:
  - `renderer.md` forbids runtime shader compilation and requires build/tool pipeline shader manifests, backend targets, and validation.
  - Each new shader must be added to the shader manifest.
- Practical risk:
  - Non-Vulkan backends have no shader binaries.
  - Missing shader variants are found at runtime instead of during build/test.
  - Base shader definitions cannot reliably bind programs by stable IDs.
- Visible rendering failure it can cause:
  - Startup reports missing shaders or renders black after selecting a backend that does not use SPIR-V.
- Recommended correction:
  - Add `tools/shader_compile/` or `build_tools/compile_shaders.py` plus a shader manifest.
  - Compile all required V1 shaders for Vulkan/SPIR-V, Direct3D, and Metal targets as applicable.
  - Add `BgfxShaderLibrary` under `src/render_bgfx/` to load shader binaries by `RenderProgramHandle` and backend target.
  - Add shader manifest completeness tests.

### R-008: Material/Base Shader System And PBR Path Are Absent

- Severity: High
- Priority: P1
- Owner: Renderer
- Evidence:
  - `src/BgfxWidget.cpp:1112-1117` creates only cube/grid programs and a few uniforms.
  - `src/BgfxWidget.cpp:1284-1285` hardcodes light direction and base color.
  - `shaders/fs_cube.sc:10-14` implements simple ambient plus diffuse lighting, not metallic/roughness PBR.
  - No `src/render/material/*`, base shader registry, material instance, texture binding, or glTF material mapper exists in the frozen source layout.
- Violated rule:
  - `renderer.md` requires materials to be instances of base shaders and V1 to ship `OpaquePbr`, `AlphaCutoutPbr`, `TransparentPbr`, and `OverlayUnlit`.
  - Material parameters, texture slots, UI schemas, validation, and glTF mapping must come from base shader definitions.
- Practical risk:
  - The future material inspector has no renderer-owned schema to display.
  - glTF alpha modes, texture color-space flags, normals/tangents, double-sided state, and emissive strength cannot be mapped consistently.
- Visible rendering failure it can cause:
  - All content is forced through prototype diffuse shading and hardcoded red color.
  - Imported materials will look unrelated to glTF/Godot/Unity/Unreal expectations.
- Recommended correction:
  - Implement `MaterialSystem`, `BaseShaderRegistry`, `MaterialInstance`, parameter blocks, texture slots, and validation before importing real materials.
  - Add V1 base shaders in the order specified by `renderer.md`: `OpaquePbr`, `AlphaCutoutPbr`, `TransparentPbr`, then `OverlayUnlit`.
  - Add material schema tests, glTF alphaMode mapping tests, and texture color-space flag tests.

### R-009: Picking And Stable Render IDs Are Missing

- Severity: High
- Priority: P1
- Owner: Renderer
- Evidence:
  - No `PickingPass`, picking shader, ID target, or readback request API exists in the frozen source layout.
  - `shaders/` contains only cube and grid prototype shaders.
  - `src/BgfxWidget.cpp:1292-1342` submits visual geometry only.
- Violated rule:
  - `renderer.md` requires picking to use explicit ID targets/readbacks on request, not per-frame readback or color-managed buffers.
  - V1 pass order includes `PickingPass` before lit rendering.
- Practical risk:
  - Future selection may be implemented by reading color-managed visual pixels or by UI hit tests that do not match rendered objects.
  - Object/face/edge/vertex selection cannot be tested independently of the viewport widget.
- Visible rendering failure it can cause:
  - Click selection can pick the wrong object or fail after tone mapping, MSAA, overlays, or transparent surfaces are added.
- Recommended correction:
  - Add `src/render/overlays/picking.hpp` and a renderer API for request-scoped picking readbacks.
  - Add `PickingIdTarget` as integer or unorm non-color-managed render graph resource.
  - Add picking ID encode/decode tests and request-only readback tests.

### R-010: GPU Resource Lifetime Is Manual And Not Centralized

- Severity: Medium
- Priority: P2
- Owner: Renderer
- Evidence:
  - `src/BgfxWidget.cpp:1084-1148` creates buffers, programs, and uniforms directly in one method.
  - `src/BgfxWidget.cpp:1151-1234` manually destroys every handle in a long repeated sequence.
  - Resource validity and user-facing diagnostics are reduced to `m_resourcesReady`.
- Violated rule:
  - `renderer.md` requires bgfx resources to be owned by `render_bgfx`, with project-owned handles mapping to backend handles and clear create/update/destroy/recreate rules.
  - Architecture guidance prefers RAII and explicit ownership.
- Practical risk:
  - Partial failures can leave resources alive but unusable until shutdown.
  - Future passes will duplicate handle lifetime code and error paths.
  - Tests cannot exercise resource lifetime without a Qt widget.
- Visible rendering failure it can cause:
  - Missing shader or resize failures can leave a blank viewport with resources in an unclear state.
- Recommended correction:
  - Add RAII wrappers in `src/render_bgfx/bgfx_resources.*` and typed caches such as `RenderMeshCache`, `BgfxShaderLibrary`, and `BgfxFramebufferPool`.
  - Ensure resource creation returns structured diagnostics and invalid project handles instead of leaking bgfx details upward.
  - Add resource lifetime smoke tests where practical with a fake backend for renderer-core logic.

### R-011: Renderer Diagnostics, Debug Views, Tests, And Golden Images Are Absent

- Severity: Medium
- Priority: P2
- Owner: Renderer
- Evidence:
  - `src/BgfxWidget.h:28-32` exposes only string signals for renderer ready/stats/error.
  - `src/BgfxWidget.cpp:1350-1368` reports only viewport size, view count, and approximate FPS.
  - `CMakeLists.txt` has no renderer test target, no golden image target, and no render graph/shader/material validation tests.
  - The frozen source layout has no `tests/render_tests` or `tests/golden_images`.
- Violated rule:
  - `renderer.md` requires debug views, render graph dumps, shader manifest validation, color-space tests, material tests, light unit tests, overlay tests, picking tests, and golden image reference scenes.
- Practical risk:
  - Visual regressions will be diagnosed manually by looking at the viewport.
  - Agents can change renderer behavior without executable guardrails.
- Visible rendering failure it can cause:
  - Double-gamma, wrong normals, broken alpha, incorrect overlay pass ordering, and picking ID corruption may not be caught until much later.
- Recommended correction:
  - Add renderer-core unit tests as features land: graph dependency tests first, then shader manifest, color-space, material schema, light-unit, overlay exclusion, and picking tests.
  - Add `FrameStats` with pass/draw/resource counters and renderer diagnostics with structured severity.
  - Add golden image infrastructure only after deterministic render graph and color-space foundations exist.

### R-012: Renderer Performance Scaling Hooks Are Not Present

- Severity: Medium
- Priority: P2
- Owner: Renderer
- Evidence:
  - `src/BgfxWidget.cpp:727-729` renders on a fixed 16 ms Qt timer.
  - `src/BgfxWidget.cpp:1292-1342` duplicates grid and cube submission per viewport pane.
  - There is no culling, draw packet sorting, batching, instancing, dirty-resource upload path, or per-frame allocation accounting in renderer-owned code.
- Violated rule:
  - `renderer.md` defines V1 budgets for visible draw packets, instancing, picking readback on request only, and frame stats.
  - `architecture.md` expects render snapshots and caches rather than direct mutable drawing paths.
- Practical risk:
  - Scaling from a cube to many objects will require rewriting the frame path rather than extending it.
  - Quad viewport mode can multiply draw work without shared packet preparation.
- Visible rendering failure it can cause:
  - Future modeling scenes may stutter or waste GPU work, and there will be no frame stats to identify why.
- Recommended correction:
  - Add `FrameBuilder` to prepare draw packets once per frame snapshot and per-view camera.
  - Add frustum culling, stable sort keys, optional instancing groups, and `FrameStats`.
  - Keep high-frequency UI invalidation coalesced at the viewport host boundary.

## Cross-Domain Concerns

These concerns affect renderer integration but are primarily owned by UI/software architecture.

### CD-R-001: Qt Viewport Host Owns Camera Navigation And Renderer Settings

- Evidence:
  - `src/BgfxWidget.h:82-138` defines camera/navigation data types inside the widget.
  - `src/BgfxWidget.cpp:798-830` starts navigation directly from Qt mouse events.
  - `src/BgfxWidget.cpp:1300-1319` converts widget camera state into view/projection matrices in the render path.
- Renderer impact:
  - Renderer cannot receive neutral `RenderCamera` data from `FrameSnapshot`.
  - Tools cannot submit camera/tool overlays without coupling to widget internals.
- Recommended ownership:
  - UI architect should define `viewport_controller` and `viewport_render_host` ownership.
  - Renderer should define the renderer-facing `RenderCamera`, viewport size, debug mode, and overlay snapshot data accepted by `FrameSnapshot`.

### CD-R-002: Inspector UI Presents Prototype Renderer Truth Directly

- Evidence:
  - `src/MainWindow.cpp:117-125` uses `QTreeWidget` rows for `Selection` and `Renderer`, including hardcoded `bgfx Vulkan`.
  - `src/MainWindow.cpp:136-142` presents hardcoded cube/light/animation values.
  - `src/MainWindow.cpp:145-150` toggles cube animation through the viewport widget.
- Renderer impact:
  - UI-visible renderer settings are direct widget controls rather than `ViewportSettings`, material schemas, or diagnostics supplied through renderer-facing services.
- Recommended ownership:
  - UI architect should own panel/action/model design.
  - Renderer should expose structured diagnostics, debug view modes, viewport settings, and material UI schema data without exposing bgfx handles.

### CD-R-003: Build Target Boundaries Do Not Enforce Renderer Layering

- Evidence:
  - `CMakeLists.txt:34-49` creates a single executable target that compiles UI and renderer prototype files together and links Qt, bgfx, and bx in the same target.
- Renderer impact:
  - Include/link boundaries cannot prevent future UI, document, importer, or material code from depending on bgfx.
- Recommended ownership:
  - Software architect should decide project-wide target layout.
  - Renderer needs at least `modeling_render` and `modeling_render_bgfx` or equivalent boundaries, with `modeling_render_bgfx` the only target linking bgfx/bx.

## Suggested Renderer Implementation Batches

These batches are suggested input for the master audit. They should be accepted, reordered, or modified there before implementation.

### Batch 1: Renderer/Core And bgfx Backend Boundary

- Create `src/render/renderer.hpp`, `src/render/renderer_config.hpp`, `src/render/renderer_types.hpp`.
- Create `src/render_bgfx/bgfx_backend.*`, `bgfx_handles.hpp`, `bgfx_resources.*`, `bgfx_shader_library.*`, `bgfx_framebuffer_pool.*`.
- Move bgfx initialization, platform data, reset, shader loading, resource handles, and submit calls out of `BgfxWidget`.
- Add project-owned handle types and structured renderer diagnostics.
- Add boundary tests or architecture checks that reject `<bgfx/*>` includes outside `src/render_bgfx/`.

### Batch 2: Frame Snapshot, Render World, And Prototype Scene Fixture

- Add `FrameSnapshot`, `FrameBuilder`, `RenderWorld`, `RenderObject`, `RenderCamera`, `RenderLight`, `RenderEnvironment`, `ViewportSettings`, and `FrameStats`.
- Convert the prototype cube/grid scene into snapshot data consumed by `Renderer::render(const FrameSnapshot&)`.
- Keep document/tool integration outside this batch unless the master audit explicitly includes it.
- Add tests for snapshot immutability and draw packet construction.

### Batch 3: Minimal Render Graph And Frame Resources

- Add `RenderGraph`, `RenderPass`, `RenderResourceDesc`, and `RenderResourceRegistry`.
- Add initial passes: `FrameSetupPass`, `ResourceUploadPass`, `DepthPrepass` placeholder if needed, `OpaquePrototypePass`, `ToneMapPass` placeholder or explicit pass, `PresentPass`.
- Add named resources: `Backbuffer`, `SceneDepth`, `HdrSceneColor`, `ToneMappedColor`.
- Add dependency validation, resize-aware framebuffer recreation, and graph debug dump.
- Add render graph dependency and resize tests.

### Batch 4: Color-Space Foundation, HDR Target, Tone Mapping, Shader Manifest

- Add shared color-space conversion helpers and tests.
- Add `RGBA16F` HDR scene color and final tone mapping/display conversion.
- Enable `BGFX_RESET_SRGB_BACKBUFFER` where valid or implement manual final conversion exactly once.
- Introduce shader manifest and compile supported backend targets.
- Add shader manifest completeness, sRGB/linear texture flag, and tone mapper smoke tests.

### Batch 5: Material System And V1 PBR Base Shaders

- Add `MaterialSystem`, `BaseShaderRegistry`, `MaterialInstance`, `MaterialParameterBlock`, texture binding definitions, validation, and UI schema data.
- Implement `OpaquePbr` first, then `AlphaCutoutPbr`, `TransparentPbr`, and `OverlayUnlit`.
- Add default textures and texture color-space declarations.
- Add material schema validation tests and glTF alphaMode mapping tests when the asset mapper lands.

### Batch 6: Overlays, Grid, Selection Foundations, And Picking

- Move grid into `OverlaySystem` using `OverlayCommand`.
- Add `OverlayDepthTestedPass`, `OverlayXRayPass`, and `OverlayAlwaysOnTopPass` after `ToneMapPass`.
- Add picking ID target, picking shaders, request-scoped readback API, and ID encode/decode tests.
- Add overlay exclusion tests proving overlays do not enter lit, post, tone-map, fog, bloom, shadow, or picking paths unless explicitly requested by `PickingPass`.

### Batch 7: Renderer Diagnostics, Debug Views, Performance Stats, Golden Readiness

- Add debug views for final color, base color, normals, roughness, metallic, AO, emissive, depth, shadow map, object ID, UVs, and instance ID as features exist.
- Add per-pass and per-frame stats: draw counts, culled counts, resource upload counts, readback requests, framebuffer recreations.
- Add golden image harness only after render graph, color-space, and material foundations are deterministic enough to produce stable images.

## Minimum Renderer Test Set To Add Over Time

- bgfx include boundary scan.
- Backend capability selection tests.
- Shader manifest completeness tests.
- Render graph dependency/order/resize tests.
- Color-space and sRGB/linear texture flag tests.
- Tone mapper smoke tests.
- Base shader schema validation tests.
- glTF alphaMode material mapping tests.
- Light unit conversion tests.
- Overlay exclusion tests.
- Picking ID encode/decode and request-only readback tests.
- Frustum culling and instancing batch key tests.
- Golden image reference scenes once deterministic rendering exists.

## Final Note For Main Agent

After the master audit authorizes implementation and the renderer changes are implemented, return to the renderer architect with the changed files, build/test results, known deviations, and the relevant plan or audit batch. The renderer architect should review the implementation against `renderer.md`, `architecture.md`, this report if adopted by the master audit, and any implementation plan. Approval should be explicit before deleting any renderer plan file.
