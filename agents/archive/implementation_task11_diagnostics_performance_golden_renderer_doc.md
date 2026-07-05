# Implementation Plan: Task #11 Diagnostics, Performance Hooks, And Golden Images

Owner: renderer architect A49
Task: project-board #11, "Mature diagnostics, performance hooks, and golden images"
Status: active renderer implementation plan
Authority: `agents/plans/audit_20260704_full_architecture_master.md`, Batch 10
Primary architecture authority: `agents/architecture/renderer.md`
Supporting authorities: `agents/architecture/architecture.md`, `agents/architecture/ui.md`, `agents/tests-policy.md`
Workspace baseline used for this plan: current files under `C:\Users\Drako\Desktop\_quader-qt-base` on 2026-07-05

This is a planning deliverable only. Do not implement code from this file until the PM assigns implementation work for #11. When implementation begins, the executor must set a Codex `/goal` for task #11 or the PM assignment before editing files. The goal is complete only after the assigned implementation scope is implemented, verified, documented when required, and handed back through PM and renderer/UI authority review.

When the implementation is complete, return to the renderer architect with this plan path, changed files, implementation notes, verification results, and known deviations. If renderer authority approves, the main/root agent should move this plan from `agents/plans/` to `agents/archive/`. Do not delete the plan automatically.

## Goal

Mature Crimson observability before optimization work starts:

- add typed renderer diagnostics snapshots suitable for UI presentation without exposing GPU/runtime internals
- expand frame counters and performance counters around graph passes, packet preparation, culling, batching, uploads, resources, picking, overlays, and diagnostics
- introduce CPU-side render culling, deterministic draw packet sorting, instancing batch keys, and dirty-upload accounting before deeper optimization
- define benchmark hooks for mesh traversal/ops and renderer upload paths without fragile timing thresholds in normal tests
- establish a stable golden-image harness strategy after #9 render graph/color/material/overlay/picking foundations, without adding brittle placeholder images
- define explicit UI architect handoff points for diagnostics panel and status presentation

This is V1 renderer maturity work. It does not introduce V2 renderer features.

## Current Workspace Findings

The current workspace already includes the #9-approved Crimson V1 correctness foundation:

- `src/crimson/renderer.hpp` exposes `Renderer::status()`, `latest_frame_stats()`, and `debug_dump_last_frame_graph()`.
- `src/crimson/renderer_diagnostics.hpp` defines structured diagnostics, capability statuses, and names.
- `src/crimson/frame/frame_stats.hpp` reports basic frame, queue, picking, resource, diagnostic, and FPS counters.
- `src/crimson/graph/render_graph.*` declares the approved V1 pass order: `FrameSetupPass`, `ResourceUploadPass`, `DepthPrepass`, `PickingPass`, `OpaquePbrPass`, `AlphaCutoutPbrPass`, `TransparentPbrPass`, `ToneMapPass`, overlay passes, and `PresentPass`.
- `src/crimson/gpu/gpu_device.cpp` owns bgfx submission and populates aggregate `FrameStats`.
- `src/crimson/gpu/gpu_frame_resources.*`, `gpu_mesh_cache.*`, `gpu_material_cache.*`, and `gpu_program_cache.*` own runtime resources inside `src/crimson/gpu/`.
- `src/crimson/gpu/gpu_pbr_pass.*` prepares PBR packets and transparent sorting, but it has no frustum culling, instancing batch keys, dirty-upload counters, or upload benchmarks yet.
- `src/ui/viewport/crimson_viewport_render_host.*` adapts Crimson into UI-facing `ViewportFrameStats`, currently only width, height, view count, and FPS.
- `src/ui/qt_app/main_window.cpp` displays compact renderer/status labels, not a diagnostics panel.
- Existing tests under `tests/unit/crimson/` cover stats/diagnostics, render graph isolation, color space, material schemas, overlays, picking, shader manifest lookup, and GPU shell behavior.

The plan must extend these contracts rather than creating a parallel diagnostics system.

## Strict Non-Goals

Do not implement these in task #11:

- no diagnostics panel UI implementation in this renderer plan
- no Qt item model, dock widget, status-bar redesign, workspace action, or panel layout implementation
- no direct UI dependency from Crimson
- no graphics-runtime headers outside `src/crimson/gpu/`
- no runtime shader compilation
- no PBR model expansion, IBL, shadows, SSAO, bloom, fog, clustered lighting, GPU-driven visibility, occlusion culling, mesh shaders, deferred rendering, ray tracing, or real-time global illumination
- no picking-driven document selection
- no topology operations or document mutation
- no direct rendering from half-edge mesh internals
- no placeholder golden images, blank PNGs, copied screenshots, timestamped captures, or unstable whole-window screenshots
- no exact frame-time pass/fail thresholds in CTest
- no benchmark executable in the default pass/fail suite unless it validates correctness or algorithmic sanity only
- no new third-party dependency unless PM and owning architect explicitly approve it

## Module Families In Scope

Renderer-owned:

- `src/crimson/`
- `src/crimson/diagnostics/` new
- `src/crimson/pipeline/` new, for CPU draw packet building/culling/sorting/batching if the executor splits current `gpu_pbr_pass` CPU logic out of `src/crimson/gpu/`
- `src/crimson/mesh/` optional, for prepared render mesh upload descriptors and revision metadata
- `src/crimson/gpu/`
- `tests/unit/crimson/`
- `tests/render_tests/` new, for golden harness support and capture/comparison tests
- `tests/golden_images/` only after approved real baselines exist; a README/metadata-only scaffold is acceptable, but no placeholder image files
- `benchmarks/` new, opt-in targets only
- `CMakeLists.txt` and `CMakePresets.json` only for test/benchmark target registration and build options

Renderer handoff to UI, no UI implementation:

- `src/ui/viewport/viewport_types.hpp`
- `src/ui/viewport/viewport_render_host.hpp`
- `src/ui/viewport/crimson_viewport_render_host.*`

Cross-domain benchmark handoff:

- `benchmarks/mesh_traversal_bench.cpp`
- `benchmarks/mesh_ops_bench.cpp`
- software architect owns any benchmark code that exercises mesh traversal/ops behavior beyond renderer upload fixtures

Out of scope:

- `src/ui/panels/*` diagnostics panel implementation
- `src/ui/models/*` diagnostics model implementation
- `src/io/*`, `assets/gltf/*`, `assets/textures/*`, and runtime asset import

## Dependency Direction

Keep the dependency graph as follows:

```text
UI viewport host
    -> public Crimson renderer/diagnostics snapshots
        -> Crimson core frame, graph, material, overlay, picking, pipeline data
            -> src/crimson/gpu runtime implementation
```

Rules:

- `src/crimson/diagnostics/**`, `src/crimson/pipeline/**`, and public Crimson headers must not include `bgfx/`, `bx/`, `bimg/`, Qt, UI, or importer headers.
- `src/crimson/gpu/**` may include graphics-runtime headers and maps project-owned handles to runtime handles.
- UI may copy public Crimson stats/diagnostics into UI-owned DTOs; UI must not include `src/crimson/gpu/**`.
- Benchmark code may link to `crimson` or mesh public modules, but benchmark hooks must not become production dependencies in lower layers.
- Golden test fixtures must consume renderer snapshots and public renderer APIs; they must not mutate document truth or depend on UI widgets.

## Public Renderer API Additions

Keep existing APIs, then add a typed diagnostics snapshot:

File: `src/crimson/renderer.hpp`

```cpp
class Renderer final {
public:
    [[nodiscard]] RendererStatus status() const;
    [[nodiscard]] std::optional<FrameStats> latest_frame_stats() const;
    [[nodiscard]] RendererDiagnosticsSnapshot diagnostics_snapshot() const;
    [[nodiscard]] std::string debug_dump_last_frame_graph() const;
};
```

New files:

```text
src/crimson/diagnostics/renderer_diagnostics_snapshot.hpp
src/crimson/diagnostics/renderer_diagnostics_snapshot.cpp
src/crimson/diagnostics/frame_profiler.hpp
src/crimson/diagnostics/frame_profiler.cpp
src/crimson/diagnostics/diagnostic_ring.hpp
src/crimson/diagnostics/diagnostic_ring.cpp
```

Core types:

```cpp
enum class RendererMetricUnit : std::uint8_t {
    Count,
    Bytes,
    Microseconds,
    Frames,
};

enum class RendererCounterDomain : std::uint8_t {
    Frame,
    Graph,
    Culling,
    Packets,
    Instancing,
    Upload,
    Resources,
    Picking,
    Overlay,
    Diagnostics,
};

struct RendererCounter {
    RendererCounterDomain domain;
    std::string name;
    std::uint64_t value = 0;
    RendererMetricUnit unit = RendererMetricUnit::Count;
};

struct RenderPassStats {
    std::string pass_name;
    std::uint32_t resource_read_count = 0;
    std::uint32_t resource_write_count = 0;
    std::uint32_t draw_call_count = 0;
    std::uint32_t draw_packet_count = 0;
    std::uint64_t cpu_time_us = 0;
};

struct RendererDiagnosticsSnapshot {
    RendererStatus status;
    std::optional<FrameStats> latest_frame_stats;
    std::vector<RenderPassStats> pass_stats;
    std::vector<RendererCounter> counters;
    std::vector<RendererDiagnostic> recent_diagnostics;
    std::string frame_graph_dump;
};
```

Lifetime:

- `RendererDiagnosticsSnapshot` is a value copy for UI and tooling.
- `RendererStatus::diagnostics` remains a public status record, but `Renderer::diagnostics_snapshot()` should use a bounded recent-diagnostic ring to prevent unbounded growth in long sessions.
- The ring buffer owns copies of `RendererDiagnostic`; it must not store pointers into GPU resources.
- `FrameStats` and pass stats are overwritten each frame. UI snapshots are copies and remain valid after the next frame.
- Diagnostics must include subsystem, resource name, and frame index where applicable.

## Expanded Frame Stats

