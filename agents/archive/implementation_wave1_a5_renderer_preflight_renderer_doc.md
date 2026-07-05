# Implementation Plan: Wave 1 A5 Renderer Preflight

Status: renderer architecture plan  
Scope: project-board tasks #4 and #5, with future #9 constraints preflighted  
Owner: renderer architect  
Repo: `C:\Users\Drako\Desktop\_quader-qt-base`  
Created: 2026-07-04

## 1. Purpose

This plan is the renderer-side implementation authority for Wave-1 assignment A5. It covers:

- Task #4: extract the viewport and Crimson GPU/runtime boundary while preserving the current prototype viewport.
- Task #5: introduce immutable renderer snapshots and a minimal render graph.
- Task #9: constrain the future Crimson V1 correctness sequence so #4/#5 do not create APIs that must be undone for color, material, overlay, picking, or diagnostics work.

This plan does not authorize PBR, picking-driven selection, glTF import, material inspector UI, IBL, shadows, golden images, or production mesh/document rendering. Those belong to later board tasks and the #9 sequence.

## 2. Required Inputs Read

The plan is based on:

- `AGENTS.md`
- `agents/workflow.md`
- `agents/task_board.md`
- `agents/tests-policy.md`
- `project_board.md`
- `agents/plans/audit_20260704_full_architecture_master.md`
- `agents/architecture/architecture.md`
- `agents/architecture/renderer.md`
- `agents/architecture/ui.md`
- Current prototype files:
  - `CMakeLists.txt`
  - `CMakePresets.json`
  - `README.md`
  - `src/main.cpp`
  - `src/MainWindow.*`
  - `src/BgfxWidget.*`
  - `shaders/*.sc`

## 3. Current Prototype Findings

`BgfxWidget` is the extraction source. It currently owns all of these responsibilities in one Qt widget:

- Qt native child surface setup: `WA_NativeWindow`, `WA_PaintOnScreen`, `winId()`.
- Viewport event handling: mouse, wheel, key, focus, split handles.
- Camera/navigation state: perspective and orthographic cameras, quad view selection, fly/orbit/pan.
- View layout state: single/quad layout, split fractions, splitter handles.
- Graphics-runtime ownership: bgfx init, shutdown, reset, resource creation/destruction, shader loading, submission, frame advance.
- Runtime handles in a public UI header: `bgfx::VertexLayout`, `bgfx::VertexBufferHandle`, `bgfx::ProgramHandle`, `bgfx::UniformHandle`.
- Prototype render data: hardcoded cube vertices/indices, grid quad, simple light and base color.
- Prototype pass order: background clear, per-view grid draw, per-view cube draw, `bgfx::frame()`.
- String diagnostics: `rendererReady(QString)`, `renderStatsChanged(QString)`, `renderError(QString)`.

The first boundary violation to remove is `#include <bgfx/bgfx.h>` from `src/BgfxWidget.h`. The final state after #4 must have no public UI header exposing bgfx or any future graphics-runtime type.

## 4. Blocking Conditions

Hard blockers before implementation:

- Task #2 must provide real module targets and a documented CTest path. A5 relies on target boundaries to prove only Crimson links the graphics runtime.
- The UI A4 viewport preflight must be reconciled at the assumption level. Direct agent-to-agent communication is not required, but the implementation must not contradict UI ownership of widgets, Qt event translation, viewport layout state, or camera interaction state.
- No implementation may proceed directly from this plan if the master audit is superseded or if the source tree has materially changed around `BgfxWidget`, `MainWindow`, CMake targets, or shader layout.

Soft coordination dependency:

- If task #3 has already introduced `app` composition and `UiContext`, use it for renderer service construction. If #3 is not implemented yet, keep a temporary `MainWindow` wiring path, but do not let that temporary path leak graphics-runtime headers back into UI public headers.

Task #9 is blocked until #4 and #5 are complete and approved.

## 5. Non-Goals

Do not implement in #4/#5:

- PBR base shader system.
- Material inspector schema.
- Runtime shader compilation.
- Shader manifest or multi-target shader output layout, except preserving current offline shader copy.
- HDR scene target, exposure, tone mapping, or color-space conversion.
- Picking target or readback.
- glTF, texture import, IBL, shadows, transparency, bloom, fog, SSAO, or golden images.
- Rendering directly from the future half-edge mesh or any document truth.
- Qt debug panels or diagnostics UI beyond consuming structured renderer status.

## 6. Module Families Involved

#4/#5 may touch:

- `src/crimson/`
- `src/crimson/gpu/`
- `src/crimson/frame/`
- `src/crimson/scene/`
- `src/crimson/graph/`
- `src/ui/viewport/` only through UI-owned widget/controller handoff files
- `src/app/` or a temporary app wiring location for service ownership
- `shaders/` only to preserve the existing prototype shader build
- `tests/render_tests/` or the test location established by task #2
- CMake target definitions created by task #2

#4/#5 must not touch:

- `assets/gltf`
- `assets/textures`
- `assets/environment`
- `tools/ibl_bake`
- PBR shader directories not yet introduced
- Document, command, tool, mesh topology code, except through neutral future-facing IDs if task #2/#3 already created foundation types

## 7. Ownership And Dependency Direction

Renderer ownership:

- `crimson::Renderer` owns renderer lifetime, frame submission entry point, renderer status, and public Crimson APIs.
- `crimson::gpu::GpuDevice` owns graphics-runtime initialization, selected backend, native platform data conversion, reset, frame begin/end, and submission.
- `crimson::gpu::*` owns all bgfx includes and bgfx handles during the current runtime implementation.
- `crimson::GpuResourceRegistry` or equivalent internal tables map project-owned handles to runtime handles.
- `crimson::RenderGraph` owns pass/resource declarations, dependency validation, resize generation, and debug dumps.
- `crimson::FrameSnapshot` owns immutable per-frame render input.

UI ownership:

- UI owns `ViewportWidget`, Qt events, widget lifetime, splitter handles, cursor behavior, focus, and native Qt surface acquisition.
- UI owns `ViewportController`, `ViewportLayoutState`, `ViewportCameraController`, and interactive navigation state.
- UI may include Crimson public headers that expose only project-owned handles, value data, diagnostics, and `NativeSurfaceDescriptor`.
- UI must not include `src/crimson/gpu/*`, `<bgfx/...>`, `<bx/...>`, or future graphics-runtime headers.

App ownership:

- `app` or the temporary composition location owns construction order and service lifetime.
- Renderer must outlive any UI object that calls it, or UI must hold only a checked non-owning pointer/reference cleared on shutdown.

## 8. File Layout To Implement

Renderer public/core files:

```text
src/crimson/renderer.hpp
src/crimson/renderer.cpp
src/crimson/renderer_config.hpp
src/crimson/renderer_types.hpp
src/crimson/renderer_diagnostics.hpp
src/crimson/native_surface.hpp
```

Frame and scene files:

```text
src/crimson/frame/frame_snapshot.hpp
src/crimson/frame/frame_builder.hpp
src/crimson/frame/frame_builder.cpp
src/crimson/frame/frame_stats.hpp
src/crimson/scene/render_world.hpp
src/crimson/scene/render_world.cpp
src/crimson/scene/render_object.hpp
src/crimson/scene/render_camera.hpp
src/crimson/scene/render_light.hpp
src/crimson/scene/render_environment.hpp
src/crimson/scene/viewport_settings.hpp
```

Render graph files:

```text
src/crimson/graph/render_graph.hpp
src/crimson/graph/render_graph.cpp
src/crimson/graph/render_pass.hpp
src/crimson/graph/render_resource_desc.hpp
src/crimson/graph/render_resource_registry.hpp
src/crimson/graph/render_graph_debug_dump.hpp
```

GPU/runtime private files:

