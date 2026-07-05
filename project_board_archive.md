# Project Board Archive

Completed tasks and fixed bugs are moved here from `project_board.md`.
Entries are grouped newest-first by archive date and internal dev version.


## 2026-07-06

### 0.1.1-dev.13

#27 [priority:critical][type:bug][area:renderer] Fix viewport placement, grid depth, and default texture regressions - Box placement, vertical-plane creation, grid depth/color, and default texture quality regressed together.
  Archived: [at:2026-07-06T00:00:32+02:00][dev-version:0.1.1-dev.13][from:In Progress][status:fixed] Workflow-authorized archive.
  Resolution: Confirmed fixed in C:\Users\Drako\Desktop\_quader-qt-base\build\qt-mingw-debug\quader.exe after rework: box creation/winding is correct in real viewport use, the perspective grid/unlit viewport behavior from the earlier fixes is preserved, and the default reference albedo now renders crisp at grazing angles via the corrected BGFX anisotropy reset/sampler path.
  Created: 2026-07-05T22:31:33+02:00
  Brief: Regression cluster from the current textured-material viewport path. User-visible symptoms: perspective grid flickers/bleeds based on camera position and depth precision; viewport/material color-space looks wrong; box creation no longer reliably creates new meshes on top of existing meshes; box tool no longer works on vertical planes; copied default albedo texture looks low quality/shimmery, likely from missing mipmap/filter setup. Treat as one task, not separate bugs. Reference source of truth: C:\Users\Drako\Desktop\quader-windows\quader-app. Source refs inspected: src\viewport\viewport_grid_placement_policy.hpp:18-162 for camera-following grid placement; src\render\viewport_grid_renderer.cpp:50-55,170-228 for sRGB-to-linear grid colors and depth-culling setup; src\render\png_texture_loader.cpp:26-105 and src\render\texture_binding_service.cpp:124-143 for sRGB texture upload with generated mipmaps; src\editor\interactions\box_tool.cpp:121-360 for construction-plane projection/snap behavior; src\editor\tools\base\two_stage_box_tool.cpp:94-158 for pointer flow; src\editor\commands\modeling\commit_box_preview_command.cpp:88-108 for commit/select behavior. Current likely files: src\ui\viewport\viewport_controller.cpp surface_hit_for_ray/ray_for_point/dispatch_tool_pointer, src\tools\box_tool.cpp construction_point_from_event and vertical-plane drag handling, src\crimson\overlays\grid_overlay.cpp, shaders\fs_grid.sc, src\crimson\gpu\gpu_overlay_renderer.cpp, src\crimson\gpu\gpu_material_cache.cpp/.hpp, tests\unit\tools\tool_manager_tests.cpp, tests\unit\ui\viewport_tests.cpp, tests\unit\crimson\overlay_tests.cpp, tests\unit\crimson\material_system_tests.cpp. Intended fix: preserve box creation on document surfaces and vertical planes by making viewport surface hits and locked construction-plane ray intersections robust; stabilize perspective grid depth without drawing through meshes; keep viewport colors in the correct sRGB/linear path; upload the default albedo with appropriate mip levels/filtering comparable to the reference app; avoid broad project asset-import UI scope from #18. Verification: focused tool/viewport/crimson tests, shader manifest validation, qt-mingw-debug build, qt-mingw-debug-tests build, deploy, dev-build archive, and board validation; do not run clang-format/clang-tidy/static-analysis or CTest presets that include them without explicit permission.
  Freshness: [status:fresh][checked:2026-07-05T20:31:41Z] Fresh consolidated regression task created from current screenshots and current/reference code inspection.
  Final owner: codex







  Authority: [owner:renderer][status:changes-requested][agent:quader-workflow] Review rework: boxes are still visibly inverted in the viewport and the copied default albedo appears too blurred compared to the reference Quader app.
  Rework: [owner:renderer][agent:quader-workflow][checked:2026-07-05T21:10:36Z] Fix the remaining task #27 regressions in the same task: (1) created boxes must not appear inverted in real viewport use, including committed mesh winding/render extraction/cull orientation, and (2) the default material albedo must render crisp like C:\Users\Drako\Desktop\quader-windows\quader-app instead of mushy/blurry, including UV scale, texture dimensions, mip/filter/sampler choices, and source asset parity.






## 2026-07-05

### 0.1.0-dev.2




















