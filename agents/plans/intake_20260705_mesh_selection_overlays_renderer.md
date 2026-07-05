# Task Intake Domain Findings - Renderer

## Scope

Renderer scout scope: Crimson selection-overlay architecture, object/component overlay parity, picking data flow, depth/X-ray/always-on-top behavior, inverted-normal overlay validation, renderer tests, render diagnostics, and renderer boundaries for a new cross-domain task.

This is intake only. No implementation was performed and `project_board.md` was not mutated.

Recommended title: Add mesh and component selection modes with parity selection overlays and flip normals support.

Recommended task type: enhancement with required complementary core operation.

Recommended priority: critical.

Recommended area: multi-domain, with renderer as a primary owner for overlay and picking behavior.

Final owner: multi-domain. Renderer owns Crimson overlay extraction/rendering, picking contracts, pass/resource behavior, and renderer tests. Core owns document selection truth, component-mode semantics, selection operations, undoable flip mesh normals, and component picking policy if CPU-based. UI owns selection-mode controls, shortcuts, action routing, viewport input modifiers, marquee/paint/multi-select interaction, and viewport-host data adapters. Software architect should finalize board wording and sequencing.

Active plan path: none found for this specific task. Adjacent active plan context exists in `agents/plans/audit_20260704_full_architecture_master.md`.

## Required Docs Checked

- `AGENTS.md`: renderer is named `crimson`; do not use runtime-specific names; board mutations require software architect authority.
- `agents/workflow.md`: task intake scout returns findings only; reference/parity tasks require exact source refs; implementation must go through execution split gate; broad style/static-analysis checks are gated.
- `agents/task_board.md`: board command and metadata policy checked; no board mutation performed.
- `agents/tests-policy.md`: selection, picking, geometry, commands, undo/redo, render overlays, and golden image expectations must protect real behavior; do not replace meaningful checks with superficial coverage.
- `agents/documentation-policy.md`: implementation must update existing documented symbols/APIs/schema/behavior in place if changed.
- `agents/hindsight.md`: H-20260705-001, H-20260705-002, H-20260705-003 are active constraints.
- `agents/code-style.md`: C++ style policy and gated clang-format/clang-tidy/static-analysis rules checked.
- `agents/architecture/architecture.md`: renderer consumes prepared data and does not own modeling logic.
- `agents/architecture/renderer.md`: overlays are unlit and separate from lighting/post; picking has an ID pass/readback policy; selection overlays are intended to use overlay commands and ID buffers.
- `agents/architecture/ui.md`: checked because this task touches viewport hosting and UI-facing renderer settings; UI must not include graphics-runtime headers.
- Nested `AGENTS.md`: only root `AGENTS.md` was found.

## Current-Code Findings

Crimson has the right high-level renderer boundary but not the selection overlay machinery needed for this task.

