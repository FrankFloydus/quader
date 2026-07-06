# Implementation Plan: Task 28 Current-Code Audit Remediation

Task: #28 Implement current-code audit remediation
Audit source: `agents/plans/audit_20260706_current_code_master.md`
Status: In progress

## Objective

Implement the actionable remediation slice from the 2026-07-06 architecture and beautify audit without weakening validation, architecture boundaries, style policy, or active board work.

## Scope

1. Replace public prototype-era viewport/frame contracts with production-named Crimson viewport frame input types.
2. Make the live GPU mesh cache revision-aware so unchanged document mesh uploads do not recreate buffers every frame.
3. Fix diagnostics model refresh behavior so stable diagnostics topology updates use `dataChanged` instead of full model reset.
4. Make command merge hooks real by teaching `CommandHistory` to merge a successfully executed command into the previous undo command when allowed.
5. Return `InvalidAttribute` for invalid mesh attribute IDs separately from `AttributeTypeMismatch`.
6. Add an agent-safe test preset/path that excludes clang-format and clang-tidy CTest entries.
7. Update documentation and `dev-changelog.md` for durable workflow/API behavior changes.

## Non-Goals

- Do not implement task #13 transform gizmos.
- Do not implement task #18 project/assets.
- Do not alter #25 selection parity behavior beyond keeping builds/tests compatible.
- Do not broaden #26 PBR material behavior beyond preserving existing shader/material tests.
- Do not run clang-format, clang-tidy, static-analysis targets, or broad CTest presets containing those checks without immediate explicit permission.
- Do not hand-edit `project_board.md` or `project_board_archive.md`.

## Files And Symbols

Renderer frame contracts:

- Rename `src/crimson/prototype_viewport.hpp` to `src/crimson/viewport_frame.hpp`.
- Rename public types:
  - `PrototypeCameraProjection` -> `ViewportCameraProjection`
  - `PrototypeCamera` -> `ViewportCamera`
  - `PrototypeViewportRect` -> `ViewportFrameRect`
  - `PrototypeViewportView` -> `ViewportFrameView`
  - `PrototypeViewportFrame` -> `ViewportFrameInput`
  - `is_valid_prototype_view` -> `is_valid_viewport_frame_view`
- Update `src/crimson/frame/frame_builder.hpp/.cpp`:
  - include `crimson/viewport_frame.hpp`
  - `FrameBuilder::build_prototype_snapshot(...)` -> `FrameBuilder::build_snapshot(...)`
  - diagnostics should say "viewport frame input", not "prototype viewport frame".
- Update UI render host conversion in `src/ui/viewport/crimson_viewport_render_host.cpp`.
- Rename UI request flag in `src/ui/viewport/viewport_types.hpp` and `src/ui/viewport/viewport_controller.*`:
  - `prototype_animation_enabled` -> `scene_animation_enabled`
  - `set_prototype_animation_enabled` -> `set_scene_animation_enabled`
  - `prototype_animation_enabled()` -> `scene_animation_enabled()`
- Update CMake source list and tests.

Renderer naming cleanup:

- Rename `make_minimal_prototype_render_graph(...)` to `make_minimal_viewport_render_graph(...)`.
- Rename minimal graph pass `PrototypeOpaquePass` to `ViewportOpaquePass`.
- Rename `ShaderProgramId::PrototypeLitCube` to `ViewportLitCube`.
- Rename `ShaderProgramId::PrototypeGridOverlay` to `ViewportGridOverlay`.
- Rename `RenderQueue::PrototypeOpaque` to `ViewportOpaque`.
- Update shader-library mappings while keeping existing shader binary filenames.

Mesh upload cache:

- Add uploaded revision state to `crimson::gpu::GpuMeshResource`.
- In `GpuMeshCache::upload_mesh(...)`, validate the descriptor, resolve any existing resource, and if the existing resource has the same revision, increment `skipped_clean_resource_count` and return true before allocating bgfx buffers.
- Set the uploaded revision on newly created or replaced resources.
- Keep `RenderMeshUploadTracker` tests intact; add a focused pure test if a testable helper is introduced.

Diagnostics model:

- Update `DiagnosticsItemModel::reload_from_service()` to rebuild a candidate tree and compare topology.
- If the topology is stable, replace row values in-place and emit `dataChanged` over affected rows, preserving top-level and child indexes so `QTreeView` expansion can survive frame-stat refreshes.
- Use model reset only when group/child structure changes or diagnostics become unavailable with a different row topology.
- Add a regression test in `tests/unit/ui/ui_diagnostics_model_tests.cpp` using `QSignalSpy` to prove a same-topology refresh emits `dataChanged` and no reset.

Commands:

- Update `CommandHistory::execute(...)`:
  - Reject null commands as today.
  - Execute the new command first.
  - If execution applies and the undo stack has a command that can merge the new command, call `merge_with(std::move(command))`.
  - If merge succeeds, clear redo and return the merge result without pushing a new undo command.
  - If merge reports `NotMergeable`, push the already-executed command normally so document state and history remain consistent.
  - If merge returns any other failure, return that failure without dropping the executed command only if no concrete command currently uses this path; prefer tests that keep merge implementations successful or not-mergeable.
- Add a command-history test with a local mergeable command that mutates a counter/document-visible name and proves one undo reverts both merged operations.

Mesh attributes:

- In both mutable and immutable `MeshAttributes::value<T>(...)`, check `is_valid_attribute_id(id)` before typed storage lookup.
- Return `MeshErrorCode::InvalidAttribute` for bad IDs.
- Keep `AttributeTypeMismatch` for valid IDs with the wrong value type.
- Add tests for invalid attribute id and wrong type.

Build/test workflow:

- Add `ctest` labels to `clang_format` and `clang_tidy`, for example `style`.
- Add a test preset such as `qt-mingw-debug-runtime` that runs with `excludeLabel: "style"` or the CTest preset equivalent supported by the current preset schema.
- Update `AGENTS.md` and `agents/tests-policy.md` so the normal agent-safe runtime path is explicit and the style/static-analysis permission lock remains intact.
- Do not remove style/static-analysis tests.

## Ordered Edits

1. Core correctness slice:
   - MeshAttributes invalid-id error split plus tests.
   - CommandHistory merge behavior plus tests.
2. UI diagnostics slice:
   - Diagnostics model stable-topology update path plus signal regression test.
3. Renderer upload slice:
   - Add revision to `GpuMeshResource`.
   - Skip clean uploads in `GpuMeshCache::upload_mesh(...)`.
4. Renderer naming/API slice:
   - Rename viewport frame header/types/methods.
   - Rename shader IDs/render queue/minimal graph factory and tests.
   - Update all callers.
5. Workflow docs/presets:
   - Add style labels and runtime test preset.
   - Update docs and `dev-changelog.md`.
6. Verification and board closeout:
   - Run board validation.
   - Run architecture boundary check.
   - Run targeted tests by executable/CTest filter that avoid style gates.
   - Build `qt-mingw-debug` and `qt-mingw-debug-tests`.
   - Deploy and archive dev build unless blocked.
   - Move #28 to review when complete; if #20 is fully fixed, coordinate bug closeout or note it as resolved-by #28.

## Validation Plan

Allowed without extra permission:

- `python tools\project_board.py validate`
- `python tools\check_architecture.py --root .`
- `cmake --preset qt-mingw-debug`
- `cmake --build --preset qt-mingw-debug --parallel 20`
- `cmake --build --preset qt-mingw-debug-tests --parallel 20`
- Targeted test executables or CTest filters that do not include clang-format/clang-tidy.
- New runtime-only test preset after confirming it excludes style labels.
- `cmake --build --preset qt-mingw-debug-deploy --parallel 20`
- `python tools/dev_builds.py current`
- `python tools/dev_builds.py archive --preset qt-mingw-debug`

Permission-gated:

- `ctest --preset qt-mingw-debug` before the new safe preset exists.
- `check_format`, `check_clang_tidy`, `check_static_analysis`, raw clang-format, raw clang-tidy.

## Acceptance Criteria

- No `Prototype*` public viewport/frame DTOs remain in Crimson frame-builder/UI render-host contracts.
- No public render graph factory, shader program ID, or render queue name introduced by this task contains "Prototype".
- Repeated same-revision render mesh uploads increment skipped-clean upload stats and avoid GPU buffer recreation.
- Stable diagnostics refreshes no longer emit a model reset.
- Command merge hooks are honored by `CommandHistory`.
- Invalid mesh attribute IDs report `InvalidAttribute`.
- Agents have a documented runtime test preset/path that excludes style/static-analysis gates.
- Board validation and architecture check pass.
- Focused tests for changed behavior pass.

## Implementation Notes

- Public Crimson viewport frame contracts now live in `src/crimson/viewport_frame.hpp`.
- `FrameBuilder::build_snapshot(...)` replaces the prototype-named builder entry point.
- Shader program IDs, render queue names, minimal render graph names, and UI scene animation flags now use viewport/scene terminology.
- `GpuMeshCache::upload_mesh(...)` validates descriptors but skips bgfx buffer recreation when an existing GPU mesh resource already holds the same uploaded revision.
- `DiagnosticsItemModel` now compares candidate topology and emits `dataChanged` for same-topology refreshes; full reset remains for structural changes.
- `DocumentSelectionAdapter` now mirrors document object selections into the scene tree only in object mode. Component-mode object ids remain document edit targets and no longer leave object rows selected.
- `CommandHistory` now invokes merge hooks after successful command execution when the previous undo command accepts the new command.
- `MeshAttributes::value<T>(...)` now returns `InvalidAttribute` for invalid attribute ids before checking typed storage.
- `qt-mingw-debug-runtime` is the broad agent-safe runtime CTest preset. `clang_format` and `clang_tidy` remain registered but are labeled `style` and excluded by that preset.

## Verification Results

- `cmake --preset qt-mingw-debug`: passed.
- `cmake --build --preset qt-mingw-debug --parallel 20`: passed.
- `cmake --build --preset qt-mingw-debug-tests --parallel 20`: passed.
- Focused runtime CTest selection through `qt-mingw-debug-runtime`: passed, 92/92.
- `ctest --preset qt-mingw-debug-runtime`: passed, 272/272.
- `python tools\check_architecture.py --root .`: passed.
- `python tools\project_board.py validate`: passed.
- `cmake --build --preset qt-mingw-debug-deploy --parallel 20`: passed.
- `python tools\dev_builds.py archive --preset qt-mingw-debug`: archived `0.1.1-dev.14`.
