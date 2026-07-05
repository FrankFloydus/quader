# Implementation Plan: Task #9 Crimson V1 Rendering Correctness Foundation

Owner: renderer architect A29  
Task: project-board #9, "Build Crimson V1 rendering correctness foundation"  
Status: active renderer implementation plan  
Authority: `agents/plans/audit_20260704_full_architecture_master.md`, Batch 8  
Primary architecture authority: `agents/architecture/renderer.md`  
Supporting authorities: `agents/architecture/architecture.md`, `agents/architecture/ui.md`, `agents/tests-policy.md`  
Workspace baseline used for this plan: current files under `C:\Users\Drako\Desktop\_quader-qt-base` on 2026-07-04  

Do not use `agents/archive/implementation_wave1_a5_renderer_preflight_renderer_doc.md` as active authority for this task. This plan supersedes renderer preflight guidance for #9 only.

## Goal

Build the Crimson V1 correctness foundation required by master-audit Batch 8:

- backend selection and capability diagnostics
- shader manifest plus multi-target shader output layout
- linear HDR scene target, tone-map/final conversion, and color-space tests
- material/base-shader system for `OpaquePbr`, `AlphaCutoutPbr`, `TransparentPbr`, and `OverlayUnlit`
- overlay command system with the grid as an overlay, separated from lit/post paths
- picking target and request-scoped readback IDs
- structured diagnostics, GPU resource RAII/caches, and expanded frame stats

This is a renderer correctness foundation, not full renderer maturity. It should preserve the runnable prototype while replacing prototype-only rendering assumptions with explicit Crimson contracts.

## Current Workspace Findings

The workspace is already past master-audit Batches 3 and 4:

- `src/crimson/` exists and is linked as the `crimson` target.
- Graphics-runtime includes are currently confined to `src/crimson/gpu/`.
- `Renderer`, `RendererConfig`, `RendererStatus`, `FrameSnapshot`, `FrameBuilder`, `RenderWorld`, `RenderGraph`, `RenderResourceRegistry`, and project-owned render handles already exist.
- `src/ui/viewport/crimson_viewport_render_host.*` owns the UI-to-Crimson adapter and keeps Qt in `src/ui/`.
- CTest has unit tests for Crimson public types, GPU shell behavior, frame snapshots, and render graph validation.

The current renderer is still prototype-shaped:

- `CrimsonViewportRenderHost` hard-codes `GraphicsBackendPreference::Vulkan` and `shaders/spirv`.
- `GpuDevice` still owns prototype cube/grid vertex data, uniforms, shader program creation, draw submission, and frame stats directly.
- `RenderQueue` contains only `PrototypeOpaque`.
- `FrameSnapshot` contains `GridOverlayPacket` instead of a general overlay command list.
- The render graph writes directly to `BackbufferColor` and has no HDR scene target, tone-map target, picking target, or V1 correctness pass order.
- Shader compilation is still a CMake custom command for four SPIR-V prototype shaders only.
- No material/base-shader schema exists.
- No picking request/result API exists.
- `FrameStats` has only frame size, view count, graph pass count, draw call count, visible object count, diagnostics count, and FPS.

## Strict Non-Goals

Do not implement these in task #9:

- no deferred renderer
- no clustered-forward renderer
- no GPU-driven visibility, mesh shaders, virtualized geometry, ray tracing, real-time global illumination, SSAO, bloom, fog, glass refraction, transmission, volume, IOR rendering, TAA, SSR, screen-space refraction, area lights, clearcoat, anisotropy, sheen, or iridescence
- no IBL import/baking/runtime package loader
- no directional shadows, point shadows, or spot shadows
- no glTF importer or file I/O implementation
- no material inspector UI or document-backed material editing UI
- no picking-driven document selection
- no topology, document, command, or tool mutation work
- no golden-image harness unless a later renderer authority update explicitly adds it; stable golden image maturity remains task #11 territory
- no new third-party dependency without explicit approval
- no runtime shader compilation
- no graphics-runtime headers outside `src/crimson/gpu/`
- no UI code including `src/crimson/gpu/**`
- no importer, material authoring, or renderer-core public API depending on graphics-runtime handles
- no overlay draw commands in lit passes
- no overlays in HDR lighting, tone-map source, bloom/fog/post chains, or picking readback unless a pass explicitly declares picking ownership
- no per-frame picking readback without an explicit request
- no direct rendering from half-edge mesh internals

## Module Families In Scope

Owned by #9:

- `src/crimson/`
- `src/crimson/gpu/`
- `shaders/`
- `build_tools/`
- `tests/unit/crimson/`
- optionally `tests/render_tests/` for pure renderer-correctness tests if the build organization is changed to add that folder
- CMake shader/build/test entries in `CMakeLists.txt`
- narrowly scoped UI adapter changes under `src/ui/viewport/` only where needed to pass backend preference, shader root, diagnostics, stats, and picking request/result data through the existing viewport render-host boundary

Out of scope for #9:

- `assets/gltf/`
- `assets/textures/` except if the implementation chooses tiny default texture fixtures; generated 1x1 GPU defaults are preferred
- `assets/environment/`
- document material editing, importer registration, file dialogs, material inspector panels, and selection sync

## Dependency Boundaries

Keep these boundaries exact:

- `src/crimson/gpu/**` is the only place that may include `bgfx/`, `bimg/`, or `bx/`.
- Public Crimson headers may expose project-owned handles, renderer settings, snapshots, material schemas, diagnostics, stats, and picking payloads. They must not expose bgfx handle types or runtime enums.
- UI viewport code may include public Crimson headers through `crimson_viewport_render_host.*`. It must not include `crimson/gpu/**`.
- `crimson` may depend on `modeling_foundation`, `modeling_math`, and current document snapshot-facing types. It must not depend on Qt Widgets.
- Material schemas and shader manifests are Crimson data. Material inspector UI is not implemented here.
- Picking results are renderer-facing interaction data, not document selection truth.

## Public Crimson API Additions And Changes

### Renderer Result Shape

Replace the narrow `Renderer::render()` result with a typed frame result:

File: `src/crimson/renderer.hpp`

```cpp
struct FrameRenderResult {
    FrameStats stats;
    std::vector<PickingResult> completed_picking_results;
};

class Renderer final {
public:
    [[nodiscard]] quader::foundation::Result<FrameRenderResult, RendererDiagnostic> render(
        const FrameSnapshot& snapshot);

    [[nodiscard]] RendererStatus status() const;
    [[nodiscard]] std::optional<FrameStats> latest_frame_stats() const;
    [[nodiscard]] std::string debug_dump_last_frame_graph() const;
};
```