```text
src/crimson/gpu/gpu_device.hpp
src/crimson/gpu/gpu_device.cpp
src/crimson/gpu/gpu_handles.hpp
src/crimson/gpu/gpu_resources.hpp
src/crimson/gpu/gpu_resources.cpp
src/crimson/gpu/shader_library.hpp
src/crimson/gpu/shader_library.cpp
src/crimson/gpu/framebuffer_pool.hpp
src/crimson/gpu/framebuffer_pool.cpp
src/crimson/gpu/gpu_submit.hpp
src/crimson/gpu/gpu_submit.cpp
```

Temporary prototype adapter:

```text
src/app/prototype/prototype_render_scene.hpp
src/app/prototype/prototype_render_scene.cpp
```

If `src/app/` has not been introduced by task #3, put the temporary adapter in the app/executable target under a clearly named compatibility path, not in `src/crimson/`. The renderer must consume the resulting handles and snapshots, not own the prototype scene truth.

UI handoff files expected from the UI side:

```text
src/ui/viewport/viewport_widget.hpp
src/ui/viewport/viewport_widget.cpp
src/ui/viewport/viewport_controller.hpp
src/ui/viewport/viewport_controller.cpp
src/ui/viewport/viewport_camera_controller.hpp
src/ui/viewport/viewport_camera_controller.cpp
src/ui/viewport/viewport_layout_state.hpp
src/ui/viewport/viewport_render_host.hpp
src/ui/viewport/viewport_render_host.cpp
```

The renderer implementation must not create Qt widgets or redesign these files. It only specifies the renderer-facing contracts they call.

## 9. Public Renderer API

Use namespace `crimson`.

`renderer_types.hpp`:

```cpp
struct RenderMeshHandle {
    uint32_t index = 0;
    uint32_t generation = 0;
};

struct RenderTextureHandle {
    uint32_t index = 0;
    uint32_t generation = 0;
};

struct RenderMaterialHandle {
    uint32_t index = 0;
    uint32_t generation = 0;
};

struct RenderProgramHandle {
    uint32_t index = 0;
    uint32_t generation = 0;
};

struct RenderFrameBufferHandle {
    uint32_t index = 0;
    uint32_t generation = 0;
};

struct RenderEnvironmentHandle {
    uint32_t index = 0;
    uint32_t generation = 0;
};

struct ViewportExtent {
    uint32_t width_px = 1;
    uint32_t height_px = 1;
    float device_pixel_ratio = 1.0f;
};
```

Handle rules:

- `index == 0 && generation == 0` is invalid for every handle type.
- Handles are value types. They never expose runtime handles.
- A destroyed resource invalidates only matching generation.
- Public handles may be stored by UI, app services, future document render caches, and snapshots.
- Only `src/crimson/gpu/` maps them to runtime handles.

`native_surface.hpp`:

```cpp
enum class NativeSurfacePlatform {
    Windows,
    LinuxX11,
    LinuxWayland,
    MacOS,
};

struct NativeSurfaceDescriptor {
    NativeSurfacePlatform platform;
    void* native_window_handle = nullptr;
    void* native_display_handle = nullptr;
    void* native_layer_handle = nullptr;
    ViewportExtent extent;
};
```

This type is neutral host data. UI may fill it from Qt. `GpuDevice` converts it to bgfx platform data internally. UI must not create `bgfx::PlatformData`.

`renderer_config.hpp`:

```cpp
enum class GraphicsBackendPreference {
    Auto,
    Vulkan,
    Metal,
    Direct3D12,
    Direct3D11,
};

struct RendererConfig {
    GraphicsBackendPreference backend_preference = GraphicsBackendPreference::Auto;
    std::filesystem::path shader_root;
    bool vsync = true;
    bool enable_debug_text = true;
};
```

`renderer.hpp`:

```cpp
class Renderer final {
public:
    static Result<std::unique_ptr<Renderer>, RendererDiagnostic> create(
        const RendererConfig& config,
        const NativeSurfaceDescriptor& surface);

    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    Result<void, RendererDiagnostic> resize(const ViewportExtent& extent);
    Result<FrameStats, RendererDiagnostic> render(const FrameSnapshot& snapshot);

    Result<RenderMeshHandle, RendererDiagnostic> create_static_mesh(const RenderMeshDesc& desc);
    void destroy_mesh(RenderMeshHandle handle);

    Result<RenderProgramHandle, RendererDiagnostic> load_program(ShaderProgramId id);

    RendererStatus status() const;
    std::string debug_dump_last_frame_graph() const;
};
```

If task #2 has not yet introduced `Result`, use the project foundation result type from #2. Do not invent a second result system in Crimson.

## 10. Diagnostics API

`renderer_diagnostics.hpp`:

```cpp
enum class RendererDiagnosticSeverity {
    Info,
    Warning,
    Error,
    Fatal,
};

enum class RendererDiagnosticCode {
    SurfaceUnavailable,
    BackendUnsupported,
    CapabilityMissing,
    RuntimeInitializationFailed,
    ShaderFileMissing,
    ShaderProgramCreationFailed,
    ResourceCreationFailed,
    FrameGraphValidationFailed,
    InvalidFrameSnapshot,
    ResizeFailed,
};

struct RendererDiagnostic {
    RendererDiagnosticSeverity severity;
    RendererDiagnosticCode code;
    std::string message;
    std::string detail;
};

struct RendererStatus {
    std::string backend_name;
    bool initialized = false;
    std::vector<RendererDiagnostic> diagnostics;
};
```

Diagnostics rules:

- Crimson core uses `std::string`, not `QString`.
- UI formats diagnostics for labels, status bars, notification services, or future panels.
- Do not lower renderer warnings to avoid noisy status output. Fix or route them.
- Device selection and shader/resource failures must include actionable file/backend/resource names.

## 11. FrameSnapshot And RenderWorld Data Shape

`render_camera.hpp`:

```cpp
enum class CameraProjection {
    Perspective,
    Orthographic,
};

struct RenderCamera {
    Mat4 view_from_world;
    Mat4 clip_from_view;
    Vec3 position_world;
    float near_plane_m = 0.05f;
    float far_plane_m = 1000.0f;
    float vertical_fov_degrees = 60.0f;
    float orthographic_height_m = 24.0f;
    float exposure_ev100 = 12.0f;
    float exposure_compensation_ev = 0.0f;
    CameraProjection projection = CameraProjection::Perspective;
};
```

`frame_snapshot.hpp`:

```cpp
struct RenderView {
    uint32_t view_index = 0;
    ViewportRect rect_px;
    RenderCamera camera;
    std::string debug_name;
};

struct FrameSnapshot {
    uint64_t frame_index = 0;
    ViewportExtent target_extent;
    std::span<const RenderView> views;
    std::span<const RenderObject> objects;
    std::span<const RenderLight> lights;
    RenderEnvironment environment;
    std::span<const OverlayCommand> overlays;
    ViewportSettings viewport_settings;
    DebugViewMode debug_view = DebugViewMode::FinalColor;
};
```

Snapshot rules:

- `FrameSnapshot` is immutable during `Renderer::render`.
- The snapshot must contain one or four `RenderView` entries for the current prototype.
- The renderer never reads `QWidget`, `QMouseEvent`, `QTimer`, document objects, mesh records, or future half-edge mesh data while rendering.
- Spans point to storage owned by `FrameBuilder` or a frame arena whose lifetime exceeds `Renderer::render`.
- Later document integration replaces the prototype scene provider with a document-to-render-world builder without changing `Renderer::render(snapshot)`.

`render_world.hpp`:

```cpp
class RenderWorld final {
public:
    RenderObjectId add_object(RenderObject object);
    void update_object(RenderObjectId id, RenderObject object);
    void remove_object(RenderObjectId id);
    std::span<const RenderObject> objects() const;
};
```

