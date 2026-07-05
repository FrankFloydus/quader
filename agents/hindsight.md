# Critical Hindsight Register

This register records durable lessons that future agents should not need to
rediscover. It is not a changelog, task board, plan archive, or historical
narrative. Keep entries short, current-facing, and backed by evidence.

## When To Record

Record only reusable critical knowledge that prevents likely repeated mistakes:

- hidden build, deploy, tooling, or verification traps;
- parity/reference mismatches and adaptation rules;
- renderer math, coordinate-space, shader, or resource-lifetime conventions;
- workflow holes, stale assumptions, and rejected shortcuts;
- non-obvious architecture constraints or ownership invariants;
- licensing/reuse constraints for reference material;
- review findings that expose a repeatable failure mode.

Do not record normal progress, generic best practices, one-off bugs, ordinary
changelog items, active task todos, or plan content that belongs in
`agents/plans/`.

## Hindsight Gate

Before intake, task revalidation, implementation, audit/beautify, deviation
review, bug review, final review, documentation maintenance, or performance
work, search/read this file for entries matching the relevant area. Reference
matching IDs in reports, plans, task briefs, or review decisions only when they
change or constrain the work. If no entry applies, do not add noisy boilerplate.

## Candidate Flow

Workers, scouts, architects, docs maintainers, and performance agents must not
bury critical lessons only in final prose. When they discover reusable critical
knowledge, they add this block to their report:

```text
Hindsight Candidate:
Area/tags:
Lesson:
Evidence:
Applies when:
```

Root routes candidates to the owning architect for acceptance. Root or
`quader-docs-mantainer` records only accepted entries. Accepted entries must
include concrete evidence: file paths, line ranges, docs, task ids, archived task
ids, plans, or external links.

## Entry Template

Copy this exact shape for new accepted entries:

```text
### H-YYYYMMDD-NNN - Short title

- Area/tags: area, tag
- Status: active | superseded
- Owner: software | core | build-workflow | ui | renderer | docs | performance
- Applies when: Short trigger condition.
- Hindsight: One or two sentences with the reusable lesson.
- Evidence: `path/to/file.ext:line`, task/archive id, plan path, or URL.
- Last checked: YYYY-MM-DD
- Review when: Condition that could make this stale.
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