File: `src/crimson/frame/frame_stats.hpp`

Extend `FrameStats` and supporting input structs with additive fields. Keep existing field names stable.

Required new counters:

```cpp
struct FrameCullingStats {
    std::uint32_t input_object_count = 0;
    std::uint32_t visible_object_count = 0;
    std::uint32_t culled_object_count = 0;
    std::uint32_t invalid_bounds_count = 0;
};

struct FramePacketStats {
    std::uint32_t draw_packet_count = 0;
    std::uint32_t opaque_packet_count = 0;
    std::uint32_t alpha_cutout_packet_count = 0;
    std::uint32_t transparent_packet_count = 0;
    std::uint32_t overlay_packet_count = 0;
    std::uint32_t sorted_packet_count = 0;
};

struct FrameInstancingStats {
    std::uint32_t batch_count = 0;
    std::uint32_t instanced_batch_count = 0;
    std::uint32_t single_draw_batch_count = 0;
    std::uint32_t submitted_instance_count = 0;
    std::uint32_t saved_draw_call_count = 0;
};

struct FrameUploadStats {
    std::uint32_t mesh_create_count = 0;
    std::uint32_t mesh_update_count = 0;
    std::uint32_t mesh_destroy_count = 0;
    std::uint32_t material_upload_count = 0;
    std::uint32_t texture_upload_count = 0;
    std::uint32_t skipped_clean_resource_count = 0;
    std::uint64_t uploaded_vertex_bytes = 0;
    std::uint64_t uploaded_index_bytes = 0;
    std::uint64_t uploaded_texture_bytes = 0;
};

struct FrameTimingStats {
    std::uint64_t cpu_frame_us = 0;
    std::uint64_t cpu_snapshot_validation_us = 0;
    std::uint64_t cpu_packet_build_us = 0;
    std::uint64_t cpu_resource_upload_us = 0;
    std::uint64_t cpu_submit_us = 0;
};
```

`FrameStats` should include flattened copies of the highest-value counters for status labels plus nested structs for diagnostics panels/tests. Avoid forcing the UI to parse strings.

Timing rules:

- Timing values are observability, not correctness thresholds.
- Tests assert that durations are copied and named correctly using injected/fake durations where possible.
- Do not fail tests because a local machine is slow.
- GPU timing is not required for V1 #11. Add a capability flag and empty/zero GPU timing fields only if the runtime can expose them cleanly without new architecture.

## Diagnostics Collection

Use a bounded `DiagnosticRing` in Crimson core or `GpuDevice::Impl`:

```cpp
class DiagnosticRing final {
public:
    explicit DiagnosticRing(std::size_t capacity = 256);
    void push(RendererDiagnostic diagnostic);
    [[nodiscard]] std::vector<RendererDiagnostic> snapshot() const;
    void clear() noexcept;
};
```

Integration:

- Keep existing `RendererStatus` for startup status/capabilities.
- All existing `push_diagnostic` paths in `GpuDevice`, frame resources, program cache, material cache, picking, and future upload code should go through one helper that appends to both `RendererStatus` and `DiagnosticRing`.
- Use new diagnostic codes only when useful:
  - `PerformanceCounterInvalid`
  - `GoldenCaptureUnsupported`
  - `GoldenReadbackFailed`
  - `UploadSkippedInvalidResource`
  - `BenchmarkConfigurationInvalid`
- If the executor adds codes, update `renderer_diagnostic_code_name()` and tests.

UI-facing severity mapping remains UI-owned. Renderer only reports severity and structured context.

## CPU Packet Pipeline

Current packet preparation lives in `src/crimson/gpu/gpu_pbr_pass.*`. #11 should split CPU packet work out of `src/crimson/gpu/` if the implementation touches culling, sorting, or batching substantially.

New files:

```text
src/crimson/pipeline/draw_packet.hpp
src/crimson/pipeline/draw_packet.cpp
src/crimson/pipeline/frustum_culler.hpp
src/crimson/pipeline/frustum_culler.cpp
src/crimson/pipeline/draw_packet_sort.hpp
src/crimson/pipeline/draw_packet_sort.cpp
src/crimson/pipeline/instancing_batcher.hpp
src/crimson/pipeline/instancing_batcher.cpp
```

Core data:

```cpp
struct DrawPacket {
    RenderObjectId object_id = 0;
    RenderMeshHandle mesh;
    RenderMaterialHandle material;
    BaseShaderId base_shader = BaseShaderId::OpaquePbr;
    RenderQueue queue = RenderQueue::Opaque;
    ShaderProgramId program = ShaderProgramId::OpaquePbr;
    std::array<float, 16> world_from_object = identity_transform();
    quader::math::Aabb world_bounds;
    std::uint32_t submesh_index = 0;
    float camera_distance_sq = 0.0F;
    bool double_sided = false;
};

struct DrawPacketBuildResult {
    std::vector<DrawPacket> opaque;
    std::vector<DrawPacket> alpha_cutout;
    std::vector<DrawPacket> transparent;
    FrameCullingStats culling;
    FramePacketStats packets;
};

struct InstanceBatchKey {
    RenderMeshHandle mesh;
    RenderMaterialHandle material;
    BaseShaderId base_shader;
    ShaderProgramId program;
    RenderQueue queue;
    std::uint32_t submesh_index = 0;
    bool double_sided = false;
};

struct InstanceBatch {
    InstanceBatchKey key;
    std::vector<DrawPacket> instances;
};
```

