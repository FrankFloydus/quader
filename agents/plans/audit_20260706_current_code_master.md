# Current Code Audit and Beautify Pass - 2026-07-06

## Scope

- Repository: `C:\Users\Drako\Desktop\_quader-qt-base`
- Branch/worktree state at audit start: clean
- Focus: current architecture boundaries, Qt model behavior, Crimson viewport/render integration, tests, and beautify/naming debt
- Required artifact: this file under `agents/plans/`

## References Read

- `AGENTS.md`
- `agents/workflow.md`
- `agents/task_board.md`
- `agents/tests-policy.md`
- `agents/documentation-policy.md`
- `agents/hindsight.md`
- `agents/code-style.md`
- `agents/architecture/architecture.md`
- `agents/architecture/ui.md`
- `agents/architecture/renderer.md`
- `agents/architecture/reference/architecture-full.md`
- `agents/architecture/reference/ui-full.md`
- `agents/architecture/reference/renderer-full.md`
- `agents/reference/tests-policy-full.md`
- `agents/reference/documentation-policy-full.md`

## Current Strengths

- Architecture boundaries validate with `python tools/check_architecture.py --root .`.
- Board metadata validates with `python tools/project_board.py validate`.
- Runtime CTest passes with the safe preset: 313/313 tests passed using `ctest --preset qt-mingw-debug-runtime`.
- Qt dependencies remain confined to app/UI boundaries; core document, mesh, tools, and Crimson layers are Qt-free.
- bgfx/bimg/bx dependencies remain confined to `src/crimson/gpu`.
- The safe runtime preset excludes style/static-analysis labels, matching the repo policy that style and static analysis require immediate explicit permission.
- Several older findings are already fixed:
  - Command merge semantics are tested and used by command history.
  - Diagnostics model refreshes same-topology data with `dataChanged` instead of resetting.
  - `MeshAttributes` invalid-id lookups now preserve invalid-id diagnostics.
  - Runtime GPU mesh uploads skip clean revisions in `GpuMeshCache`.
  - Public renderer-facing enum names now use viewport/crimson terminology instead of prototype terminology.

## Architecture Audit Findings

### A1. Viewport document mesh preparation still performs full CPU work for clean meshes

Severity: P2

Evidence:

- `src/ui/viewport/crimson_viewport_render_host.cpp` computes a mesh revision by walking every vertex and face in `revision_for_mesh`.
- `src/ui/viewport/crimson_viewport_render_host.cpp` builds a fresh `RenderMeshUpload` in `make_mesh_upload`, including bounds, normals, generated UVs, triangle indices, and vectors.
- `CrimsonViewportRenderHost::append_document_render_data` calls `make_crimson_viewport_mesh_upload(*object)` for every document object on every rendered frame.
- `src/crimson/gpu/gpu_mesh_cache.cpp` can skip clean GPU uploads, but that decision happens after the UI adapter has already rebuilt and copied the upload payload.

Impact:

Clean frames still pay mesh traversal, triangulation, vector allocation, and snapshot copy cost before the GPU cache can skip unchanged resources. Diagnostics may report clean GPU skips while the UI thread still does meaningful CPU work. This creates avoidable frame-time risk as object count or topology size grows.

Required fix:

Add a UI-side document render mesh cache keyed by document object id and render revision. It should submit an upload only when the cached revision is missing or stale, keep enough cached metadata to emit render objects on clean frames, prune deleted objects, and clear when the renderer surface is recreated or destroyed.

### A2. Rich document change kinds are flattened before property models can make precise updates

Severity: P2

Evidence:

- `src/document/document_events.hpp` exposes typed changes such as `ObjectRenamed`, `TransformChanged`, `MeshTopologyChanged`, `MeshGeometryChanged`, and `SelectionChanged`.
- `src/ui/services/document_ui_controller.cpp` maps object rename, transform, topology, and geometry changes into the same `object_changed(ObjectId)` signal.
- `src/ui/models/property_item_model.cpp` connects that coarse signal to `refresh_object`.
- `PropertyItemModel::refresh_object` rebuilds the whole descriptor model whenever the selected object or a referenced object changes.
- `src/app/application.cpp` drains preview transform changes with `document_ui.refresh_from_document()`, so live transform previews can trigger property model resets repeatedly.

Impact:

The model loses the change kind needed to update value cells without resetting. During live transform preview or gizmo interaction, this can disturb edit focus and table selection and it does more UI model work than necessary.

Required fix:

Preserve the typed `DocumentChange` through `DocumentUiController` with an additional signal while retaining existing coarse signals for compatibility. Update `PropertyItemModel` to use the typed change and refresh same-topology descriptors with `dataChanged` instead of a model reset for selected-object rename/transform/geometry value updates.

## Beautify Findings

### B1. Prototype-era cube/grid shader and fallback paths remain after document rendering took over

Severity: P3

Evidence:

- `shaders/manifest.json` still uses `PrototypeLitCube` and `PrototypeGridOverlay` ids.
- The runtime still exposes `ViewportLitCube` and `ViewportGridOverlay` shader ids even though the live runtime loads PBR, overlay, picking, and post shaders instead.
- `src/crimson/gpu/gpu_mesh_cache.cpp` still owns a built-in unit-box mesh path for a prototype cube fallback.
- `src/ui/actions/action_id.hpp` still exposes an inert `CreateCube` menu action while the Box tool is the real box-creation workflow.
- `CMakeLists.txt` and shader tests still reference root shader source names `vs_cube.sc`, `fs_cube.sc`, `vs_grid.sc`, and `fs_grid.sc`.

Impact:

This is mostly cleanup debt, but it obscures the now-settled public renderer and modeling workflow. The repo policy says the renderer is `crimson` and public planned names should avoid graphics-runtime/prototype-era names.

Required fix:

Remove unused prototype cube/grid programs and cube fallback paths, keep the live grid overlay on the `OverlayUnlit` shader path, rename the live overlay source files, update CMake shader source lists, update runtime shader lookup, and update tests.

## Verification Already Run

```powershell
python tools/project_board.py validate
python tools/check_architecture.py --root .
ctest --preset qt-mingw-debug-runtime
```

Result: all passed before implementation.

## Implementation Notes

- A1 and A2 are runtime/UI C++ changes and require build plus targeted runtime tests.
- B1 changes shader source names and compiled shader output names, so the shader compile path and deploy preset must be exercised.
- Because this audit has no pre-existing task id, implementation must create a board task first, create exactly one implementation plan for that task, then stop at `In Review`.
