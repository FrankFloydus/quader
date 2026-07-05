# crimson — Strict Architecture and Implementation Plan

Status: revised renderer plan  
Renderer name: `crimson`  
Primary target: standalone modeling viewport  
Graphics runtime: implementation detail; do not encode it in target, folder, or planned class names.
Default graphics device policy: Vulkan on Windows/Linux, Metal on macOS, Direct3D fallback on Windows  
Long-term purpose: a renderer good enough to preview assets inside the modeler with lighting, shadows, IBL, materials, overlays, and debug visualization similar in spirit to Godot, Unity, and Unreal viewports.

---

## 0. Scope

This renderer is a **modeling viewport renderer**. It is not a game renderer, cinematic renderer, path tracer, Nanite-style geometry system, Lumen-style GI system, or full engine renderer replacement.

The renderer exists to answer these questions inside the modeler:

- Does the model read correctly under real viewport lighting?
- Do normals, tangents, UVs, material channels, alpha modes, and emissive values look correct?
- Does the mesh behave correctly with shadows, IBL, transparent preview, grid, gizmos, and selection overlays?
- Does a glTF material imported into the modeler look close enough to what the same asset looks like in Godot, Unity HDRP, and Unreal?

The renderer is built around five permanent rules:

1. **Linear HDR lighting first. Display conversion last.**
2. **Materials are instances of base shaders. A material exposes only the parameters of its base shader.**
3. **Lit geometry, transparent geometry, post effects, and overlays are separate render domains.**
4. **Overlays, gizmos, grid, selection, debug lines, and editor visualization never receive lighting, fog, bloom, SSAO, or tone mapping.**
5. **The renderer consumes prepared render data. It never owns modeling logic.**

---

## 1. Architecture decisions

### 1.1 Renderer type

Use a **forward PBR renderer in V1** and a **clustered-forward PBR renderer in V2**.

V1 uses a forward renderer because it is straightforward, reliable, and excellent for a modeling viewport. It handles opaque PBR, alpha cutout, simple transparent materials, IBL, one shadowed directional light, unshadowed point and spot lights, overlays, and correct color handling.

V2 upgrades the direct lighting path to clustered-forward lighting. This supports many point and spot lights without switching the renderer to a full deferred design. Clustered-forward also keeps transparent PBR materials on the same lighting model as opaque materials.

Do not implement a deferred renderer for V1. Do not implement GPU-driven visibility, virtualized geometry, mesh shaders, ray-traced reflections, or real-time global illumination in V1 or V2.

### 1.2 Render graph

Use a **small render graph** from the first commit.

The render graph exists to name passes, declare inputs and outputs, allocate transient frame resources, and prevent pass-order chaos. It does not need resource aliasing or automatic async compute in V1.

V1 render graph features:

- named render passes
- named frame resources
- declared read/write relationships
- fixed execution order validated by dependencies
- debug dump of the frame graph
- resize-aware framebuffer recreation

V2 render graph features:

- pass pruning
- transient resource aliasing
- pass timing
- attachment lifetime visualization
- screenshot capture per graph node

### 1.3 Graphics runtime boundary

Graphics-runtime details are hidden inside `src/crimson/gpu/`.

No modeling code includes graphics-runtime headers. No material authoring code includes graphics-runtime headers. No UI code includes graphics-runtime headers. No glTF importer includes graphics-runtime headers.

Only the Crimson GPU layer owns:

- native vertex buffer handles
- native index buffer handles
- native texture handles
- native framebuffer handles
- native program handles
- native render states
- graphics device selection
- shader binary loading
- transient and instance buffers

The rest of Crimson talks through engine-owned handles:

```cpp
RenderMeshHandle
RenderTextureHandle
RenderMaterialHandle
RenderProgramHandle
RenderFrameBufferHandle
RenderEnvironmentHandle
```

### 1.4 Driver policy

Graphics device priority is fixed:

```text
Windows: Vulkan -> Direct3D 12 -> Direct3D 11
Linux:   Vulkan
macOS:   Metal
```

Crimson queries runtime-supported graphics devices at startup and selects the highest-priority available device. If the selected device lacks a required V1 capability, startup fails with a clear diagnostic.

Required V1 capabilities:

```text
instancing
2D textures
cube textures
floating-point render targets
render-to-texture
shadow map depth textures
sRGB texture sampling
sRGB backbuffer or manual final sRGB conversion
```

The shader build pipeline compiles every shader for every supported graphics target. Runtime shader compilation is forbidden.

### 1.5 Lighting model

Use a glTF-compatible metallic/roughness PBR model.

The BRDF uses:

```text
GGX / Trowbridge-Reitz normal distribution
Smith masking-shadowing
Schlick Fresnel
Lambert diffuse for V1
energy-preserving metal/dielectric split
```

V1 uses punctual direct lights plus IBL. V2 adds clustered punctual lights, more shadow types, and mature transparent/refraction support.

### 1.6 Light units

Use user-facing light values close to Godot, Unity HDRP, Unreal, and glTF conventions.

```text
Directional light: lux
Point light:       lumens in UI, converted to candela internally
Spot light:        lumens in UI, converted to candela internally using cone solid angle
Environment/sky:   intensity multiplier and EV offset in V1; nits display in V2
Emissive material: color + emissive strength; nits-compatible field in V2
Camera exposure:   EV100 + exposure compensation
World scale:       1 unit = 1 meter
```

No light requires absurd values. A sunny day uses roughly `100000 lux`, a household bulb uses roughly `600-1200 lm`, a candle uses roughly `13 lm`, and a street light uses tens of thousands of lumens.

### 1.7 Color system

The renderer works in **linear HDR** until tone mapping.

Color rules:

```text
Base color textures:      sRGB sampling
Emissive textures:        sRGB sampling
Metallic/roughness maps:  linear sampling
Normal maps:              linear sampling
Occlusion maps:           linear sampling
Height/thickness maps:    linear sampling
ID/picking buffers:       integer or unorm data, no color conversion
Lighting target:          RGBA16F linear HDR
Post-processing:          linear HDR until tone mapping
Tone-mapped target:       linear SDR
Backbuffer:               sRGB conversion at final write
Overlay colors:           authored as sRGB UI colors, converted to linear SDR before final write
```

V1 enables runtime sRGB texture sampling and sRGB swapchain conversion when available. If the active device/backbuffer path cannot provide automatic sRGB conversion, the final composite shader performs manual linear-to-sRGB conversion exactly once.

### 1.8 Material architecture: base shader system

A material is not a bag of every possible parameter.

A material is:

```cpp
struct MaterialInstance {
    MaterialId id;
    BaseShaderId base_shader_id;
    MaterialParameterBlock parameters;
    MaterialTextureBindings textures;
    MaterialRenderOverrides overrides;
};
```

A base shader defines:

```text
id
name
render domain
render queue
depth behavior
blend behavior
shadow behavior
required vertex attributes
parameter schema
texture slots
shader programs
supported debug views
UI inspector schema
glTF import mapping
validation rules
```