- Current document selection already stores object IDs and component references, but there is no renderer-facing component mode or sticky edit-target policy. Selection object/component sets are mutually exclusive and are normalized/deduped. Relevant files: `src/document/selection.hpp:24-53`, `src/document/selection.hpp:65-88`, `src/document/selection.cpp:86-130`, `src/document/document.cpp:188-229`, `src/document/document.cpp:358-365`, `tests/unit/document/document_tests.cpp:183-222`.
- Current selection commands are narrow: replace selection and clear selection. There is no parity-ready select all, inverse selection, additive/remove/toggle multi-select command surface, or mode-aware component selection command set in the current repo. Relevant files: `src/commands/selection_commands.hpp:57-82`, `src/commands/selection_commands.cpp:59-84`, `tests/unit/commands/command_tests.cpp:237-248`.
- Current Crimson overlays are generic grid/line payloads. `OverlayPrimitive::SolidTriangles` is declared, but no concrete triangle payload, point payload, style role, source kind, object/component element metadata, source-wire depth stamp, face-fill pass, or component-handle depth policy exists. Relevant files: `src/crimson/overlays/overlay_command.hpp:20-115`, `src/crimson/overlays/overlay_system.hpp:22-92`, `src/crimson/overlays/overlay_system.cpp:30-140`, `src/crimson/gpu/gpu_overlay_renderer.cpp:153-253`, `tests/unit/crimson/overlay_tests.cpp:62-203`.
- `FrameSnapshot` can carry overlays, grid payloads, line payloads, and picking requests, but cannot carry semantic selection overlay frames or point/face-fill/depth-stamp payloads. Relevant files: `src/crimson/frame/frame_snapshot.hpp:77-105`, `src/crimson/frame/frame_snapshot.cpp:78-90`, `src/crimson/frame/frame_builder.cpp:94-116`.
- The Qt Crimson viewport host builds document render meshes and tool preview overlays only. It does not extract selection overlays from document selection or component modes. Relevant files: `src/ui/viewport/crimson_viewport_render_host.cpp:487-501`, `src/ui/viewport/crimson_viewport_render_host.cpp:639-683`.
- Current mesh upload derives flat normals from face winding. This is good for validating flip-normal behavior, but it also means inverted normals are real geometry state, not a renderer cull-state trick. Relevant files: `src/ui/viewport/crimson_viewport_render_host.cpp:240-280`.
- Current picking types include object/face/edge/vertex payload shapes, but GPU picking renders object IDs only. `picking_payload_for_object` always returns `PickingElementKind::Object`; shaders output one uniform ID per draw. Relevant files: `src/crimson/picking/picking.hpp:22-113`, `src/crimson/picking/picking.cpp:77-83`, `src/crimson/gpu/gpu_picking.cpp:58-80`, `src/crimson/gpu/gpu_picking.cpp:233-300`, `shaders/picking/picking.vs.sc:1-10`, `shaders/picking/picking.fs.sc:1-10`, `tests/unit/crimson/picking_tests.cpp:58-112`.
- UI has a CPU ray-to-face path that uses Crimson's shared camera ray helper, which aligns with H-20260705-001. It currently returns surface hits for tools, not full selection mutation or component selection policy. Relevant files: `src/ui/viewport/viewport_controller.cpp:424-490`.
- Mesh traversal helpers are available for extracting selected face edges, face vertices, and component overlay primitives, but Crimson must not own topology/editing rules. Relevant files: `src/mesh/core/mesh_traversal.hpp:29-113`.
- No current flip mesh/flip face normals operation was found in this repo by targeted search. The complementary flip operation is a core/command/UI dependency for inverted-normal overlay validation.

## Reference-App Findings

Source refs: `C:\Users\Drako\Desktop\quader-windows\quader-app` | `docs/overlay-handling.md:39-406`; `src/editor/modeling/runtime/session_selection_overlay.cpp:207-245`; `src/editor/modeling/runtime/session_selection_overlay.cpp:401-439`; `src/editor/modeling/runtime/session_selection_overlay.cpp:663-846`; `src/editor/modeling/runtime/session_selection_overlay.cpp:887-948`; `src/render/viewport_overlay_frame.hpp:20-222`; `src/render/viewport_overlay_policy.hpp:45-151`; `src/render/viewport_overlay_policy.cpp:23-199`; `src/render/viewport_overlay_policy.cpp:201-340`; `src/render/overlay/viewport_overlay_style_resolver.cpp:35-88`; `src/render/overlay/viewport_overlay_style_resolver.cpp:90-166`; `src/render/overlay/viewport_overlay_style_resolver.cpp:187-312`; `src/render/overlay/overlay_renderable_builder.cpp:82-184`; `src/render/overlay/overlay_renderable_builder.cpp:270-448`; `src/render/overlay/overlay_renderable_builder.cpp:522-840`; `src/render/overlay/overlay_camera_projector.cpp:23-149`; `src/render/overlay/overlay_face_fill_builder.cpp:15-41`; `src/render/overlay/overlay_line_mesh_builder.cpp:18-97`; `src/render/overlay/overlay_point_mesh_builder.cpp:16-88`; `src/render/overlay/overlay_proxy_factory.cpp:93-128`; `src/render/overlay/overlay_proxy_factory.cpp:435-563`; `src/render/viewport_renderer.cpp:238-271`; `src/editor/interactions/tool_viewport_projection.cpp:49-123`; `src/mesh/polygon/document_picking.hpp:22-63`; `src/mesh/polygon/quader_poly_document_picking_overlay.cpp:63-131`; `src/mesh/polygon/quader_poly_document_picking_overlay.cpp:272-294`; `src/mesh/polygon/quader_poly_document_picking_overlay.cpp:366-483`; `src/mesh/polygon/quader_poly_document_picking_overlay.cpp:559-629`; `src/editor/modeling/runtime/session_picking.cpp:1713-1785`; `src/editor/modeling/runtime/session_picking.cpp:1977-2148`; `src/editor/interactions/runtime_command.cpp:145-184`; `src/editor/interactions/runtime_command.cpp:1287-1301`; `src/editor/interactions/select_tool.cpp:558-606`; `src/editor/interactions/select_tool.cpp:705-820`; `src/editor/interactions/select_tool.cpp:904-919`; `src/editor/modeling/operations/selection_operations.cpp:209-213`; `src/mesh/polygon/operations/flip_face_normals_operation.cpp:37-80`; `tests/core_tests.cpp:1668-1845`; `tests/core_tests.cpp:2463-2480`; `tests/core_tests.cpp:13273-13369`; `tests/render_extraction_tests.cpp:178-229`; `tests/render_extraction_tests.cpp:275-367`; `tests/modeling_domain/qdr_modeling_runtime_picking_tests.cpp:650-787`.

