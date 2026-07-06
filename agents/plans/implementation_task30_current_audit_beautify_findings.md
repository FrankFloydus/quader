# Implementation Plan - Task #30 Current Audit and Beautify Findings

## Source

- Board task: #30
- Audit artifact: `agents/plans/audit_20260706_current_code_master.md`

## Goals

- Fix A1: avoid repeated document mesh upload construction on clean viewport frames.
- Fix A2: preserve typed document changes into UI models and avoid property model resets when descriptor topology is unchanged.
- Fix B1: remove unused prototype cube/grid shader and fallback paths, then keep the live overlay shader under production names.

## Work Items

- [x] Add a `ViewportDocumentRenderCache` around document mesh render data.
- [x] Clear and prune viewport mesh cache at renderer lifecycle and object-list boundaries.
- [x] Add viewport cache tests proving unchanged meshes do not resubmit uploads and changed meshes do.
- [x] Add typed document change signal to `DocumentUiController`.
- [x] Update `PropertyItemModel` to use same-topology `dataChanged` refreshes for value-only object changes.
- [x] Add property model test for preview transform updates without model reset.
- [x] Remove unused viewport cube/grid shader ids, built-in unit-box fallback mesh, and inert Create Cube action.
- [x] Rename live overlay shader sources and compiled outputs.
- [x] Update shader manifest, CMake source lists, runtime lookup, and shader/UI tests.
- [x] Build app and tests.
- [x] Run targeted runtime tests.
- [x] Deploy debug runtime.
- [x] Archive dev build `0.1.1-dev.31` after repairing dev-build counter recovery.
- [x] Validate board metadata and move #30 to `In Review`.

## Verification

Planned commands:

```powershell
cmake --build --preset qt-mingw-debug --parallel 20
cmake --build --preset qt-mingw-debug-tests --parallel 20
ctest --preset qt-mingw-debug-runtime -R "(ui_model_tests|ui_viewport_tests|crimson_shader_manifest_tests|crimson_gpu_shell_tests)" --output-on-failure
cmake --build --preset qt-mingw-debug-deploy --parallel 20
python tools/dev_builds.py archive --preset qt-mingw-debug
python tools/project_board.py validate
```

No clang-format, clang-tidy, style/static-analysis target, or style-inclusive CTest preset is planned.

## Verification Results

- `cmake --build --preset qt-mingw-debug --parallel 20`: passed.
- `cmake --build --preset qt-mingw-debug-tests --parallel 20`: passed.
- Focused runtime tests for UI models/actions, viewport cache, shader manifest/library, render packet pipeline, material packets, and public renderer types: 102/102 passed.
- `ctest --preset qt-mingw-debug-runtime -R "(crimson_shader_manifest_tests|crimson_shader_manifest_compiled_outputs)" --output-on-failure`: 4/4 passed after generated shader cleanup.
- Generated shader output check found no `fs_cube`, `vs_cube`, `fs_grid`, or `vs_grid` files in `build/qt-mingw-debug/compiled_shaders` or `build/qt-mingw-debug/shaders` after the CMake cleanup.
- `cmake --build --preset qt-mingw-debug-deploy --parallel 20`: passed.
- Initial `python tools/dev_builds.py archive --preset qt-mingw-debug`: blocked because `C:\Users\Drako\Desktop\quader-dev-builds\0.1.1-dev.28` already existed while dev-build metadata still reported current `0.1.1-dev.27`.
- Dev-build repair: `tools/dev_builds.py` now derives the next archive counter from both `DEV_VERSION` and existing archive folders, then skips existing archive directories before writing the recovered counter through the tool.
- Dev-build repair smoke test with temporary archive folders `0.1.1-dev.28` and `0.1.1-dev.30`: predicted next counter `31`.
- Real workspace next archive prediction after repair: `0.1.1-dev.31`.
- Rebuild after repair: `cmake --build --preset qt-mingw-debug --parallel 20`: passed (`ninja: no work to do`).
- Redeploy after repair: `cmake --build --preset qt-mingw-debug-deploy --parallel 20`: passed.
- `python tools/dev_builds.py archive --preset qt-mingw-debug --json`: created `0.1.1-dev.31` at `C:\Users\Drako\Desktop\quader-dev-builds\0.1.1-dev.31` and updated `DEV_VERSION` to `31`.
- `python tools/dev_builds.py launch --latest --dry-run --json`: resolved latest to `C:\Users\Drako\Desktop\quader-dev-builds\0.1.1-dev.31\quader.exe`.
- Archived shader output check found no `fs_cube`, `vs_cube`, `fs_grid`, or `vs_grid` files in `0.1.1-dev.31`.
- `python tools/project_board.py validate`: passed, validating 7 board entries and 23 archived entries.
- `python tools/project_board.py review --workflow-authorized --id 30`: moved #30 to `In Review`.
- Full safe runtime preset was run after rebuilding all tests and exposed two failures in the active #25 source-wire area:
  - `crimson_gpu_resource_tests.GpuResource.MixedOpenStampSetsDoNotBypassSourceWireFiltering`
  - `crimson_gpu_resource_tests.GpuResource.SourceWireDepthFilterDoesNotSkipNonIncidentTrianglesWithMatchingEndpoints`

Those files were not changed by task #30 and #25 already owns source-wire overlay filtering rework.