This produces the behavior requested for future shader types:

```text
OpaquePbr material:       exposes opaque PBR fields only
AlphaCutoutPbr material:  exposes opaque PBR fields + alpha cutoff
TransparentPbr material:  exposes transparency fields, no refraction fields
GlassPbr material:        exposes transmission, IOR, thickness, attenuation
PsxStylized material:     exposes PSX-specific fields such as posterization and dithering
OverlayUnlit material:    exposes editor/debug overlay color and line style only
```

The UI never displays refraction parameters for an opaque material. The UI never displays metallic/roughness parameters for a PSX shader unless that PSX base shader declares them.

### 1.9 Overlays and debug rendering

Overlays are rendered after lit geometry and after post-processing.

Overlay rendering uses a separate unlit domain:

```text
Grid
Axis lines
Selection outline
Selected vertices
Selected edges
Selected faces
Hover highlight
Transform gizmo
Pivot marker
Normals/tangents visualization
Bounding boxes
Camera frustum
Light icons
Measurement guides
Debug text
Debug graphs
```

Overlay passes never write to the lighting HDR buffer. Overlay passes never enter the bloom/glow chain. Overlay passes never enter the fog pass. Overlay passes never enter SSAO. Overlay passes never cast shadows and never receive shadows.

Overlays use scene depth as read-only input for occlusion modes:

```text
DepthTested:   hidden by scene geometry
XRay:          visible through scene with reduced alpha
AlwaysOnTop:   visible over everything
```

---

## 2. Repository structure

```text
src/
  app/
    main.cpp
    app.cpp
    app.hpp
    window_sdl.cpp
    window_sdl.hpp

  crimson/
    renderer.hpp
    renderer.cpp
    renderer_config.hpp
    renderer_types.hpp

    frame/
      frame_snapshot.hpp
      frame_builder.hpp
      frame_stats.hpp

    scene/
      render_world.hpp
      render_object.hpp
      render_camera.hpp
      render_light.hpp
      render_environment.hpp
      render_bounds.hpp

    graph/
      render_graph.hpp
      render_graph.cpp
      render_pass.hpp
      render_resource_desc.hpp
      render_resource_registry.hpp

    material/
      material_instance.hpp
      material_system.hpp
      material_system.cpp
      base_shader.hpp
      base_shader_registry.hpp
      base_shader_registry.cpp
      material_parameter.hpp
      material_ui_schema.hpp
      material_validation.hpp

    mesh/
      render_mesh.hpp
      render_mesh_cache.hpp
      render_mesh_cache.cpp
      vertex_layouts.hpp
      tangent_generation.hpp

    lights/
      light_units.hpp
      light_units.cpp
      light_culling.hpp
      shadow_settings.hpp

    environment/
      ibl_environment.hpp
      ibl_baker.hpp
      brdf_lut.hpp

    overlays/
      overlay_system.hpp
      overlay_system.cpp
      grid_renderer.hpp
      gizmo_renderer.hpp
      selection_renderer.hpp
      debug_draw.hpp
      picking.hpp

    post/
      exposure.hpp
      tone_mapping.hpp
      bloom.hpp
      fog.hpp
      ssao.hpp

    debug/
      debug_views.hpp
      renderdoc_capture.hpp
      gpu_markers.hpp

    gpu/
      gpu_device.hpp
      gpu_device.cpp
      gpu_handles.hpp
      gpu_resources.hpp
      gpu_resources.cpp
      shader_library.hpp
      shader_library.cpp
      framebuffer_pool.hpp
      framebuffer_pool.cpp
      render_state_cache.hpp
      render_state_cache.cpp
      gpu_submit.hpp
      gpu_submit.cpp

  assets/
    gltf/
      gltf_importer.hpp
      gltf_importer.cpp
      gltf_material_mapper.hpp
      gltf_material_mapper.cpp

    textures/
      image_loader.hpp
      image_loader.cpp
      texture_importer.hpp
      texture_importer.cpp

    environment/
      hdr_loader.hpp
      hdr_loader.cpp
      ibl_importer.hpp
      ibl_importer.cpp

  tools/
    shader_compile/
      shader_compile.cpp

    ibl_bake/
      ibl_bake.cpp

shaders/
  common/
    common.sh
    math_pbr.sh
    color_space.sh
    lighting_units.sh
    ibl.sh
    shadows.sh
    vertex_common.sh

  pbr/
    opaque_pbr.vs.sc
    opaque_pbr.fs.sc
    alpha_cutout_pbr.vs.sc
    alpha_cutout_pbr.fs.sc
    transparent_pbr.vs.sc
    transparent_pbr.fs.sc

  shadow/
    shadow_depth.vs.sc
    shadow_depth.fs.sc
    shadow_alpha_cutout.fs.sc

  sky/
    skybox.vs.sc
    skybox.fs.sc

  post/
    fullscreen_triangle.vs.sc
    tone_map.fs.sc
    bloom_prefilter.fs.sc
    bloom_downsample.fs.sc
    bloom_upsample.fs.sc
    fog.fs.sc
    ssao.fs.sc

  overlays/
    overlay_line.vs.sc
    overlay_line.fs.sc
    overlay_solid.vs.sc
    overlay_solid.fs.sc
    grid.vs.sc
    grid.fs.sc
    picking.vs.sc
    picking.fs.sc

assets_default/
  environments/
    studio_default.hdr
    studio_default.ibl
  textures/
    default_white.png
    default_black.png
    default_flat_normal.png
    default_mr.png
    default_ao.png

build_tools/
  compile_shaders.py
  bake_ibl.py
  validate_shader_manifest.py

tests/
  render_tests/
    color_space_tests.cpp
    material_tests.cpp
    light_unit_tests.cpp
    graph_tests.cpp
    gltf_import_tests.cpp
    overlay_tests.cpp
  golden_images/
    v1_reference_scene_vulkan.png
    v1_reference_scene_d3d12.png
    v1_reference_scene_metal.png
```

---

## 3. Core renderer data flow

```text
Application / modeling document
        ↓
RenderWorld update
        ↓
FrameBuilder creates immutable FrameSnapshot
        ↓
CPU culling + draw packet sorting + batching
        ↓
RenderGraph builds passes for the frame
        ↓
GpuDevice executes passes
        ↓
Final color + overlays + present
```

The renderer receives immutable data for the current frame. The frame snapshot contains cameras, render objects, lights, environment, material handles, mesh handles, overlay commands, and debug options.

```cpp
struct FrameSnapshot {
    RenderCamera camera;
    RenderEnvironment environment;
    std::span<const RenderObject> objects;
    std::span<const RenderLight> lights;
    std::span<const OverlayCommand> overlays;
    DebugViewMode debug_view;
    ViewportSettings viewport;
};
```

The renderer transforms the snapshot into render packets:

```cpp
enum class RenderQueue : uint8_t {
    ShadowCaster,
    Opaque,
    AlphaCutout,
    Transparent,
    Sky,
    Post,
    OverlayDepthTested,
    OverlayXRay,
    OverlayAlwaysOnTop,
    Picking,
};

struct DrawPacket {
    RenderMeshHandle mesh;
    RenderMaterialHandle material;
    BaseShaderId base_shader;
    RenderQueue queue;
    Mat4 world_from_object;
    Bounds world_bounds;
    ObjectId object_id;
    SubmeshId submesh_id;
    uint32_t instance_count;
};
```

---

## 4. Base shader system

### 4.1 Base shader definition

```cpp
struct BaseShaderDefinition {
    BaseShaderId id;
    std::string_view name;

    RenderDomain domain;
    RenderQueue default_queue;
    DepthMode depth_mode;
    BlendMode blend_mode;
    CullMode cull_mode;
    ShadowMode shadow_mode;

    VertexAttributeMask required_attributes;
    std::vector<MaterialParameterDesc> parameters;
    std::vector<MaterialTextureSlotDesc> texture_slots;
    std::vector<ShaderProgramDesc> programs;
    std::vector<MaterialUiFieldDesc> ui_schema;

    ValidateMaterialFn validate;
    BuildGpuParameterBlockFn build_gpu_block;
    MapGltfMaterialFn map_gltf;
};
```

### 4.2 V1 base shaders

V1 ships exactly these base shaders:

#### `OpaquePbr`

Render domain: `LitSurface`  
Queue: `Opaque`  
Depth: test on, write on  
Blend: off  
Shadow: casts and receives shadows  
Fog: V2 only  
Bloom/glow: V2 only through emissive extraction

Parameters exposed in UI:

```text
Base Color
Base Color Texture
Metallic
Roughness
Metallic/Roughness Texture
Normal Texture
Normal Strength
Ambient Occlusion Texture
Ambient Occlusion Strength
Emissive Color
Emissive Texture
Emissive Strength
Double Sided
UV Set
UV Tiling
UV Offset
```

Parameters not exposed:

```text
Opacity
Alpha Cutoff
Transmission
IOR
Thickness
Attenuation Color
Attenuation Distance
Refraction Strength
Dithering
Posterization
```

#### `AlphaCutoutPbr`

Render domain: `LitSurface`  
Queue: `AlphaCutout`  
Depth: test on, write on  
Blend: off  
Shadow: casts alpha-tested shadows and receives shadows

Parameters exposed in UI:

```text
All OpaquePbr parameters
Alpha Cutoff
Alpha Source Channel
```

Parameters not exposed:

```text
Opacity Blend
Transmission
IOR
Thickness
Refraction Strength
```

#### `TransparentPbr`

Render domain: `TransparentSurface`  
Queue: `Transparent`  
Depth: test on, write off  
Blend: alpha blend  
Shadow: does not cast shadows in V1  
Sort: back-to-front per object center

Parameters exposed in UI:

```text
Base Color
Base Color Texture
Opacity
Metallic
Roughness
Metallic/Roughness Texture
Normal Texture
Normal Strength
Emissive Color
Emissive Texture
Emissive Strength
Double Sided
```

Parameters not exposed:

```text
IOR
Transmission
Thickness
Attenuation Distance
Screen Refraction
Caustics
```

#### `OverlayUnlit`

Render domain: `Overlay`  
Queue: overlay queues only  
Depth: read-only depth test by overlay mode  
Blend: alpha blend  
Shadow: no cast, no receive  
Lighting: none  
Fog: none  
Bloom/glow: none  
Tone mapping: none

Parameters exposed in UI or debug commands:

```text
Color
Opacity
Line Width Mode
Depth Mode
Pattern
```

### 4.3 V2 base shaders

V2 adds exactly these base shaders:

#### `GlassPbr`

Render domain: `RefractiveSurface`  
Queue: `TransparentRefraction`  
Depth: test on, write off  
Blend: alpha blend / transmission composite  
Shadow: no shadow casting in V2  
Refraction source: resolved scene color and scene depth

Parameters exposed in UI:

```text
Base Color
Base Color Texture
Transmission
Roughness
Normal Texture
Normal Strength
IOR
Thickness
Thickness Texture
Attenuation Color
Attenuation Distance
Refraction Strength
Emissive Color
Emissive Texture
Emissive Strength
Double Sided
```

#### `UnlitSurface`

Render domain: `UnlitSurface`  
Queue: `Opaque` or `Transparent` by blend mode  
Lighting: none  
Fog: V2 setting, default off

Parameters exposed:

```text
Color
Color Texture
Opacity
Alpha Cutoff
Emissive Strength
```

#### `PsxStylized`

Render domain: `StylizedSurface`  
Queue: opaque or alpha cutout according to base shader settings  
Lighting: custom base shader behavior

Parameters exposed:

```text
Base Color
Color Texture
Vertex Snap Amount
Affine Texture Warp Amount
Color Posterization Steps
Dither Strength
Dither Pattern
Texture Filtering Mode
Lighting Quantization Steps
```

This base shader proves the material system supports non-PBR materials without polluting `OpaquePbr` with stylized parameters.

---

## 5. Render graph pass structure

### 5.1 V1 pass order

V1 executes this pass order:

```text
1. FrameSetupPass
2. ResourceUploadPass
3. ShadowDirectionalPass
4. DepthPrepass
5. PickingPass
6. OpaquePbrPass
7. AlphaCutoutPbrPass
8. SkyboxPass
9. TransparentPbrPass
10. ToneMapPass
11. OverlayDepthTestedPass
12. OverlayXRayPass
13. OverlayAlwaysOnTopPass
14. PresentPass
```

Pass behavior:

| Pass                    | Output                   | Notes                                              |
| ----------------------- | ------------------------ | -------------------------------------------------- |
| `ShadowDirectionalPass` | directional shadow map   | Uses opaque and alpha-cutout shadow casters.       |
| `DepthPrepass`          | scene depth              | Opaque and alpha cutout only.                      |
| `PickingPass`           | object/element ID target | No lighting, no color management, no post effects. |
| `OpaquePbrPass`         | HDR scene color          | Linear HDR lighting.                               |
| `AlphaCutoutPbrPass`    | HDR scene color          | Same lighting as opaque, alpha-tested.             |
| `SkyboxPass`            | HDR scene color          | Uses environment cubemap.                          |
| `TransparentPbrPass`    | HDR scene color          | Alpha blend, sorted, no refraction in V1.          |
| `ToneMapPass`           | linear SDR color         | ACES, AgX, Reinhard, Linear modes.                 |
| Overlay passes          | linear SDR color         | Unlit editor visualization.                        |
| `PresentPass`           | sRGB backbuffer          | Final display conversion.                          |

### 5.2 V2 pass order

V2 executes this pass order:

```text
1. FrameSetupPass
2. ResourceUploadPass
3. LightClusterBuildPass
4. ShadowAtlasDirectionalPass
5. ShadowAtlasSpotPass
6. ShadowCubemapPointPass
7. DepthPrepass
8. NormalRoughnessPrepass
9. PickingPass
10. SSAOPass
11. OpaqueClusteredPbrPass
12. AlphaCutoutClusteredPbrPass
13. SkyboxPass
14. TransparentPbrPass
15. GlassRefractionPass
16. FogPass
17. BloomPrefilterPass
18. BloomDownsamplePasses
19. BloomUpsampleCompositePass
20. ToneMapPass
21. DebugViewCompositePass
22. OverlayDepthTestedPass
23. OverlayXRayPass
24. OverlayAlwaysOnTopPass
25. PresentPass
```

V2 keeps overlays after all lighting and post-processing. Debug visualizations that intentionally replace the final image use `DebugViewCompositePass`; editor overlays still render after it.

---

## 6. V1 implementation plan

V1 is the first production-quality proof of concept. It proves that the renderer architecture, materials, lights, shadows, IBL, color space, overlays, and glTF material mapping are correct.

### 6.1 V1 acceptance criteria

V1 is complete when all items in this list pass:

```text
The app opens a resizable viewport using Crimson.
The renderer selects Vulkan on Windows/Linux when available.
The renderer selects Metal on macOS.
The renderer falls back to Direct3D 12 or Direct3D 11 on Windows when Vulkan is unavailable.
All V1 shaders compile offline for Vulkan, Direct3D 12, Direct3D 11, and Metal.
The viewport renders to RGBA16F linear HDR.
Base color and emissive textures sample as sRGB.
Metallic/roughness, normal, AO, ID, and shadow textures sample as linear data.
The final presented image performs exactly one linear-to-sRGB conversion.
The default scene renders with physically plausible light values.
The default directional light casts a shadow.
Point and spot lights affect PBR surfaces.
HDR IBL affects diffuse and specular lighting.
OpaquePbr, AlphaCutoutPbr, TransparentPbr, and OverlayUnlit exist as base shaders.
Material UI schemas expose only parameters owned by the active base shader.
glTF imports base color, metallic, roughness, normal, AO, emissive, alpha mode, double-sided flag, and texture transforms used by V1.
Alpha cutout casts alpha-tested shadows.
Transparent materials sort back-to-front and alpha blend.
Camera frustum culling removes invisible objects.
Identical mesh/material draw packets use GPU instancing.
Grid, selection, gizmo, hover, and debug lines render after tone mapping and do not receive fog, bloom, lights, or shadows.
Object picking works from an ID pass.
Debug views exist for final color, base color, normal, roughness, metallic, AO, emissive, depth, shadow map, and picking ID.
Golden image tests exist for color space, PBR material, shadow, IBL, alpha cutout, transparent blend, and overlays.
```

### 6.2 V1 non-goals

V1 does not include:

```text
SSAO
Bloom/glow
Fog
Glass refraction
Transmission/volume/IOR rendering
Point light shadows
Spot light shadows
Clustered light culling
Automatic render graph resource aliasing
GPU occlusion culling
TAA
SSR
screen-space refraction
area lights
clearcoat
anisotropy
sheen
iridescence
volumetric lighting
runtime shader compilation
host engine integration
```

These items are assigned to V2 or explicitly excluded.

### 6.3 V1 milestone 1 — Crimson GPU platform layer

Deliver files:

```text
src/crimson/gpu/gpu_device.hpp
src/crimson/gpu/gpu_device.cpp
src/crimson/gpu/gpu_handles.hpp
src/app/window_sdl.hpp
src/app/window_sdl.cpp
```

Implement:

```text
window creation
native window handle extraction
graphics runtime init
graphics device selection policy
graphics runtime reset on resize
sRGB backbuffer reset flag
RGBA16F HDR framebuffer creation
depth framebuffer creation
graphics capability validation
frame begin/end
resource destruction queue
GPU debug names
```

Graphics device selection implementation:

```cpp
GraphicsDevice choose_device(Platform platform, Span<GraphicsDevice> supported) {
    if (platform == Platform::MacOS) {
        return require_supported(GraphicsDevice::Metal, supported);
    }

    if (platform == Platform::Windows) {
        return first_supported({
            GraphicsDevice::Vulkan,
            GraphicsDevice::Direct3D12,
            GraphicsDevice::Direct3D11,
        }, supported);
    }

    if (platform == Platform::Linux) {
        return require_supported(GraphicsDevice::Vulkan, supported);
    }

    fail("Unsupported platform for V1 renderer");
}
```

### 6.4 V1 milestone 2 — shader build pipeline

Deliver files:

```text
build_tools/compile_shaders.py
build_tools/validate_shader_manifest.py
shaders/common/*.sh
shaders/pbr/*.sc
shaders/shadow/*.sc
shaders/sky/*.sc
shaders/post/*.sc
shaders/overlays/*.sc
```

Implement:

```text
single command to compile all shaders
graphics-target shader output folders
shader manifest file
CI validation that every listed shader has binaries for every graphics target
a hard error when a runtime shader file is missing
```

Shader output layout:

```text
bin/shaders/vulkan/
bin/shaders/dx11/
bin/shaders/dx12/
bin/shaders/metal/
```

Shader rule:

```text
V1 uses one main shader variant per base shader and binds default textures for missing slots.
```

This keeps V1 stable and prevents a shader permutation explosion.

### 6.5 V1 milestone 3 — color-space foundation

Deliver files:

```text
src/crimson/post/exposure.hpp
src/crimson/post/tone_mapping.hpp
shaders/common/color_space.sh
shaders/post/tone_map.fs.sc
```

Implement:

```text
linear HDR render target: RGBA16F
manual exposure using EV100 and exposure compensation
ACES fitted tone mapper
AgX approximation tone mapper
Reinhard tone mapper
Linear clamp tone mapper
debug bypass path for raw HDR inspection
sRGB texture import flags
sRGB backbuffer path
manual final sRGB fallback path
```

V1 tone mapping enum:

```cpp
enum class ToneMapper : uint8_t {
    AcesFitted,
    AgxApprox,
    Reinhard,
    Linear,
};
```

Color-space test cases:

```text
A 0.5 sRGB base color texture appears as correct linear value after sampling.
A linear roughness value of 0.5 remains 0.5 in shader.
A normal map is never sampled as sRGB.
An emissive texture is sampled as sRGB and multiplied in linear space.
The final pass performs one display conversion and never double-gammas.
```

### 6.6 V1 milestone 4 — render scene model

Deliver files:

```text
src/crimson/scene/render_world.hpp
src/crimson/scene/render_object.hpp
src/crimson/scene/render_camera.hpp
src/crimson/scene/render_light.hpp
src/crimson/frame/frame_snapshot.hpp
src/crimson/frame/frame_builder.hpp
```

Implement:

```text
camera view/projection
world transform per object
AABB per object
mesh handle per submesh
material handle per submesh
object ID
visibility flags
light list
environment handle
overlay command list
```

V1 camera fields:

```cpp
struct RenderCamera {
    Mat4 view;
    Mat4 projection;
    Vec3 position;
    float near_plane_m;
    float far_plane_m;
    float vertical_fov_degrees;
    float exposure_ev100;
    float exposure_compensation_ev;
};
```