`RenderWorld` is renderer-facing prepared state, not document truth. It stores render handles, transforms, bounds, object IDs, and visibility flags only.

## 12. Prototype Scene Preservation

The temporary prototype scene provider must:

- Own the rotating cube animation state outside Crimson.
- Upload the current hardcoded cube through `Renderer::create_static_mesh`.
- Use project-owned `RenderMeshHandle` and internal built-in program/material placeholders.
- Produce one `RenderObject` for the cube with transform, world bounds, object ID, mesh handle, and material/program placeholder.
- Produce grid overlay commands or grid settings for each `RenderView`.
- Convert UI camera/controller state into `RenderCamera` values.

The temporary provider must not:

- Store bgfx handles.
- Call `bgfx::*`.
- Include `<bgfx/...>` or `<bx/...>`.
- Pretend to be a real document or material system.
- Introduce document mutation, selection truth, or topology state.

## 13. Minimal RenderGraph For Task #5

Implement a small graph that is useful immediately and compatible with #9.

Core types:

```cpp
enum class RenderResourceFormat {
    BackbufferColor,
    BackbufferDepth,
    Rgba8Unorm,
    Rgba16Float,
    D24S8,
    D32Float,
    R32Uint,
};

enum class RenderResourceAccess {
    Read,
    Write,
    ReadWrite,
};

struct RenderResourceDesc {
    std::string name;
    RenderResourceFormat format;
    ViewportExtent extent;
    bool external = false;
};

struct RenderPassDesc {
    std::string name;
    std::vector<ResourceUse> resources;
};
```

Required graph behavior:

- Passes are named.
- Resources are named.
- Passes declare reads and writes.
- Manual pass order is validated against dependencies.
- A pass that reads an undeclared or unwritten resource fails validation.
- A pass that writes an already-written resource without read/write intent fails validation unless explicitly allowed.
- Resize increments a graph/resource generation and recreates resize-dependent GPU resources.
- `debug_dump_last_frame_graph()` returns pass/resource order in stable text.

Minimal #5 pass order:

```text
1. FrameSetupPass
2. BackbufferClearPass
3. PrototypeOpaquePass
4. OverlayGridPass
5. PresentPass
```

Minimal #5 resources:

```text
BackbufferColor          external, per surface
BackbufferDepth          external, per surface
PrototypeCubeMesh        persistent GPU resource, not graph transient
PrototypeGridMesh        persistent GPU resource, not graph transient
```

Pass rules:

- `FrameSetupPass` is CPU-only. It validates snapshot views, object handles, and viewport extents.
- `BackbufferClearPass` writes `BackbufferColor` and `BackbufferDepth`.
- `PrototypeOpaquePass` writes color/depth using prototype cube packets only.
- `OverlayGridPass` reads depth, writes color with blending, and is unlit.
- `PresentPass` owns final frame submission/advance.

Even before #9 tone mapping exists, grid must be modeled as an overlay pass, not as lit scene geometry. This keeps the #9 overlay exclusion rule intact.

## 14. GPU Boundary Details

Only files under `src/crimson/gpu/` may include:

```cpp
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <bx/math.h>
```

`GpuDevice` responsibilities:

- Convert `NativeSurfaceDescriptor` to runtime platform data.
- Select backend. For #4, preserve current Vulkan behavior on Windows unless the current runtime exposes safe fallback detection. Do not hardcode Vulkan into public APIs.
- Initialize/shutdown runtime with RAII.
- Reset swapchain/backbuffer on resize.
- Own frame begin/end.
- Own runtime capability query.
- Own runtime shader creation and program creation.
- Own runtime buffer creation/destruction.
- Set debug names where supported.
- Convert `RenderView` and graph pass packets into runtime view/submission calls.

Internal handle mapping:

```text
RenderMeshHandle -> GpuMeshResource
RenderProgramHandle -> runtime shader program
RenderFrameBufferHandle -> runtime framebuffer or external backbuffer
```