The UI adapter can ignore `completed_picking_results` until tool selection work is ready.

### Backend And Capability Status

Keep backend internals inside `src/crimson/gpu/`, but expose project-owned status data:

File: `src/crimson/renderer_diagnostics.hpp`

```cpp
enum class RendererCapability {
    Instancing,
    Texture2D,
    TextureCube,
    FloatingPointRenderTarget,
    RenderToTexture,
    DepthTexture,
    SrgbTextureSampling,
    SrgbBackbuffer,
    ManualSrgbFinalConversion,
    IntegerPickingTarget,
    Rgba8PickingTarget,
    Readback,
};

struct RendererCapabilityStatus {
    RendererCapability capability;
    bool supported = false;
    std::string detail;
};

struct RendererStatus {
    std::string backend_name;
    bool initialized = false;
    std::vector<RendererCapabilityStatus> capabilities;
    std::vector<RendererDiagnostic> diagnostics;
};
```

Add diagnostic codes:

```cpp
ShaderManifestInvalid
ShaderTargetMissing
ColorSpaceInvalid
MaterialSchemaInvalid
MaterialValidationFailed
PickingReadbackFailed
FrameBufferUnsupported
ResourceLifetimeError
```

Diagnostics should include subsystem/resource context without exposing runtime handles:

```cpp
struct RendererDiagnostic {
    RendererDiagnosticSeverity severity;
    RendererDiagnosticCode code;
    std::string message;
    std::string detail;
    std::string subsystem;      // "gpu", "shader", "material", "graph", "picking", etc.
    std::string resource_name;  // public resource/pass/material/shader name when applicable
    std::uint64_t frame_index = 0;
};
```

### Renderer Configuration

File: `src/crimson/renderer_config.hpp`

Keep `GraphicsBackendPreference`, but #9 must stop hard-coding Vulkan in the UI host. The default config remains `Auto`.

Add display and tone-map settings:

```cpp
enum class ToneMapper : std::uint8_t {
    AcesFitted,
    AgxApprox,
    Reinhard,
    Linear,
};

struct DisplayConversionSettings {
    bool prefer_srgb_backbuffer = true;
    bool allow_manual_final_srgb = true;
};

struct RendererConfig {
    GraphicsBackendPreference backend_preference = GraphicsBackendPreference::Auto;
    std::filesystem::path shader_root; // runtime root containing vulkan/dx11/dx12/metal folders
    bool vsync = true;
    bool enable_debug_text = true;
    ToneMapper default_tone_mapper = ToneMapper::AcesFitted;
    DisplayConversionSettings display_conversion;
};
```

### Frame Snapshot Shape

Files:

- `src/crimson/frame/frame_snapshot.hpp`
- `src/crimson/frame/frame_snapshot.cpp`
- `src/crimson/frame/frame_builder.hpp`
- `src/crimson/frame/frame_builder.cpp`

Evolve `FrameSnapshot` to carry immutable render data for the new domains:

```cpp
class FrameSnapshot final {
public:
    std::span<const RenderView> views() const noexcept;
    std::span<const RenderObject> objects() const noexcept;
    std::span<const RenderLight> lights() const noexcept;
    std::span<const OverlayCommand> overlays() const noexcept;
    std::span<const PickingRequest> picking_requests() const noexcept;
    ViewportSettings viewport_settings() const noexcept;
    DebugViewMode debug_view() const noexcept;
};
```

Replace or deprecate `grid_overlays()` with `overlays()`. During the transition, `FrameBuilder::build_prototype_snapshot()` should still produce a cube object plus a grid overlay command so the existing viewport remains visibly useful.

### Render Object And Queues

File: `src/crimson/scene/render_object.hpp`

Replace `RenderQueue::PrototypeOpaque` with the V1 queues needed by this task:

```cpp
enum class RenderDomain : std::uint8_t {
    LitSurface,
    TransparentSurface,
    Overlay,
    Picking,
    Post,
};

enum class RenderQueue : std::uint8_t {
    Opaque,
    AlphaCutout,
    Transparent,
    OverlayDepthTested,
    OverlayXRay,
    OverlayAlwaysOnTop,
    Picking,
};

struct RenderObject {
    RenderObjectId object_id = 0;
    RenderMeshHandle mesh;
    RenderMaterialHandle material;
    BaseShaderId base_shader = BaseShaderId::OpaquePbr;
    std::array<float, 16> world_from_object = identity_transform();
    quader::math::Aabb world_bounds;
    RenderQueue queue = RenderQueue::Opaque;
    std::uint32_t submesh_index = 0;
    bool visible = true;
    bool pickable = true;
};
```

`ShaderProgramId` should no longer be selected by arbitrary `RenderObject` data once the material/base-shader system is active. Shader program selection belongs to the base shader definition and material system.

## Backend Selection And Capability Diagnostics

Files:

- `src/crimson/gpu/gpu_device.hpp`
- `src/crimson/gpu/gpu_device.cpp`
- `src/crimson/gpu/gpu_capabilities.hpp`
- `src/crimson/gpu/gpu_capabilities.cpp`
- `tests/unit/crimson/backend_capability_tests.cpp`

Implementation requirements:

- Keep `choose_backend()` pure and testable.
- Keep platform priority:
  - Windows: Vulkan, Direct3D12, Direct3D11
  - Linux: Vulkan
  - macOS: Metal
- UI host must pass `GraphicsBackendPreference::Auto` unless user settings later provide an explicit preference.
- Query runtime-supported backends inside `src/crimson/gpu/`.
- Validate required V1 capabilities after backend selection and before resource creation:
  - instancing
  - 2D textures
  - cube textures
  - floating-point render targets
  - render-to-texture
  - depth textures usable by the V1 passes
  - sRGB texture sampling
  - either sRGB backbuffer support or manual final linear-to-sRGB conversion path
  - either `R32U` picking target or RGBA8 picking fallback
  - readback path for request-scoped picking
- Failure to meet a required capability is a fatal structured diagnostic. Optional fallback selection is allowed only for documented alternatives, such as RGBA8 picking when `R32U` is unavailable or manual final sRGB conversion when backbuffer sRGB is unavailable.
- `RendererStatus` must retain the selected backend name and capability status list for UI/status diagnostics.