Rules:

- `DrawPacket`, `InstanceBatchKey`, and culling code use project-owned handles only.
- The CPU packet pipeline must not include graphics-runtime headers.
- Frustum culling consumes immutable `FrameSnapshot` data and camera values; it must not inspect document or mesh internals.
- Opaque and alpha-cutout packet sorting should be deterministic and state-friendly: queue, program/base shader, material, mesh, submesh, object ID.
- Transparent sorting remains back-to-front by camera distance, with object ID and submesh tie-breakers.
- Instancing keys must not include object transform or object ID. Instance data carries transform/object ID separately.
- Culling should treat invalid/non-finite bounds conservatively: keep the object visible and increment `invalid_bounds_count`.
- For the prototype cube, if bounds are not meaningful yet, use a deterministic fallback AABB and mark this clearly in code comments/tests.

GPU integration:

- `src/crimson/gpu/gpu_pbr_pass.*` should become submission-only or a thin adapter from `DrawPacket`/`InstanceBatch` to bgfx calls.
- GPU instancing can be staged:
  - first slice builds deterministic `InstanceBatchKey` and counters while still submitting one draw per packet
  - second slice writes runtime instance data and submits instanced draws where the active backend capability reports instancing
- If runtime instancing cannot be wired cleanly in #11, keep batch keys/counters and return a deviation report before claiming GPU instancing complete.

## Dirty Resource Uploads

Do not render directly from half-edge mesh. Upload APIs consume prepared renderer data only.

New/changed files:

```text
src/crimson/mesh/render_mesh.hpp
src/crimson/mesh/render_mesh_upload.hpp
src/crimson/mesh/render_mesh_cache.hpp
src/crimson/mesh/render_mesh_cache.cpp
src/crimson/gpu/gpu_mesh_cache.hpp
src/crimson/gpu/gpu_mesh_cache.cpp
```

Renderer-core descriptors:

```cpp
struct RenderMeshRevision {
    std::uint64_t geometry_version = 0;
    std::uint64_t topology_version = 0;
    std::uint64_t attribute_version = 0;
};

struct RenderMeshUploadDesc {
    RenderMeshHandle handle;
    RenderMeshRevision revision;
    std::span<const float> position_normal_interleaved;
    std::span<const std::uint32_t> indices;
    VertexAttributeMask attributes;
    quader::math::Aabb bounds;
};

struct RenderMeshUploadRecord {
    RenderMeshHandle handle;
    RenderMeshRevision uploaded_revision;
    std::uint64_t vertex_bytes = 0;
    std::uint64_t index_bytes = 0;
};
```

Rules:

- The public upload descriptor is prepared render data, not half-edge mesh data.
- GPU buffer handles remain in `src/crimson/gpu/`.
- `GpuMeshCache` updates a mesh only when its `RenderMeshRevision` changes.
- Clean resources increment `skipped_clean_resource_count`.
- Destroyed resources are released through RAII and counted in `FrameUploadStats`.
- The prototype cube can remain a Crimson-owned fixture, but it should use the same upload accounting path if touched by this task.
- Tests should validate dirty/clean revision behavior with CPU-only resource-table fakes where possible; runtime bgfx buffer creation tests must stay under `src/crimson/gpu` test targets.

## Render Graph Pass Ordering And Resources

Normal runtime pass order remains the #9-approved order:

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

#11 may add per-pass stats and profiling labels. It must not reorder lit, picking, tone-map, overlay, or present domains.

Capture-enabled graph variant for golden tests:

```text
...
11. OverlayAlwaysOnTopPass
12. GoldenCapturePass
13. PresentPass
```

`GoldenCapturePass` is optional and request-scoped:

- reads `ToneMappedColor`
- writes `GoldenCaptureColor`
- uses the same final display conversion path as `PresentPass`
- schedules readback only for an explicit golden capture request
- never runs during normal viewport rendering
- never writes `HdrSceneColor`, `SceneDepth`, `PickingId`, or document state

New resource for capture-enabled graph only:

```text
GoldenCaptureColor: RGBA8Unorm, resize dependent, readback-capable, final display-space bytes
```

If bgfx/readback support or texture flags make this capture resource unreliable, implement the comparator and harness scaffolding but do not add active golden render tests. Report the capture blocker to renderer authority instead of checking in placeholder baselines.

## Shader And Material Changes

Expected shader changes are narrow:

- no new material base shaders
- no PBR parameter expansion
- no runtime shader compilation
- no overlay changes that put overlays into HDR/lit/post paths
- optional reuse of the existing `Present` shader for `GoldenCapturePass`
- if a separate capture shader is required, add it through `shaders/manifest.json`, `build_tools/compile_shaders.py`, `build_tools/validate_shader_manifest.py`, and `tests/unit/crimson/shader_manifest_tests.cpp`