Lifetime rules:

- `Renderer` destructor destroys GPU resources before runtime shutdown.
- Resource destruction is idempotent for invalid handles.
- A resource table generation increments on destroy/reuse.
- `resize()` does not invalidate mesh/program handles.
- Resize-dependent graph resources and framebuffers are recreated when extent changes.
- `FrameSnapshot` never owns GPU resources.

## 15. Shader Handling For #4/#5

#4/#5 preserve the existing offline shader build:

```text
shaders/vs_cube.sc
shaders/fs_cube.sc
shaders/vs_grid.sc
shaders/fs_grid.sc
shaders/varying.def.sc
```

Renderer-side changes:

- Move shader file loading out of `BgfxWidget` into `src/crimson/gpu/shader_library.*`.
- Public API refers to `ShaderProgramId`, not file names or runtime handles.
- Internal #5 program IDs:
  - `ShaderProgramId::PrototypeLitCube`
  - `ShaderProgramId::PrototypeGridOverlay`
- Shader loading returns `RenderProgramHandle`.
- Missing shader files produce `RendererDiagnosticCode::ShaderFileMissing`.
- Program creation failure produces `RendererDiagnosticCode::ShaderProgramCreationFailed`.

Do not implement the #9 shader manifest in #4/#5. But do not design APIs around hardcoded `/shaders/spirv/` being permanent. `RendererConfig::shader_root` must make the future manifest and multi-target layout possible.

## 16. Material And Asset Pipeline Constraints

#4/#5 may use internal prototype material constants:

```text
cube base color: current red prototype value
light direction: current simple directional fake light
grid colors: current grid constants
```

They must not introduce a public material system. The #9 sequence owns:

1. Backend selection and capability diagnostics.
2. Shader manifest and multi-target output layout.
3. Linear HDR scene target and final display conversion.
4. `OpaquePbr`, then `AlphaCutoutPbr`, `TransparentPbr`, and `OverlayUnlit`.
5. Overlay system and grid commands.
6. Picking target and request-scoped readback.
7. Structured diagnostics, resource RAII/caches, and frame stats.

Any temporary #5 material/program field must be named as a prototype or internal draw packet field and must be easy to replace with `RenderMaterialHandle` plus `BaseShaderId` in #9.

No glTF, texture, IBL, or asset-pipeline code is part of #4/#5.

## 17. Viewport Integration Handoff Assumptions

Coordinate with the UI plan through these assumptions:

- UI `ViewportWidget` replaces `BgfxWidget` as the QWidget host.
- UI `ViewportWidget` keeps Qt attributes needed for native rendering and exposes a `NativeSurfaceDescriptor` through a narrow adapter.
- UI `ViewportController` handles mouse, wheel, key, focus, active pane, and navigation mode.
- UI `ViewportLayoutState` owns single/quad mode and split fractions.
- UI `ViewportCameraController` owns interactive camera state and outputs immutable `RenderCamera` values.
- UI `ViewportRenderHost` is a small adapter that calls `crimson::Renderer::resize` and `crimson::Renderer::render`.
- UI formats `RendererStatus` and `FrameStats` into labels/status messages. Crimson does not emit Qt signals.
- If UI keeps Qt signals for compatibility, the signal payloads are derived from typed Crimson diagnostics/stats at the UI boundary.
- The renderer never calls QWidget APIs and never receives Qt events.

The UI side should keep current visible behavior:

- Single viewport by default.
- F1/four viewport action still toggles quad view if UI plan retains the shortcut.
- Orbit, pan, fly, wheel zoom, and splitter dragging preserve current user behavior.
- Prototype cube/grid still render.

## 18. FrameStats

`frame_stats.hpp`:

```cpp
struct FrameStats {
    uint64_t frame_index = 0;
    uint32_t width_px = 0;
    uint32_t height_px = 0;
    uint32_t view_count = 0;
    uint32_t graph_pass_count = 0;
    uint32_t draw_call_count = 0;
    uint32_t visible_object_count = 0;
    uint32_t diagnostic_count = 0;
};
```