The reference app's overlay model is semantic first, renderable second.

- It projects editor/runtime overlay primitives into a render overlay frame with separate line, triangle, point, and icon arrays. Each primitive carries layer, style role, source kind, color, pixel width/size, object ID, and component element refs. Crimson should adapt this semantic contract, not flatten everything into `OverlayCommand + LineList`.
- Layer alone is insufficient. The reference uses source kind to distinguish object selection, component selection, component hover, source wire, tools, diagnostics, and external overlays. Component-selected/hover handles override the layer's default draw-on-top policy and become depth-tested.
- Source wire policy is sticky and mode-specific. Face mode sticks only when selected faces exist; edge mode sticks only when selected edges exist; vertex mode sticks only when selected vertices exist. Object selection alone is not a sticky component source. Fallback order is active sticky object, any sticky object, hover candidate, active selected object, first selected object.
- Source wire and depth stamps are separate. `SourceWire` is rendered as crisp screen-space expanded draw-on-top topology. `SourceWireDepthStamp` is metadata/no visible batch that lets component handles and source-wire visibility reason about same-object occlusion and inverted normals.
- Face fills are two-pass depth-stamped fills: depth-only pass, then equal-depth color pass. The reference disables culling for these selected/hover face fills and handles inverted normals by relying on face depth stamps and two-sided transparency rather than duplicating triangles or changing lit mesh culling.
- Component handles do not use render visibility as picking truth. Reference picking uses editor geometry with `pick_backfaces = true` and `prefer_front_facing_faces = false` for editor surface picking so flipped/backfacing meshes pick the nearest visible surface. Source-wire handles are evaluated before shaded fallback and rejected only when occluded by nearest visible same-mesh surface.
- Projection/conventions must be adapted, not copied. The reference renderer uses Filament cameras/views and reversed depth with `GreaterOrEqual` style depth functions. Crimson uses bgfx and its own `current_render_homogeneous_depth()`/camera helpers. H-20260705-001 requires rendered view, GPU picking, and UI/tool rays to share Crimson's projection convention.
- The flip operation in the reference is simple and important: `flip_face_normals` reverses selected face winding. Crimson/Core should implement the operation in core/commands and let render upload naturally derive inverted normals from winding.

## Overlay-Handling Concepts To Port Or Adapt

Port conceptually:

- Add a renderer-facing `SelectionOverlayFrame` or equivalent Crimson-owned semantic overlay payload with lines, points, triangles, and metadata-only depth stamps.
- Add selection overlay roles/layers equivalent to selected visible face, selected face edge, selected edge, unselected vertex, selected vertex, hover face, hover face edge, hover edge, hover vertex, selected X-ray face, selected face mask, source wire, source-wire depth stamp, open edge if needed, and future tool/diagnostic coexistence.
- Preserve style-role resolution separately from layer. Colors and pixel sizes should come from UI/editor settings or renderer-safe style input, then convert authored UI sRGB to linear SDR before rendering, consistent with Crimson docs.
- Preserve source kind separately from layer: selection, hover, component selection, component hover, source wire, tool, diagnostic, scene wireframe, external.
- Implement source-wire depth stamps as metadata/no visible render batches. Use them for same-object component handle/source-wire visibility and inverted-normal checks.
- Implement face fills as depth-stamped two-pass overlay triangles in Crimson's render graph. This needs explicit pass/resource planning because the current overlay renderer only draws grid/line primitives.
- Implement screen-space expanded line and point quads for stable pixel thickness/size. Current Crimson lines are submitted as primitive lines and will not provide reference-equivalent thickness/feathering across graphics backends.
- Keep source wire draw-on-top and do not depth-test it directly. Depth-test only component selected/hover edge and vertex handles that carry component source.
- Keep overlays outside lit/HDR/post domains. Selection overlays must not receive lighting, shadows, IBL, fog, bloom, SSAO, tone mapping, or picking IDs unless explicitly part of the picking pass.

Adapt to Crimson:

- Use Crimson render graph queues/resources and bgfx depth conventions instead of Filament views/materials.
- Use Crimson camera/projection helpers for rendered view, GPU object picking, CPU component picking, hover previews, and overlay projection. Do not import reference `tool_viewport_projection.cpp` math literally.
- Keep CPU component picking authoritative unless a GPU component-ID pass can reproduce the reference policy exactly. The reference explicitly separates visual overlay visibility from picking truth, so a naive GPU render-ID pick for visible handles would not match parity.
- If GPU component picking is added, encode object/submesh/face/edge/vertex IDs through `PickingPayload`, avoid readback every frame, keep the target integer/unorm with no color conversion, and prove parity against CPU geometry policy.

## Duplicate And Overlap Risks

- No direct active board duplicate for mesh/object/component selection overlays was found.
- High overlap with active `#13 Add Godot-style transform gizmos to the viewport` in `project_board.md:8-10`. That task also owns Crimson overlay/picking/gizmo rendering and must coordinate shared overlay primitives, screen-space line/point rendering, camera-scaled handles, and picking contracts.
- Adjacent historical plan context exists in `agents/plans/audit_20260704_full_architecture_master.md:349-354`, which sequences overlay system and picking foundations, and `agents/plans/audit_20260704_full_architecture_master.md:409`, which warns not to implement picking-driven selection/golden images before renderer foundations.
- UI bugs `#20` and `#21` are not duplicates. They touch diagnostics/fly navigation and should not block this task, but viewport input routing changes should avoid conflicting with `#21`.

## Recommended Board Brief Bullets

- Add document-backed mesh selection and component selection modes for object, vertex, edge, and face selection with parity semantics against `C:\Users\Drako\Desktop\quader-windows\quader-app`.
- Implement select, deselect, clear, select all, inverse selection, additive multi-select, remove/toggle selection, and preserve-selection-on-selected-hit behavior for both meshes and mesh components.
- Replicate the reference sticky source-wire policy exactly: mode-specific sticky component source only when that component kind has selected elements, with fallback to hover candidate, active selected object, then first selected object.
- Add complementary flip mesh normals operation by reversing selected face winding through core/commands, with undo/redo and UI action integration. Do not fake inverted normals in Crimson.
- Build a Crimson semantic selection overlay frame with line, point, triangle, style role, source kind, object/component element metadata, and metadata-only source-wire depth stamps.
- Implement selection overlay rendering parity: reference colors, widths, vertex sizes/outlines, draw order, source-wire draw-on-top behavior, component handle depth-tested behavior, depth-stamped two-pass face fills, and inverted-normal behavior.
- Keep modeling selection truth outside Crimson. Crimson receives renderer-facing snapshots/overlay payloads and never mutates document selection.
- Keep UI free of graphics-runtime headers. UI maps selection mode/actions/input into commands and renderer-safe snapshot/overlay inputs only.
- Keep all overlay shaders unlit and outside tone mapping/post-processing. Add shader manifest/build entries for new overlay shaders; runtime shader compilation remains forbidden.
- Use shared Crimson camera/projection helpers for rendering, CPU picking, GPU picking, and overlay projection. Do not copy Filament projection/depth conventions literally.
- For component picking, preserve reference behavior that picking truth is not render visibility. CPU geometry picking may be authoritative; any GPU component-ID path must prove identical hit ordering and occlusion behavior.