The capability checks may use runtime-specific data only inside `src/crimson/gpu/`. Public tests should validate Crimson capability policy with fake capability structs, not bgfx calls that require a GPU.

## Shader Manifest And Multi-Target Output Layout

Files:

- `build_tools/compile_shaders.py`
- `build_tools/validate_shader_manifest.py`
- `shaders/manifest.json`
- `shaders/common/color_space.sh`
- `shaders/common/material_pbr.sh`
- `shaders/common/fullscreen.sh`
- `shaders/pbr/opaque_pbr.vs.sc`
- `shaders/pbr/opaque_pbr.fs.sc`
- `shaders/pbr/alpha_cutout_pbr.vs.sc`
- `shaders/pbr/alpha_cutout_pbr.fs.sc`
- `shaders/pbr/transparent_pbr.vs.sc`
- `shaders/pbr/transparent_pbr.fs.sc`
- `shaders/post/fullscreen_triangle.vs.sc`
- `shaders/post/tone_map.fs.sc`
- `shaders/post/present.fs.sc`
- `shaders/overlays/overlay_line.vs.sc`
- `shaders/overlays/overlay_line.fs.sc`
- `shaders/overlays/overlay_solid.vs.sc`
- `shaders/overlays/overlay_solid.fs.sc`
- `shaders/overlays/grid.vs.sc`
- `shaders/overlays/grid.fs.sc`
- `shaders/picking/picking.vs.sc`
- `shaders/picking/picking.fs.sc`
- `src/crimson/gpu/shader_library.hpp`
- `src/crimson/gpu/shader_library.cpp`
- `src/crimson/gpu/shader_library_runtime.hpp`
- `tests/unit/crimson/shader_manifest_tests.cpp`

Output layout:

```text
<build-dir>/compiled_shaders/vulkan/
<build-dir>/compiled_shaders/dx11/
<build-dir>/compiled_shaders/dx12/
<build-dir>/compiled_shaders/metal/
<exe-dir>/shaders/vulkan/
<exe-dir>/shaders/dx11/
<exe-dir>/shaders/dx12/
<exe-dir>/shaders/metal/
```

Runtime lookup:

```cpp
enum class ShaderTarget : std::uint8_t {
    Vulkan,
    Direct3D11,
    Direct3D12,
    Metal,
};

struct ShaderBinaryRef {
    ShaderProgramId program;
    ShaderStage stage;
    ShaderTarget target;
    std::filesystem::path relative_path;
};

class ShaderLibrary final {
public:
    explicit ShaderLibrary(std::filesystem::path shader_root);
    std::filesystem::path shader_path(ShaderProgramId program, ShaderStage stage, ShaderTarget target) const;
};
```

Manifest rules:

- Every shader program used by a base shader or pass must have vertex and fragment stages in `shaders/manifest.json`.
- Every stage must list source path and expected output basename.
- The validator must fail if a manifest entry references a missing source, unknown shader target, duplicate program/stage, or missing expected binary after compilation.
- Runtime code must load compiled binaries only. It must never call shaderc or compile shader source.
- Existing prototype shader names may remain as compatibility entries only until their call sites are removed.
- CMake should depend on `compile_shaders` before building the executable, and post-build copy must copy the target folders, not a single `spirv` folder.

Shader target argument details are implementation work against the checked-in bgfx shaderc version. The worker must verify the shaderc arguments for `vulkan`, `dx11`, `dx12`, and `metal` before completing the shader slice. If this bgfx shaderc version cannot generate one target, stop and return to renderer authority instead of silently dropping the target.

## Linear HDR, Tone Mapping, And Color Space

Files:

- `src/crimson/color/color_space.hpp`
- `src/crimson/color/color_space.cpp`
- `src/crimson/post/exposure.hpp`
- `src/crimson/post/exposure.cpp`
- `src/crimson/post/tone_mapping.hpp`
- `src/crimson/post/tone_mapping.cpp`
- `src/crimson/scene/viewport_settings.hpp`
- `src/crimson/graph/render_graph.hpp`
- `src/crimson/graph/render_graph.cpp`
- `src/crimson/gpu/gpu_device.cpp`
- `src/crimson/gpu/gpu_frame_resources.hpp`
- `src/crimson/gpu/gpu_frame_resources.cpp`
- `tests/unit/crimson/color_space_tests.cpp`
- `tests/unit/crimson/render_graph_correctness_tests.cpp`

Color-space rules:

- Base color texture slots: sRGB.
- Emissive texture slots: sRGB.
- Metallic/roughness, normal, occlusion, picking, depth, and shadow-like data slots: linear/data.
- Lighting target: `HdrSceneColor`, `RGBA16F`, linear HDR.
- Tone-map output: `ToneMappedColor`, linear SDR, `RGBA16F` preferred for correctness.
- Present path performs final linear-to-sRGB conversion exactly once.
- Overlay colors are authored as UI sRGB, converted to linear SDR by the overlay system, and composited after tone mapping.
- Picking target uses `R32U` when supported; fallback `RGBA8` encoded IDs must be treated as data, not color.

CPU reference helpers:

```cpp
struct ColorSrgb { float r, g, b, a; };
struct ColorLinear { float r, g, b, a; };

ColorLinear srgb_to_linear(ColorSrgb color) noexcept;
ColorSrgb linear_to_srgb(ColorLinear color) noexcept;

float exposure_multiplier_from_ev100(float ev100, float compensation_ev) noexcept;
ColorLinear apply_tone_mapper(ToneMapper mapper, ColorLinear hdr) noexcept;
```

Viewport settings:

```cpp
struct ViewportSettings {
    std::uint32_t clear_color_rgba8 = 0x020202ff;
    ToneMapper tone_mapper = ToneMapper::AcesFitted;
    float exposure_ev100 = 12.0F;
    float exposure_compensation_ev = 0.0F;
    bool draw_lit_surfaces = true;
    bool draw_grid_overlay = true;
    bool draw_overlays = true;
};
```

`draw_prototype_opaque` should be removed after the cube is represented as a normal render object with an `OpaquePbr` material. If a transition flag is needed for one slice, keep it private to the builder and remove it before authority review.

## V1 Render Graph For Task #9

Task #9 render graph pass order:

```text
1. FrameSetupPass
2. ResourceUploadPass
3. DepthPrepass
4. PickingPass
5. OpaquePbrPass
6. AlphaCutoutPbrPass
7. TransparentPbrPass
8. ToneMapPass
9. OverlayDepthTestedPass
10. OverlayXRayPass
11. OverlayAlwaysOnTopPass
12. PresentPass
```

Task #9 intentionally omits shadow, skybox, IBL, bloom, fog, SSAO, and glass/refraction passes.

Resources:

```text
BackbufferColor              external, resize dependent, final display target
BackbufferDepth              external if needed by runtime host
HdrSceneColor                RGBA16F, resize dependent, linear HDR
SceneDepth                   D24S8 or D32F, resize dependent
PickingId                   R32U if available, else RGBA8 data, resize dependent, no MSAA
ToneMappedColor              RGBA16F linear SDR, resize dependent
```

Pass resource declarations:

```text
FrameSetupPass               CPU pass, no resource writes
ResourceUploadPass           CPU/GPU upload pass, no graph color writes
DepthPrepass                 writes SceneDepth
PickingPass                  reads SceneDepth, writes PickingId only when request exists
OpaquePbrPass                reads SceneDepth, read-writes HdrSceneColor
AlphaCutoutPbrPass           reads SceneDepth, read-writes HdrSceneColor
TransparentPbrPass           reads SceneDepth, read-writes HdrSceneColor
ToneMapPass                  reads HdrSceneColor, writes ToneMappedColor
OverlayDepthTestedPass       reads SceneDepth, read-writes ToneMappedColor
OverlayXRayPass              reads SceneDepth, read-writes ToneMappedColor
OverlayAlwaysOnTopPass       read-writes ToneMappedColor
PresentPass                  reads ToneMappedColor, read-writes BackbufferColor
```

Clear/load/store behavior:

- `HdrSceneColor` clears once per frame to linear clear color.
- `SceneDepth` clears once per frame before `DepthPrepass`.
- `PickingId` clears to `0` only when a picking request exists.
- `ToneMappedColor` is written by `ToneMapPass`.
- Overlay passes load existing `ToneMappedColor` and store modified `ToneMappedColor`.
- `PresentPass` writes the final display conversion to the backbuffer exactly once.

Resize behavior:

- `RenderGraph::resize()` updates all resize-dependent resource descs.
- `GpuFrameResources` recreates target resources only when extent, format, sample count, or resize generation changes.
- Old GPU resources are destroyed through RAII/destruction queue inside `src/crimson/gpu/`.

Tests must assert pass order, resource formats, dependencies, resize generation, and the fact that overlay passes do not read or write `HdrSceneColor`.

## Material And Base-Shader System

Files:

- `src/crimson/material/base_shader.hpp`
- `src/crimson/material/base_shader_registry.hpp`
- `src/crimson/material/base_shader_registry.cpp`
- `src/crimson/material/material_instance.hpp`
- `src/crimson/material/material_parameter.hpp`
- `src/crimson/material/material_system.hpp`
- `src/crimson/material/material_system.cpp`
- `src/crimson/material/material_texture.hpp`
- `src/crimson/material/material_ui_schema.hpp`
- `src/crimson/material/material_validation.hpp`
- `src/crimson/material/material_validation.cpp`
- `src/crimson/gpu/gpu_material_cache.hpp`
- `src/crimson/gpu/gpu_material_cache.cpp`
- `tests/unit/crimson/material_system_tests.cpp`

Public schema shape:

```cpp
enum class BaseShaderId : std::uint16_t {
    OpaquePbr,
    AlphaCutoutPbr,
    TransparentPbr,
    OverlayUnlit,
};

enum class MaterialParameterKind : std::uint8_t {
    Float,
    Vec2,
    Vec3,
    Vec4,
    ColorSrgb,
    Bool,
    Enum,
};

enum class TextureColorSpace : std::uint8_t {
    Srgb,
    Linear,
    Data,
};

struct MaterialParameterDesc {
    std::string name;
    MaterialParameterKind kind;
    MaterialParameterValue default_value;
    MaterialParameterValue min_value;
    MaterialParameterValue max_value;
};

struct MaterialTextureSlotDesc {
    std::string name;
    TextureColorSpace color_space;
    RenderTextureHandle default_texture;
    bool required = false;
};

struct BaseShaderDefinition {
    BaseShaderId id;
    std::string name;
    RenderDomain domain;
    RenderQueue default_queue;
    DepthMode depth_mode;
    BlendMode blend_mode;
    CullMode cull_mode;
    ShadowMode shadow_mode;
    std::vector<MaterialParameterDesc> parameters;
    std::vector<MaterialTextureSlotDesc> texture_slots;
    std::vector<MaterialUiFieldDesc> ui_schema;
    ShaderProgramId program;
};
```

Material ownership:

- `BaseShaderRegistry` owns immutable base shader definitions.
- `MaterialSystem` owns CPU `MaterialInstance` records and returns `RenderMaterialHandle`.
- `GpuMaterialCache` lives inside `src/crimson/gpu/` and owns runtime resources for material parameter blocks, default textures, sampler states, and shader uniforms.
- Public material handles are generation-checked. Destroying or replacing a material invalidates stale handles.

Validation rules:

- A material may contain only parameters declared by its base shader.
- Required parameter kinds must match exactly.
- Texture slot color-space declarations are fixed by the base shader.
- Missing texture slots bind default textures.
- Unknown parameters and unknown texture slots are validation failures.
- `OverlayUnlit` exposes only overlay/debug fields and never accepts PBR fields.

V1 base shaders:

- `OpaquePbr`
  - domain `LitSurface`
  - queue `Opaque`
  - depth test on, depth write on
  - blend off
  - exposes base color, base color texture, metallic, roughness, metallic/roughness texture, normal texture, normal strength, AO texture, AO strength, emissive color, emissive texture, emissive strength, double sided, UV set, UV tiling, UV offset
- `AlphaCutoutPbr`
  - domain `LitSurface`
  - queue `AlphaCutout`
  - depth test on, depth write on
  - blend off
  - exposes all `OpaquePbr` fields plus alpha cutoff and alpha source channel
- `TransparentPbr`
  - domain `TransparentSurface`
  - queue `Transparent`
  - depth test on, depth write off
  - alpha blend
  - sorted back-to-front per object center
  - exposes base color, base color texture, opacity, metallic, roughness, metallic/roughness texture, normal texture, normal strength, emissive color, emissive texture, emissive strength, double sided