#4/#5 should not add fragile timing tests. If frame time is measured for UI display, keep it advisory and do not make it a correctness test.

## 19. Renderer Worker Scopes

Recommended worker split after #2 and UI assumptions are ready:

Renderer worker R1: Crimson public types and diagnostics

- Add `renderer_types.hpp`, `renderer_config.hpp`, `renderer_diagnostics.hpp`, `native_surface.hpp`.
- Add project-owned handles and pure utility tests.
- No runtime includes outside `src/crimson/gpu/`.

Renderer worker R2: GPU runtime shell

- Add `GpuDevice`, `gpu_handles`, `gpu_resources`, `shader_library`.
- Move bgfx init/reset/shutdown/shader loading/resource creation out of UI.
- Preserve current Vulkan prototype behavior and structured diagnostics.
- Implement backend selection shape without promising the full #9 fallback matrix yet.

Renderer worker R3: Renderer facade and prototype mesh/program path

- Add `crimson::Renderer`.
- Add static mesh creation and program loading through project-owned handles.
- Move cube/grid GPU resources behind Crimson.
- Preserve prototype draw output.

Renderer worker R4: FrameSnapshot and RenderWorld

- Add render scene value types, `RenderWorld`, `FrameBuilder`, `FrameSnapshot`, and `FrameStats`.
- Add temporary prototype scene provider outside Crimson.
- Convert current per-frame cube/grid input into immutable snapshots.

Renderer worker R5: minimal RenderGraph

- Add graph resource/pass declarations, dependency validation, resize generation, debug dump.
- Execute minimal #5 pass order through `Renderer::render`.
- Add graph tests.

Renderer worker R6: architecture guardrail and integration verification

- Add or extend the architecture include/link checks created by #2 so bgfx includes are rejected outside `src/crimson/gpu/`.
- Verify public UI headers do not expose runtime types.
- Verify only the Crimson target links bgfx/bx.

## 20. Test Strategy

Follow `agents/tests-policy.md`: tests must protect behavior or architecture invariants that would affect real editor behavior.

Required #4/#5 automated tests once task #2 test harness exists:

- `renderer_handle_tests.cpp`
  - Invalid handles compare equal only to invalid handles of the same type.
  - Destroy/reuse generation changes prevent stale handle lookup.
- `renderer_backend_selection_tests.cpp`
  - Pure backend selection policy chooses Vulkan before Direct3D on Windows when all are available.
  - macOS requires Metal.
  - Linux requires Vulkan.
  - Missing required backend yields a structured diagnostic.
- `renderer_diagnostics_tests.cpp`
  - Missing shader file is reported as `ShaderFileMissing` with the requested program ID/path.
  - Invalid surface descriptor is reported as `SurfaceUnavailable`.
- `frame_snapshot_tests.cpp`
  - Builder rejects zero-sized views.
  - Builder freezes view/object arrays for the duration of render input.
  - Quad layout snapshot contains four distinct `RenderView` rects and cameras when requested.
- `render_graph_tests.cpp`
  - Valid minimal pass order validates.
  - Reading an unwritten resource fails validation.
  - Missing resource names fail validation.
  - Resize changes resource generation for resize-dependent resources.
  - Debug dump has stable pass/resource names.
- Architecture guardrail test or check target
  - Reject `<bgfx/` and `<bx/` includes outside `src/crimson/gpu/`.
  - Reject UI public headers including `src/crimson/gpu/*`.
  - Verify only the Crimson target links bgfx/bx.

Manual verification for GPU behavior until render golden tests exist:

- Configure and build.
- Run the app.
- Confirm renderer initializes.
- Confirm resize still renders.
- Confirm single/quad view behavior remains.
- Confirm cube and grid are visible.
- Confirm status UI receives typed renderer stats/diagnostics formatted by UI.