Material system changes:

- no new user-facing material fields
- stats may count live material handles and material GPU uploads
- upload counters must be driven by material revisions/dirty flags, not by arbitrary per-frame full uploads
- material debug names used in diagnostics must come from base shader/material handles and must not expose runtime handles

Color-space invariants for tests:

- overlays remain authored as UI sRGB and converted to linear SDR before compositing
- picking remains data, not color-managed
- golden capture states whether it stores final display-space sRGB bytes or linear SDR; this plan requires final display-space bytes for image comparison
- display conversion happens once for normal present and once for requested capture output, both from the same `ToneMappedColor` input

## Golden-Image Harness Strategy

Golden images are allowed only when stable and intentional. Do not add placeholder images.

New files:

```text
tests/render_tests/golden_image.hpp
tests/render_tests/golden_image.cpp
tests/render_tests/golden_image_compare_tests.cpp
tests/render_tests/render_test_scene.hpp
tests/render_tests/render_test_scene.cpp
tests/render_tests/golden_capture_tests.cpp
tests/render_tests/golden_capture_main.cpp
tests/golden_images/README.md
```

Golden harness classes:

```cpp
struct GoldenImage {
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::vector<std::uint8_t> rgba8_srgb;
};

struct GoldenTolerance {
    std::uint8_t max_channel_delta = 2;
    double max_mean_channel_delta = 0.25;
    std::uint32_t max_changed_pixels = 0;
};

struct GoldenImageDiff {
    std::uint8_t max_channel_delta = 0;
    double mean_channel_delta = 0.0;
    std::uint32_t changed_pixels = 0;
    bool passed = false;
};

class GoldenImageComparator final {
public:
    [[nodiscard]] GoldenImageDiff compare(
        const GoldenImage& actual,
        const GoldenImage& expected,
        GoldenTolerance tolerance) const;
};
```

Image file format:

- Prefer a tiny project-owned image format reader/writer for test artifacts if possible, such as uncompressed RGBA8 `.ppm`/`.pam`, to avoid a new dependency.
- If PNG is required, use a test-only adapter and keep Qt/image-codec dependencies out of the `crimson` production target. Do not add Qt includes to Crimson.
- Every baseline image must have adjacent metadata naming renderer backend, capture color space, viewport size, scene name, tolerance, and update reason.

Reference scene strategy:

- Use deterministic code fixtures in `tests/render_tests/render_test_scene.*` before adding a JSON scene parser.
- Start with no more than three approved V1 scenes:
  - `01_color_space_tone_map_grid`: red cube/grid/final color sanity
  - `02_overlay_depth_modes`: depth-tested, XRay, and always-on-top overlay separation
  - `03_picking_id_debug`: optional object ID/debug capture once debug mode is stable
- Do not include huge scenes or imported user assets.
- Do not use timestamps, random IDs, local user paths, current time, machine speed, or whole-window screenshots.

CTest policy:

- `golden_image_compare_tests` runs by default using synthetic in-memory images only. It validates the comparator and tolerance reporting.
- Golden render/capture tests are opt-in until real baselines are approved. They may be registered behind a CMake option such as `QUADER_ENABLE_RENDER_GOLDENS=ON`.
- If the option is off or no approved baseline exists, default CTest must not silently pass a fake visual test. It should run comparator/manifest checks only.
- A baseline update must be a deliberate renderer-authority action with captured images and metadata reviewed in source control.

Capture policy:

- Captures are request-scoped, like picking. No per-frame readback.
- Capture reads final display-space bytes from `GoldenCapturePass`, not UI/window screenshots.
- Capture must not affect normal `FrameStats` except explicit capture counters.
- Capture failures report `GoldenCaptureUnsupported` or `GoldenReadbackFailed` diagnostics.

## Benchmark Hooks

Tests-policy forbids fragile performance tests. Benchmarks are opt-in developer tools, not normal pass/fail tests.

New CMake option:

```cmake
option(QUADER_ENABLE_BENCHMARKS "Build Quader developer benchmark executables" OFF)
```

New files:

```text
benchmarks/benchmark_main.hpp
benchmarks/benchmark_main.cpp
benchmarks/mesh_traversal_bench.cpp
benchmarks/mesh_ops_bench.cpp
benchmarks/crimson_upload_bench.cpp
benchmarks/crimson_packet_pipeline_bench.cpp
```

Benchmark contract:

```cpp
struct BenchmarkRunConfig {
    std::uint32_t warmup_iterations = 3;
    std::uint32_t measured_iterations = 30;
    std::uint32_t fixture_size = 10000;
};

struct BenchmarkResult {
    std::string name;
    std::uint32_t iterations = 0;
    std::uint64_t median_us = 0;
    std::uint64_t min_us = 0;
    std::uint64_t max_us = 0;
    std::vector<RendererCounter> counters;
};
```

Rules:

- Use deterministic fixtures and fixed sizes.
- Print machine-readable JSONL or simple key/value output under `build/qt-mingw-debug/benchmarks/`.
- Do not fail because a run is slower than expected.
- Broad algorithmic sanity tests may remain in CTest, for example "packet building processes 10k objects without overflow and returns finite counters"; timing thresholds must not.
- Mesh traversal/ops benchmarks are software-owned once they exercise real mesh operations. Renderer #11 may add the executable scaffold and handoff, but software architect should approve mesh-operation benchmark coverage.
- Renderer upload benchmarks use prepared `RenderMeshUploadDesc` fixtures and public Crimson handles; they must not include half-edge internals or UI.

## UI Architect Handoff Points

Renderer #11 must not implement the diagnostics panel. It should define the data contracts that make the UI implementation straightforward.

Renderer-owned public data:

```text
crimson::RendererDiagnosticsSnapshot
crimson::FrameStats
crimson::RenderPassStats
crimson::RendererCounter
crimson::RendererDiagnostic
```

UI-owned adapter data:

File: `src/ui/viewport/viewport_types.hpp`

```cpp
struct ViewportRenderPassStats {
    QString pass_name;
    int draw_call_count = 0;
    int draw_packet_count = 0;
    quint64 cpu_time_us = 0;
};

struct ViewportRendererCounter {
    QString domain;
    QString name;
    quint64 value = 0;
    QString unit;
};

struct ViewportRendererDiagnosticRow {
    QString severity;
    QString code;
    QString subsystem;
    QString resource_name;
    QString message;
    QString detail;
    quint64 frame_index = 0;
};

struct ViewportDiagnosticsSnapshot {
    QString renderer_name;
    ViewportFrameStats frame;
    QVector<ViewportRenderPassStats> passes;
    QVector<ViewportRendererCounter> counters;
    QVector<ViewportRendererDiagnosticRow> diagnostics;
    QString frame_graph_dump;
};
```

Interface addition:

File: `src/ui/viewport/viewport_render_host.hpp`

```cpp
class IViewportRenderHost {
public:
    [[nodiscard]] virtual std::optional<ViewportDiagnosticsSnapshot> latest_diagnostics() const = 0;
};
```

Renderer/UI boundary rules:

- `CrimsonViewportRenderHost` maps Crimson public DTOs to UI DTOs.
- UI DTOs contain `QString`/Qt containers if useful; Crimson DTOs do not.
- Do not put `RendererDiagnosticsSnapshot` directly into a Qt item model.
- Do not expose `crimson::gpu::*` or runtime handles through `IViewportRenderHost`.
- Do not implement `PanelId::Diagnostics`, `DiagnosticsPanel`, `DiagnosticsItemModel`, `ActionId::ShowDiagnosticsPanel`, or status-bar redesign in this renderer plan.

UI architect follow-up should own:

- diagnostics panel UX
- status-bar density and summary text
- copyable diagnostics details
- model/view presentation, including `QAbstractItemModelTester`
- panel visibility action and workspace persistence
- manual UI smoke expectations

## Staged Implementation Batches

Each implementation batch is a coherent code slice. Run `cmake --build --preset qt-mingw-debug` after each slice and before starting another independent C++/CMake/shader/runtime task. In PM-coordinated batches, no more than two code tasks should pass without a successful build.

### Batch 1: Diagnostics Snapshot And Expanded Stats Contracts

Dependencies: #9 approved/In Review.

Owned files:

- `src/crimson/frame/frame_stats.*`
- `src/crimson/renderer_diagnostics.hpp`
- `src/crimson/diagnostics/*`
- `src/crimson/renderer.*`
- `src/crimson/gpu/gpu_device.*`
- `tests/unit/crimson/frame_stats_diagnostics_tests.cpp`
- CMake entries for new files

Deliver:

- `RendererDiagnosticsSnapshot`
- `DiagnosticRing`
- expanded `FrameStats` inputs/counters
- per-pass stats DTOs copied from graph/pass execution data
- diagnostic code/name updates if new codes are added
- tests for stats copying, saturating counts, diagnostic ring capacity/order, stable code names, and no unstructured diagnostics on new paths

Verification:

```powershell
cmake --build --preset qt-mingw-debug
ctest --preset qt-mingw-debug
cmake --build --preset qt-mingw-debug --target check_architecture
```

### Batch 2: CPU Packet Pipeline, Frustum Culling, Sorting, And Instancing Keys

Dependencies: Batch 1 stats contracts.

Owned files:

- `src/crimson/pipeline/*`
- `src/crimson/gpu/gpu_pbr_pass.*`
- `src/crimson/gpu/gpu_device.cpp`
- `src/crimson/scene/render_object.hpp` only if bounds or metadata need small additive fields
- `tests/unit/crimson/render_packet_pipeline_tests.cpp` new
- `tests/unit/crimson/frame_stats_diagnostics_tests.cpp`
- CMake entries

Deliver:

- CPU packet builder outside runtime-specific code or with CPU-only logic cleanly separated
- frustum culling from immutable `RenderCamera` and `RenderObject::world_bounds`
- deterministic opaque/alpha-cutout/transparent sorting
- `InstanceBatchKey` and batch construction
- stats for input, visible, culled, sorted, batched, instanced, and saved draws
- no change to #9 graph domain ordering

Tests:

- culling keeps inside/intersecting objects and rejects outside objects
- invalid/non-finite bounds are kept and counted
- opaque sort is deterministic for repeated input
- transparent sort is back-to-front with stable tie-breakers
- instancing key groups same mesh/material/base shader/program/state and does not include transform/object ID
- overlay and picking queues do not enter PBR instancing batches

Verification:

```powershell
cmake --build --preset qt-mingw-debug
ctest --preset qt-mingw-debug
cmake --build --preset qt-mingw-debug --target check_architecture
```

### Batch 3: Dirty Upload Accounting And Renderer Upload Hooks

Dependencies: Batch 1 stats; Batch 2 packet counters helpful but not required.

Owned files:

- `src/crimson/mesh/render_mesh*` new
- `src/crimson/gpu/gpu_mesh_cache.*`
- `src/crimson/gpu/gpu_material_cache.*`
- `src/crimson/gpu/gpu_device.cpp`
- `tests/unit/crimson/render_upload_tests.cpp` new
- `tests/unit/crimson/gpu_resource_tests.cpp`
- `benchmarks/crimson_upload_bench.cpp` scaffold if benchmarks option lands in this batch
- CMake entries

Deliver:

- prepared render mesh upload descriptor and revision metadata
- dirty/clean upload accounting for mesh buffers
- material/texture upload counters where current material cache can report them honestly
- skipped-clean counters
- no half-edge mesh dependency in upload APIs
- benchmark fixture path for repeated upload/update/no-op cases

Tests:

- same revision skips upload and increments clean-skip counter
- changed revision schedules update and records byte counts
- destroyed handle does not resolve after generation changes
- upload descriptors reject invalid spans/bounds through structured diagnostics

Verification:

```powershell
cmake --build --preset qt-mingw-debug
ctest --preset qt-mingw-debug
cmake --build --preset qt-mingw-debug --target check_architecture
```

### Batch 4: Viewport Diagnostics Handoff Contract

Dependencies: Batch 1 diagnostics snapshot.

Owned files:

- `src/ui/viewport/viewport_types.hpp`
- `src/ui/viewport/viewport_render_host.hpp`
- `src/ui/viewport/crimson_viewport_render_host.*`
- `tests/unit/ui/viewport_tests.cpp`

Deliver:

- UI-owned `ViewportDiagnosticsSnapshot` DTOs
- `IViewportRenderHost::latest_diagnostics()`
- adapter mapping from Crimson public diagnostics to UI DTOs
- existing status label behavior preserved
- no diagnostics panel, no new panel ID, no UI model, no dock widget implementation

Tests:

- fake or real viewport host can expose diagnostics snapshot without GPU runtime handles
- mapping preserves severity/code/subsystem/resource/frame index
- `ViewportFrameStats` remains backward-compatible for current status label

Verification:

```powershell
cmake --build --preset qt-mingw-debug
ctest --preset qt-mingw-debug
cmake --build --preset qt-mingw-debug --target check_architecture
```

UI authority review is required before any later implementation adds a diagnostics panel or changes status presentation beyond this DTO handoff.

### Batch 5: Golden Image Harness And Optional Capture Path

Dependencies: Batches 1 and 2; #9 color/material/overlay/picking graph foundations remain required.

Owned files:

- `tests/render_tests/*`
- `tests/golden_images/README.md`
- `src/crimson/graph/render_graph.*`
- `src/crimson/frame/frame_targets.*`
- `src/crimson/gpu/gpu_frame_resources.*`
- `src/crimson/gpu/gpu_device.*`
- `src/crimson/renderer.*`
- `tests/unit/crimson/render_graph_correctness_tests.cpp`
- `tests/unit/crimson/color_space_tests.cpp`
- CMake entries/options

Deliver:

- golden image comparator with deterministic synthetic tests
- reference scene builders as code fixtures
- optional capture-enabled graph variant with `GoldenCapturePass`
- request-scoped capture/readback path if runtime support is reliable
- no active golden baseline tests until real baselines are approved
- no placeholder images

Tests:

- comparator reports max/mean delta and changed pixels deterministically
- mismatch dimensions fail with useful diagnostics
- capture graph preserves normal graph order and inserts `GoldenCapturePass` before `PresentPass` only when requested
- capture pass reads `ToneMappedColor` and never touches `HdrSceneColor`, `PickingId`, or document state
- color-space test asserts capture output is final display-space bytes

Verification:

```powershell
cmake --build --preset qt-mingw-debug
ctest --preset qt-mingw-debug
cmake --build --preset qt-mingw-debug --target check_architecture
```

If capture behavior changes user-visible runtime behavior or deployed runtime files:

```powershell
cmake --build --preset qt-mingw-debug --target deploy
```

### Batch 6: Benchmark Targets And Integration Cleanup