### 6.7 V1 milestone 5 — mesh and render mesh cache

Deliver files:

```text
src/crimson/mesh/render_mesh.hpp
src/crimson/mesh/render_mesh_cache.hpp
src/crimson/mesh/render_mesh_cache.cpp
src/crimson/mesh/vertex_layouts.hpp
src/crimson/mesh/tangent_generation.hpp
```

Implement:

```text
static render mesh upload
dynamic render mesh update API
submesh ranges
vertex layout with position, normal, tangent, uv0, uv1, color0
index buffer upload
32-bit index support validation
mesh bounds
MikkTSpace-compatible tangent generation path
```

V1 vertex layout:

```cpp
struct PbrVertex {
    Vec3 position;
    Vec3 normal;
    Vec4 tangent; // xyz tangent, w handedness
    Vec2 uv0;
    Vec2 uv1;
    Color4u8 color0;
};
```

Render mesh rule:

```text
The half-edge modeling mesh is never rendered directly. The renderer consumes triangulated RenderMesh data only.
```

### 6.8 V1 milestone 6 — material base shader system

Deliver files:

```text
src/crimson/material/base_shader.hpp
src/crimson/material/base_shader_registry.hpp
src/crimson/material/base_shader_registry.cpp
src/crimson/material/material_instance.hpp
src/crimson/material/material_system.hpp
src/crimson/material/material_system.cpp
src/crimson/material/material_ui_schema.hpp
src/crimson/material/material_validation.hpp
```

Implement:

```text
BaseShaderId
base shader registry
material parameter descriptors
texture slot descriptors
UI field descriptors
material validation
material default construction per base shader
GPU parameter packing
material texture binding
base shader change operation
```

V1 base shader registry contains:

```text
OpaquePbr
AlphaCutoutPbr
TransparentPbr
OverlayUnlit
```

V1 material UI rule:

```text
The material inspector is generated from the active base shader's UI schema. No inspector code checks hardcoded material types.
```

### 6.9 V1 milestone 7 — glTF material import

Deliver files:

```text
src/assets/gltf/gltf_importer.hpp
src/assets/gltf/gltf_importer.cpp
src/assets/gltf/gltf_material_mapper.hpp
src/assets/gltf/gltf_material_mapper.cpp
```

Use `fastgltf` for the importer.

Import V1 glTF fields:

```text
baseColorFactor
baseColorTexture
metallicFactor
roughnessFactor
metallicRoughnessTexture
normalTexture
normalTexture scale
occlusionTexture
occlusion strength
emissiveFactor
emissiveTexture
alphaMode
alphaCutoff
doubleSided
TEXCOORD_0
TEXCOORD_1
```

Map glTF alpha modes:

```text
OPAQUE -> OpaquePbr
MASK   -> AlphaCutoutPbr
BLEND  -> TransparentPbr
```

Preserve but do not render V2 glTF extensions in V1:

```text
KHR_materials_transmission
KHR_materials_volume
KHR_materials_ior
KHR_materials_clearcoat
KHR_materials_specular
KHR_materials_emissive_strength
KHR_texture_transform fields beyond V1 support
```

V1 importer stores unsupported extension data in `ImportedMaterialMetadata` and emits a non-fatal import warning.

### 6.10 V1 milestone 8 — IBL pipeline

Deliver files:

```text
src/crimson/environment/ibl_environment.hpp
src/crimson/environment/brdf_lut.hpp
src/assets/environment/hdr_loader.hpp
src/assets/environment/hdr_loader.cpp
src/assets/environment/ibl_importer.hpp
src/assets/environment/ibl_importer.cpp
tools/ibl_bake/ibl_bake.cpp
```

Implement:

```text
HDR equirectangular import
offline IBL bake tool
irradiance cubemap generation
prefiltered specular cubemap generation
BRDF integration LUT generation
runtime loading of baked IBL package
default studio IBL package
skybox display from environment cubemap
environment rotation
environment intensity multiplier
environment exposure EV offset
```

V1 IBL runtime rule:

```text
The renderer consumes baked IBL packages. HDR files are converted to baked IBL packages by the asset pipeline before rendering.
```

V1 IBL package layout:

```text
*.ibl
  irradiance_cube
  prefiltered_specular_cube_mips
  brdf_lut_2d
  preview_cube
  metadata.json
```

### 6.11 V1 milestone 9 — lights and shadow

Deliver files:

```text
src/crimson/lights/light_units.hpp
src/crimson/lights/light_units.cpp
src/crimson/lights/shadow_settings.hpp
shaders/common/lighting_units.sh
shaders/common/shadows.sh
shaders/shadow/shadow_depth.vs.sc
shaders/shadow/shadow_depth.fs.sc
shaders/shadow/shadow_alpha_cutout.fs.sc
```

Implement light types:

```cpp
enum class LightType : uint8_t {
    Directional,
    Point,
    Spot,
};
```

V1 light support:

```text
1 shadowed directional light
up to 8 unshadowed point lights
up to 8 unshadowed spot lights
Kelvin temperature to RGB
color filter
range clamp for point/spot lights
inverse-square attenuation
```

V1 shadow support:

```text
directional shadow map
orthographic light projection around camera-visible scene bounds
opaque shadow casters
alpha-cutout shadow casters
PCF 3x3 filtering
constant bias
normal bias
shadow strength
```

V1 default light values:

```text
Default modeling studio directional light: 20000 lux, 5500 K, shadows on
Outdoor sun preset: 100000 lux, 5500 K, shadows on
Indoor bulb preset: point light, 800 lm, 2700 K, range 5 m
Candle preset: point light, 13 lm, 1800 K, range 1.5 m
Street light preset: point/spot light, 20000 lm, 4000 K, range 25 m
```

V1 light formulas:

```text
Point light solid angle: 4π sr
Point candela: lumens / 4π
Spot cone solid angle: 2π * (1 - cos(outer_half_angle))
Spot candela: lumens / cone_solid_angle
Punctual illuminance at distance: candela / max(distance², epsilon)
Directional illuminance: lux
```

### 6.12 V1 milestone 10 — render graph implementation

Deliver files:

```text
src/crimson/graph/render_graph.hpp
src/crimson/graph/render_graph.cpp
src/crimson/graph/render_pass.hpp
src/crimson/graph/render_resource_desc.hpp
src/crimson/graph/render_resource_registry.hpp
```

Implement:

```text
resource descriptions
external resources
transient frame resources
pass declarations
manual pass order validation
graph execution
graph debug dump
graph resize behavior
```

V1 resources:

```text
Backbuffer
HdrSceneColor_RGBA16F
SceneDepth_D24S8 or D32F
DirectionalShadowDepth
PickingId_RGBA8 or R32U when available
ToneMappedColor_RGBA8 or RGBA16F linear SDR
```

### 6.13 V1 milestone 11 — overlays, grid, gizmos, and debug draw

Deliver files:

```text
src/crimson/overlays/overlay_system.hpp
src/crimson/overlays/overlay_system.cpp
src/crimson/overlays/grid_renderer.hpp
src/crimson/overlays/gizmo_renderer.hpp
src/crimson/overlays/selection_renderer.hpp
src/crimson/overlays/debug_draw.hpp
shaders/overlays/overlay_line.vs.sc
shaders/overlays/overlay_line.fs.sc
shaders/overlays/overlay_solid.vs.sc
shaders/overlays/overlay_solid.fs.sc
shaders/overlays/grid.vs.sc
shaders/overlays/grid.fs.sc
```

Implement overlay commands:

```cpp
enum class OverlayDepthMode : uint8_t {
    DepthTested,
    XRay,
    AlwaysOnTop,
};

struct OverlayCommand {
    OverlayPrimitive primitive;
    OverlayDepthMode depth_mode;
    Color4 linear_sdr_color;
    Mat4 transform;
    float thickness_px;
};
```

V1 overlay features:

```text
infinite-looking ground grid with finite generated lines
X/Y/Z axis lines
selected object wireframe
selected edge highlight
selected vertex points
selected face tint
hover highlight
transform gizmo arrows
transform gizmo planes
pivot marker
bounding boxes
normal debug lines
tangent debug lines
light icons
camera frustum lines
debug text block
```

Overlay invariants:

```text
Overlay passes run after ToneMapPass.
Overlay shaders are unlit.
Overlay shaders do not sample IBL.
Overlay shaders do not sample shadow maps.
Overlay shaders do not enter bloom/glow.
Overlay shaders do not enter fog.
Overlay shaders do not write to picking IDs unless explicitly rendered by PickingPass.
```

### 6.14 V1 milestone 12 — picking pass

Deliver files:

```text
src/crimson/overlays/picking.hpp
src/crimson/overlays/picking.cpp
shaders/overlays/picking.vs.sc
shaders/overlays/picking.fs.sc
```

Implement:

```text
object ID output
submesh ID output
element ID path for selected editable mesh
readback on click request only
one-frame delayed readback allowed
no readback every frame
no MSAA on picking target
no tone mapping
no sRGB conversion
```

V1 picking target:

```text
Use R32U when the active graphics device supports it.
Use RGBA8 encoded IDs as the fallback.
```

### 6.15 V1 milestone 13 — CPU culling and GPU instancing

Deliver files:

```text
src/crimson/lights/light_culling.hpp
src/crimson/frame/frame_builder.hpp
src/crimson/gpu/gpu_submit.hpp
src/crimson/gpu/gpu_submit.cpp
```

Implement CPU culling:

```text
extract camera frustum planes
test object AABB against frustum
skip invisible draw packets
record culled count in frame stats
```

Implement instancing:

```text
group visible draw packets by mesh + material + base shader + render state
write instance transforms into the runtime instance data buffer
submit one draw per instance group
fallback to one draw per object only when instance count is 1
```

Instance data layout:

```cpp
struct InstanceData {
    Mat4 world_from_object;
    uint32_t object_id;
    uint32_t material_index;
    uint32_t flags;
    uint32_t padding;
};
```

### 6.16 V1 milestone 14 — debug views and tests

Deliver files:

```text
src/crimson/debug/debug_views.hpp
src/crimson/frame/frame_stats.hpp
tests/render_tests/*.cpp
tests/golden_images/*.png
```

Implement debug views:

```text
Final Color
Base Color
World Normal
Tangent Normal
Roughness
Metallic
Ambient Occlusion
Emissive
Depth
Directional Shadow Map
Object ID
UV0
UV1
Vertex Color
Instance ID
```

Implement automated tests:

```text
shader manifest completeness test
base shader schema test
glTF alphaMode mapping test
sRGB texture flag test
linear texture flag test
tone mapper smoke test
light unit conversion test
overlay exclusion test
picking ID encode/decode test
frustum culling test
instancing batch key test
golden image reference scene test
```

---

## 7. V2 mature implementation plan

V2 turns V1 into a mature renderer for day-to-day modeling work.

### 7.1 V2 acceptance criteria

V2 is complete when all items in this list pass:

```text
Clustered-forward lighting supports many point and spot lights.
Spot lights cast shadows through a shadow atlas.
Point lights cast cubemap shadows.
Directional shadows use cascades.
SSAO works from depth/normal data and affects lit geometry only.
Bloom/glow works from HDR emissive and bright highlights.
Fog works on lit scene geometry and never affects overlays.
GlassPbr supports screen-space refraction, transmission, IOR, thickness, and attenuation.
Transparent sorting is stable and deterministic.
Material base shader registry supports external base shader packages.
PsxStylized exists as a sample custom base shader.
Render graph prunes disabled passes.
Render graph shows frame resource lifetime in debug UI.
Camera culling uses a spatial acceleration structure.
Texture importer supports compressed GPU formats.
glTF importer supports V2 material extensions listed below.
Golden image tests cover Vulkan, Direct3D, and Metal.
```

### 7.2 V2 clustered-forward lighting

Implement:

```text
view-space cluster grid
light assignment per cluster
cluster buffer upload
cluster debug view
support for at least 256 visible punctual lights
per-light layer mask
per-light diffuse/specular toggles
per-light shadow index
```

Cluster grid target:

```text
X/Y: screen tiles, default 16x16 pixels
Z: logarithmic depth slices, default 24 slices
```

### 7.3 V2 shadows

Implement:

```text
directional cascaded shadow maps
spot light shadow atlas
point light cubemap shadows
per-light shadow resolution
PCF quality levels
shadow fade distance
shadow debug atlas view
stable cascades
```

Do not implement virtual shadow maps in V2.

### 7.4 V2 SSAO

Implement:

```text
normal/roughness prepass
SSAO sample kernel
blue-noise/random rotation texture
bilateral blur
AO strength
AO radius in meters
AO power
AO debug view
```

SSAO affects:

```text
opaque PBR indirect diffuse
alpha-cutout PBR indirect diffuse
```

SSAO does not affect:

```text
transparent materials
glass materials
overlays
gizmos
grid
picking IDs
shadow maps
```

### 7.5 V2 bloom and glow

Implement:

```text
HDR bright-pass prefilter
soft threshold
downsample chain
upsample chain
bloom intensity
bloom radius
glow alias in UI for emissive-driven bloom
bloom debug views
```

Bloom source:

```text
lit HDR scene color before tone mapping
```

Bloom excludes:

```text
overlays
grid
gizmos
selection highlights
debug lines
debug text
picking IDs
```

### 7.6 V2 fog

Implement:

```text
linear fog
exponential fog
height fog
fog color
fog density
fog start/end
fog height
fog falloff
fog debug view
```

Fog source:

```text
scene depth + lit HDR scene color
```

Fog excludes:

```text
overlays
grid
gizmos
selection highlights
debug lines
debug text
picking IDs
```

### 7.7 V2 glass and refraction

Implement `GlassPbr`.

Data inputs:

```text
resolved scene color before glass
scene depth
normal map
roughness
transmission
IOR
thickness
thickness texture
attenuation color
attenuation distance
```

Render method:

```text
render opaque and alpha-cutout scene
resolve scene color
render transparent non-refractive materials
render glass using screen-space refraction from resolved scene color and depth
composite into HDR scene color
```

Limitations declared in UI:

```text
Screen-space refraction only sees already-rendered visible pixels.
Off-screen objects do not appear in refraction.
Objects hidden behind the first depth layer do not produce perfect refraction.
```

This is accepted for a modeling viewport.

### 7.8 V2 material and glTF extensions

V2 glTF material support adds:

```text
KHR_materials_transmission -> GlassPbr
KHR_materials_volume -> GlassPbr thickness/attenuation
KHR_materials_ior -> GlassPbr IOR
KHR_materials_emissive_strength -> emissive strength
KHR_texture_transform -> UV transform support
KHR_materials_clearcoat -> added only after GlassPbr is stable
KHR_materials_specular -> added only after GlassPbr is stable
```

V2 material system adds:

```text
base shader packages
base shader manifest files
base shader UI schema serialization
base shader parameter versioning
material migration when base shader schema changes
```

### 7.9 V2 render graph maturity

Implement:

```text
pass pruning
transient resource aliasing
resource lifetime visualization
per-pass GPU timing
per-pass CPU timing
per-pass screenshot capture
render graph JSON dump
```

### 7.10 V2 culling and batching maturity

Implement:

```text
spatial acceleration structure for render objects
incremental bounds update
light culling structure
stable draw sorting
material/state sorting
instance batch compaction
LOD hook for future use
```

Do not implement Nanite-style virtualized geometry.

### 7.11 V2 texture and asset pipeline maturity

Implement:

```text
KTX2/BasisU texture pipeline
normal map validation
metallic/roughness channel validation
mipmap generation
IBL cache invalidation
HDR import presets
texture color-space audit report
```

---

## 8. Light unit specification

### 8.1 User-facing fields

```cpp
struct DirectionalLightUi {
    Color3 color_srgb;
    float temperature_kelvin;
    float intensity_lux;
    float angular_diameter_degrees;
    bool casts_shadow;
};

struct PointLightUi {
    Color3 color_srgb;
    float temperature_kelvin;
    float luminous_flux_lumens;
    float range_meters;
    float source_radius_meters;
    bool casts_shadow; // V2
};

struct SpotLightUi {
    Color3 color_srgb;
    float temperature_kelvin;
    float luminous_flux_lumens;
    float range_meters;
    float inner_angle_degrees;
    float outer_angle_degrees;
    float source_radius_meters;
    bool casts_shadow; // V2
};
```

### 8.2 Internal light fields

```cpp
struct GpuDirectionalLight {
    Vec3 direction_ws;
    float illuminance_lux;
    Vec3 color_linear;
    float shadow_index;
};

struct GpuPunctualLight {
    Vec3 position_ws;
    float intensity_candela;
    Vec3 direction_ws;
    float range_meters;
    Vec3 color_linear;
    float outer_cos;
    float inner_cos;
    float shadow_index;
};
```

### 8.3 Unit conversion

```cpp
float point_candela_from_lumens(float lumens) {
    return lumens / (4.0f * Pi);
}

float spot_solid_angle(float outer_half_angle_radians) {
    return 2.0f * Pi * (1.0f - cos(outer_half_angle_radians));
}

float spot_candela_from_lumens(float lumens, float outer_half_angle_radians) {
    return lumens / max(spot_solid_angle(outer_half_angle_radians), 0.001f);
}
```

### 8.4 Exposure presets

V1 exposes manual exposure as the primary control.

```text
Outdoor daylight preset: EV100 15
Bright studio preset:    EV100 12
Indoor preset:           EV100 7
Night preset:            EV100 2
```

The viewport also exposes exposure compensation:

```text
-5 EV to +5 EV
```

The renderer never uses fake light multipliers to compensate for wrong exposure. Fix exposure, tone mapping, scene scale, or color space instead.

---

## 9. PBR material parameter specification

### 9.1 Opaque PBR GPU block

```cpp
struct GpuOpaquePbrMaterial {
    Vec4 base_color_factor_linear; // alpha ignored
    Vec4 emissive_factor_linear;   // rgb + strength
    float metallic_factor;
    float roughness_factor;
    float normal_scale;
    float occlusion_strength;
    Vec4 uv_transform_0;
    Vec4 flags;
};
```

Textures:

```text
s_baseColor      sRGB
s_metallicRoughness linear, G=roughness, B=metallic
s_normal         linear
s_occlusion      linear, R=AO
s_emissive       sRGB
```

### 9.2 Alpha cutout GPU block

```cpp
struct GpuAlphaCutoutPbrMaterial {
    GpuOpaquePbrMaterial pbr;
    float alpha_cutoff;
    float alpha_source_channel;
    Vec2 padding;
};
```

### 9.3 Transparent PBR GPU block

```cpp
struct GpuTransparentPbrMaterial {
    Vec4 base_color_factor_linear; // alpha used as opacity
    Vec4 emissive_factor_linear;
    float metallic_factor;
    float roughness_factor;
    float normal_scale;
    float opacity;
    Vec4 uv_transform_0;
    Vec4 flags;
};
```

### 9.4 Glass PBR GPU block in V2

```cpp
struct GpuGlassPbrMaterial {
    Vec4 base_color_factor_linear;
    Vec4 attenuation_color_linear;
    Vec4 emissive_factor_linear;
    float transmission;
    float roughness_factor;
    float normal_scale;
    float ior;
    float thickness_factor;
    float attenuation_distance_m;
    float refraction_strength;
    float opacity;
    Vec4 uv_transform_0;
    Vec4 flags;
};
```

---

## 10. Overlay and debug render specification

### 10.1 Overlay color handling

Overlay colors are stored as UI sRGB values in editor state. Before rendering, the overlay system converts them to linear SDR. The overlay shader writes linear SDR to the tone-mapped target or the final backbuffer path.

### 10.2 Overlay pass states

```text
DepthTested overlay:
  depth test: less_equal
  depth write: off
  blend: alpha
  lighting: off

XRay overlay:
  pass A depth-tested alpha
  pass B always-on-top reduced alpha
  depth write: off
  blend: alpha
  lighting: off

AlwaysOnTop overlay:
  depth test: off
  depth write: off
  blend: alpha
  lighting: off
```

### 10.3 Grid

The grid is an overlay. It is not a world mesh. It does not cast shadows. It does not receive fog. It does not enter bloom.

Grid inputs:

```text
camera position
grid plane
major line spacing
minor line spacing
fade distance
axis colors
opacity
```

Grid output:

```text
linear SDR overlay color
```

### 10.4 Selection

Selection visualization uses overlay commands and ID buffers.

V1 selection modes:

```text
object wireframe
face tint
edge highlight
vertex points
hover highlight
selected normal/tangent display
```

