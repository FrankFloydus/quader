# Crimson Renderer Architecture Brief

Lean first-read for renderer work. The preserved full reference is `agents/architecture/reference/renderer-full.md`; read it for broad renderer audits, material/pass/graph design, or major Crimson implementation work.

Renderer name: `crimson`. Do not use graphics-runtime names in renderer folders, targets, public planned class names, or user-facing architecture names.

## Renderer Stance

Crimson is a modeling viewport renderer, not a game renderer or full engine. It helps users inspect modeling correctness: lighting, normals, UVs, materials, transparency, grid, gizmos, overlays, picking, diagnostics.

Permanent rules:

- Linear HDR lighting first; display conversion last.
- Materials are instances of base shaders and expose only that base shader's parameters.
- Lit geometry, transparent geometry, post effects, overlays, grid, gizmos, selection, and debug rendering are separate domains.
- Editor overlays are unlit and must not receive fog, bloom, SSAO, tone mapping, or PBR lighting.
- Crimson consumes prepared render data/snapshots and never owns modeling logic.

## Boundaries

- Graphics-runtime details live under `src/crimson/gpu/`.
- No modeling, material authoring, UI, tool, document, command, or I/O code includes graphics-runtime headers.
- Outside the GPU layer, use Quader/Crimson-owned handles and data packets.
- Renderer-facing scene data is snapshot-style and CPU-owned until submitted.
- Picking/selection must preserve semantic IDs; do not infer modeling truth from draw order alone.

## Current Domains

- `crimson/frame`: frame snapshots and renderer-facing scene packets.
- `crimson/gpu`: device, resources, shader loading, submission.
- `crimson/material`: base shader/material packet definitions.
- `crimson/overlays`: overlay commands, layers, semantic metadata.
- `crimson/picking`: picking IDs/results and GPU/CPU picking coordination.
- `crimson/pipeline`: draw packets and render packet pipeline.
- `ui/viewport`: Qt viewport host, camera/input/controller bridge.
- `shaders/`: bgfx shader sources and manifest; runtime detail, not naming policy.

## Renderer Math And Parity

Before renderer parity work, read matching entries in `agents/hindsight.md`.

When `C:\Users\Drako\Desktop\quader-windows\quader-app` is cited, inspect its renderer internals before deciding projection, picking, overlays, shader/material, coordinate-space, or viewport math. Treat that app as source of truth for behavior and user-visible substance; adapt only necessary implementation details for Crimson/Qt/current architecture/licensing/tests.

For implementation-facing renderer work, scan targeted folders under `C:\Users\Drako\Desktop\_SOURCES` for useful patterns and cite file/line references. Treat reference code as reference only unless reuse is explicitly safe.

## External Graphics References

For renderer/graphics tasks, browse/read relevant pages when possible, starting from:

- `https://graphicscompendium.com/index.html`
- `https://learnopengl.com/Introduction`

Use them for concepts, not as license-free source code.

## Hard Rules

- Runtime shader compilation is forbidden for normal runtime.
- sRGB/linear handling must be explicit: base/emissive color textures are sRGB, data maps are linear, final conversion happens once.
- Overlay colors are authored as UI colors and converted appropriately for final output.
- Do not fix geometry/winding defects by changing culling or making materials double-sided unless that is the explicit material behavior.
- Camera projection, viewport rays, GPU picking, and visible Crimson scene must share one convention.
- Overlay parity may require semantic metadata, not just layer order.
- GPU handles/resources must not leak into document, command, tool, or UI public APIs.

## Review Checklist

- Crimson naming is clean and backend-neutral outside `gpu`.
- Render data boundary is snapshot/packet based.
- GPU lifetime/resource ownership is isolated.
- Selection/picking preserves semantic IDs and source kind.
- Color-space and alpha-mode behavior is intentional.
- Overlay and debug draw remain unlit/editor-domain.
- Renderer tests or golden/smoke checks cover user-visible behavior where practical.

## Read Full Reference When

Read `agents/architecture/reference/renderer-full.md` when:

- auditing Crimson broadly;
- changing render graph, material/base shader system, pass order, GPU layer, shader pipeline, light/color model, overlay policy, picking, or debug views;
- writing renderer master plans;
- resolving details not covered by this brief.
