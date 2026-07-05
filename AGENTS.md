# Quader Agent Instructions

This repository is a C++20 Qt Widgets desktop application for Quader, a polygon modeling tool. The renderer is named `crimson`; do not use runtime-specific names such as bgfx in renderer folders, targets, or planned class names.

## Current Status

As of 2026-07-05:

- The app target is `quader_app`; the desktop executable is `quader.exe`.
- Source is modular under `src/app`, `src/commands`, `src/crimson`, `src/document`, `src/foundation`, `src/geometry`, `src/io`, `src/math`, `src/mesh`, `src/tools`, and `src/ui`.
- Tests exist under `tests/unit` and `tests/render_tests`; CTest also runs architecture boundary checks.
- Architecture guides live in `agents/architecture/`.
- Active workflow policy lives in `agents/workflow.md`.
- Board command policy lives in `agents/task_board.md`.
- Test policy lives in `agents/tests-policy.md`.
- Documentation policy lives in `agents/documentation-policy.md`.
- C++ code style policy lives in `agents/code-style.md`.
- Active implementation/audit plans live in `agents/plans/`; implemented, old, or outdated plans move to `agents/archive/` and are not referenced after archival.
- Completed tasks and fixed bugs move to `project_board_archive.md`, grouped by archive date and internal dev version.
- The external project board frontend lives at `C:\Users\Drako\Desktop\quader-project-board`; it is support tooling, not Quader product UI.

## Standard Commands

Configure:

```powershell
cmake --preset qt-mingw-debug
```

Build app and runtime shaders with the preset default parallelism:

```powershell
cmake --build --preset qt-mingw-debug
```

Build all default CMake targets intentionally:

```powershell
cmake --build --preset qt-mingw-debug-full
```

Test:

```powershell
cmake --build --preset qt-mingw-debug-tests
ctest --preset qt-mingw-debug
```

Run architecture checks:

```powershell
cmake --build --preset qt-mingw-debug --target check_architecture
```

Deploy runtime files:

```powershell
cmake --build --preset qt-mingw-debug-deploy
cmake --build --preset qt-mingw-release
```

Run:

```powershell
.\build\qt-mingw-debug\quader.exe
```

Validate the board:

```powershell
python tools/project_board.py validate
```

Inspect, create, list, or launch internal dev builds:

```powershell
python tools/dev_builds.py current
python tools/dev_builds.py archive --preset qt-mingw-debug
python tools/dev_builds.py list
python tools/dev_builds.py launch --version 0.1.0-dev.N
```

Run the external board frontend:

```powershell
cd C:\Users\Drako\Desktop\quader-project-board
npm install
npm run dev
```

Open `http://127.0.0.1:3000`.

## Lean Workflow v2

The daily workflow is architect-led. The retired PM agent is not part of active routing.

1. Docs-only request: use `quader-docs-mantainer`.
2. Performance audit/fix request: use `quader-performance-dev`.
3. External Next.js control-board request: use the `$quader-board-developer` skill directly.
4. New product, build, workflow, renderer, UI, core, or bug task: route to the relevant architect scout(s), then have `quader-software-architect` finalize the board entry.
5. Existing board task: root revalidates freshness, applies exact software-architect board commands, coordinates up to 6 clean worker agents, integrates results, verifies, and moves the task to review.
6. Review: the owning architect approves or rejects. Approval includes the exact archive command; root applies it and archives the task automatically.

Board authority is now software-architect authority. `quader-software-architect` is the final board authority and cross-domain architecture governor. The root chat physically runs `tools/project_board.py`, but only by applying exact commands returned by the software architect. New mutating board commands must use `--architect-authorized`.

`--pm-authorized` is a temporary compatibility alias for old commands only. Do not use it in new docs, prompts, or task flows.

Ordinary chats, scouts, workers, UI architects, renderer architects, core architects, build/workflow architects, docs agents, and fallback agents must not mutate the board directly. They return findings, reports, reviews, or implementation results to root. Root applies exact software-architect commands.