- `OverlayUnlit`
  - domain `Overlay`
  - queues overlay only
  - depth behavior from overlay command
  - alpha blend
  - no lighting, no fog, no post, no tone mapping source
  - exposes color, opacity, line width mode, depth mode, pattern

GPU parameter block layouts should follow `renderer.md` section 9. Keep the packing code centralized in `MaterialSystem`/`GpuMaterialCache`; do not duplicate ad hoc uniform setup in every pass.

The prototype cube should become a normal `RenderObject` with a default `OpaquePbr` material. The prototype grid should become an `OverlayUnlit` overlay command.

## Overlay System And Grid Commands

Files:

- `src/crimson/overlays/overlay_command.hpp`
- `src/crimson/overlays/overlay_system.hpp`
- `src/crimson/overlays/overlay_system.cpp`
- `src/crimson/overlays/grid_overlay.hpp`
- `src/crimson/overlays/grid_overlay.cpp`
- `src/crimson/gpu/gpu_overlay_renderer.hpp`
- `src/crimson/gpu/gpu_overlay_renderer.cpp`
- `tests/unit/crimson/overlay_tests.cpp`

Public overlay data:

```cpp
enum class OverlayDepthMode : std::uint8_t {
    DepthTested,
    XRay,
    AlwaysOnTop,
};

enum class OverlayPrimitive : std::uint8_t {
    Grid,
    LineList,
    SolidTriangles,
};

struct OverlayCommand {
    std::uint32_t view_index = 0;
    OverlayPrimitive primitive = OverlayPrimitive::LineList;
    OverlayDepthMode depth_mode = OverlayDepthMode::DepthTested;
    ColorSrgb color_srgb{1.0F, 1.0F, 1.0F, 1.0F};
    float opacity = 1.0F;
    float thickness_px = 1.0F;
    std::uint32_t payload_offset = 0;
    std::uint32_t payload_count = 0;
};

struct GridOverlayCommand {
    std::uint32_t view_index = 0;
    quader::math::Vec3 origin;
    quader::math::Vec3 u_axis;
    quader::math::Vec3 v_axis;
    float minor_spacing_m = 1.0F;
    float major_spacing_m = 2.0F;
    float fade_start_m = 96.0F;
    float fade_end_m = 1536.0F;
    ColorSrgb minor_color;
    ColorSrgb major_color;
    ColorSrgb axis_u_color;
    ColorSrgb axis_v_color;
};
```

Implementation rules:

- `FrameBuilder` creates grid overlay commands from cameras and view rectangles.
- `OverlaySystem` converts `ColorSrgb` to linear SDR and buckets commands into `OverlayDepthTested`, `OverlayXRay`, and `OverlayAlwaysOnTop` draw lists.
- `OverlaySystem` does not know Qt, document truth, commands, tools, or graphics-runtime handles.
- `GpuOverlayRenderer` creates runtime buffers and submits overlay draw calls.
- The grid is rendered only by overlay passes after `ToneMapPass`.
- The grid never appears in `RenderObject`, `RenderQueue::Opaque`, `HdrSceneColor`, or PBR material paths.
- XRay overlays use two subpasses internally if needed, but they remain one declared render graph pass from the graph's point of view.

Overlay tests must assert:

- grid commands are emitted as overlay commands, not render objects
- overlay pass declarations read/write `ToneMappedColor`, never `HdrSceneColor`
- overlay color conversion uses sRGB-to-linear SDR
- overlay material schema is `OverlayUnlit`, not PBR

## Picking Target And Request-Scoped IDs

Files:

- `src/crimson/picking/picking.hpp`
- `src/crimson/picking/picking.cpp`
- `src/crimson/gpu/gpu_picking.hpp`
- `src/crimson/gpu/gpu_picking.cpp`
- `tests/unit/crimson/picking_tests.cpp`

Public API:

```cpp
using PickingRequestId = std::uint64_t;

enum class PickingElementKind : std::uint8_t {
    None,
    Object,
    Submesh,
    Face,
    Edge,
    Vertex,
};

struct PickingPayload {
    RenderObjectId object_id = 0;
    std::uint32_t submesh_index = 0;
    PickingElementKind element_kind = PickingElementKind::None;
    std::uint32_t element_index = 0;
};

struct PickingRequest {
    PickingRequestId request_id = 0;
    std::uint32_t view_index = 0;
    std::uint16_t x_px = 0;
    std::uint16_t y_px = 0;
};

struct PickingResult {
    PickingRequestId request_id = 0;
    bool hit = false;
    PickingPayload payload;
};
```

ID policy:

- `0` means no hit.
- IDs are request/frame scoped, not document IDs.
- `PickingIdAllocator` assigns sequential raw `std::uint32_t` IDs for pickable draw packets for the active request.
- `PickingIdAllocator` stores a vector mapping raw IDs to `PickingPayload`.
- `R32U` target stores raw IDs directly.
- RGBA8 fallback stores the raw 32-bit ID as four data bytes. It is not color-managed.

Pass policy:

- `PickingPass` runs only when `FrameSnapshot::picking_requests()` is non-empty or when a debug view explicitly requests object ID visualization.
- `PickingPass` writes only `PickingId`.
- `PickingPass` does not write `HdrSceneColor`, `ToneMappedColor`, or `BackbufferColor`.
- No tone mapping, sRGB conversion, MSAA, lighting, materials beyond ID shaders, overlays, fog, or post effects apply to picking IDs.
- Readback happens only for requested pixels.
- One-frame delayed readback is acceptable; if used, `FrameRenderResult::completed_picking_results` may contain results from earlier requests.
- No per-frame readback without a request.

Viewport integration:

- Add UI-neutral picking request/result types to `src/ui/viewport/viewport_types.hpp`.
- Add optional request plumbing to `ViewportRenderRequest` or `IViewportRenderHost` without exposing Crimson GPU internals.
- Do not update document selection in #9. Later tool/selection work can consume `ViewportPickResult`.

Picking tests must cover:

- raw ID encode/decode, including `0`
- RGBA8 fallback byte packing without sRGB conversion
- request-scoped map invalidation between frames
- no request means no readback is scheduled
- pass/resource declarations do not touch color-managed targets

## GPU Resource RAII And Caches

Files:

- `src/crimson/gpu/gpu_handles.hpp`
- `src/crimson/gpu/gpu_resources.hpp`
- `src/crimson/gpu/gpu_resources.cpp`
- `src/crimson/gpu/gpu_frame_resources.hpp`
- `src/crimson/gpu/gpu_frame_resources.cpp`
- `src/crimson/gpu/gpu_material_cache.hpp`
- `src/crimson/gpu/gpu_material_cache.cpp`
- `src/crimson/gpu/gpu_mesh_cache.hpp`
- `src/crimson/gpu/gpu_mesh_cache.cpp`
- `src/crimson/gpu/gpu_program_cache.hpp`
- `src/crimson/gpu/gpu_program_cache.cpp`
- `tests/unit/crimson/gpu_resource_tests.cpp`

Rules:

- Runtime handle wrappers live only in `src/crimson/gpu/`.
- Move-only RAII wrappers should destroy owned bgfx handles in destructors.
- `GpuResourceTable` remains the public-handle generation table, but stored resources should be RAII resources, not raw handles plus scattered destroy calls.
- `GpuFrameResources` owns framebuffer/texture resources for graph resources and recreates resize-dependent resources when the graph resize generation changes.
- `GpuProgramCache` loads compiled shader binaries from `ShaderLibrary` and owns runtime program handles.
- `GpuMaterialCache` owns default textures, sampler states, material parameter uploads, and material handle to GPU resource mapping.
- `GpuMeshCache` may initially keep prototype cube upload support, but its API must accept prepared render mesh data and `RenderMeshHandle`; no half-edge mesh dependency.

The implementation may keep the current prototype cube mesh as a static render mesh fixture inside Crimson while document-driven mesh extraction is not ready. It must not render directly from UI widgets or document mesh internals.

## Frame Stats

File: `src/crimson/frame/frame_stats.hpp`

Expand `FrameStats`:

```cpp
struct FrameStats {
    std::uint64_t frame_index = 0;
    std::uint32_t width_px = 0;
    std::uint32_t height_px = 0;
    std::uint32_t view_count = 0;
    std::uint32_t graph_pass_count = 0;
    std::uint32_t draw_call_count = 0;
    std::uint32_t draw_packet_count = 0;
    std::uint32_t opaque_draw_count = 0;
    std::uint32_t alpha_cutout_draw_count = 0;
    std::uint32_t transparent_draw_count = 0;
    std::uint32_t overlay_draw_count = 0;
    std::uint32_t picking_request_count = 0;
    std::uint32_t picking_readback_count = 0;
    std::uint32_t live_mesh_count = 0;
    std::uint32_t live_material_count = 0;
    std::uint32_t live_texture_count = 0;
    std::uint32_t live_program_count = 0;
    std::uint32_t diagnostic_count = 0;
    double fps = 0.0;
};
```

These are correctness/per-frame observability counters, not tight performance benchmarks. Do not add fragile timing thresholds.

## Viewport Integration

Files:

- `src/ui/viewport/crimson_viewport_render_host.cpp`
- `src/ui/viewport/viewport_render_host.hpp`
- `src/ui/viewport/viewport_types.hpp`
- `src/ui/qt_app/main_window.cpp` only if status/stat display needs the new fields
- `tests/unit/ui/viewport_tests.cpp` only for UI boundary behavior

Required changes:

- `CrimsonViewportRenderHost` must use `GraphicsBackendPreference::Auto`.
- `RendererConfig::shader_root` must point to `<applicationDirPath>/shaders`, not `<applicationDirPath>/shaders/spirv`.
- Keep Qt conversion to native surface in UI.
- Keep bgfx platform conversion in `src/crimson/gpu/`.
- Keep renderer diagnostics flowing to `NotificationService` through `ViewportRenderResult`; do not create a debug panel in #9.
- If `ViewportFrameStats` expands, keep it UI-owned and derived from `FrameStats`; do not expose renderer internals.
- Add optional picking request/result plumbing only as typed UI data. Do not mutate document selection.

## Ordered Worker Slices

Each slice is a coherent implementation unit. Run `cmake --build --preset qt-mingw-debug` after each slice and before starting another independent code task. In PM batches, do not let more than two code tasks pass without a successful build.

### Slice 1: Backend Auto Selection, Capabilities, Diagnostics, And RAII Base

Dependencies: #4 and #5 approved; no other #9 slice required.

Owned files:

- `src/crimson/renderer_config.hpp`
- `src/crimson/renderer_diagnostics.hpp`
- `src/crimson/gpu/gpu_device.*`
- `src/crimson/gpu/gpu_capabilities.*`
- `src/crimson/gpu/gpu_handles.hpp`
- `src/crimson/gpu/gpu_resources.*`
- `src/ui/viewport/crimson_viewport_render_host.cpp`
- `tests/unit/crimson/backend_capability_tests.cpp`
- `tests/unit/crimson/gpu_resource_tests.cpp`
- CMake test entries

Deliver:

- UI host uses backend `Auto`.
- Capability policy and diagnostics exist and are test-covered.
- Required V1 capability validation runs after backend selection.
- Move-only runtime resource wrappers exist under `src/crimson/gpu/`.
- Existing tests for backend selection remain green.

Blocker condition:

- If runtime capability information cannot prove a required capability, stop and return to renderer authority with the exact missing runtime query.

Verification:

```powershell
cmake --preset qt-mingw-debug
cmake --build --preset qt-mingw-debug
ctest --preset qt-mingw-debug
cmake --build --preset qt-mingw-debug --target check_architecture
```

### Slice 2: Shader Manifest And Multi-Target Shader Build Pipeline

Dependencies: Slice 1 target/backend naming.

Owned files:

- `build_tools/compile_shaders.py`
- `build_tools/validate_shader_manifest.py`
- `shaders/manifest.json`
- `shaders/common/*`
- `shaders/pbr/*`
- `shaders/post/*`
- `shaders/overlays/*`
- `shaders/picking/*`
- `src/crimson/renderer_types.hpp`
- `src/crimson/gpu/shader_library.*`
- `src/crimson/gpu/shader_library_runtime.hpp`
- `CMakeLists.txt`
- `tests/unit/crimson/shader_manifest_tests.cpp`

Deliver:

- Manifest-driven shader list.
- Compiled output folders for `vulkan`, `dx11`, `dx12`, and `metal`.
- Runtime shader lookup based on selected backend target.
- Post-build copy of all shader target folders to the executable directory.
- No runtime shader compilation path.

