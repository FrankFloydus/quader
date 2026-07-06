# Critical Hindsight Register

Durable lessons future agents should not rediscover. This is not a changelog, task board, plan archive, or todo list.

Read/search relevant entries before intake, revalidation, execution, audit/beautify, review, docs, performance, or deviation handling. Cite matching IDs only when they affect the work.

Record only reusable traps/invariants: build/tool traps, parity mismatches, renderer math conventions, workflow holes, stale assumptions, ownership constraints, licensing/reuse constraints, or repeated review failures.

Do not record normal progress, generic best practices, one-off bugs, changelog items, active todos, or plan content.

Candidate format:

```text
Hindsight Candidate:
Area/tags:
Lesson:
Evidence:
Applies when:
```

Accepted entries require evidence and use:

```text
### H-YYYYMMDD-NNN - Short title
- Area/tags:
- Status: active | superseded
- Owner: software | core | build-workflow | ui | renderer | docs | performance
- Applies when:
- Hindsight:
- Evidence:
- Last checked: YYYY-MM-DD
- Review when:
```

## Accepted Entries

### H-20260705-001 - Tool rays must share Crimson camera projection

- Area/tags: renderer, viewport, picking, coordinate-space, reference-parity
- Status: active
- Owner: renderer
- Applies when: Implementing or reviewing viewport tool rays, GPU picking, pointer previews, or projection/unprojection logic for Crimson views.
- Hindsight: The rendered Crimson view, GPU picking, and viewport tools must derive rays from one shared camera/view-projection convention. Do not ship UI-only hand ray math or fix mirrored previews by flipping `right` or NDC X in isolation; prove the ray helper matches the matrices passed to Crimson.
- Evidence: `src/ui/viewport/viewport_controller.cpp:392`, `src/crimson/gpu/gpu_device.cpp:416`, `src/crimson/gpu/gpu_picking.cpp:47`, `C:\Users\Drako\Desktop\quader-windows\quader-app\src\render\viewport_renderer.cpp:244`, `C:\Users\Drako\Desktop\quader-windows\quader-app\src\editor\interactions\tool_viewport_projection.cpp:95`
- Last checked: 2026-07-05
- Review when: A shared Crimson projection/unprojection helper feeds scene rendering, GPU picking, and UI tool rays, or Crimson changes backend matrix/depth conventions.

### H-20260705-002 - Reference box preview plane basis has a signed parity rule

- Area/tags: renderer, tools, overlays, box-tool, reference-parity
- Status: active
- Owner: renderer
- Applies when: Implementing or reviewing box footprint previews, construction-plane helpers, overlay corner order, or parity with the Filament reference app.
- Hindsight: The reference app builds the box construction plane with `v = cross(normal, u)` and emits footprint corners as `{start, start + v, start + u + v, start + u}`. Reversing that sign or order can look like a renderer mirror even when the camera ray is correct, so parity work must check the plane basis and corner sequence separately from projection math.
- Evidence: `src/tools/box_tool.cpp:123`, `src/tools/box_tool.cpp:159`, `C:\Users\Drako\Desktop\quader-windows\quader-app\src\mesh\geometry\geometry.hpp:976`, `C:\Users\Drako\Desktop\quader-windows\quader-app\src\editor\interactions\box_tool.cpp:230`
- Last checked: 2026-07-05
- Review when: Box-tool footprint generation is replaced, construction-plane ownership moves out of the tool, or the reference app parity target changes.

### H-20260705-003 - Box corner builders must orient faces outward