`quader-performance-dev` is the only exception. For explicit C++ performance audit/fix work, it may create and move its own performance tasks with `--architect-authorized` after reading the active workflow and board policy. It must not complete, archive, remove, reopen, push, bump versions, or edit the public changelog.

## Active Agent Roles

- `quader-software-architect`: final architecture governor, board authority, multi-domain finalizer, conflict arbiter, master audit consolidator, and multi-domain reviewer.
- `quader-core-architect`: first-pass scout/reviewer for document, commands, tools, mesh, I/O, app/core contracts, and modeling-domain APIs.
- `quader-build-workflow-architect`: first-pass scout/reviewer for CMake, presets, tools, board/process, versioning, dev builds, agent workflow, and support tooling.
- `quader-ui-architect`: first-pass scout/reviewer for Qt Widgets UI, actions, models, panels, viewport host UI, settings, dialogs, and UI service boundaries.
- `quader-renderer-architect`: first-pass scout/reviewer for Crimson, render graph, shaders, materials, overlays, picking, renderer assets, and render tests.
- `quader-docs-mantainer`: documentation-only maintenance and documentation policy review.
- `quader-performance-dev`: implementation-capable C++ performance specialist using the `cpp-performance` skill.

Active skills:

- `$quader-board-developer`: direct-use skill for the external Next.js project board at `C:\Users\Drako\Desktop\quader-project-board`, its API routes, copied prompts, archive/changelog views, dev-build controls, and dashboard styling.

If an expected role is not listed by the multi-agent tool after its TOML changed, do not pretend it was used. Ask the user to reload the agent registry, or use a default fallback only if the user explicitly accepts it.

When spawning any agent, do not spawn full-history forks unless required. Prefer clean explorer scouts with `fork_context: false`, the repository path, relevant file paths, and scoped instructions passed in the prompt.

All spawned agents must be awaited. Root must not finalize a turn by saying to wait for agents that it spawned, unless the user explicitly stops or redirects the work.

## New Task Intake

Fresh task requests are intake-only by default. Imperative wording such as "rename X", "change Y", or "fix this bug" is not consent to edit files immediately.

Router:

- Core/modeling/document/commands/tools/mesh/I/O/app contracts: `quader-core-architect`.
- Build/CMake/presets/tools/board/process/versioning/dev-build/agent workflow: `quader-build-workflow-architect`.
- Qt Widgets/UI/action/model/panel/viewport-host UI: `quader-ui-architect`.
- Crimson/shaders/render graph/materials/overlays/picking/render tests: `quader-renderer-architect`.
- Cross-domain or unclear: spawn the relevant scouts, then `quader-software-architect` finalizes.
- No architect domain: root researches, then sends the report to `quader-software-architect` for board authority.

Scouts return `Task Intake Domain Findings`. The software architect reconciles them and returns one `Final Task Intake Report` plus exact `tools/project_board.py add --architect-authorized` or `add-bug --architect-authorized` commands, or a duplicate/no-op decision. Root applies only those exact commands.

No preflight or setup task may be invented just to allow intake or assignment. Preflight tasks are valid only when explicitly requested by the user or an active architect plan.

Reference/parity intake must include `Source refs:` with the source folder path and file-to-line mapping. Phrases such as `check this code reference`, `same`, `1:1`, `100%`, or `parity` mean match behavior and structure as closely as practical without unsafe verbatim third-party copying.

## Existing Task Execution

Before starting a board task, root runs the freshness gate:

1. Read the task, linked plans, source refs, active board metadata, and likely touched files.
2. Ask the owning/finalizing architect for revalidation if the task is domain-specific or assumptions are uncertain.
3. If valid, apply the exact software-architect `set-freshness --architect-authorized --status fresh` command.
4. Start only fresh tasks. `start` and `move --to "In Progress"` fail unless the task is fresh.

