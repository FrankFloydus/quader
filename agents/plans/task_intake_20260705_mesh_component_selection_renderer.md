# Task Intake Domain Findings - Mesh And Component Selection Renderer

## Domain Classification

- Domain: Crimson renderer, viewport overlays, picking, render snapshots, renderer-facing selection overlay extraction.
- Classification: new cross-domain enhancement task, not a bug.
- Priority recommendation: high. The user called the implementation critical and the acceptance criteria cover core editor selection UX plus renderer parity behavior.
- Area recommendation: viewport/crimson/selection.
- Recommended title: Add mesh and component selection with Crimson parity overlays and flip mesh normals.
- Final owner recommendation: multi-domain task with `quader-software-architect` final owner. Renderer authority owns Crimson overlay, picking, render-snapshot, shader/pass, and renderer verification. Core owns document selection semantics and flip mesh normals. UI owns mode controls, actions, shortcuts, and viewport input.

## Summary

Implement object mesh selection and vertex/edge/face component selection with select, deselect, select-all, inverse-select, additive/removal multi-selection across meshes, sticky component edit-mesh policy, and reference-parity overlays. Add the complementary flip mesh normals operation equivalent to the reference `flip_face_normals`/`invert_mesh` behavior.

This is not a small overlay color patch. The reference behavior depends on semantic overlay source roles, element ownership metadata, separate visualization and picking policies, screen-space expanded line/point primitives, explicit depth/pass splits, and inverted-normal handling. Crimson currently has useful foundations from #9 but only generic grid/line overlays and object-level GPU picking.

## Duplicate And Overlap Observations

- Not a duplicate of archived #9. #9 delivered Crimson renderer, shader, color, material, overlay, picking, and diagnostics foundations, but archive residuals explicitly note no headless GPU render/readback harness or golden-image validation, and no UI selection/document mutation was added by design.
- Overlaps active #13 only in shared viewport overlay/picking/gizmo infrastructure. #13 is transform gizmos, not mesh/component selection semantics. Coordinate screen-space overlay quads, pass ordering, picking contracts, and shared camera-ray behavior with #13.
- No active board item found that owns mesh selection, component selection modes, sticky component source-wire policy, inverted-normal component overlays, or flip mesh normals in the current Quader tree.

## Source refs

Reference app root: `C:\Users\Drako\Desktop\quader-windows\quader-app`