- Area/tags: renderer, tools, box-tool, winding, normals, reference-parity
- Status: active
- Owner: renderer
- Applies when: Implementing or reviewing box builders, primitive builders, or committed Box tool meshes that consume caller-provided or signed drag corners.
- Hindsight: Reference footprint order alone is not enough; any builder that consumes signed box corners must orient each quad outward against the box center before creating faces. Do not mask bad box winding in Crimson with double-sided materials, cull-state changes, or lighting hacks because the render upload derives flat normals from face winding.
- Evidence: `src/mesh/ops/box_builder.cpp:120`, `src/mesh/ops/box_builder.cpp:141`, `src/ui/viewport/crimson_viewport_render_host.cpp:269`, `shaders/pbr/opaque_pbr.fs.sc:13`, `C:\Users\Drako\Desktop\quader-windows\quader-app\src\editor\modeling\runtime\session.cpp:174`, `C:\Users\Drako\Desktop\quader-windows\quader-app\src\editor\modeling\runtime\session.cpp:2046`, `C:\Users\Drako\Desktop\quader-windows\quader-app\tests\modeling_domain\tool\qdr_modeling_runtime_tool_box_tests.cpp:273`
- Last checked: 2026-07-05
- Review when: Box mesh construction moves to a centralized primitive builder, face-winding canonicalization becomes a mesh invariant, or Crimson changes document-mesh normal generation.

### H-20260705-004 - Component source-wire ownership is mode-specific

- Area/tags: UI, renderer, selection, overlays, reference-parity, sticky-selection
- Status: active
- Owner: software
- Applies when: Implementing or reviewing mesh/component selection overlays, sticky active edit mesh behavior, hover replacement, component-mode transitions, or source-wire regressions.
- Hindsight: Component-mode source-wire ownership is mode-specific. A selected face makes source wire sticky only in face mode, a selected edge only in edge mode, and a selected vertex only in vertex mode; object selection alone is only a fallback when there is no mode-matching selected component or hover candidate.
- Evidence: `C:\Users\Drako\Desktop\quader-windows\quader-app\docs\overlay-handling.md:83`, `C:\Users\Drako\Desktop\quader-windows\quader-app\docs\overlay-handling.md:267`, `C:\Users\Drako\Desktop\quader-windows\quader-app\src\editor\modeling\runtime\session_selection_overlay.cpp:401`, `C:\Users\Drako\Desktop\quader-windows\quader-app\src\editor\modeling\runtime\session_selection_overlay.cpp:450`, `C:\Users\Drako\Desktop\quader-windows\quader-app\tests\modeling_domain\qdr_modeling_runtime_overlay_tests.cpp:550`
- Last checked: 2026-07-05
- Review when: Selection mode ownership, source-wire fallback policy, or the Windows reference parity target changes.

### H-20260705-005 - Source-wire parity needs semantic depth-stamp metadata

- Area/tags: renderer, selection-overlays, picking, inverted-normals, reference-parity, source-wire
- Status: active
- Owner: renderer
- Applies when: Planning or reviewing Crimson selection overlays, component handles, source wire, inverted-normal validation, or future gizmo/diagnostic overlay features that need layer/source/depth semantics.
- Hindsight: Source-wire parity is not an overlay layer toggle. It requires semantic source kind plus metadata-only depth stamps; non-component `SourceWire` lines and source vertices draw on top and must not be CPU-culled through depth stamps. Component-mode source/selected/hover edge lines are edit-wire overlays: their bucket and render state are depth-tested when they carry component source. In the Qt viewport, component source/selected/hover vertex handles must also be depth-tested so occluded edit handles do not show through room geometry. Generic and edit-wire overlay lines are device-space quads with feathered coverage; component edit wires use their dedicated edit-wire depth bias and must not inherit generic line bias. Selected/hover face fills still need depth-stamped two-pass, two-sided rendering.
- Evidence: `C:\Users\Drako\Desktop\quader-windows\quader-app\docs\overlay-handling.md:132`, `C:\Users\Drako\Desktop\quader-windows\quader-app\src\render\viewport_overlay_policy.cpp:73`, `C:\Users\Drako\Desktop\quader-windows\quader-app\tests\render_extraction_tests.cpp:1413`, `docs/windows-overlay-parity-reference.md`, `src/crimson/overlays/overlay_system.cpp:385`, `src/crimson/gpu/gpu_device.cpp:614`
- Last checked: 2026-07-06
- Review when: Crimson gains a semantic selection-overlay frame with depth-stamp metadata and parity tests for flipped/inverted mesh overlays, or the Windows reference parity target changes.