#23 [priority:high][type:enhancement][area:tools] Implement reference-style interactive box creation tool and remove prototype cube - Add a reference-style Box viewport tool with snapping, previews, surface placement, undoable commit/material assignment, and remove the prototype cube.
  Archived: [at:2026-07-05T18:08:53+02:00][dev-version:0.1.0-dev.2][from:In Review][status:completed] Architect-authorized archive.
  Created: 2026-07-05T10:46:06+02:00
  Brief: Source: Final Task Intake Report for reference-style interactive Box tool. Area scope: tools/document/ui/renderer. Architecture-sensitive feature work under agents/plans/audit_20260704_full_architecture_master.md; require a follow-up implementation plan before code starts. Do not implement directly in MainWindow, ViewportWidget, or prototype-only renderer paths. Source refs: C:\Users\Drako\Desktop\quader-windows\quader-app | Tool metadata/shortcut: src/editor/tools/base/viewport_tool_descriptor_catalog.cpp:83-90; src/editor/metadata/host_contract.cpp:782-783. Box state/math: src/editor/interactions/box_tool.hpp:20-54,79-124; src/editor/interactions/box_tool.cpp:130-149,161-188,190-228,248-279,281-323,325-398,400-446,493-608. Viewport tool event flow: src/editor/tools/base/two_stage_box_tool.cpp:41-158; src/editor/tools/interactive/box_tool.hpp:16-18; src/editor/tools/viewport_tool.hpp:94-104,172; src/editor/tools/viewport_tool_ports.hpp:29,52-56,99; src/editor/tools/viewport_tool_ports.cpp:53-107,208-209. Runtime/session commands: src/editor/interactions/runtime_command.cpp:232-354; src/editor/modeling/services/modeling_session_service.cpp:2101-2180,2475-2512; src/editor/modeling/services/runtime_tool_command_service.cpp:134-149,342-390. Preview/overlay/render refs: src/editor/projection/tool_preview_mesh_projection.cpp:19-52; src/render/overlay/viewport_box_overlay_builder.hpp:16-22; src/render/overlay/viewport_box_overlay_builder.cpp:28-41,90-119; src/render/tool_preview_renderable_cache.cpp:36-57,73-90. Commit/material refs: src/editor/commands/modeling/commit_box_preview_command.cpp:37-168; src/assets/material_document.hpp:39-41; src/assets/material_document.cpp:133-139; src/editor/session/project_asset_bootstrap_service.cpp:21-43. Test refs: tests/viewport_tool_catalog_tests.cpp:67-75,245-367; tests/viewport_interaction_controller_tests.cpp:1210-1262; tests/modeling_domain/tool/qdr_modeling_runtime_tool_box_tests.cpp:15-71,157-188,200-240; tests/modeling_domain/operation/qdr_modeling_runtime_operation_create_box_tests.cpp:9-69. Scope: register Box as a real tool/action with B shortcut through the QAction/action-state path; route neutral viewport hover and left-button events into modeling tool services without UI owning modeling state; add software-side box construction state, ray/plane/grid snapping, hover footprint, footprint drag, height/mesh preview, commit/cancel, undoable document command flow, and a mesh box builder under mesh ops. Extend document/material contracts so new boxes receive explicit default PBR values: base color 0.5 neutral gray, roughness 1.0, metallic 0.0. Extend renderer-facing snapshot/overlay/picking contracts so footprint hover, footprint drawing, and mesh preview render as overlays/previews without entering lit scene queues before commit. Surface placement must use hit position/normal for grid, horizontal surfaces, and vertical surfaces while keeping Crimson from owning modeling truth. Remove the default rotating prototype cube in this same task. Acceptance: pressing B activates Box; footprint hover is visible; footprint overlay is visible while drawing; mesh preview overlay is visible before commit; footprint snaps to current grid size; creation works on grid, horizontal surfaces, and vertical surfaces like the reference; committed mesh uses default material values; default rotating cube is removed; end state allows interactive creation of new meshes in the viewport; no selection behavior is required unless separately authorized. Likely files: CMakeLists.txt, src/app/application.cpp, src/app/app_services.hpp, src/tools/*, src/mesh/ops/*, src/geometry/*, src/document/*, src/commands/*, src/ui/actions/*, src/ui/qt_app/main_window.cpp, src/ui/viewport/*, src/crimson/frame/*, src/crimson/overlays/*, src/crimson/picking/*, src/crimson/material/*, and related tests. agents/tests-policy.md applies. agents/code-style.md applies. Expected coverage: mesh box topology and degenerate rejection; document command undo/redo and material assignment values; tool hover/footprint/height/snap/cancel/commit state tests; ground/horizontal/vertical surface placement tests using hit position and normal; UI action wiring for Box/B/checkable state and viewport event routing; renderer tests for footprint/preview overlay separation, no default cube in empty document snapshot, and material instance values; update tests that expect the prototype cube. Coordination: not a duplicate of #13 transform gizmos, but overlaps viewport input, snapping, overlays, picking, ray/surface math, and tool-state contracts; coordinate or sequence with #13. Not a duplicate of #21 fly mouse capture, but both touch viewport key/mouse routing; B/tool actions must not fire while fly/navigation capture is active. #9 is archived/approved; build on its Crimson overlay/picking/material foundations, do not recreate prototype paths. #22 style tooling is in progress; revalidate CMake/style/static-analysis assumptions before implementation. Verification: cmake --build --preset qt-mingw-debug; ctest --preset qt-mingw-debug; cmake --build --preset qt-mingw-debug --target check_architecture; cmake --build --preset qt-mingw-debug --target check_format; cmake --build --preset qt-mingw-debug --target check_clang_tidy; cmake --build --preset qt-mingw-debug --target deploy; python tools/dev_builds.py archive --preset qt-mingw-debug; python tools/project_board.py validate.
  Freshness: [status:fresh][checked:2026-07-05T12:47:47Z] Revalidated against current workspace, active board overlaps, source refs, current tools/document/UI/Crimson foundations, and added agents/plans/implementation_task23_reference_box_tool.md.
  Final owner: multi-domain; software finalizes app/core/document/commands/tools/mesh boundaries and arbitrates cross-domain scope. UI owns Qt actions, shortcuts, and viewport event routing. Renderer owns Crimson overlays, picking/surface-hit data, material instances, frame snapshot/render graph changes, and renderer verification.
  Plans: agents/plans/audit_20260704_full_architecture_master.md
  Coordination: Task-specific implementation plan added: agents/plans/implementation_task23_reference_box_tool.md. Treat it as active with agents/plans/audit_20260704_full_architecture_master.md before implementation.


  Authority: [owner:software][status:pending][agent:pending] Header blocker cleared by modified/untracked C++ header scan; software architect owns final review. Preserve closeout line: Dev build checkpoint deferred: skipped because the current worktree contains broad concurrent documentation/comment/style changes unrelated to #23, so a dev-build archive would produce a contaminated snapshot. Next owner/action: archive only after the unrelated documentation/style changes are separated or accepted into the same checkpoint.







#24 [priority:high][type:enhancement][area:build] Add fast local app build presets and build cache hygiene - Make local app iteration build the app runtime by default while preserving explicit full validation paths.
  Archived: [at:2026-07-05T14:13:31+02:00][dev-version:0.1.0-dev.2][from:In Review][status:completed] Architect-authorized archive.
  Created: 2026-07-05T12:33:23+02:00
  Brief: Source: Final Task Intake Report from quader-software-architect on build compilation pain. Current findings: build/qt-mingw-debug/build.ninja all includes quader.exe, runtime shader copy, about 35 test executables, quader_renderer_benchmarks.exe, and third-party all targets; current local cache has QUADER_ENABLE_BENCHMARKS=ON even though the source default is OFF; sccache 0.16.0 is installed but no compiler launcher is configured. Scope: add a CMake app-runtime target such as quader_app_runtime depending on copy_runtime_shaders; collect GTest/Qt test executables into a quader_tests aggregate target; change normal debug/release build presets so cmake --build --preset qt-mingw-debug and release build the app runtime target instead of Ninja all; add explicit full and tests build presets such as qt-mingw-debug-full and qt-mingw-debug-tests; keep CTest and #22 clang-format/clang-tidy/static-analysis quality gates unchanged; set normal configure presets to QUADER_ENABLE_BENCHMARKS=OFF; document benchmark opt-in and optional sccache launcher usage. Do not optimize shader compilation, do not alter clang-format/tidy scope, and do not weaken #22 validation semantics. Likely files: CMakeLists.txt, CMakePresets.json, README.md, AGENTS.md, agents/workflow.md, dev-changelog.md. Coordination: active #22 is In Progress and owns overlapping CMake/docs/dev-changelog files. This task must not start until #22 reaches In Review, unless the same main/root executor serializes the overlapping edits; in all cases revalidate freshness before start because #22 may change CMake, docs, and cache assumptions. agents/tests-policy.md applies because this changes build/test validation workflow and verification commands; command-level build/test validation is the meaningful coverage, not brittle tests of CMake internals. agents/code-style.md applies only if implementation touches C++ source, test source, benchmarks, clang-format, clang-tidy, or style/static-analysis validation beyond preserving #22 behavior. Third-party library decision: adopt optional sccache integration, not default-required. Candidate: sccache 0.16.0 compiler wrapper/cache. Scoped responsibility: optional local compiler launcher configuration and documentation for faster rebuilds. Commercial/license suitability: optional developer-side tool, not vendored or shipped in Quader runtime; executor must keep usage opt-in and document fallback. Maturity/reliability signal: installed locally and widely used as a C/C++ compiler cache. Integration boundary: CMake compiler launcher cache variables or documented preset/environment option only, with no source-level dependency. Expected benefit: faster local rebuilds after the cache warms. Risks: first run may rebuild, local tool availability/version differences, and stale/incorrect launcher paths. Fallback if rejected or unavailable: no compiler launcher and normal compiler invocation. Decision state: adopt optional integration. Verification: cmake --preset qt-mingw-debug; cmake --build --preset qt-mingw-debug; cmake --build --preset qt-mingw-debug-tests; ctest --preset qt-mingw-debug; cmake --build --preset qt-mingw-debug-full; cmake --build --preset qt-mingw-debug --target check_architecture; after #22 lands, cmake --build --preset qt-mingw-debug --target check_static_analysis; python tools/project_board.py validate. Build checkpoint: no deploy or dev-build archive required unless implementation changes runtime packaging or deploy/runtime files; this is build tooling/docs behavior rather than user-visible runtime behavior. AGENTS.md status check required: update root instructions and workflow docs before handoff because standard build preset semantics and verification commands change.
  Freshness: [status:fresh][checked:2026-07-05T10:47:57Z] Software architect revalidated after #22 reached In Review: #24 still matches current CMake, preset, and cache state; #22 static-analysis gates are landed and must be preserved.
  Final owner: software owner: CMakeLists.txt app-runtime/test aggregate targets, CMakePresets.json build/configure presets, README.md, AGENTS.md, agents/workflow.md, and dev-changelog.md


  PM: [run:20260705_task_24_fast_local_build_presets][status:coordinating][manager:quader-project-manager] Starting A1 main-root execution after #22 reached In Review; preserve #22 clang-format, clang-tidy, CTest, and check_static_analysis semantics.
  Authority: [owner:software][status:pending][agent:pending] Software architect owns final approval for CMake preset semantics, CTest/static-analysis preservation, workflow docs, and optional sccache boundary.
  Assignment: [key:A1][role:main-root-executor][agent:main-root][status:running][authority:software] Implement full #24 fast local build preset/cache hygiene scope after setting a Codex /goal; preserve #22 static-analysis gates and stop for any workaround/deviation or mid-work bug.


  Coordination: A1 may proceed. Required verification: cmake --preset qt-mingw-debug; cmake --build --preset qt-mingw-debug; cmake --build --preset qt-mingw-debug-tests; ctest --preset qt-mingw-debug; cmake --build --preset qt-mingw-debug-full; cmake --build --preset qt-mingw-debug --target check_architecture; cmake --build --preset qt-mingw-debug --target check_static_analysis; python tools/project_board.py validate. Verify release preset build if release build preset is edited. No deploy/dev-build archive unless deploy/runtime packaging behavior changes.




#22 [priority:high][type:enhancement][area:build] Add Quader clang-format and clang-tidy validation - Add Quader-owned clang-format and clang-tidy quality gates for formatting, static analysis, naming, and type-oriented validation.
  Archived: [at:2026-07-05T14:13:31+02:00][dev-version:0.1.0-dev.2][from:In Review][status:completed] Architect-authorized archive.
  Created: 2026-07-05T09:58:06+02:00
  Brief: Source: Updated Final Task Intake Report for Quader clang-format and clang-tidy validation. Current findings: Quader has no root .clang-format or .clang-tidy, and no clang-format or clang-tidy CMake/CTest integration. Existing validation is architecture-only through check_architecture in CMakeLists.txt and CTest architecture_boundaries. agents/architecture/architecture.md requires both .clang-format and .clang-tidy, says formatting is owned by clang-format, and says clang-tidy should cover naming, bug-prone, modernize, performance, and selected C++ Core Guidelines checks. Source refs: C:\Users\Drako\Desktop\_QUADER_GODOT\quader-godot | .clang-format:1-199; key .clang-format ranges: 6-185 base/style/C++ rules, 86-92 include categories, 161-175 comment spacing and tab rules, 183-185 C++ language/standard. No .clang-tidy source ref exists; C:\Users\Drako\Desktop\_QUADER_GODOT\quader-godot\.clang-tidy was checked and does not exist. Scope: clang-format is style, whitespace, and include-order formatting validation seeded from the provided reference .clang-format; clang-tidy is Quader-owned static/type/naming validation based on architecture guidance, not reference parity. Likely files and targets: root .clang-format, root .clang-tidy, tools/check_clang_format.py, tools/check_clang_tidy.py, CMakeLists.txt targets check_format and check_clang_tidy, optional aggregate check_static_analysis, CMAKE_EXPORT_COMPILE_COMMANDS support, CTest entries clang_format and clang_tidy, README.md, AGENTS.md, and dev-changelog.md. Intended path: seed .clang-format from the reference and adapt only where Quader requires it, especially documented C++20 compatibility if needed; add no-write format validation over project-owned C/C++ files under src, tests, benchmarks, and relevant root/build tooling while excluding build, generated, dependency, and FetchContent output; create a narrow .clang-tidy profile using Quader naming conventions and architecture guidance for naming, bug-prone, modernize, performance, and selected cppcoreguidelines checks; wire clang-tidy through a compile-database script that checks only Quader-owned translation units. Risks: reference .clang-format uses Standard: c++17 while Quader is C++20; clang-format version drift; noisy clang-tidy profiles; MinGW, Qt, generated code, and fetched dependencies must be excluded; missing clang-format, clang-tidy, or compile database must produce clear errors; validation must not mutate source formatting unless a separate explicit fix step is run. agents/tests-policy.md applies because this adds validation checks. The checks themselves are the meaningful automated verification; do not add brittle tests of clang tool internals. Third-party/tooling decision: adopt external LLVM clang-format and clang-tidy command-line tools; scoped responsibility is project formatting and Quader-owned static/type/naming validation; commercially suitable mature LLVM tooling aligned with architecture guidance; integration boundary is local config plus scripts, CMake targets, and CTest entries; expected benefit is deterministic quality gating; risks are local tool availability and version drift; fallback is clear missing-tool diagnostics and setup documentation; decision state: adopt. Verification: cmake --preset qt-mingw-debug; cmake --build --preset qt-mingw-debug; cmake --build --preset qt-mingw-debug --target check_format; cmake --build --preset qt-mingw-debug --target check_clang_tidy; ctest --preset qt-mingw-debug -R clang_format|clang_tidy; ctest --preset qt-mingw-debug; cmake --build --preset qt-mingw-debug --target check_architecture; python tools/project_board.py validate. Build checkpoint: no deploy or dev-build archive required because this is internal tooling/docs validation and does not change runnable application behavior.
  Freshness: [status:fresh][checked:2026-07-05T08:07:49Z] Revalidated current workspace: root .clang-format/.clang-tidy and clang format/tidy scripts/targets remain absent; architecture.md still requires formatting/static-analysis gates, and task #22 matches current CMake/CTest/docs state.
  Final owner: software owner: root .clang-format, root .clang-tidy, clang format/tidy validation scripts, CMake/CTest quality-gate wiring, README.md, AGENTS.md, and dev-changelog.md



  PM: [run:20260705_task_22_clang_validation][status:coordinating][manager:pending] Starting software/build tooling implementation for clang-format and clang-tidy validation.
  Authority: [owner:software][status:pending][agent:pending] Software architect owns final approval for build/tooling/workflow validation.
  Assignment: [key:A1][role:main-root-executor][agent:pending][status:planned][authority:software] Implement .clang-format/.clang-tidy configs, validation scripts, CMake/CTest wiring, docs, and dev changelog for task #22.


#17 [priority:high][type:enhancement][area:mesh] Add OpenMesh behind Quader mesh types - Use OpenMesh as the half-edge backend while preserving Quader-owned mesh IDs, validation, and document-facing APIs.
  Archived: [at:2026-07-05T09:50:20+02:00][dev-version:0.1.0-dev.2][from:In Review][status:completed] PM-authorized archive.
  Brief: Corrected objective from A18 software authority: swap Quader's current vector-backed topology storage to a private OpenMesh PolyMesh backend while preserving Quader-owned public IDs, generations, validation diagnostics, attributes, and document/command/I/O APIs. A14 dependency/guardrail work and A16 public API narrowing are accepted only as preparatory slices. Amend agents/plans/implementation_task17_openmesh_mesh_core_doc.md before further implementation, then implement the storage-swap slice: private OpenMesh storage, Quader ID/handle maps, deletion/generation/compaction policy, validation rewrite, focused mesh/I/O tests, and no OpenMesh public leakage. Tests-policy applies. Verification: configure, build, ctest, check_architecture, project_board validate.
  Final owner: Mesh core storage boundary, validation, fixtures, and all callers relying on record access
  Plans: agents/plans/audit_20260704_full_architecture_master.md

  Freshness: [status:fresh][checked:2026-07-05T06:19:43Z] Revalidated after A19: amended active plan matches current workspace facts; OpenMesh dependency/guardrails and API narrowing are prep only, and current HalfEdgeMesh remains vector-backed for authorized Slice 3 storage swap.

  PM: [run:20260705_task_17_openmesh_storage_swap][status:ready-for-review][manager:quader-project-manager] Replacement PM reconciled root A21 after user-authorized direct implementation; no new agents requested; verified work is ready for review registration.
  Authority: [owner:software][status:approved][agent:quader-software-architect] Software authority approved A21 final implementation review. Private OpenMesh-backed Polyhedron preserves Quader-owned IDs/generations, validation/compaction behavior, caller boundaries, tests-policy-aligned coverage, and verification passed configure, build, CTest 191/191, check_architecture, and project_board validate.
  Assignment: [key:A2][role:quader-software-architect][agent:019f3019-fedf-7283-a23e-6a1b7818ac89][status:integrated][authority:software] PM integrated A2 OpenMesh planning report. Active follow-up plan: agents/plans/implementation_task17_openmesh_mesh_core_doc.md.
  Coordination: Root A21 completed the OpenMesh storage swap and cleanup. Verification passed configure, build, mesh_core_tests 12/12, full CTest 191/191, check_architecture, project_board validate, and grep audits for stale HalfEdge/OpenMesh leakage terms. Deploy and dev-build archive were skipped because this was internal mesh-core/backend storage work with no user-visible runtime behavior checkpoint requested.
  Assignment: [key:A13][role:quader-software-architect][agent:019f308f-c1e1-7ba3-b2a4-fe69ae23524a][status:integrated][authority:software] PM integrated A13 revalidation: #17 is fresh and may start implementation with plan Slice 0/1 only.
  Assignment: [key:A14][role:worker][agent:019f3099-8e25-7a42-8044-0590df158ba4][status:integrated][authority:none] PM integrated A14 OpenMesh Slice 0/1 report. Pinned dependency, static modeling_mesh_core target, private OpenMeshCore link, placeholder translation units, and leakage guardrail implemented; verification passed configure, build, CTest 188/188, check_architecture, and board validation.
  Assignment: [key:A15][role:quader-software-architect][agent:019f30a7-147d-78b3-995f-a3ca767454af][status:integrated][authority:software] PM integrated A15 software authority approval for A14 OpenMesh Slice 0/1.
  Assignment: [key:A16][role:worker][agent:019f30ae-345f-7be2-8dd7-161cb817e559][status:integrated][authority:none] PM integrated A16 Slice 2 public mesh API narrowing. Verification passed configure, build, CTest 189/189, check_architecture, board validation, and grep audits for mutable record accessor/OpenMesh include leakage.
  Assignment: [key:A17][role:quader-software-architect][agent:pending][status:blocked][authority:software] Paused old-path Slice 2 approval review; superseded by urgent corrected-intent architecture review.
  Assignment: [key:A18][role:quader-software-architect][agent:019f30cb-5f9a-7cd3-88eb-6467cdc2b035][status:integrated][authority:software] PM integrated A18 urgent corrected-intent review: plan revision required; #17 means swapping vector-backed topology storage to private OpenMesh PolyMesh backend.
  Assignment: [key:A19][role:quader-software-architect][agent:019f30cb-5f9a-7cd3-88eb-6467cdc2b035][status:integrated][authority:software] Integrated Galileo A19 plan amendment; agents/plans/implementation_task17_openmesh_mesh_core_doc.md now authorizes corrected Slice 3 OpenMesh storage swap.
  Assignment: [key:A20][role:worker][agent:019f30ef-e0c9-7a23-a677-4ec4740df990][status:blocked][authority:none] Superseded by explicit user direction; root is taking over corrected Slice 3 implementation directly and A20 must not continue or be integrated.
  Assignment: [key:A21][role:root-worker][agent:root][status:integrated][authority:none] PM integrated root A21 report: corrected Slice 3 OpenMesh storage swap is complete; public Polyhedron now uses private OpenMeshStorage with Quader-owned IDs, generations, validation, compaction, copy/move behavior, rename cleanup, docs/plan/changelog updates, and required verification passed.



#16 [priority:high][type:enhancement][area:mesh] Add glm behind Quader math types - Adopt glm as math backend without exposing glm as the app-wide public API.
  Archived: [at:2026-07-05T08:05:33+02:00][dev-version:0.1.0-dev.2][from:In Review][status:completed] PM-authorized archive.
  Brief: Source: Final Task Intake Report for third-party foundation dependencies. Add pinned glm dependency, keep quader::math value types and functions as the public contract, centralize GLM config macros, and confine glm includes to math implementation or detail adapters. Preserve coordinate conventions, epsilon behavior, finite checks, and current consumers in UI, document, Crimson, geometry, and mesh. Extend architecture checks to flag accidental glm:: leakage outside approved math internals if practical. Likely files: CMakeLists.txt, src/math/*, src/geometry/*, src/ui/viewport/*, src/crimson/*, src/mesh/core/*, tests/unit/math/*, affected consumer tests. Tests-policy applies; assignment packets that plan, modify, or review tests must list agents/tests-policy.md as a required read. Verification: cmake --preset qt-mingw-debug; cmake --build --preset qt-mingw-debug; ctest --preset qt-mingw-debug; cmake --build --preset qt-mingw-debug --target check_architecture; python tools/project_board.py validate.
  Final owner: src/math Quader wrappers and adapters plus updated math and geometry tests
  Plans: agents/plans/audit_20260704_full_architecture_master.md

  Freshness: [status:fresh][checked:2026-07-05T02:10:57Z] Revalidated current workspace: math types remain Quader-owned plain wrappers with no glm dependency/includes; task remains valid.






  PM: [run:20260705_tasks_12_17_foundation_deps][status:ready-for-review][manager:pending] Software authority approved; ready for user validation.
  Authority: [owner:software][status:approved][agent:019f3078-9c21-74a3-936a-2cf47dab9f34] Approved: GLM is routed through modeling_math, public math contracts remain Quader-owned, and verification passed. Residual transitive compile dependency is accepted for #16.
  Assignment: [key:A5][role:explorer][agent:019f301a-b6d8-7200-8a0d-bcf0a8bb0c5a][status:integrated][authority:none] PM integrated A5 no-edit glm impact map.
  Coordination: Software authority approved #16; move to In Review for user validation. Residuals are non-blocking: GLM is transitively compiled through inline math wrappers, benchmark leakage checks are not covered, and recurring Ninja warning remains.
  Assignment: [key:A11][role:worker][agent:019f3062-6721-7db2-a304-fcc867de2ebd][status:integrated][authority:none] PM integrated A11 glm math backend implementation. Verification passed configure, build, CTest 188/188, check_architecture, board validation, and architecture glm leakage checks.
  Assignment: [key:A12][role:quader-software-architect][agent:019f3078-9c21-74a3-936a-2cf47dab9f34][status:integrated][authority:software] PM integrated A12 software authority approval for A11 glm math backend implementation.

















#15 [priority:high][type:enhancement][area:architecture] Replace Quader logging backend with spdlog - Use spdlog internally while preserving Quader-owned logging APIs.
  Archived: [at:2026-07-05T08:05:33+02:00][dev-version:0.1.0-dev.2][from:In Review][status:completed] PM-authorized archive.
  Brief: Source: Final Task Intake Report for third-party foundation dependencies. Add pinned spdlog dependency, convert modeling_foundation from header-only/interface as needed, keep LogLevel, log_message, sink/test hooks, and QUADER_LOG_* as Quader contracts, and prevent spdlog:: types from leaking into public foundation, UI, renderer, or document headers. Avoid hidden global logger behavior beyond a controlled foundation-owned logger and sink. Likely files: CMakeLists.txt, src/foundation/logging.hpp, new src/foundation/logging.cpp, src/foundation/assert.hpp, tests/unit/foundation/*, docs/dev changelog if dependency or build notes change. Tests-policy applies; assignment packets that plan, modify, or review tests must list agents/tests-policy.md as a required read. Verification: cmake --preset qt-mingw-debug; cmake --build --preset qt-mingw-debug; ctest --preset qt-mingw-debug; cmake --build --preset qt-mingw-debug --target check_architecture; python tools/project_board.py validate.
  Final owner: Foundation logging contract and foundation tests
  Plans: agents/plans/audit_20260704_full_architecture_master.md

  Freshness: [status:fresh][checked:2026-07-05T02:10:54Z] Revalidated current workspace: foundation logging remains header-only fwrite/sink implementation with no spdlog dependency or logging.cpp; task remains valid.

  PM: [run:20260705_tasks_12_17_foundation_deps][status:ready-for-review][manager:pending] Software authority approved; ready for user validation.
  Authority: [owner:software][status:approved][agent:019f3062-0523-7412-81e6-5ecb880ee785] Approved: spdlog remains private to foundation, public logging API stays Quader-owned, contract tests and verification passed. Residual version-tag pin is acceptable.
  Assignment: [key:A4][role:explorer][agent:019f301a-751b-79e2-8d90-65e599ae529e][status:integrated][authority:none] PM integrated A4 no-edit spdlog impact map.
  Assignment: [key:A9][role:worker][agent:019f3050-12d7-7e42-84e2-b05a138570c6][status:integrated][authority:none] PM integrated A9 spdlog backend implementation. Verification passed configure, build, CTest 180/180, check_architecture, board validation, and spdlog leakage check.
  Assignment: [key:A10][role:quader-software-architect][agent:019f3062-0523-7412-81e6-5ecb880ee785][status:integrated][authority:software] PM integrated A10 software authority approval for A9 spdlog backend implementation.
  Coordination: Software authority approved #15; move to In Review for user validation. Residuals are non-blocking: spdlog pin uses v1.15.3 tag and fallback stderr formatting is not contract-tested.


#14 [priority:high][type:enhancement][area:build] Adopt GoogleTest for Quader unit tests - Add GoogleTest as the C++ test library while keeping CTest presets as the runner.
  Archived: [at:2026-07-05T08:05:33+02:00][dev-version:0.1.0-dev.2][from:In Review][status:completed] PM-authorized archive.
  Brief: Source: Final Task Intake Report for third-party foundation dependencies. Add pinned GoogleTest dependency through CMake imported targets, include GoogleTest, convert current hand-rolled test executables to TEST or TEST_F cases, use gtest_discover_tests or equivalent CTest registration, and keep Qt6::Test only where needed for Qt model helpers. Do not test GoogleTest itself. Likely files: CMakeLists.txt, CMakePresets.json if needed, tests/unit/**, README.md, AGENTS.md, tools/check_architecture.py if GTest include boundaries are enforced. Tests-policy applies; assignment packets that plan, modify, or review tests must list agents/tests-policy.md as a required read. Verification: cmake --preset qt-mingw-debug; cmake --build --preset qt-mingw-debug; ctest --preset qt-mingw-debug; cmake --build --preset qt-mingw-debug --target check_architecture; python tools/project_board.py validate.
  Final owner: CMake test dependency setup and all current unit test source files
  Plans: agents/plans/audit_20260704_full_architecture_master.md

  Freshness: [status:fresh][checked:2026-07-05T02:10:50Z] Revalidated current workspace: tests remain standalone CTest executables with hand-rolled assertion macros and no GoogleTest dependency or gtest discovery; task remains valid.

  PM: [run:20260705_tasks_12_17_foundation_deps][status:ready-for-review][manager:pending] Software authority approved; ready for user validation.
  Authority: [owner:software][status:approved][agent:019f303f-870f-7432-8c11-937a2b2c222c] Approved: GoogleTest adoption has no blocking findings; configure, build, CTest 177/177, check_architecture, and board validation passed.
  Assignment: [key:A1][role:worker][agent:019f3011-db2e-7d51-862a-ddd424e9c407][status:integrated][authority:none] PM integrated A1 GoogleTest adoption report. Verification passed configure, build, CTest 177/177, check_architecture, and board validation; route to software authority review.
  Assignment: [key:A6][role:quader-software-architect][agent:019f303f-870f-7432-8c11-937a2b2c222c][status:integrated][authority:software] PM integrated A6 software authority approval for A1 GoogleTest adoption.
  Coordination: Software authority approved #14; move to In Review for user validation. Residual: deploy not run because this was test/build harness work; recurring non-fatal Ninja warning remains.

#12 [priority:high][type:enhancement][area:build-runtime] Rename the desktop executable to quader.exe - The desktop app executable should be named quader.exe for local builds, archives, launch controls, and future user-facing runtime bundles.
  Archived: [at:2026-07-05T08:05:33+02:00][dev-version:0.1.0-dev.2][from:In Review][status:completed] PM-authorized archive.
  Brief: Context: internal dev-build archives are being introduced, and archive/launch tooling should not hard-code the old QuaderQtBgfx.exe name long-term. Update CMake target/output naming and any docs, scripts, frontend launcher paths, dev-build archive/launch tooling, and launch controls that still assume QuaderQtBgfx.exe. Keep internal target naming scoped and avoid renderer/backend names in the product executable. Verification: configure, build, deploy, dev-build archive/launch tooling, and frontend launch controls after the rename.
  Final owner: CMake target/output naming, docs/scripts/frontend launcher paths, dev-build archive/launch tooling

  Freshness: [status:fresh][checked:2026-07-05T02:10:46Z] Revalidated current workspace: CMake, README, AGENTS, dev-build tooling, and frontend launch code still carry QuaderQtBgfx.exe fallback/name assumptions; rename remains valid.

  PM: [run:20260705_tasks_12_17_foundation_deps][status:ready-for-review][manager:pending] Software authority approved; ready for user validation.
  Authority: [owner:software][status:approved][agent:019f304f-c21a-7890-a0a4-c9d054917601] Approved: internal target quader_app, runtime output quader.exe, target-based shader/deploy refs, and legacy archive launch support are acceptable. Residual stale build exe and historical prose are non-blocking.
  Assignment: [key:A3][role:explorer][agent:019f301a-3fec-7f03-a430-b195c9d82ccd][status:integrated][authority:none] PM integrated A3 no-edit executable rename impact map.
  Assignment: [key:A7][role:worker][agent:019f303f-f231-7582-b268-6a237832eac9][status:integrated][authority:none] PM integrated A7 quader.exe rename implementation. Verification passed configure, build, CTest 183/183, check_architecture, deploy, frontend typecheck/build, dev-build tooling checks, and board validation.
  Assignment: [key:A8][role:quader-software-architect][agent:019f304f-c21a-7890-a0a4-c9d054917601][status:integrated][authority:software] PM integrated A8 software authority approval for A7 quader.exe rename.
  Coordination: Software authority approved #12; move to In Review for user validation. Residuals are non-blocking: stale generated QuaderQtBgfx.exe may remain locally and old archives intentionally keep legacy executable names.


#11 [priority:low][type:enhancement][area:crimson] Mature diagnostics, performance hooks, and golden images - Add performance counters, diagnostics presentation, and stable visual regression support.
  Archived: [at:2026-07-05T08:05:33+02:00][dev-version:0.1.0-dev.2][from:In Review][status:completed] PM-authorized archive.
  Brief: Source: agents/plans/audit_20260704_full_architecture_master.md Batch 10. Add render culling, draw packet sorting, instancing batch keys, dirty resource uploads, expanded FrameStats, UI diagnostics panel/status presentation fed by typed services, golden image harness after render graph/color/material foundations are deterministic, and benchmark hooks for mesh traversal/ops and renderer upload paths. Tests-policy applies. Verification: performance counters exist before optimization work; golden images are stable and not brittle placeholders.
  Final owner: src/crimson performance paths, diagnostics services/panel, tests/golden image harness, benchmark hooks
  Plans: agents/plans/audit_20260704_full_architecture_master.md












  PM: [run:20260704_tasks_1_11_batch][status:ready-for-review][manager:pending] #11 multi-authority approved; ready for user validation. Do not mark complete.
  Authority: [owner:multi][status:approved][agent:019f2f93-d59f-7071-9105-6f142063ea3f] Approved: renderer authority A61 approved A58 repairs and A51/A53 renderer scaffold; UI authority A62 approved A59 diagnostics panel/status presentation. Residuals accepted as non-blocking: no GPU instanced submission, no runtime golden capture, no real golden baselines yet, and recurring Ninja recovery warning.
  Coordination: A61 renderer approval and A62 UI approval are complete. Root integrated verification passed: build, CTest 37/37, check_architecture, board validation, deploy, serial configure, and final build. Move #11 from In Progress to In Review for user validation.

  Freshness: [status:fresh][checked:2026-07-05] PM revalidated current workspace: #8 and #9 are authority-approved/In Review, so #11 diagnostics/performance/golden planning is unblocked.

  Assignment: [key:A49][role:quader-renderer-architect][agent:019f2f5d-5c80-7e72-9c0b-70634164fcce][status:integrated][authority:renderer] PM integrated #11 renderer plan. Implementation should follow staged batches and return to renderer authority after verification; UI diagnostics panel remains a UI-authority follow-up.
  Assignment: [key:A51][role:worker][agent:019f2f65-870f-7a32-a559-d2ee7ef930ea][status:integrated][authority:none] PM integrated A51 #11 foundation slice. Residuals: GPU instanced submission not wired, golden-image harness and UI diagnostics panel deferred by scope, recurring Ninja recovery warning did not fail commands.
  Assignment: [key:A53][role:worker][agent:019f2f73-68a6-7e23-82e5-0b6094098edc][status:integrated][authority:none] PM integrated A53 viewport diagnostics handoff and golden comparator scaffold. Residuals: golden runtime capture, golden baselines, and UI diagnostics panel remain unresolved by design.
  Assignment: [key:A55][role:quader-renderer-architect][agent:019f2f7c-7127-7102-a3da-ff04fbf86ab3][status:integrated][authority:renderer] PM integrated A55 renderer changes-requested review. Required repair: pass view aspect into packet culling with non-square test, wire live upload counters or narrow claimed scope, and bound diagnostics snapshot collection.
  Assignment: [key:A56][role:quader-ui-architect][agent:019f2f7c-b9d2-7481-90f6-00e57468e954][status:integrated][authority:ui] PM integrated A56 UI diagnostics panel plan. Implementation should follow planned UI-only batches and return to UI authority review.
  Assignment: [key:A58][role:worker][agent:pending][status:integrated][authority:none] PM integrated A58 renderer repair after root current-workspace verification passed on combined A58/A59 tree.
  Assignment: [key:A59][role:worker][agent:pending][status:integrated][authority:none] PM integrated A59 UI diagnostics panel/status presentation slice.
  Assignment: [key:A61][role:quader-renderer-architect][agent:019f2f93-d59f-7071-9105-6f142063ea3f][status:integrated][authority:renderer] PM integrated A61 renderer authority approval for #11.
  Assignment: [key:A62][role:quader-ui-architect][agent:pending][status:integrated][authority:ui] PM integrated A62 UI authority approval for #11 diagnostics panel/status presentation.


#10 [priority:low][type:enhancement][area:io] Add I/O and asset boundaries - Introduce importer/exporter interfaces and deterministic material mapping.
  Archived: [at:2026-07-05T08:05:33+02:00][dev-version:0.1.0-dev.2][from:In Review][status:completed] PM-authorized archive.
  Brief: Source: agents/plans/audit_20260704_full_architecture_master.md Batch 9. Add importer/exporter interfaces after mesh/document validation exists, keep file dialogs in UI services, make importers produce documents or fragments plus structured diagnostics, map glTF materials through Crimson material schemas without graphics-runtime internals, and start with static registration. Tests-policy applies. Verification: import/export has no UI dependency, imported meshes validate before editable state, material mapping is deterministic and test-covered.
  Final owner: src/io importer/exporter interfaces, UI file-dialog service integration, glTF material mapping tests
  Plans: agents/plans/audit_20260704_full_architecture_master.md

  PM: [run:20260704_tasks_1_11_batch][status:ready-for-review][manager:pending] Software authority approved #10; ready for user validation. Do not mark complete.
  Authority: [owner:software][status:approved][agent:019f2f83-4e36-79d0-824a-b57cea6be6ea] Approved: A50, A52, A54, A57, and A60 satisfy #10. Core I/O contracts, static registry/services, document-fragment validation gates, UI file-dialog boundary, and renderer-approved glTF-to-Crimson material mapping preserve architecture boundaries. Accepted residuals: no production parser/exporter, no document material assignment, no texture asset pipeline, and import remains parsed-not-applied until future authorized work.
  Coordination: Software authority approved #10; move from In Progress to In Review for user validation. After these board updates and validation pass, agents/plans/implementation_task10_io_asset_boundaries_doc.md may be moved to agents/archive. Keep the master audit active while linked tasks remain.
  Freshness: [status:fresh][checked:2026-07-05] PM revalidated current workspace: #6, #7, and #9 are authority-approved/In Review, so #10 I/O and asset boundaries are unblocked for planning.


  Assignment: [key:A48][role:quader-software-architect][agent:019f2f5d-2ab9-7e20-876a-8c57fd85c1e4][status:integrated][authority:software] PM integrated #10 software architecture plan. Implementation should follow the active plan and return to software authority review after implementation; glTF-to-Crimson material mapping requires renderer authority review.

  Assignment: [key:A50][role:worker][agent:019f2f63-abc7-7bc3-96d9-f502871fafee][status:integrated][authority:none] PM integrated #10 core I/O batches 1-3 from the active software plan. Remaining #10 work includes UI file-dialog boundary, glTF-to-Crimson material mapping with renderer review, and final software review.
  Assignment: [key:A52][role:worker][agent:019f2f6d-4243-7d21-a58a-74edd3683117][status:integrated][authority:none] PM integrated A52 UI file-dialog boundary slice. Remaining #10 implementation is deterministic glTF-to-Crimson material mapping, followed by renderer review and final software authority review.
  Assignment: [key:A54][role:worker][agent:019f2f77-ef72-7c10-af7e-9e4149b080cf][status:integrated][authority:none] PM integrated A54 glTF-to-Crimson material mapping slice. Route to renderer authority for focused schema, color-space, alpha-mode, deterministic ordering, and GPU-boundary review before final software approval.
  Assignment: [key:A57][role:quader-renderer-architect][agent:019f2f7f-5602-75d3-a20c-bd519cb9b0d6][status:integrated][authority:renderer] PM integrated A57 renderer approval for #10 material mapping slice.
  Assignment: [key:A60][role:quader-software-architect][agent:019f2f83-4e36-79d0-824a-b57cea6be6ea][status:integrated][authority:software] PM integrated A60 software authority approval for #10.


#9 [priority:medium][type:enhancement][area:crimson] Build Crimson V1 rendering correctness foundation - Add renderer device, shader, color, material, overlay, picking, and diagnostics foundations.
  Archived: [at:2026-07-05T08:05:33+02:00][dev-version:0.1.0-dev.2][from:In Review][status:completed] PM-authorized archive.
  Brief: Source: agents/plans/audit_20260704_full_architecture_master.md Batch 8. Implement backend selection/capability diagnostics, shader manifest and multi-target output layout, linear HDR scene target and tone-map/final conversion, color-space tests, material/base-shader system for OpaquePbr then AlphaCutoutPbr, TransparentPbr, OverlayUnlit, overlay system/grid commands, picking target/readback, structured diagnostics, resource RAII/caches, and frame stats. Tests-policy applies. Verification: no runtime shader compilation; overlays stay out of lit/post paths; picking uses non-color-managed request-scoped IDs; renderer tests cover each feature as it lands.
  Final owner: src/crimson, src/crimson/gpu, shaders, shader tools/manifests, renderer tests
  Plans: agents/plans/audit_20260704_full_architecture_master.md

  PM: [run:20260704_tasks_1_11_batch][status:ready-for-review][manager:pending] #9 renderer authority approved; ready for user validation. Do not mark complete.
  Authority: [owner:renderer][status:approved][agent:019f2f54-0396-7283-b358-54f09ac13788] Renderer authority approved #9 after A45 repair. A44 blockers repaired and #19 double-cube fix user-verified.
  Coordination: A47 approved #9. Runtime submission follows graph-backed DepthPrepass -> HDR PBR -> ToneMap -> overlays -> Present; public RenderObject no longer exposes arbitrary shader program selection; prototype one red cube plus grid behavior is preserved. Residuals: no headless GPU render/readback harness or golden-image validation.

  Assignment: [key:A29][role:quader-renderer-architect][agent:019f2ebc-34f7-7401-a3fe-c35804de3943][status:integrated][authority:renderer] PM integrated fresh #9 renderer plan: agents/plans/implementation_task9_crimson_v1_correctness_renderer_doc.md.
  Assignment: [key:A31][role:worker][agent:019f2ec9-e2c0-7861-b18c-e7a01ecd54f4][status:integrated][authority:none] PM integrated backend/capability diagnostics and GPU RAII/resource base slice. Worker verification passed configure, build, CTest 18/18, check_architecture, and board validation.
  Assignment: [key:A32][role:worker][agent:019f2eca-3198-7ac3-8956-4c5f807b0ea1][status:integrated][authority:none] PM integrated shader manifest and multi-target shader output layout slice. Worker verification passed shader manifest validation, configure, build, CTest 18/18, check_architecture, and board validation.

  Assignment: [key:A35][role:worker][agent:019f2ed8-53ce-7b80-965d-9bc78dd8b7b6][status:integrated][authority:none] PM integrated HDR scene target, tone-map/final conversion scaffolding, render graph resource integration, and post shader manifest scaffolding. Worker verification passed manifest validation, configure, build, CTest 21/21, check_architecture, board validation, and compile_shaders.
  Assignment: [key:A36][role:worker][agent:019f2ed8-a605-76e3-a051-f14c7ad9c3f1][status:integrated][authority:none] PM integrated CPU material schema and base-shader descriptor slice. Worker verification passed configure, build, CTest 21/21, check_architecture, and board validation.
  Assignment: [key:A38][role:worker][agent:019f2ee8-ec7a-7aa2-893b-d455e5124e03][status:integrated][authority:none] PM integrated GPU material/PBR runtime integration. Prototype cube now submits through OpaquePbr packet/material/program/cache path. Worker verification passed manifest validation, configure, build, CTest 22/22, check_architecture, board validation, compile_shaders, and compiled-output validation. Root restored #13 after stale board state; current board validates with 13 entries.
  Assignment: [key:A41][role:worker][agent:019f2efb-0229-7c41-b8cc-7fcbb6a4af6c][status:integrated][authority:none] PM integrated Crimson overlay/grid command migration. Grid now enters FrameSnapshot as immutable OverlayUnlit commands and is submitted by a separate GPU overlay renderer after tone-map graph passes. Worker verification passed manifest validation, configure, build, CTest 23/23, check_architecture, board validation, and compile_shaders. Root rebuilt QuaderQtBgfx.exe from current tree after A41.
  Assignment: [key:A42][role:worker][agent:019f2f05-96f7-7b72-a256-1c7614343bdd][status:integrated][authority:none] PM integrated #9 picking request/readback slice. Worker verification passed shader manifest validation, compile_shaders, configure, build, CTest 24/24, check_architecture, and board validation. Root rebuilt QuaderQtBgfx.exe from current tree after A42. Residual: no headless GPU render/readback harness; no UI selection or document mutation added by design.
  Assignment: [key:A43][role:worker][agent:019f2f13-8dbd-7b12-a294-96390dfc046f][status:integrated][authority:none] PM integrated final Crimson diagnostics/frame stats/runtime shader deploy cleanup. Worker verification passed shader manifest validation, configure, compile_shaders, compiled-output validation, build, CTest 25/25, check_architecture, deploy, and board validation. Root rebuilt QuaderQtBgfx.exe from current tree after A43.
  Assignment: [key:A44][role:quader-renderer-architect][agent:019f2f1d-d840-7483-9b68-447da15cb029][status:integrated][authority:renderer] PM integrated renderer authority changes-requested review. Required repair: real graph-backed HDR/tone-map/present frame resources and runtime submission, plus removal or quarantine of public RenderObject shader program selection.
  Assignment: [key:A45][role:worker][agent:019f2f23-5e90-7aa3-9cac-236a830ab50a][status:integrated][authority:none] PM integrated A45 #9 repair. Worker verification passed shader manifest validation, compile_shaders, configure, build, CTest 25/25, check_architecture, deploy, and board validation. User manually verified the linked critical double-cube bug fixed and said batch work can continue.
  Assignment: [key:A47][role:quader-renderer-architect][agent:019f2f54-0396-7283-b358-54f09ac13788][status:integrated][authority:renderer] PM integrated A47 renderer authority approval for #9.


#8 [priority:medium][type:enhancement][area:ui] Replace prototype inspector with UI models and panels - Move inspector and workspace UI onto document-backed models and services.
  Archived: [at:2026-07-05T08:05:33+02:00][dev-version:0.1.0-dev.2][from:In Review][status:completed] PM-authorized archive.
  Brief: Source: agents/plans/audit_20260704_full_architecture_master.md Batch 7. Replace inline inspector placeholder with panel classes, add workspace save/restore/reset through SettingsService and PanelHost, add DocumentItemModel, PropertyItemModel, named roles, property descriptors, and selection adapters after document selection exists. Route document-backed edits through commands. Tests-policy applies. Verification: inspector no longer owns document truth; QAbstractItemModelTester is used where applicable; workspace state persists with versioned keys.
  Final owner: src/ui panels, models, delegates/adapters, settings keys, UI model tests
  Plans: agents/plans/audit_20260704_full_architecture_master.md

  PM: [run:20260704_tasks_1_11_batch][status:ready-for-review][manager:pending] UI authority approved; ready for user review.
  Authority: [owner:ui][status:approved][agent:019f2efa-cc95-7dd3-8354-608287d9b474] Approved: document-backed Scene and Properties panels, UI models/descriptors, command-routed edits, selection adapter, panel visibility actions, workspace v2 persistence, and UI tests satisfy agents/plans/implementation_task8_ui_models_panels_ui_doc.md. Residual: no human manual GUI smoke.
  Coordination: UI authority approved #8; move to In Review for user validation. Do not mark complete.

  Assignment: [key:A33][role:quader-ui-architect][agent:019f2eca-829c-73e1-8742-ce2eab5649af][status:integrated][authority:ui] PM integrated active #8 UI plan: agents/plans/implementation_task8_ui_models_panels_ui_doc.md.
  Assignment: [key:A34][role:worker][agent:019f2ed6-75e3-76f0-89ad-83c2074303a5][status:integrated][authority:none] PM integrated #8 batch 1 DocumentUiController/action-wiring refactor. Worker verification passed configure, build, CTest 18/18, check_architecture, and board validation.
  Assignment: [key:A37][role:worker][agent:019f2ee0-4608-72e0-a9b6-5dc8b02a65c9][status:integrated][authority:none] PM integrated #8 batch 2 model roles/descriptors, DocumentItemModel, PropertyItemModel, selection adapter, delegate support, and QAbstractItemModelTester coverage. Worker verification passed configure, build, CTest 22/22, check_architecture, and board validation.
  Assignment: [key:A39][role:worker][agent:019f2eec-f6c8-71e3-bb0d-df935f633476][status:integrated][authority:none] PM integrated #8 batch 3 panels/workspace slice. ScenePanel and PropertiesPanel replaced prototype inspector, panel visibility actions added, workspace v2 persistence moved through SettingsService/PanelHost. Worker verification passed configure, build, CTest 22/22, focused ui_shell_services, check_architecture, and board validation. Residual: no manual GUI smoke; final redeploy blocked by running QuaderQtBgfx.exe PID 47136.
  Assignment: [key:A40][role:quader-ui-architect][agent:019f2efa-cc95-7dd3-8354-608287d9b474][status:reported][authority:ui] UI authority approved task #8 after final review of A34/A37/A39 integration. Verification passed: configure, QuaderQtBgfx executable build, CTest 22/22, check_architecture, board validation, and deploy. Residual: no human manual GUI smoke.


#7 [priority:high][type:enhancement][area:commands] Add document, selection, commands, and tool shell - Introduce document truth, command history, undo/redo, and initial tool interfaces.
  Archived: [at:2026-07-05T08:05:33+02:00][dev-version:0.1.0-dev.2][from:In Review][status:completed] PM-authorized archive.
  Brief: Source: agents/plans/audit_20260704_full_architecture_master.md Batch 6. Add Document, scene object store, object IDs, transforms, active selection, dirty flags, typed document events, ICommand, CommandResult, CommandHistory, undo/redo tests, initial non-topology commands, ToolManager shell, and tool preview/command interfaces. Wire Undo, Redo, and simple document-safe actions through ActionRegistry and CommandHistory. Tests-policy applies. Verification: user-visible document mutations go through commands; undo/redo drives action state; UI/renderer update from events or snapshots.
  Final owner: src/document, src/commands, src/tools, UI action wiring, command tests
  Plans: agents/plans/audit_20260704_full_architecture_master.md

  PM: [run:20260704_tasks_1_11_batch][status:ready-for-review][manager:pending] Software authority approved; ready for user review.
  Authority: [owner:software][status:approved][agent:019f2ec0-1b8d-7da3-86c2-68f3026f7c86] Approved: document truth, command history, toolkit-neutral tools, and UI action wiring satisfy task #7. Accepted residual scope: single-object DeleteSelection only; no macro/multi-delete, create/duplicate/file I/O, #8 panels/models, viewport picking, or topology tools.
  Coordination: Software authority approved #7; move to In Review for user validation.

  Assignment: [key:A18][role:worker][agent:019f2e87-ba61-7aa2-9433-d3c9c53baef3][status:integrated][authority:none] PM integrated A18 document/selection slice; configure, build, CTest 9/9, check_architecture, and board validation passed.
  Assignment: [key:A19][role:worker][agent:019f2e88-0562-78f2-a09d-19e736b0bff6][status:integrated][authority:none] PM integrated blocked A19 report; no code changes were made and command work is being replaced by A23 after A18 APIs landed.
  Assignment: [key:A20][role:worker][agent:019f2ea4-c60a-7bc2-959d-23a9a3b557d3][status:integrated][authority:none] PM integrated A20 toolkit-neutral tool shell; root reconfigured and rebuilt QuaderQtBgfx successfully after concurrent #5 source registration.
  Assignment: [key:A23][role:worker][agent:019f2e97-d94f-74c1-b402-aea0b4dbf20e][status:integrated][authority:none] PM integrated A23 command-history slice; configure, build after patch, CTest 10/10, check_architecture, and board validation passed.
  Assignment: [key:A27][role:worker][agent:019f2eb4-8dad-7640-8b57-d0f5fb0a4825][status:integrated][authority:none] PM integrated A27 UI action wiring; configure, build, CTest 14/14, check_architecture, and board validation passed.

  Assignment: [key:A30][role:quader-software-architect][agent:019f2ec0-1b8d-7da3-86c2-68f3026f7c86][status:reported][authority:software] Software authority approved final task #7 implementation.


#6 [priority:high][type:enhancement][area:mesh] Implement mesh core and validation - Create independent mesh data structures, validation, and fixtures.
  Archived: [at:2026-07-05T08:05:32+02:00][dev-version:0.1.0-dev.2][from:In Review][status:completed] PM-authorized archive.
  Brief: Source: agents/plans/audit_20260704_full_architecture_master.md Batch 5. Implement mesh/core IDs, records, creation/deletion primitives, attribute storage hooks, traversal helpers, validation, and required math/geometry value types and algorithms. Add mesh fixtures and unit tests. Tests-policy applies. Verification: mesh code has no Qt, UI, command, tool, or renderer dependency; tests cover invalid IDs, boundary placeholders, and basic topology invariants.
  Final owner: src/modeling mesh, math, geometry modules and tests
  Plans: agents/plans/audit_20260704_full_architecture_master.md













  PM: [run:20260704_tasks_1_11_batch][status:waiting-on-authority][manager:pending] A13 integrated; awaiting software authority review.
  Authority: [owner:software][status:approved][agent:019f2e7c-9dc8-71d3-8e82-8c957b3f3969] Approved: mesh/math/geometry core preserves dependency direction; foundation IDs/Result/diagnostics usage, validation, traversal, attribute hooks, fixtures, and tests satisfy Batch 5. Boundary placeholders accepted as scoped residual risk.
  Assignment: [key:A6][role:explorer][agent:019f2e4e-9ada-76e2-92ce-eb535cae4654][status:integrated][authority:none] PM integrated mesh scout report.
  Coordination: Software authority approved #6; move to In Review for user validation. Full boundary-loop topology and adjacent-face stitching remain deferred.
  Assignment: [key:A13][role:worker][agent:019f2e6d-d3dd-71b2-9f36-69d7461789cf][status:integrated][authority:none] PM integrated A13 mesh/math/geometry core report; worker verification passed configure, build, CTest, check_architecture, and board validation.
  Assignment: [key:A16][role:quader-software-architect][agent:019f2e7c-9dc8-71d3-8e82-8c957b3f3969][status:reported][authority:software] Software authority approved the #6 mesh/math/geometry core implementation.

#5 [priority:high][type:enhancement][area:crimson] Introduce renderer snapshots and minimal render graph - Move rendering to immutable frame input and explicit pass ordering.
  Archived: [at:2026-07-05T08:05:32+02:00][dev-version:0.1.0-dev.2][from:In Review][status:completed] PM-authorized archive.
  Brief: Source: agents/plans/audit_20260704_full_architecture_master.md Batch 4. Add FrameSnapshot, FrameBuilder, RenderWorld, RenderObject, RenderCamera, RenderLight, RenderEnvironment, ViewportSettings, FrameStats, minimal RenderGraph, RenderPass, RenderResourceDesc, RenderResourceRegistry, named resources, dependency validation, resize behavior, and debug dump. Convert prototype cube/grid input to snapshot or fixture data. Tests-policy applies. Verification: renderer renders from immutable input, graph order is explicit and test-covered, Crimson GPU receives prepared packets/resources only.
  Final owner: src/crimson frame data and render graph files, prototype render adaptation, render graph tests
  Plans: agents/plans/audit_20260704_full_architecture_master.md

  PM: [run:20260704_tasks_1_11_batch][status:ready-for-review][manager:pending] Renderer authority approved; ready for user review.
  Authority: [owner:renderer][status:approved][agent:019f2eb4-2a42-7a52-97ec-d9131c2a80b7] Approved: immutable FrameSnapshot and FrameBuilder handoff, explicit/tested minimal RenderGraph order, prepared prototype cube/grid packets into Crimson GPU submission, grid kept as OverlayGridPass, and no task #9 PBR/material/picking/color/tone-map/shader-manifest work landed.
  Assignment: [key:A5][role:quader-renderer-architect][agent:019f2e4e-828f-7753-971b-c41111750256][status:integrated][authority:renderer] PM integrated renderer snapshot/render-graph preflight.
  Coordination: Renderer authority approved #5; move to In Review for user validation. Residual risk remains no interactive viewport smoke test.

  Assignment: [key:A26][role:worker][agent:019f2ea2-9a35-72c1-ada8-2632a2eb7853][status:integrated][authority:none] PM integrated A26 renderer snapshot/render graph implementation; configure, build, CTest 13/13, check_architecture, and board validation passed.
  Assignment: [key:A28][role:quader-renderer-architect][agent:019f2eb4-2a42-7a52-97ec-d9131c2a80b7][status:reported][authority:renderer] Renderer authority approved task #5 implementation.


#4 [priority:critical][type:enhancement][area:viewport] Extract viewport and Crimson GPU boundary - Split Qt viewport hosting from Crimson runtime ownership while preserving the prototype viewport.
  Archived: [at:2026-07-05T08:05:32+02:00][dev-version:0.1.0-dev.2][from:In Review][status:completed] PM-authorized archive.
  Brief: Source: agents/plans/audit_20260704_full_architecture_master.md Batch 3. Add ViewportWidget, ViewportController, ViewportLayoutState, ViewportCameraController, narrow viewport render-host interface, Renderer config/types, project-owned render handles, structured diagnostics, and src/crimson/gpu device/resource shells. Move graphics-runtime includes, handles, platform-data conversion, init/shutdown/reset, shader loading, and submit calls out of public UI headers. Tests-policy applies. Verification: no public UI header exposes graphics-runtime types; only crimson links the runtime; viewport controller tests run without GPU; prototype still renders.
  Final owner: src/ui viewport files, src/crimson, src/crimson/gpu, BgfxWidget replacement path, CMake target links
  Plans: agents/plans/audit_20260704_full_architecture_master.md

  PM: [run:20260704_tasks_1_11_batch][status:ready-for-review][manager:pending] Authority approved; ready for user review.
  Authority: [owner:multi][status:approved][agent:019f2e9c-ffa5-7cb3-9312-d34149d120a7] Approved: renderer A22 and UI A25 approved task #4 after A24 teardown repair.
  Assignment: [key:A4][role:quader-ui-architect][agent:019f2e4e-64ca-7913-b77a-f32a1158c31a][status:integrated][authority:ui] PM integrated UI viewport preflight.
  Assignment: [key:A5][role:quader-renderer-architect][agent:019f2e4e-828f-7753-971b-c41111750256][status:integrated][authority:renderer] PM integrated renderer boundary preflight.
  Coordination: Task #4 approved by renderer A22 and UI A25 after A24 lifecycle repair. Residual risks: no interactive GUI smoke test, Vulkan/SPIR-V path remains until future renderer packaging work.
  Assignment: [key:A9][role:worker][agent:019f2e59-d176-72f0-aebd-a718e41eca8b][status:integrated][authority:none] PM integrated A9 Crimson shell implementation; verification passed and BgfxWidget allowlist remains temporary.
  Assignment: [key:A11][role:quader-renderer-architect][agent:019f2e68-f1ba-7550-a868-1b1c43201287][status:reported][authority:renderer] Renderer authority approved the A9 Crimson public/GPU shell slice for continuing #4 extraction; this is not full task #4 approval.
  Assignment: [key:A15][role:worker][agent:019f2e7a-6f4a-7362-bda0-4ded39e6ac55][status:integrated][authority:none] PM integrated A15 viewport/Crimson extraction report; configure, build, CTest 8/8, check_architecture, and board validation passed.
  Assignment: [key:A21][role:quader-ui-architect][agent:019f2e97-47c7-74c1-bf76-a022828f3262][status:reported][authority:ui] UI authority changes requested: MainWindow-owned ViewportController/IViewportRenderHost can be destroyed before the Qt-owned ViewportWidget central widget, leaving ViewportWidget::~ViewportWidget() with a dangling controller reference. Boundary checks and CTest passed; no GUI smoke test was run.
  Assignment: [key:A22][role:quader-renderer-architect][agent:019f2e97-8d13-7402-9b62-27addc48dd93][status:reported][authority:renderer] Renderer authority approved A15 task #4 Crimson GPU/runtime boundary. Runtime includes are confined to src/crimson/gpu, public UI/Crimson adapter APIs do not expose graphics-runtime handles, bgfx/bx are linked only through crimson PRIVATE, structured diagnostics are present, and no #5 snapshot/render-graph or #9 PBR/material/picking work was added. Verification passed: check_architecture, CTest 9/9, and board validation.
  Assignment: [key:A24][role:worker][agent:root][status:reported][authority:none] Fixed #4 viewport teardown ordering after A21 UI review and added UI lifecycle smoke test; configure/build/CTest/check_architecture/board validation passed.
  Assignment: [key:A25][role:quader-ui-architect][agent:019f2e9c-ffa5-7cb3-9312-d34149d120a7][status:reported][authority:ui] UI authority approved A24 teardown repair. MainWindow now deletes the central ViewportWidget before viewport controller/render-host members are released; UI viewport boundary and non-GPU UI tests remain acceptable.


#3 [priority:critical][type:enhancement][area:architecture] Build foundation and app shell services - Add foundation primitives, composition root, and initial UI service layer.
  Archived: [at:2026-07-05T08:05:32+02:00][dev-version:0.1.0-dev.2][from:In Review][status:completed] PM-authorized archive.
  Brief: Source: agents/plans/audit_20260704_full_architecture_master.md Batch 2. Add strong IDs, Result/error helpers, assertions/logging stubs, app composition root, Qt bootstrap/style policy, UiContext, ActionId, ActionRegistry, ActionStateUpdater, SettingsService, NotificationService, PanelId, PanelHost, and refactor MainWindow into a shell receiving UiContext. Disable unavailable document/tool actions. Tests-policy applies. Verification: visible prototype behavior is preserved except disabled unavailable actions; services are testable without Crimson GPU construction.
  Final owner: src/app, src/foundation, src/ui service files, MainWindow refactor, related tests
  Plans: agents/plans/audit_20260704_full_architecture_master.md

  PM: [run:20260704_tasks_1_11_batch][status:ready-for-review][manager:pending] Software authority approved; ready for user review.
  Authority: [owner:software][status:approved][agent:019f2e80-936f-7e31-a05f-82fe9ec8f18e] Approved: foundation primitives, app composition root, UI shell services, disabled unavailable document/tool/create actions, service tests, and boundary checks satisfy task #3. BgfxWidget runtime dependency remains scoped to task #4.
  Assignment: [key:A3][role:quader-software-architect][agent:019f2e4e-469e-77a0-b715-cfe0f37d2afe][status:integrated][authority:software] PM integrated software preflight plan.
  Assignment: [key:A4][role:quader-ui-architect][agent:019f2e4e-64ca-7913-b77a-f32a1158c31a][status:integrated][authority:ui] PM integrated UI preflight plan.
  Coordination: Software authority approved task #3; move to In Review for user validation. Residual BgfxWidget graphics-runtime ownership remains task #4 work.
  Assignment: [key:A8][role:worker][agent:019f2e59-a43b-7ae3-a4eb-e543c2317d67][status:integrated][authority:none] PM integrated A8 foundation-only implementation; configure, build, CTest, check_architecture, and board validation passed.
  Assignment: [key:A10][role:quader-software-architect][agent:019f2e68-c5a0-7062-ba73-456ae8d51514][status:integrated][authority:software] PM integrated A10 approval of A8 foundation contracts only; full #3 remains pending.
  Assignment: [key:A12][role:worker][agent:019f2e69-274d-7ee3-9656-55479cd1aef1][status:integrated][authority:none] PM integrated A12 app/UI shell report; local configure, build, CTest, check_architecture, and board validation passed.
  Assignment: [key:A14][role:quader-ui-architect][agent:019f2e7a-0d2f-7530-b4d7-57683dd47f83][status:reported][authority:ui] UI authority approved A12 app/UI shell slice: MainWindow receives UiContext, canonical actions and disabled unavailable document/tool/create actions are in place, settings/notification services and PanelHost/prototype inspector contracts are acceptable, Fusion policy is scoped, and UI tests avoid GPU construction.
  Assignment: [key:A17][role:quader-software-architect][agent:019f2e80-936f-7e31-a05f-82fe9ec8f18e][status:reported][authority:software] Software authority approved final task #3 implementation.


#2 [priority:critical][type:enhancement][area:build] Split module targets and add test harness - Create enforceable CMake module boundaries and documented CTest support.
  Archived: [at:2026-07-05T08:05:32+02:00][dev-version:0.1.0-dev.2][from:In Review][status:completed] PM-authorized archive.
  Brief: Source: agents/plans/audit_20260704_full_architecture_master.md Batch 1. Split CMakeLists.txt into thin initial module targets for foundation, math, geometry, mesh, document, commands, tools, crimson, modeling_ui, and the app executable. Add CTest, a test preset, architecture guardrails for Qt Widgets and graphics-runtime includes, and update README/AGENTS with test commands. Tests-policy applies. Verification: configure/build preset works, ctest preset passes, boundary checks run.
  Final owner: CMakeLists.txt, CMakePresets.json, README.md, AGENTS.md, tests and architecture-check targets
  Plans: agents/plans/audit_20260704_full_architecture_master.md

  PM: [run:20260704_tasks_1_11_batch][status:ready-for-review][manager:pending] Software authority approved; ready for user review.
  Authority: [owner:software][status:approved][agent:019f2e59-7919-77d3-91ec-0621ef04bf56] Approved: thin module targets, CTest preset, check_architecture target/test, docs, and scoped temporary BgfxWidget allowlist are acceptable under the active master audit.
  Assignment: [key:A2][role:worker][agent:019f2e4e-2ab0-7ac0-bc3a-f79128e97288][status:integrated][authority:none] PM integrated A2 report; task #2 is ready for software authority review.
  Coordination: Software authority approved Batch 1 build/test guardrails; ready for user review.
  Assignment: [key:A7][role:quader-software-architect][agent:019f2e59-7919-77d3-91ec-0621ef04bf56][status:reported][authority:software] Software authority approved module split, CTest preset, architecture checks, and temporary BgfxWidget allowlist.


#1 [priority:critical][type:documentation][area:architecture] Freeze prototype growth - Make the master audit the guardrail for architecture-sensitive work.
  Archived: [at:2026-07-05T08:05:32+02:00][dev-version:0.1.0-dev.2][from:In Review][status:completed] PM-authorized archive.
  Brief: Source: agents/plans/audit_20260704_full_architecture_master.md Batch 0. Preserve prototype behavior, but do not add real document mutation, topology editing, selection truth, import/export, material editing, picking-driven selection, PBR, render graph features, or debug panels directly to MainWindow or BgfxWidget. Verification: confirm new feature work references this master audit or a follow-up implementation plan before implementation.
  Final owner: AGENTS.md, agents/workflow.md, agents/task_board.md, and active planning prompts
  Plans: agents/plans/audit_20260704_full_architecture_master.md

  PM: [run:20260704_tasks_1_11_batch][status:ready-for-review][manager:pending] Software authority approved; ready for user review.
  Authority: [owner:software][status:approved][agent:019f2e59-7919-77d3-91ec-0621ef04bf56] Approved: guardrails in AGENTS.md, workflow, task-board docs, and dev-changelog match the active master audit.
  Assignment: [key:A1][role:worker][agent:019f2e4e-0411-7290-b618-52d928c32f07][status:integrated][authority:none] PM integrated A1 report; task #1 is ready for software authority review.
  Coordination: Software authority approved Batch 0 guardrail docs; ready for user review.
  Assignment: [key:A7][role:quader-software-architect][agent:019f2e59-7919-77d3-91ec-0621ef04bf56][status:reported][authority:software] Software authority approved Batch 0 documentation guardrails.


#19 [priority:critical][type:bug][area:crimson] Critical viewport double-cube regression after #9/A45 renderer repair - After A45 interim fix and relaunch, the prototype viewport shows two cubes: a large white cube offset above the grid that appears to follow camera/view behavior, and a second black cube or silhouette near the axes. Screenshot evidence: C:\Users\Drako\AppData\Local\Temp\codex-clipboard-2c97f476-b03e-45e3-9e5c-bc717e2312be.png.
  Archived: [at:2026-07-05T07:27:40+02:00][dev-version:0.1.0-dev.2][from:Bugs][status:fixed] PM-authorized archive.
  Resolution: Fixed the double-cube color/depth split by deriving fullscreen render-target sampling UVs from bgfx runtime origin for tone-map and present, removing the present-only flip, and preserving the red single-cube prototype viewport.
  Brief: Blocks #9 and A45 approval. User reported double cube after A45 changed DepthOnly/OpaquePbr pass behavior. Renderer architect must triage before repair continues. Expected behavior is the preserved prototype single cube/grid viewport with #9 graph/PBR/overlay/picking foundations. Do not move #9 to In Review until fixed and renderer-reviewed.
  Freshness: [status:needs-refresh][checked:never] Awaiting start-gate revalidation.
  Final owner: renderer
  Plans: agents/plans/audit_20260704_full_architecture_master.md
  PM: [run:20260704_tasks_1_11_batch][status:ready-for-review][manager:pending] #19 no longer blocks #9 or batch progress after user manual verification and renderer approval. Leave recorded/open; do not complete/remove without explicit user request.
  Authority: [owner:renderer][status:approved][agent:019f2f54-0396-7283-b358-54f09ac13788] Renderer authority approved linked #19 fix as part of #9 re-review; user manually verified the double-cube bug fixed.
  Coordination: #19 blocking gate is satisfied and no longer blocks batch progress. Keep bug recorded/open in Bugs unless the user explicitly asks to complete, remove, or archive it.
  Assignment: [key:A46][role:quader-renderer-architect][agent:019f2f3a-ea1d-7211-a7fa-3f5ac44c33dd][status:reported][authority:renderer] Renderer triage directions reported. Diagnosis: visible double-cube is likely one cube with color/depth path mismatch; black foreground silhouette is scene depth occluding the depth-tested grid; white cube is PBR/HDR/tone-map/present color output misaligned or flipped, with prototype material defaulting to white. Repair A45 should fix runtime pass target binding, depth-only clear/write policy, tone-map/present orientation, and preserved prototype cube color. Manual user verification is required before #19 can be considered unblocked.
