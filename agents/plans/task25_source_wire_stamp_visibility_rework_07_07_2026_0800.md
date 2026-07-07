# Task 25 Source-Wire Stamp Visibility Rework

Timestamp: 07_07_2026_0800

## Scope

Implement the delegated overlay-system port checklist for the active task #25 rework, focused on source-wire depth-stamp consumption, editable vertex visibility/depth override, CPU component-picking parity, and wireframe viewport x-ray parity.

## Plan

1. Add a Crimson overlay-side `SourceWireDepthStampVisibilityFilter` that consumes prepared `SourceWireDepthStamp` metadata, groups triangles by view/object, detects inside-out stamp volumes, culls outward same-mesh occluded editable vertex points, and overrides inside-out/camera-inside editable vertex points to `AlwaysOnTop`.
2. Use the filter in `GpuDevice` point submission per view without mutating prepared overlay lists, splitting editable vertex point batches by effective depth mode while leaving non-editable point overlays alone.
3. Reuse the filter in CPU component handle picking so outside hidden handles remain rejected and inside inverted-room handles remain pickable; keep face picking two-sided.
4. Fix wireframe viewport mode so scene topology is x-ray/draw-on-top and filled mesh render objects are not submitted as hidden depth proxies in wireframe mode.
5. Add focused Crimson overlay unit tests for stamp visibility/depth-mode policy and run targeted build/tests allowed by the project workflow.

## Non-Goals

- Do not render `SourceWireDepthStamp` metadata.
- Do not rewrite the broader overlay style resolver in this rework.
- Do not change shader routing for component edit-wire lines beyond the existing sampled scene-depth path.