Blocker condition:

- If bgfx shaderc cannot compile one target with this workspace's checked-in bgfx.cmake version, stop and return to renderer authority. Do not reduce the target list without approval.

Verification:

```powershell
cmake --preset qt-mingw-debug
cmake --build --preset qt-mingw-debug --target compile_shaders
python build_tools/validate_shader_manifest.py --manifest shaders/manifest.json --compiled build/qt-mingw-debug/compiled_shaders
cmake --build --preset qt-mingw-debug
ctest --preset qt-mingw-debug
cmake --build --preset qt-mingw-debug --target check_architecture
```

### Slice 3: HDR Scene Target, Tone Mapping, Display Conversion, And Graph Resources

Dependencies: Slice 2 shaders and shader library.

Owned files:

- `src/crimson/color/*`
- `src/crimson/post/*`
- `src/crimson/scene/viewport_settings.hpp`
- `src/crimson/graph/*`
- `src/crimson/gpu/gpu_frame_resources.*`
- `src/crimson/gpu/gpu_device.cpp`
- `shaders/common/color_space.sh`
- `shaders/post/fullscreen_triangle.vs.sc`
- `shaders/post/tone_map.fs.sc`
- `shaders/post/present.fs.sc`
- `tests/unit/crimson/color_space_tests.cpp`
- `tests/unit/crimson/render_graph_correctness_tests.cpp`

Deliver:

- `HdrSceneColor`, `SceneDepth`, `ToneMappedColor`, `PickingId`, and `BackbufferColor` resource declarations.
- Task #9 pass order.
- `ToneMapPass` and `PresentPass`.
- Exactly-one final display conversion policy.
- CPU reference color conversion and tone mapper tests.
- Render graph tests proving overlays do not read/write HDR scene color.

Verification:

```powershell
cmake --build --preset qt-mingw-debug
ctest --preset qt-mingw-debug
cmake --build --preset qt-mingw-debug --target check_architecture
```

### Slice 4: Material/Base-Shader CPU System

Dependencies: Slice 1 diagnostics and Slice 3 render queues/domains. Can start after Slice 2 if Slice 3 is not yet integrated, but final integration requires Slice 3.

Owned files:

- `src/crimson/material/*`
- `src/crimson/renderer_types.hpp`
- `src/crimson/scene/render_object.hpp`
- `tests/unit/crimson/material_system_tests.cpp`
- CMake entries

Deliver:

- `BaseShaderId`, base shader definitions, schema registry, material instances, validation, defaults, UI schema metadata.
- `OpaquePbr`, `AlphaCutoutPbr`, `TransparentPbr`, `OverlayUnlit` definitions.
- Tests proving each material exposes only its own fields and texture color-space declarations.
- No GPU or bgfx dependencies in material public headers.

Verification:

```powershell
cmake --build --preset qt-mingw-debug
ctest --preset qt-mingw-debug
cmake --build --preset qt-mingw-debug --target check_architecture
```

### Slice 5: Material GPU Cache And PBR Pass Integration

Dependencies: Slices 2, 3, and 4.

Owned files:

- `src/crimson/gpu/gpu_material_cache.*`
- `src/crimson/gpu/gpu_mesh_cache.*`
- `src/crimson/gpu/gpu_program_cache.*`
- `src/crimson/gpu/gpu_device.cpp`
- `src/crimson/frame/frame_builder.*`
- `src/crimson/scene/render_world.*`
- `shaders/pbr/*`
- `tests/unit/crimson/material_system_tests.cpp`
- `tests/unit/crimson/render_graph_correctness_tests.cpp`

Deliver:

- Prototype cube rendered as `OpaquePbr` material through material/base-shader path.
- Alpha cutout and transparent base shaders exist and can produce draw packets even if the prototype scene only exercises opaque by default.
- Transparent queue sorting is deterministic for same-depth ties.
- Default textures are created in `src/crimson/gpu/` and bound according to texture color-space slots.
- Existing prototype behavior remains visibly runnable.

Verification:

```powershell
cmake --build --preset qt-mingw-debug
ctest --preset qt-mingw-debug
cmake --build --preset qt-mingw-debug --target check_architecture
```

### Slice 6: Overlay System And Grid Command Migration

Dependencies: Slice 3 tone-mapped target and Slice 4 `OverlayUnlit` schema.

Owned files:

- `src/crimson/overlays/*`
- `src/crimson/gpu/gpu_overlay_renderer.*`
- `src/crimson/frame/frame_snapshot.*`
- `src/crimson/frame/frame_builder.*`
- `src/crimson/gpu/gpu_device.cpp`
- `shaders/overlays/*`
- `tests/unit/crimson/overlay_tests.cpp`
- `tests/unit/crimson/frame_snapshot_tests.cpp`

Deliver:

- `GridOverlayPacket` replaced by grid/overlay command data.
- Grid rendered after `ToneMapPass`.
- Overlay passes use `ToneMappedColor` and `SceneDepth`, not `HdrSceneColor`.
- Overlay shaders are unlit and do not sample PBR resources.
- Frame snapshot tests updated for overlay command immutability.

Verification:

```powershell
cmake --build --preset qt-mingw-debug
ctest --preset qt-mingw-debug
cmake --build --preset qt-mingw-debug --target check_architecture
```

### Slice 7: Picking Target, Request Plumbing, And Readback

Dependencies: Slice 3 graph resources and Slice 5 draw packet/object IDs. Slice 6 is not strictly required, but it should be complete first so picking and overlays remain clearly separate.

Owned files:

- `src/crimson/picking/*`
- `src/crimson/gpu/gpu_picking.*`
- `src/crimson/frame/frame_snapshot.*`
- `src/crimson/renderer.*`
- `src/crimson/gpu/gpu_device.cpp`
- `src/ui/viewport/viewport_types.hpp`
- `src/ui/viewport/viewport_render_host.hpp`
- `src/ui/viewport/crimson_viewport_render_host.cpp`
- `shaders/picking/*`
- `tests/unit/crimson/picking_tests.cpp`
- `tests/unit/ui/viewport_tests.cpp` if UI request/result plumbing changes

Deliver:

- `PickingRequest`, `PickingResult`, request-scoped ID allocator, `R32U` target with RGBA8 fallback.
- No readback without request.
- One-frame delayed readback support if needed.
- UI gets typed pick results but does not mutate selection.
- Tests cover encoding, request scoping, no-readback path, and pass isolation.