## Acceptance Criteria

- User can select, deselect, clear, select all, inverse select, and multi-select different mesh objects.
- User can select, deselect, clear, select all, inverse select, and multi-select vertices, edges, and faces across eligible meshes according to active selection mode.
- Object selection and component selection mode transitions match reference behavior, including preserving active edit mesh where required.
- Sticky source-wire policy is replicated 100 percent, including mode-specific stickiness and fallback behavior.
- Object-mode selected meshes render selected topology lines and object selection face fill/mask behavior without emitting component `SourceWire` or `SourceWireDepthStamp`.
- Component modes render source wire immediately for the active/hover component edit mesh and selected/hover component highlights over it.
- Other selected meshes do not emit full component source wire unless selected by the sticky/fallback policy.
- `SourceWire` is draw-on-top, stable pixel-width, and not directly depth-tested.
- Component selected/hover edge and vertex handles are depth-tested with the reference-equivalent same-mesh visibility behavior.
- Selected/hover face fills use reference-equivalent two-pass depth-stamped, two-sided overlay behavior and remain correct for inverted normals.
- Overlay colors, widths, vertex sizes, outlines, hover/remove colors, draw order, and suppression behavior match the reference defaults or documented adapted values.
- Flip mesh normals reverses selected face winding through a core command and causes Crimson mesh upload/flat normals and overlays to reflect inverted normals without culling hacks.
- Picking and hover behavior match reference policy for normal and flipped/inverted meshes, including nearest visible surface on flipped geometry and rejection of occluded interior handles from outside.
- Golden/manual overlay checks include normal mesh and inverted-normal mesh cases. Golden image automation should only be required after the stable Crimson capture path is approved.

## Tests And Checks

Tests-policy applies.

Recommended focused tests:

- Core document/command tests for selection modes, select all, inverse, additive/remove/toggle, deselect/clear, active object/edit target preservation, cross-object component selection, invalid component refs, undo/redo, and no-op behavior.
- Core mesh operation tests for flip normals/flip selected faces: reverse winding, preserve IDs where intended, reject empty selections, undo/redo, and render-upload normal direction changes.
- Picking tests for object, face, edge, and vertex hit ordering; backface/inverted normal picking; source-wire handle occlusion; preserve-selection-on-selected-hit; multi-click select-all face/edge/vertex parity; and geometry ray-distance behavior.
- Renderer unit tests for semantic overlay bucketing, layer/source/style preservation, sRGB-to-linear conversion, draw order, depth modes, source-wire depth stamps as metadata/no visible batches, component handle depth overrides, and face-fill two-pass state.
- GPU/renderer tests for overlay pass order after tone map/present setup, no lighting/post/fog/bloom on selection overlays, no picking ID writes from regular overlay passes, and no per-frame readback.
- UI viewport tests for selection mode actions, shortcuts, input modifiers, selection result application, viewport-host overlay extraction, and no graphics-runtime include leaks.
- Render tests/manual checks for selected object, selected face/edge/vertex, hover remove/add colors, sticky source wire, normal cube, flipped/inverted cube, interior/exterior inverted-room source-wire behavior, and component handles hidden/visible through same-mesh occlusion.
- Golden image tests only after the Crimson final-display capture path is stable and renderer authority approves baselines. Current `tests/golden_images/README.md:1-20` says baselines are metadata-only until then.

Verification/build checkpoint notes:

- Implementation should at minimum build `cmake --build --preset qt-mingw-debug` and run targeted unit/render tests relevant to changed files.
- For C++/shader/runtime changes, follow dev build checkpoint policy from `agents/workflow.md`; renderer review should reject final approval without an archived dev build path or valid deferred blocker.
- Gated checks requiring immediate explicit user permission before execution unless the prompt says `run now without asking`: raw `clang-format`, raw `clang-tidy`, `check_format`, `check_clang_tidy`, `check_static_analysis`, and any CTest preset known to include style/static-analysis gates.

## Architecture Constraints

- Do not move modeling selection, topology edits, sticky policy ownership, or flip-normal operation ownership into Crimson.
- Do not make UI code include bgfx or any graphics-runtime header.
- Do not introduce runtime-specific names in planned folders, targets, or public renderer class names. Use `crimson`.
- Do not copy proprietary Windows app code verbatim. Use it as parity reference only.
- Do not use render visibility as selection/picking truth where the reference does not. Visual overlay behavior and pick acceptance are related but separate.
- Do not weaken overlay parity by substituting generic line lists for semantic source/layer/style/component metadata.
- Do not hide bad winding or inverted normals with double-sided lit materials, cull-state changes, lighting hacks, or shader-side normal fixes. H-20260705-003 applies.
- Do not alter Crimson camera/projection/ray conventions in one subsystem only. H-20260705-001 applies.
- Keep source-wire depth stamps metadata-only unless a later plan explicitly defines a debug visualization.
- New overlay shaders must be offline-compiled and added to the shader manifest/build outputs.
- If implementation changes documented Crimson APIs, shader/material schemas, or documented behavior/invariants, update existing documentation in the same edit.

## Dependencies On Core And UI

Core dependencies:

- Selection mode model for object, vertex, edge, face.
- Mode-aware selection command API and undo/redo support.
- Select all, inverse select, additive/remove/toggle, deselect/clear semantics.
- Sticky active edit mesh/source-wire policy state.
- Component topology extraction helpers for selected/hover face edges, edge vertices, vertex positions, and source-wire/depth-stamp mesh triangles.
- Flip selected face normals/flip mesh operation that reverses winding and integrates with command history.
- Component picking policy if CPU picking remains authoritative.

UI dependencies:

- Selection-mode actions/shortcuts and toolbar/status presentation.
- Input modifier mapping for replace/add/remove/toggle and preserve-selection-on-selected-hit.
- Viewport click/double-click/drag/paint/marquee routing.
- Renderer-safe overlay settings/style values for colors, line widths, point sizes, and hover/remove state.
- Mapping of pick results and CPU hit results to document commands without embedding renderer runtime headers.

Renderer dependencies:

- Semantic overlay snapshot payloads.
- Point and solid triangle overlay payloads.
- Screen-space line and point quad expansion.
- Source-wire depth stamp metadata.
- Face-fill depth/color passes and render graph resources.
- Component-aware picking payload path or explicit CPU-picking boundary.
- Diagnostics/debug views for overlay layer/source/depth mode and picking IDs.

## Renderer Resources Checked

- https://graphicscompendium.com/index.html - confirmed the reference chapters used for graphics fundamentals and framebuffers/culling.
- https://graphicscompendium.com/opengl/20-framebuffers - supports the need for separate framebuffer attachments/resources when drawing or reading depth/color data for overlay face fills and picking.
- https://graphicscompendium.com/opengl/24-clipping-culling - supports treating winding/culling/depth as explicit pipeline concerns and not masking bad face orientation in Crimson.
- https://learnopengl.com/Introduction - checked as the authoritative LearnOpenGL index for the relevant advanced OpenGL topics.
- https://learnopengl.com/Advanced-OpenGL/Depth-testing - supports separate depth-tested versus draw-on-top overlay behavior and why depth buffer precision/bias must be explicit.
- https://learnopengl.com/Advanced-OpenGL/Framebuffers - supports offscreen color/depth attachments, readback targets, and separate framebuffer passes for picking/face-fill validation.
- https://learnopengl.com/Advanced-OpenGL/Face-culling - supports the flip-normal requirement as winding-state behavior and why inverted normals should not be hidden by culling hacks.
- https://learnopengl.com/Advanced-OpenGL/Blending - supports transparent face-fill overlay blending and the need to separate alpha overlays from opaque/lit scene rendering.