Root coordinates implementation directly:

- Use up to 6 clean worker agents when the work is parallelizable.
- Each worker receives a scoped prompt, required docs, paths, constraints, and `/goal` instruction.
- Workers set a Codex `/goal` before edits. Completion means the assigned scope is implemented, verified, documented when required, and returned to root.
- Workers must not mutate the board.
- Root integrates worker results, resolves non-architectural merge issues, runs the required build/tests, deploys when runtime behavior changed, and moves the task to review through exact software-architect board commands.

Executors must not silently apply workarounds or weaken the accepted plan, validation strictness, source-reference parity, style/tooling rules, or acceptance criteria. If a workaround or plan deviation seems necessary, stop and return a `Workaround/Deviation Report` to root for architect review.

If the user reports a bug or asks for a quick fix inside a running executor chat, the executor stops and returns a `Mid-work Bug Report` to root. Root routes it through the correct architect before any fix continues.

## Review And Archive

Review goes to the owning architect:

- Core/app/build/workflow/cross-domain: software architect, with core or build/workflow input when useful.
- UI: UI architect.
- Renderer: renderer architect.
- Performance: performance-dev may move to review, but final archive still follows software-architect board authority unless the user explicitly directs otherwise.

The review prompt must ask the architect to approve or reject against the task brief, active plan, current code, and verification results. If approved, the architect returns the exact archive command:

```powershell
python tools/project_board.py complete --architect-authorized --id <id>
```

For fixed bugs, the command must include:

```powershell
--resolution "<concise bug fix summary>"
```

Root applies the exact command and runs `python tools/project_board.py validate`. No extra PM or user closeout is required after architect approval.

Plan files are archived after their relevant work is implemented and approved. Move old or completed plans from `agents/plans/` to `agents/archive/`. Do not delete plan files automatically. Once archived, do not reference them from active plans, audits, implementation notes, or architecture instructions.

## Architecture Rules

Read the relevant architecture file before non-trivial code changes:

- Overall app, dependency direction, document/commands/tools/mesh boundaries: `agents/architecture/architecture.md`.
- Qt Widgets UI, actions, panels, item models, settings, viewport widget integration: `agents/architecture/ui.md`.
- Crimson renderer, render graph, PBR, materials, shaders, overlays, picking, render tests: `agents/architecture/renderer.md`.

Keep these boundaries:

- Mesh, geometry, document, and commands must not depend on Qt Widgets.
- UI dispatches commands for document mutation; it does not own document truth.
- Crimson consumes prepared render data and snapshots; it does not own modeling logic.
- Renderer work lives under `src/crimson/`.
- Overlays, grid, gizmos, selection, picking, and debug rendering stay separate from lit/post-processed scene rendering.

## Audit And Beautify

`audit` reports architecture drift, dependency-boundary flaws, ownership problems, correctness risks, and missing verification.

`beautify`, `beautify code`, or `API beautify` reviews public and cross-module APIs for elegance, concise call sites, naming, ownership clarity, parameter shape, typed IDs/results, and unnecessary ceremony. It is not formatting or whitespace cleanup.

`audit and beautify` produces two separate sections: `Architecture Audit Findings` and `Beautify Findings`. Beautify examples include preferring intent-level APIs such as `mesh->bevel(options)` over generic string dispatch when the behavior belongs to the domain object, and compact options objects over long parameter lists.

Architects do not implement during audit/beautify unless the user separately authorizes implementation.

## External Research Rules

For every task directly involving a Qt widget or Qt Widgets API surface, the UI architect performs a quick Qt-resource scan. Official Qt docs are primary; Stack Overflow and specialized Qt/C++ UI articles may support. Reports include `Qt resources checked:` with URLs and concrete findings.

For Crimson or graphics-programming intake, revalidation, planning, deviation/bug review, or final review, the renderer architect browses relevant pages and reads selected content fully. Use these first when relevant:

- https://graphicscompendium.com/index.html
- https://learnopengl.com/Introduction

Renderer implementation-facing reviews also scan `C:\Users\Drako\Desktop\_SOURCES` for useful reference patterns and include file-to-line references when useful. Treat third-party code as reference only unless license and project policy make reuse explicitly safe.

All architects consider battle-tested third-party libraries for narrow scoped responsibilities during audits/plans/reviews, but they surface candidates as explicit decisions. Do not propose replacing central Quader systems such as the app architecture, document/modeling core, command/tool model, Crimson renderer, Qt Widgets shell, task board workflow, or build/release workflow.

## Build Checkpoints And Dev Builds

For C++/CMake/shader/runtime changes:

- Run `cmake --build --preset qt-mingw-debug` after each coherent implementation slice and before starting another independent code task. This is the fast app/shader runtime build; use `qt-mingw-debug-tests`, `ctest --preset qt-mingw-debug`, and `qt-mingw-debug-full` when verification requires test executables, the registered CTest suite, or Ninja's full `all` target.
- Do not go more than two implemented code tasks without a successful build.
- Before architect review or `In Review`, run relevant tests/builds.
- If user-visible runtime behavior changed or the executable must launch from Explorer, run `cmake --build --preset qt-mingw-debug-deploy` for debug or `cmake --build --preset qt-mingw-release` for release. The release preset deploys runtime DLLs by default; use `qt-mingw-release-runtime` only for a compile-only release app/shader build.
- After successful build/deploy for stable runnable behavior, executable/runtime packaging, shaders/assets, CMake/runtime dependency changes, or executable-observed bug fixes, create a dev-build archive unless the work is docs/task-board/control-board/tests/planning only or known concurrent unverified work would contaminate the snapshot.
- If archiving is deferred, report `Dev build checkpoint deferred:` with the reason.

Do not edit `DEV_VERSION` manually. It increments only after `tools/dev_builds.py archive` succeeds. Root `VERSION` is public release state and changes only in the push/release workflow.

## Documentation, Tests, Style, Git

Read `agents/tests-policy.md` before planning, writing, modifying, deleting, or reviewing tests, test harnesses, fixtures, golden images, fuzz/property tests, sanitizer checks, validation tests, or manual verification substitutes.

Read `agents/documentation-policy.md` before changing docs, comments, Doxygen/API documentation, changelogs, README, architecture docs, agent instructions, or generated docs.

Read `agents/code-style.md` before non-trivial C++ source, tests, benchmarks, clang-format, clang-tidy, formatting, naming, or style/static-analysis validation. Do not run clang-format or clang-tidy verification as a pre-commit substitute unless the user asks.

Record durable development changes in `dev-changelog.md`. User-facing `changelog.md` contains only user-visible features and important fixes, and is normally updated during push/release flow.

Keep unrelated user changes intact. Do not use destructive git commands such as `git reset --hard` or `git checkout --` unless the user explicitly asks.

## External Board Frontend

The Next.js control board at `C:\Users\Drako\Desktop\quader-project-board` is support tooling outside this repo. When the user directly asks to change the dashboard, board view, archive view, changelog view, API routes, or styling, implement the frontend maintenance directly unless the user explicitly asks to create a Quader project-board task.

Use the `$quader-board-developer` skill for non-trivial external board frontend/API work. The skill documents the board app layout, how it reads `project_board.md`, how it launches current/archived builds, and how copied Codex prompts must preserve Lean Workflow v2.

The `$quader-board-developer` workflow may edit the external board app directly, but it is not board-content authority. Any actual `project_board.md` or `project_board_archive.md` mutation still requires exact `quader-software-architect` commands with `--architect-authorized`.

For frontend behavior changes, run the relevant frontend checks (`npm run typecheck`, `npm run build` when available) and record durable behavior changes in `dev-changelog.md` when they affect the project workflow.