Do not add golden image tests in #4/#5. Golden images become useful after #9 establishes deterministic color, graph, material, and overlay foundations.

## 21. Verification Commands

Always run:

```powershell
python tools/project_board.py validate
cmake --preset qt-mingw-debug
cmake --build --preset qt-mingw-debug
```

After task #2 adds and documents a CTest preset, also run the documented test preset. If task #2 names it `qt-mingw-debug`, the command should be:

```powershell
ctest --preset qt-mingw-debug
```

For local runtime verification:

```powershell
cmake --build --preset qt-mingw-debug --target deploy
.\build\qt-mingw-debug\QuaderQtBgfx.exe
```

If no test preset exists at implementation time, that is a task #2 blocker and must be reported rather than silently replacing automated tests with manual checks.

## 22. Acceptance Criteria For #4

#4 is renderer-acceptable when:

- `src/BgfxWidget.h` no longer includes `<bgfx/bgfx.h>` or stores bgfx handles.
- No public UI header exposes any graphics-runtime type.
- All bgfx/bx includes are under `src/crimson/gpu/`.
- Only the Crimson target links bgfx/bx.
- `GpuDevice` owns runtime init/shutdown/reset/platform-data conversion.
- `Renderer` exposes project-owned handles, typed diagnostics, and a narrow `render(snapshot)` path.
- UI passes a neutral `NativeSurfaceDescriptor`, not runtime platform data.
- Prototype viewport still renders through Crimson.
- Renderer diagnostics are structured before UI formatting.

## 23. Acceptance Criteria For #5

#5 is renderer-acceptable when:

- Prototype rendering uses immutable `FrameSnapshot` input.
- `RenderWorld` or the temporary prototype scene provider owns prepared render data, not `ViewportWidget`.
- `Renderer::render(const FrameSnapshot&)` is the frame entry point.
- The minimal render graph has named passes, named resources, declared reads/writes, dependency validation, resize-aware resource generation, and debug dumps.
- Minimal pass order is explicit: `FrameSetupPass`, `BackbufferClearPass`, `PrototypeOpaquePass`, `OverlayGridPass`, `PresentPass`.
- Grid is an overlay pass and not part of lit scene geometry.
- GPU submission receives prepared packets/resources only.
- Graph/snapshot/handle tests pass through the #2 test harness.

## 24. #9 Correctness Sequence Constraints

After #4/#5 approval, #9 must implement in this order:

1. Backend selection and capability diagnostics.
2. Shader manifest and multi-target shader output layout.
3. Linear HDR scene target, tone-map/final conversion, and color-space tests.
4. Material/base-shader system with `OpaquePbr`, then `AlphaCutoutPbr`, `TransparentPbr`, and `OverlayUnlit`.
5. Overlay system and grid as overlay commands.
6. Picking target, request-scoped readback, and picking ID tests.
7. Structured renderer diagnostics, resource RAII/caches, and expanded frame stats.

#4/#5 APIs must preserve these future invariants:

- Display conversion happens last and exactly once once #9 adds HDR/tone mapping.
- Overlay domains remain separate from lit geometry and post effects.
- Picking uses explicit ID targets/readbacks on request, not per-frame readback or color-managed buffers.
- Materials remain instances of base shaders and do not become a global all-fields struct.
- Color-space behavior is declared for every future texture slot and render target.
- Runtime shader compilation stays forbidden.

## 25. Return For Approval

When #4/#5 implementation is complete, return to the renderer architect with:

- This plan path.
- Changed files.
- CMake target layout relevant to Crimson and UI.
- Confirmation of all files that still include `<bgfx/...>` or `<bx/...>`.
- Public API headers for `Renderer`, handles, diagnostics, `FrameSnapshot`, and `RenderGraph`.
- Build and test command results.
- Manual viewport verification notes.
- Any deviations from this plan.

If approved, the main agent should move this plan from `agents/plans/` to `agents/archive/`. Do not delete the plan automatically.