- `docs/overlay-handling.md:44-84` - ownership split and selection-mode status matrix.
- `docs/overlay-handling.md:90-129` - sticky component source-wire policy and owner inference.
- `docs/overlay-handling.md:132-198` - source wire, depth stamps, picking separation, inverted-normal and face-fill requirements.
- `docs/overlay-handling.md:205-232` - render policy invariants, colors/depth behavior, cache expectations.
- `docs/overlay-handling.md:235-346` - bgfx renderer translation: semantic overlay data, source kind, pass ordering, state mapping, batching/caching.
- `docs/overlay-handling.md:360-416` - regression checklist and required test suites.
- `src/editor/modeling/runtime/session.cpp:271-296` - maps selection mode to component mode and checks mode-specific selected components.
- `src/editor/modeling/runtime/session_selection_overlay.cpp:207-295` - emits source-wire lines and `SourceWireDepthStamp` metadata.
- `src/editor/modeling/runtime/session_selection_overlay.cpp:401-444` - sticky source-wire object inference.
- `src/editor/modeling/runtime/session_selection_overlay.cpp:662-805` - object vs component overlay emission, selected faces/edges/vertices.
- `src/editor/modeling/runtime/session_selection_overlay.cpp:887-940` - hover-only source-wire and component hover overlays.
- `src/editor/modeling/runtime/session_picking.cpp:520-562` - editor surface pick policy and nearest component occlusion.
- `src/editor/modeling/runtime/session_picking.cpp:1391-1518` - hover update, source-wire-first component hover, selected-hover checks.
- `src/editor/modeling/runtime/session_picking.cpp:1525-1648` - object/component select, add/remove/replace, sticky active target behavior.
- `src/render/viewport_overlay_frame.hpp:20-47` - overlay layers including `SourceWire` and `SourceWireDepthStamp`.
- `src/render/viewport_overlay_frame.hpp:61-122` - style roles, source kinds, and element refs.
- `src/render/viewport_overlay_frame.hpp:134-174` - line/triangle/point records carry layer, style, source, and element metadata.
- `src/render/viewport_overlay_policy.cpp:23-167` - depth, draw-on-top, component-depth, and depth-bias policy.
- `src/render/viewport_overlay_policy.cpp:201-333` - exact width, point size, and color policy.
- `src/render/overlay/overlay_renderable_builder.cpp:82-156` - render states, reversed-depth face-fill/depth behavior, edit-wire depth bias.
- `src/render/overlay/overlay_renderable_builder.cpp:270-390` - source-wire depth-stamp visibility filter and inside-out scoring.
- `src/render/overlay/overlay_proxy_factory.cpp:93-125` - Filament depth/cull/transparency mapping.
- `src/render/overlay/overlay_proxy_factory.cpp:201-212` - triangle renderables, no culling/shadows.
- `src/mesh/polygon/operations/flip_face_normals_operation.cpp:37-80` - flip selected face normals by reversing face winding.
- `tests/modeling_domain/operation/qdr_document_operation_flip_normals_tests.cpp:11-20` - selected face winding flip test.
- `tests/modeling_domain/operation/qdr_modeling_runtime_operation_invert_mesh_tests.cpp:9-18` - selected object invert/flip normals behavior.

Current repo root: `C:\Users\Drako\Desktop\_quader-qt-base`

- `project_board.md:8-10` - active #13 gizmo task overlap.
- `project_board_archive.md:239-247` - archived #9 renderer foundation and residuals.
- `project_board_archive.md:256-257` - #9 overlay/picking slices and explicit no UI selection/document mutation residual.
- `src/crimson/overlays/overlay_command.hpp:20-54` - current overlay command has depth, primitive, color, opacity, thickness, payload span only.
- `src/crimson/overlays/overlay_command.hpp:106-112` - line payload has only start/end.
- `src/crimson/gpu/gpu_overlay_renderer.cpp:215-257` - current line rendering uses bgfx `PT_LINES`, not screen-space expanded quads.
- `src/crimson/picking/picking.hpp:25-45` - picking payload enum already reserves object/submesh/face/edge/vertex payloads.
- `src/crimson/gpu/gpu_picking.cpp:52-80` - pick view uses shared Crimson camera-ray helper.
- `src/crimson/gpu/gpu_picking.cpp:223-306` - GPU picking currently submits only object-level payloads.
- `src/ui/viewport/crimson_viewport_render_host.cpp:479-525` - frame builds objects, tool preview overlays, and picking requests.
- `src/ui/viewport/crimson_viewport_render_host.cpp:624-693` - appends document render data and tool preview overlays, no selection overlay extraction.
- `src/document/selection.hpp:23-95` - current object/component selection data model.
- `src/document/selection.cpp:86-123` - object and component selections are mutually exclusive and structural only.
- `src/crimson/scene/render_camera_projection.cpp:41-53` - current camera basis convention.
- `src/crimson/scene/render_camera_projection.cpp:148-174` - shared viewport-point-to-ray helper.

## Quader Windows renderer parity checked

Reference renderer internals inspected:

- `docs/overlay-handling.md`
- `src/editor/modeling/runtime/session.cpp`
- `src/editor/modeling/runtime/session_selection_overlay.cpp`
- `src/editor/modeling/runtime/session_picking.cpp`
- `src/render/viewport_overlay_frame.hpp`
- `src/render/viewport_overlay_policy.hpp`
- `src/render/viewport_overlay_policy.cpp`
- `src/render/overlay/overlay_renderable_builder.cpp`
- `src/render/overlay/overlay_line_mesh_builder.cpp`
- `src/render/overlay/overlay_point_mesh_builder.cpp`
- `src/render/overlay/overlay_face_fill_builder.cpp`
- `src/render/overlay/overlay_proxy_factory.cpp`
- `src/mesh/polygon/quader_poly_document_picking_overlay.cpp`
- `src/mesh/polygon/operations/flip_face_normals_operation.cpp`

Renderer internals affecting recommendation:

- Reference overlay records carry semantic source kind, style role, owner object id, and component identifiers. Crimson's current `OverlayCommand` does not.
- Reference `SourceWire` is draw-on-top editable topology and `SourceWireDepthStamp` is metadata. Do not collapse these into one rendered batch.
- Reference component selected/hover handles may use layers that are normally draw-on-top, but source kind forces depth-tested component-handle batches. Layer alone is not enough.
- Reference selected/hover face fills use two-pass depth-stamped, two-sided transparent rendering for inverted faces.
- Reference picking intentionally differs from visualization: surface picking uses backfaces and source-wire handle picking is occlusion-filtered against nearest same-mesh visible surface.
- Reference renderer uses Filament with reversed depth; Crimson uses bgfx/current depth conventions. Match behavior and policy, not literal Filament types or reversed-depth compare names.

Adaptation requirements:

- Normal-depth Crimson component-handle depth tests should use the bgfx equivalent such as `LEQUAL`; reversed-depth rules from the reference must only be used if Crimson changes to reversed depth.
- Do not import Filament class structure, material instances, or cache code. Keep reference code as behavioral reference only.
- Preserve Crimson runtime isolation and do not expose bgfx types through UI/core selection APIs.

## Proposed Renderer Scope

- Extend renderer-facing semantic overlay records to include primitive kind, layer/style role, source kind, owner object id, face/edge/vertex refs, resolved color, pixel width/size, depth mode, and viewport id.
- Add point overlays and screen-space-expanded line/point quad builders for stable pixel sizing. Do not use `BGFX_STATE_PT_LINES` for selection/source-wire parity overlays.
- Preserve `SourceWire` as crisp draw-on-top editable topology.
- Preserve `SourceWireDepthStamp` as metadata/no draw batch, usable for camera-dependent source-wire visibility filtering.
- Add explicit overlay phases or equivalent strict submit ordering:
  1. shaded scene/depth,
  2. optional overlay depth proxy if needed,
  3. selected/hover face-fill depth,
  4. selected/hover face-fill equal-depth color,
  5. depth-tested selected/hover component handles,
  6. draw-on-top source wire, object selected wire, hover helpers, tool lines, diagnostics.
- Add face-fill rendering that disables culling and handles inverted faces without duplicating reversed triangles.
- Extend picking to support component payloads or a renderer-safe CPU/GPU hybrid while preserving shared Crimson camera rays and request-scoped non-color-managed IDs.
- Add diagnostics/counters where useful for overlay primitive counts, split batches, source-wire depth-stamp presence, and component-picking requests.

## Architecture Constraints

- Do not move modeling logic or selection policy into Crimson GPU draw code.
- Build semantic selection overlays outside Crimson; Crimson consumes already-classified primitives.
- Do not let UI include graphics-runtime headers.
- Do not introduce runtime-specific names in public Crimson folders, targets, or class names.
- Do not weaken the reference sticky source-wire policy.
- Do not mask bad winding or flip-normal issues with double-sided lit materials, cull-state hacks, or lighting workarounds.
- If implementation changes documented Crimson APIs, overlay behavior, shader/material schema, or documented C++ symbols, update the existing documentation in place in the same edit.

## Sequencing Recommendation