V2 selection modes:

```text
screen-space outline using ID mask dilation
selection silhouette
hidden-edge X-ray mode
```

### 10.5 Gizmos

Gizmos are overlay geometry. They are never lit.

V1 gizmos:

```text
translate arrows
translate planes
rotate rings as line primitives
scale handles
pivot marker
```

V2 gizmos:

```text
constant-screen-size handles
axis snapping visualization
measurement labels
angle labels
```

---

## 11. Performance rules

### 11.1 Hard rules

```text
No runtime shader compilation.
No per-frame full mesh buffer rebuild for unchanged meshes.
No per-frame glTF parsing.
No per-frame HDR environment baking.
No readback every frame for picking.
No overlay draw commands in the lighting pass.
No post effects applied to overlays.
No shader variant explosion in V1.
No material system that exposes every possible parameter on every material.
No direct rendering from the half-edge modeling mesh.
No graphics-runtime-specific code outside `src/crimson/gpu/` or future host adapters.
```

### 11.2 V1 budgets

```text
Default target: 60 FPS at 1920x1080 on a mid-range desktop GPU
Opaque visible draw packets before instancing: 10000
Instanced draws per frame target: under 2000
Point lights: 8
Spot lights: 8
Directional lights with shadow: 1
Shadow map: 2048²
HDR scene color: RGBA16F
Picking readback: click/request only
```

### 11.3 V2 budgets

```text
Clustered punctual lights: 256 visible lights
Directional cascades: 4
Spot shadow atlas: configurable, default 4096²
Point shadow cubemaps: limited by shadow budget
Bloom chain: half-res start
SSAO: half-res default
Glass refraction: half-res scene color sampling option
```

---

## 12. Agentic AI implementation discipline

The renderer is built by agents only if each task has a narrow contract.

### 12.1 Agent rules

```text
One agent owns one milestone at a time.
Each pull request touches one module family.
Each pull request includes tests or a debug view.
Each new render pass declares graph inputs and outputs.
Each new material field is added through the base shader schema.
Each new texture slot declares color-space behavior.
Each new shader is added to the shader manifest.
Each new graphics-runtime feature is validated through capability checks.
Each new overlay feature proves it bypasses lighting, fog, bloom, and tone mapping.
```

### 12.2 Forbidden agent behavior

```text
Do not add a global Material struct with every possible future parameter.
Do not add graphics-runtime includes outside `src/crimson/gpu/`.
Do not add direct glTF dependencies to the renderer core.
Do not add post-processing to overlays.
Do not solve V2 tasks while implementing V1.
Do not introduce a new shader variant without a manifest entry and a reason.
Do not change light units without updating light_unit_tests.cpp.
Do not change color-space behavior without updating color_space_tests.cpp.
Do not hide renderer warnings by lowering log levels.
```

### 12.3 V1 agent task order

```text
1. Crimson GPU platform layer and window
2. shader compile pipeline
3. color-space foundation and tone map pass
4. render scene snapshot
5. mesh upload and render mesh cache
6. base shader registry and material system
7. OpaquePbr shader
8. AlphaCutoutPbr shader and alpha-tested shadow
9. TransparentPbr shader
10. glTF import mapping
11. IBL package loader and default environment
12. directional shadow
13. point and spot light direct lighting
14. render graph pass structure
15. overlay system, grid, selection, gizmo
16. picking pass
17. instancing
18. debug views
19. golden image tests
20. performance counters
```

---

## 13. Reference scenes

V1 ships these test scenes:

```text
01_color_space_spheres
02_metal_roughness_grid
03_normal_map_tangent_test
04_alpha_cutout_foliage_card
05_transparent_planes
06_directional_shadow_test
07_point_spot_light_test
08_ibl_studio_test
09_gltf_damaged_helmet_style_test
10_overlay_grid_selection_gizmo_test
11_picking_id_test
```

Each scene has:

```text
Vulkan golden image
Direct3D golden image on Windows
Metal golden image on macOS
JSON scene description
expected frame stats range
```

---

## 14. Crucial implementation details added beyond the original request

The following items are required for a renderer that feels correct in a modeling app:

```text
MikkTSpace-compatible tangent generation
normal/tangent debug visualization
material color-space audit
stable object IDs for picking
separate render mesh cache from modeling mesh
default textures for every material slot
base shader UI schema
per-pass frame stats
shader manifest validation
shadow bias controls
environment rotation
exposure presets
post-processing exclusion mask for overlays
```

These items prevent common PBR viewport failures: wrong normal maps, double gamma, broken alpha shadows, unreadable UI overlays, unstable picking, inconsistent glTF imports, and material inspectors full of irrelevant controls.

---

## 15. Research anchors

The plan is based on these technical anchors:

- glTF 2.0 defines metallic/roughness PBR materials, base color, metallic, roughness, normal, occlusion, emissive, alpha modes, and double-sided behavior: <https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html>
- glTF PBR extensions define transmission, volume, IOR, emissive strength, clearcoat, specular, and other material capabilities: <https://www.khronos.org/gltf/pbr/>
- Godot physical light units use lux for directional lights, lumens for omni/spot lights, nits for environment/background intensity, Kelvin temperature, and camera exposure controls: <https://docs.godotengine.org/en/4.4/tutorials/3d/physical_light_and_camera_units.html>
- Unity HDRP uses physical light units such as candela, lumen, lux, nits, and EV100, and uses 1 Unity unit = 1 meter for physical light behavior: <https://docs.unity3d.com/Packages/com.unity.render-pipelines.high-definition@14.0/manual/Physical-Light-Units.html>
- Unreal physical lighting uses lux for directional lights, cd/m² for sky/emissive luminance, and candela/lumen/unitless for point/spot/rect lights with inverse-square falloff: <https://dev.epicgames.com/documentation/unreal-engine/using-physical-lighting-units-in-unreal-engine>
- LearnOpenGL is useful for implementation-level refreshers on PBR, IBL, gamma correction, HDR, bloom, SSAO, instancing, culling, and framebuffer workflows: <https://learnopengl.com/>
- Graphics Programming Compendium is useful for fundamentals: meshes, coordinate transforms, lighting, PBR, shadows, SSAO, culling, spatial data structures, color, and Cook-Torrance GGX: <https://graphicscompendium.com/>
- Filament's PBR documentation is a strong reference for practical real-time PBR math and implementation tradeoffs: <https://google.github.io/filament/Filament.md.html>
- The Frostbite FrameGraph architecture describes render passes and resources as a graph to keep rendering features modular while maintaining efficiency: <https://www.gdcvault.com/play/1024612/FrameGraph-Extensible-Rendering-Architecture-in>

---

## 16. Final mantra

```text
The modeling viewport renders believable assets, not blockbuster frames.

The material base shader owns parameters.
The render graph owns pass order.
Crimson owns GPU handles.
The overlay system owns editor visualization.
The modeler owns modeling logic.

When the scene looks wrong, fix color space, exposure, units, normals, tangents, or material mapping before touching light intensity.
```