Verification:

```powershell
cmake --build --preset qt-mingw-debug
ctest --preset qt-mingw-debug
cmake --build --preset qt-mingw-debug --target check_architecture
```

### Slice 8: Diagnostics, Frame Stats, Runtime Deploy, And Integration Cleanup

Dependencies: Slices 1 through 7.

Owned files:

- `src/crimson/frame/frame_stats.hpp`
- `src/crimson/renderer_diagnostics.hpp`
- `src/crimson/renderer.*`
- `src/crimson/gpu/gpu_device.cpp`
- `src/ui/viewport/crimson_viewport_render_host.cpp`
- `src/ui/qt_app/main_window.cpp` only if stat/status presentation changes
- `tests/unit/crimson/frame_stats_diagnostics_tests.cpp`
- `tests/unit/crimson/render_graph_correctness_tests.cpp`
- `tests/unit/ui/viewport_tests.cpp` if presentation data changes

Deliver:

- Expanded stats populated from real queue/pass/resource counters.
- Structured diagnostics include subsystem/resource/frame context.
- Prototype-only flags removed or quarantined behind clearly named transition code.
- Runtime shader folders deploy with the executable.
- No architecture boundary violations.
- Manual viewport smoke checklist documented in the worker report.

Verification:

```powershell
cmake --preset qt-mingw-debug
cmake --build --preset qt-mingw-debug
ctest --preset qt-mingw-debug
cmake --build --preset qt-mingw-debug --target check_architecture
cmake --build --preset qt-mingw-debug --target deploy
python tools/project_board.py validate
```

Manual smoke after deploy:

```powershell
.\build\qt-mingw-debug\QuaderQtBgfx.exe
```

Manual expected behavior:

- app opens without renderer initialization error
- renderer status shows selected backend
- cube remains visible
- grid remains visible and is not affected by tone-map exposure changes beyond final overlay composition
- resizing viewport does not lose render targets
- no shader missing diagnostics appear for the selected backend

## Test Strategy

Tests must protect user-visible renderer behavior and invariants, not private call order. Use small deterministic unit tests first. Avoid golden images in #9 unless renderer authority updates this plan.

Required tests:

- `backend_capability_tests.cpp`
  - backend priority and explicit preference behavior
  - missing required capability produces structured fatal diagnostic
  - fallback capability alternatives are recorded
- `shader_manifest_tests.cpp`
  - every manifest program has vertex/fragment source
  - every configured target has expected binary path after compile
  - runtime lookup never points to shader source files
- `color_space_tests.cpp`
  - sRGB 0.5 decodes to expected linear value
  - linear roughness 0.5 remains 0.5
  - normal/AO/metallic/roughness/picking slots are not sRGB
  - emissive/base color slots are sRGB
  - display conversion policy chooses exactly one final conversion path
  - tone mappers return finite values for representative HDR inputs
- `render_graph_correctness_tests.cpp`
  - task #9 pass order is stable
  - resources have required formats
  - overlays never read/write `HdrSceneColor`
  - picking never reads/writes color-managed targets
  - resize updates resource generations
- `material_system_tests.cpp`
  - base shader schemas contain only declared fields
  - unknown fields are rejected
  - `OverlayUnlit` rejects PBR fields
  - texture slots have correct color space
  - alpha mode queues are assigned correctly for the three PBR base shaders
- `overlay_tests.cpp`
  - grid is emitted as overlay command, not lit object
  - overlay colors convert from sRGB to linear SDR
  - overlay command buckets map to depth-tested, XRay, and always-on-top passes
- `picking_tests.cpp`
  - raw ID encode/decode, including `0`
  - RGBA8 fallback byte packing
  - request-scoped IDs do not leak across frames
  - no request means no readback scheduled
- `gpu_resource_tests.cpp`
  - resource handles reject stale generations
  - RAII wrappers destroy resources once
  - resize-generation changes recreate frame resources
- `frame_stats_diagnostics_tests.cpp`
  - stats counters update from prepared draw/overlay/picking counts
  - diagnostic count matches accumulated status
  - diagnostic names remain stable

Manual verification is required because the current project has no stable headless render-image harness. The manual verification does not replace the tests above.

## Build And Verification Checkpoints

For every coherent C++/CMake/shader/runtime slice:

```powershell
cmake --build --preset qt-mingw-debug
```

Before another independent code task starts:

```powershell
ctest --preset qt-mingw-debug
cmake --build --preset qt-mingw-debug --target check_architecture
```

Before renderer authority review:

```powershell
cmake --preset qt-mingw-debug
cmake --build --preset qt-mingw-debug
ctest --preset qt-mingw-debug
cmake --build --preset qt-mingw-debug --target check_architecture
cmake --build --preset qt-mingw-debug --target deploy
python tools/project_board.py validate
```

Do not archive a dev build or bump `DEV_VERSION` unless the user or PM explicitly asks for a preserved dev snapshot.

## Residual Risks

- bgfx shaderc multi-target arguments may differ by target/profile in the checked-in bgfx.cmake version. Slice 2 must verify this early and route blockers back to renderer authority.
- Without a headless render harness, some visual correctness still needs manual smoke verification. Golden images remain a planned later maturity task.
- The current prototype has no document-backed render meshes or material assignments. #9 may use Crimson-owned prototype render fixtures, but it must not invent document truth.
- Picking will produce typed hit data but cannot drive selection until document/tool/UI selection integration is ready.
- Transparent PBR can be schema- and queue-correct in #9, but complex order-independent transparency is out of scope.
- Manual final sRGB fallback must be tested carefully to avoid double conversion when a backend supports sRGB backbuffer.

## Authority Review Gates

- If implementation discovers a design gap in this plan, stop and return to renderer architect before inventing a local architecture.
- If shader multi-target output cannot be produced as planned, return to renderer architect with shaderc command output and target details.
- If UI integration needs controls, panels, or settings beyond the viewport host seam, request UI architect input before implementing UI.
- After all slices are integrated and verified, return to renderer architect with:
  - this plan path
  - changed files
  - implementation notes
  - build/test/deploy results
  - known deviations
- Renderer architect approval is required before #9 moves to `In Review`.
- If approved, instruct the main/root agent or PM to move this plan from `agents/plans/` to `agents/archive/`. Do not delete this file automatically.