1. Core/UI selection data and commands: object/component mode state, select, deselect, select all, inverse select, additive/remove selection, multi-mesh selection, undo/redo, and action wiring.
2. Core mesh operation: add flip mesh normals by reversing selected/all face winding as the reference `flip_face_normals` does.
3. Semantic overlay extraction: implement source-wire owner inference, hover candidate behavior, selected component overlays, and inverted-normal depth-stamp metadata before renderer batching.
4. Crimson overlay schema and renderer: add semantic primitives, point support, screen-space line/point quads, pass ordering, depth states, face fills, and tests.
5. Picking integration: object/component payload mapping, source-wire-first component hits, surface fallback, backface/flipped behavior, and UI event mapping.
6. Runtime verification: targeted tests, manual parity scenes, deploy/dev-build checkpoint.

## Risks

- A layer-only overlay API will fail parity because selected/hover component handles need depth-tested batches while related layers may otherwise draw on top.
- Current object-only GPU picking is insufficient for component selection acceptance.
- Current bgfx `PT_LINES` cannot guarantee reference-stable thickness or point handles.
- Inverted-normal behavior can regress if source-wire visualization is treated as picking truth.
- Current document selection stores object or component selection mutually exclusively and lacks mode-specific sticky selection policy; this needs careful core/UI ownership.
- Reference uses reversed depth; copying compare functions literally into Crimson would be wrong unless Crimson's depth convention changes.
- #9 left no golden/headless GPU readback harness, so visual regressions need strong headless policy tests plus manual parity checks.

## Tests And Verification Required

Tests-policy applies.

Required test areas:

- Selection semantics: object/vertex/edge/face modes, select, deselect, select all, inverse select, additive/remove multi-selection across meshes, sticky source-wire owner, mode-specific selected components.
- Commands: selection commands and flip mesh normals undo/redo.
- Mesh operation: face winding reversal, selected/all object behavior, triangulation/render-upload validity after flip.
- Overlay extraction: object mode no `SourceWire`; component mode fallback source wire; hover replacement; sticky component source wire; selected component overlays preserve source-wire topology.
- Crimson overlay: exact colors/widths/sizes, source-kind batch splitting, source-wire draw-on-top/no depth, component handles depth-tested, source-wire depth stamps produce no render batches, selected/hover face fills two-sided and two-pass, overlays remain after tone map and out of HDR/lit/post paths.
- Picking: object and component payloads, flipped/backfacing mesh picks, source-wire-first component hover/select, nearest visible same-mesh occlusion, outside-vs-inside inverted room behavior.
- UI: mode hotkeys/actions, viewport pointer modifiers, action state, scene panel/document selection synchronization.

Verification candidates:

- `cmake --build --preset qt-mingw-debug`
- Targeted non-style CTest filters for document selection, command, mesh, UI viewport, Crimson overlay, Crimson picking, and render graph tests.
- `cmake --build --preset qt-mingw-debug --target deploy` and archived dev build checkpoint for runtime-sensitive renderer work.
- `python tools/project_board.py validate` only when software architect issues board commands.

Gated checks requiring immediate explicit permission before execution:

- clang-format
- clang-tidy
- style/static-analysis targets
- CTest presets that include style/static-analysis gates

## Applicable Hindsight IDs

- `H-20260705-001` - Tool rays, GPU picking, and rendered Crimson view must share one camera/projection convention. Applies directly to component picking and hover parity.
- `H-20260705-003` - Do not mask winding/normal errors with double-sided materials or cull-state hacks. Applies to flip mesh normals and inverted-normal overlay behavior.
- `H-20260705-002` - Only applies if implementation touches box/footprint plane basis or reuses those projection helpers.

## Renderer resources checked