## Local Renderer References Checked

- `C:\Users\Drako\Desktop\_SOURCES\filament-main\docs_src\src_mdbook\src\notes\framegraph.md:70-109` - useful conceptual reference for picking as a color attachment beside depth in a named framegraph pass.
- `C:\Users\Drako\Desktop\_SOURCES\filament-main\filament\include\filament\View.h:690-751` - supports the idea that UI overlays/custom render targets can bypass post-processing, and transparent picking may require extra depth work.
- `C:\Users\Drako\Desktop\_SOURCES\filament-main\filament\include\filament\View.h:846-960` - supports async picking/readback latency as an acceptable renderer contract; Crimson already allows delayed readback.
- `C:\Users\Drako\Desktop\_SOURCES\filament-main\filament\src\PostProcessManager.cpp:680-732` - useful conceptual reference for a picking render pass with color and depth targets, including integer/UNORM fallback ideas. Reference only; do not copy.

No third-party local reference should override the Windows parity app or Quader architecture.

## Quader Windows Renderer Parity Checked

Checked reference folders/files:

- `C:\Users\Drako\Desktop\quader-windows\quader-app\docs\overlay-handling.md:39-406`
- `C:\Users\Drako\Desktop\quader-windows\quader-app\src\editor\modeling\runtime\session_selection_overlay.cpp:207-245`, `401-439`, `663-846`, `887-948`
- `C:\Users\Drako\Desktop\quader-windows\quader-app\src\render\viewport_overlay_frame.hpp:20-222`
- `C:\Users\Drako\Desktop\quader-windows\quader-app\src\render\viewport_overlay_policy.hpp:45-151`
- `C:\Users\Drako\Desktop\quader-windows\quader-app\src\render\viewport_overlay_policy.cpp:23-199`, `201-340`
- `C:\Users\Drako\Desktop\quader-windows\quader-app\src\render\overlay\viewport_overlay_style_resolver.cpp:35-312`
- `C:\Users\Drako\Desktop\quader-windows\quader-app\src\render\overlay\overlay_renderable_builder.cpp:82-840`
- `C:\Users\Drako\Desktop\quader-windows\quader-app\src\render\overlay\overlay_camera_projector.cpp:23-149`
- `C:\Users\Drako\Desktop\quader-windows\quader-app\src\render\overlay\overlay_face_fill_builder.cpp:15-41`
- `C:\Users\Drako\Desktop\quader-windows\quader-app\src\render\overlay\overlay_line_mesh_builder.cpp:18-97`
- `C:\Users\Drako\Desktop\quader-windows\quader-app\src\render\overlay\overlay_point_mesh_builder.cpp:16-88`
- `C:\Users\Drako\Desktop\quader-windows\quader-app\src\render\overlay\overlay_proxy_factory.cpp:93-128`, `435-563`
- `C:\Users\Drako\Desktop\quader-windows\quader-app\src\render\viewport_renderer.cpp:238-271`
- `C:\Users\Drako\Desktop\quader-windows\quader-app\src\editor\interactions\tool_viewport_projection.cpp:49-123`
- `C:\Users\Drako\Desktop\quader-windows\quader-app\src\mesh\polygon\document_picking.hpp:22-63`
- `C:\Users\Drako\Desktop\quader-windows\quader-app\src\mesh\polygon\quader_poly_document_picking_overlay.cpp:63-131`, `272-294`, `366-483`, `559-629`
- `C:\Users\Drako\Desktop\quader-windows\quader-app\src\editor\modeling\runtime\session_picking.cpp:1713-1785`, `1977-2148`
- `C:\Users\Drako\Desktop\quader-windows\quader-app\src\editor\interactions\runtime_command.cpp:145-184`, `1287-1301`
- `C:\Users\Drako\Desktop\quader-windows\quader-app\src\editor\interactions\select_tool.cpp:558-606`, `705-820`, `904-919`
- `C:\Users\Drako\Desktop\quader-windows\quader-app\src\mesh\polygon\operations\flip_face_normals_operation.cpp:37-80`
- `C:\Users\Drako\Desktop\quader-windows\quader-app\tests\core_tests.cpp:1668-1845`, `2463-2480`, `13273-13369`
- `C:\Users\Drako\Desktop\quader-windows\quader-app\tests\render_extraction_tests.cpp:178-229`, `275-367`
- `C:\Users\Drako\Desktop\quader-windows\quader-app\tests\modeling_domain\qdr_modeling_runtime_picking_tests.cpp:650-787`