Dependencies: Batches 2 and 3 for renderer benchmarks. Software architect input required before claiming mesh traversal/ops benchmark coverage complete.

Owned files:

- `benchmarks/*`
- `CMakeLists.txt`
- `CMakePresets.json` only if needed for opt-in benchmark configuration
- `README.md` or `dev-changelog.md` only if benchmark commands become durable workflow docs

Deliver:

- `QUADER_ENABLE_BENCHMARKS` option, default `OFF`
- renderer upload benchmark
- renderer packet/culling/batching benchmark
- mesh traversal/ops benchmark scaffold or software-owned handoff note
- JSONL or stable key/value output
- no benchmark added to default CTest pass/fail suite
- documentation of command line and output location

Verification:

```powershell
cmake --preset qt-mingw-debug
cmake --build --preset qt-mingw-debug
ctest --preset qt-mingw-debug
cmake --build --preset qt-mingw-debug --target check_architecture
```

Optional benchmark build verification:

```powershell
cmake --preset qt-mingw-debug -DQUADER_ENABLE_BENCHMARKS=ON
cmake --build --preset qt-mingw-debug
```

Do not require benchmark timing numbers for task acceptance.

## Required Tests And Verification

Tests-policy applies to all planned tests, golden images, fixtures, benchmark hooks, and manual verification substitutes.

Required automated tests:

- `frame_stats_diagnostics_tests.cpp`
  - expanded stats copy correctly
  - totals use saturating arithmetic where needed
  - diagnostic ring caps and preserves newest entries
  - new diagnostic names are stable
- `render_packet_pipeline_tests.cpp`
  - frustum culling behavior
  - invalid bounds policy
  - deterministic sort keys
  - instancing batch key equality/inequality
  - overlay/picking queues stay out of lit batching
- `render_upload_tests.cpp`
  - dirty revision upload
  - clean revision skip
  - upload byte counters
  - invalid upload diagnostics
- `render_graph_correctness_tests.cpp`
  - normal #9 pass order unchanged
  - optional capture graph adds only `GoldenCapturePass`
  - capture pass resource isolation
- `golden_image_compare_tests.cpp`
  - comparator exact match
  - controlled deltas
  - dimension mismatch
  - readable diff stats
- `viewport_tests.cpp`
  - UI diagnostics DTO mapping without GPU internals

Required commands before renderer authority review:

```powershell
cmake --preset qt-mingw-debug
cmake --build --preset qt-mingw-debug
ctest --preset qt-mingw-debug
cmake --build --preset qt-mingw-debug --target check_architecture
python tools/project_board.py validate
```

If user-visible runtime behavior, runtime files, capture path, shader deployment, or viewport status behavior changes:

```powershell
cmake --build --preset qt-mingw-debug --target deploy
```

Manual verification is allowed only for behavior that cannot yet be validated automatically. If used, report exact steps and what was observed. Manual verification must not replace the deterministic unit tests above.

## Documentation Expectations

Update durable docs only when behavior or commands become part of workflow:

- `dev-changelog.md`: record durable diagnostics, benchmark, golden harness, or build/test target additions.
- `README.md`: update only if user/developer commands change in a way normal contributors need.
- `AGENTS.md`: update only if standard commands, architecture guardrails, or task workflow change.

Do not update user-facing `changelog.md` for internal diagnostics, benchmark scaffolding, or golden harness infrastructure during ordinary implementation.

## Authority Review Points

Return to renderer architect if:

- capture/readback cannot be implemented without weakening color-space or graph boundaries
- GPU instancing requires runtime-specific handles outside `src/crimson/gpu/`
- dirty upload hooks require direct half-edge mesh access
- benchmark work needs timing thresholds in CTest
- the UI handoff requires panel/model/status implementation beyond DTO mapping
- golden baselines would be unstable or machine-specific
- implementation discovers a design gap not covered by this plan

UI architect review is required before:

- adding a diagnostics panel
- adding diagnostics panel actions or workspace persistence
- changing `MainWindow` status presentation beyond the existing label data shape
- adding Qt models for diagnostics

Software architect review is required before:

- claiming mesh traversal/ops benchmarks complete
- adding shared performance infrastructure under `src/foundation/`
- changing mesh/document/tool dependency direction for benchmark hooks

Renderer authority approval is required before #11 moves to `In Review`.

## Residual Risks

- Current bgfx readback support may not be sufficient for stable final-color capture on every backend. If capture is unreliable, keep comparator/scaffold only and report a blocker instead of adding fake goldens.
- The current renderer has only prototype render mesh data. Upload benchmarks can use prepared fixtures, but document-driven render mesh extraction remains outside #11.
- CPU timings are useful for local diagnostics but are not reproducible enough for pass/fail tests.
- UI diagnostics panel scope is deliberately deferred. The handoff DTO may need UI architect refinement before panel implementation.
- Mesh traversal/ops benchmark ownership is cross-domain; renderer should not design mesh algorithms or benchmark mesh internals without software authority.
- Golden images become useful only after stable capture, approved baselines, and backend-specific metadata exist.