- https://graphicscompendium.com/opengl/18-lookat-matrix - supports treating camera basis/projection conventions as explicit and not assuming the reference app maps directly to Crimson.
- https://graphicscompendium.com/opengl/24-clipping-culling - supports careful depth precision and z-fighting handling for overlays.
- https://learnopengl.com/Advanced-OpenGL/Depth-testing - supports explicit depth-test/depth-write choices for overlay passes.
- https://learnopengl.com/Advanced-OpenGL/Framebuffers - supports separate offscreen/picking/depth target reasoning.
- https://graphicscompendium.com/raytracing/02-camera - supports deriving picking rays from the same camera basis used for rendering.

Findings supported by external scan:

- Use one camera basis/projection convention for rendered view and picking rays.
- Keep depth-test, depth-write, and framebuffer target choices explicit per pass.
- Avoid solving overlay z-fighting by moving geometry in screen X/Y; use tiny depth-domain bias only where required.

## Local renderer references checked

Reference corpus root: `C:\Users\Drako\Desktop\_SOURCES`

- `filament-main\third_party\imgui\imgui_draw.cpp:803-833` - anti-aliased/thick line generation reserves expanded geometry, useful as a conceptual reference only.
- `filament-main\third_party\imgui\imgui_draw.cpp:856-866` - line geometry width is expanded around the segment, supporting Crimson screen-space line-quad direction.
- `filament-main\third_party\imgui\imgui_draw.cpp:957-989` - thick line path computes inner/outer points for stable thickness, reference only.

No narrower reusable renderer architecture reference was found in `_SOURCES` for Quader's semantic selection overlay ownership; the Quader Windows app is the useful parity reference for this task.

## Likely Files And Modules

Renderer:

- `src/crimson/overlays/*`
- `src/crimson/gpu/gpu_overlay_renderer.*`
- `src/crimson/gpu/gpu_picking.*`
- `src/crimson/picking/*`
- `src/crimson/graph/*`
- `src/crimson/frame/*`
- `src/crimson/scene/render_camera_projection.*`
- shader manifest and overlay/picking shaders if screen-space quads require new shader inputs.

Viewport bridge:

- `src/ui/viewport/crimson_viewport_render_host.*`
- `src/ui/viewport/viewport_controller.*`
- `src/ui/viewport/tool_preview_overlay_adapter.*` if generalized overlay adapters are reused.

Core/UI dependencies:

- `src/document/selection.*`
- `src/commands/selection_commands.*`
- new selection-mode/action command surfaces as assigned by software/UI architect.
- `src/mesh/core/*` or `src/mesh/ops/*` for flip mesh normals.
- `src/tools/*` for select tool/input integration.
- `src/ui/actions/*`, `src/ui/models/*`, `src/ui/panels/*` as needed.

Tests:

- `tests/unit/crimson/overlay_tests.cpp`
- `tests/unit/crimson/picking_tests.cpp`
- `tests/unit/mesh/*`
- `tests/unit/document/*`
- `tests/unit/commands/*`
- `tests/unit/ui/*`
- possible new renderer policy tests for semantic overlay extraction/batching.

## Active Plan Path

No active implementation plan exists for this new task yet. This intake report is:

`agents/plans/task_intake_20260705_mesh_component_selection_renderer.md`

## Dependencies

- Depends on archived #9 Crimson foundation.
- Must coordinate with active #13 where overlay/picking/gizmo primitives or shared screen-space builders overlap.
- Needs software architect consolidation before board entry.
- Needs UI architect input for mode controls/input/action behavior.
- Needs core architect input for selection data model, commands, undo/redo, and flip mesh normals operation.

## Hindsight Candidate

- Area/tags: renderer, overlays, picking, selection-parity.
- Lesson: Crimson selection overlays need source kind plus element refs in renderer-facing records because layer alone cannot express source-wire draw-on-top and component selected/hover handles depth-tested in the same visual layer family.
- Evidence: reference `docs/overlay-handling.md:241-261` requires source kind and states layer alone is insufficient; current `src/crimson/overlays/overlay_command.hpp:34-54` has no source kind or element refs.
- Applies when: implementing semantic overlays, batching rules, component selection rendering, or reviewing renderer-facing overlay API changes.