Renderer internals that affected this recommendation:

- Reference projection/picking/rendering uses Filament and reversed-depth conventions. Crimson must adapt pass/depth state to bgfx and existing Crimson projection helpers.
- Reference source wire is not just an overlay layer; it is a semantic component-edit topology source with source kind, object/component IDs, style role, and metadata depth stamps.
- Reference inverted-normal behavior depends on face winding, two-sided depth-stamped face fill, and CPU/editor picking options, not a single render cull switch.
- Reference render extraction uses camera-dependent screen-space line/point expansion and layer build signatures/caching. Crimson currently lacks this and should plan it as renderer infrastructure shared with future gizmos.

Parts that must be adapted rather than matched literally:

- Filament material/proxy state calls, layer masks, and `GreaterOrEqual` depth functions must map to Crimson render graph/bgfx states.
- Reference projection math must not be copied into Crimson if it diverges from `render_camera_ray_from_viewport_point` and the matrices submitted to bgfx.
- Reference scene/document structures are not Quader current core structures. Extract behavior and data contracts only.

## Risks

- High cross-domain risk: this task combines selection truth, UI input, command history, picking, renderer overlays, shaders, and render tests.
- High parity risk: sticky source wire and inverted-normal overlays are easy to approximate incorrectly.
- High renderer infrastructure risk: current Crimson overlay system is not sufficient for face fills, points, screen-space expanded lines, depth stamps, or component source kinds.
- Picking risk: GPU picking is object-only today, while component selection parity may require CPU/editor geometry picking plus renderer camera/ray consistency.
- Documentation risk: Crimson overlay API/schema changes likely touch documented renderer behavior and must update docs in the same edit.
- Verification risk: golden images are not yet a stable ordinary gate; manual/render diagnostics may be needed until capture path is approved.

## Blockers

No blocker for creating a board task from this intake.

Implementation blockers:

- Needs software architect board entry and cross-domain implementation plan before code work.
- Needs core/UI dependencies for selection mode truth, commands, flip normals, and input/action routing.
- Needs a renderer plan for semantic overlay frames, new overlay primitives, pass/resource order, shaders, and picking boundary before implementation.

## Hindsight Candidate

Area/tags: renderer, selection-overlays, picking, inverted-normals, reference-parity

Lesson: Source-wire parity is not an overlay layer toggle. It requires semantic source kind plus metadata-only depth stamps; `SourceWire` itself stays draw-on-top, while component selected/hover handles become depth-tested only when they carry component source. Face fills need depth-stamped two-pass, two-sided rendering to remain correct on flipped/inverted meshes.

Evidence: `C:\Users\Drako\Desktop\quader-windows\quader-app\docs\overlay-handling.md:132-199`; `C:\Users\Drako\Desktop\quader-windows\quader-app\src\render\viewport_overlay_policy.cpp:73-138`; `C:\Users\Drako\Desktop\quader-windows\quader-app\src\render\overlay\overlay_renderable_builder.cpp:270-448`; `C:\Users\Drako\Desktop\quader-windows\quader-app\tests\core_tests.cpp:1806-1845`; current Crimson gap in `src/crimson/overlays/overlay_command.hpp:20-115`.

Applies when: planning or reviewing Crimson selection overlays, component handles, source wire, inverted-normal validation, or future gizmo/diagnostic overlay features that need layer/source/depth semantics.
