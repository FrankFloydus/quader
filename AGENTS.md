# Quader Agent Instructions

This repository is a C++20 Qt Widgets + bgfx prototype for Quader, a polygon modeling application. Treat the current source tree as a prototype and the files under `agents/architecture/` as the target architecture guidance.

## Current Status

As of 2026-07-04:

- The app is a minimal Qt Widgets desktop shell with a bgfx Vulkan viewport.
- The CMake app target is `quader_app`, and its desktop executable output is `quader.exe`.
- Source is still flat and small:
  - `src/main.cpp`
  - `src/MainWindow.*`
  - `src/BgfxWidget.*`
- Shaders are minimal prototype shaders:
  - `shaders/vs_cube.sc`
  - `shaders/fs_cube.sc`
  - `shaders/vs_grid.sc`
  - `shaders/fs_grid.sc`
  - `shaders/varying.def.sc`
- `CMakeLists.txt` fetches `bgfx.cmake`, builds bgfx/shaderc, compiles SPIR-V shaders, and links Qt Widgets.
- The README says the viewport draws a rotating red cube with simple directional diffuse lighting.
- The final architecture is not implemented yet. Do not treat the current flat `src/` layout as the desired long-term module layout.
- Architecture guides now exist:
  - `agents/architecture/architecture.md`: overall app architecture.
  - `agents/architecture/ui.md`: Qt Widgets UI architecture.
  - `agents/architecture/renderer.md`: Crimson/PBR renderer architecture.
- Agent workflow guides now exist:
  - `agents/workflow.md`: documentation, user-facing changelog, dev changelog, task board, versioning, push, and git workflow.
  - `agents/task_board.md`: root `project_board.md` task board rules and commands.
  - `agents/tests-policy.md`: required testing policy for agents that plan, write, modify, delete, or review tests.
- `agents/plans/` is the handoff directory for architect-authored implementation and audit plans.
- `agents/archive/` is the storage location for implemented, old, or outdated plans. Archived plans are not active authority and should not be referenced by future plans or audits after they are moved.
- `project_board_archive.md` is the chronological archive for completed tasks and fixed bugs, grouped by archive date and internal dev version.
- Specialized Codex agent profiles are configured outside the repo:
  - `C:\Users\Drako\.codex\agents\quader-software-architect.toml`
  - `C:\Users\Drako\.codex\agents\quader-ui-architect.toml`
  - `C:\Users\Drako\.codex\agents\quader-renderer-architect.toml`
  - `C:\Users\Drako\.codex\agents\quader-project-manager.toml`
  - `C:\Users\Drako\.codex\agents\quader-performance-dev.toml`

## Standard Commands

Configure:

```powershell
cmake --preset qt-mingw-debug
```

Build:

```powershell
cmake --build --preset qt-mingw-debug
```

Test:

```powershell
ctest --preset qt-mingw-debug
```

Run architecture boundary checks directly:

```powershell
cmake --build --preset qt-mingw-debug --target check_architecture
```

Deploy local runtime files:

```powershell
cmake --build --preset qt-mingw-debug --target deploy
```

Run:

```powershell
.\build\qt-mingw-debug\quader.exe
```

Inspect internal dev-build version state:

```powershell
python tools/dev_builds.py current
```

Archive the deployed runtime bundle as the next internal dev build:

```powershell
python tools/dev_builds.py archive --preset qt-mingw-debug
```

List or launch archived dev builds:

```powershell
python tools/dev_builds.py list
python tools/dev_builds.py launch --version 0.1.0-dev.N
```

CTest currently runs GoogleTest-discovered unit/render tests plus the architecture boundary checks. The direct boundary-check target uses the same script.

Validate the task board:

```powershell
python tools/project_board.py validate
```

Archive a completed task or fixed bug after explicit user authorization and an exact PM-issued command:

```powershell
python tools/project_board.py complete --pm-authorized --id 25
python tools/project_board.py complete --pm-authorized --id 19 --resolution "Fixed the runtime target binding and restored the single-cube viewport behavior."
```

Search archived tasks or reopen one after an exact PM-issued command:

```powershell
python tools/project_board.py archive-search --query "diagnostics" --full
python tools/project_board.py reopen-archive --pm-authorized --id 25 --reason "User requested follow-up work from archived task context."
```

Mark a task fresh after current-workspace revalidation:

```powershell
python tools/project_board.py set-freshness --pm-authorized --id 25 --status fresh --checked now --note "Revalidated current files and active board metadata."
```

List active board entries:

```powershell
python tools/project_board.py list
```

Run the project board frontend:

```powershell
cd C:\Users\Drako\Desktop\quader-project-board
npm install
npm run dev
```

Open:

```text
http://127.0.0.1:3000
```

The control board frontend exposes:

- `GET /api/dev-version` for public version, internal dev version, latest archive, and archive list.
- `POST /api/dev-builds/archive` to create the next local dev-build archive.
- `POST /api/dev-builds/launch` to launch the current build or a selected archived dev build.
- Board, Task Archive, Dev Changelog, and Changelog views. Task Archive shows completed tasks and fixed bugs grouped by date and internal dev version. Dev Changelog shows launch controls only for sections with matching archived builds.

Maintenance of the external Next.js control-board frontend at `C:\Users\Drako\Desktop\quader-project-board` is repository support tooling, not Quader product UI architecture. When the user directly asks to change the web dashboard, board view, changelog view, frontend API routes, or control-board styling, implement the requested frontend maintenance directly unless the user explicitly asks to create a project-board task for it. Still update workflow docs, `dev-changelog.md`, and frontend tests/build checks when behavior changes.

## Workflow Authorities

Read `agents/workflow.md` before changing documentation policy, `changelog.md`, `dev-changelog.md`, version metadata, the task board, commit/push behavior, or repository workflow.

Read `agents/task_board.md` before using board commands, changing `project_board.md`, saving task clue images, moving tasks, or marking tasks complete.

Read `agents/tests-policy.md` before planning, writing, modifying, deleting, or reviewing tests, test harnesses, fixtures, golden images, fuzz/property tests, sanitizer checks, validation tests, or manual verification substitutes. This applies to the main/root agent, specialized architects, project-manager assignments, workers, scouts, and any future agent that could affect tests.

Root `VERSION` is the public app version source of truth. `CMakeLists.txt` reads it during configure. Root `DEV_VERSION` is the internal dev-build counter and displays as `<VERSION>-dev.<DEV_VERSION>`. Internal dev-build archives live outside the repository by default at `C:\Users\Drako\Desktop\quader-dev-builds`; set `QUADER_DEV_BUILDS_ROOT` only for nonstandard layouts. Do not bump `VERSION` or create a user-facing `changelog.md` version entry during ordinary implementation, review, inspection, planning, task-board completion, or internal dev-build archiving. `DEV_VERSION` increments only after `tools/dev_builds.py archive` confirms a runnable archive. Version and user-facing changelog updates happen during the push workflow only. Durable development changes should be recorded in `dev-changelog.md`.

Board authority is PM-gated. The project manager is the sole authority for board content, task creation, assignment metadata, and allowed status-transition decisions. The main/root chat may physically run `tools/project_board.py`, but only to apply exact commands returned by `quader-project-manager`. Every mutating `tools/project_board.py` command requires `--pm-authorized`; main/root must add that flag only when the exact command was returned by the PM. Ordinary chats, architects, workers, scouts, and fallback agents must never add, edit, or move board entries directly.

Performance developer exception: `quader-performance-dev` is the only non-PM role that may supersede the PM for explicit C++ performance audit/fix workflows. When the user asks to audit and fix performance, optimize hot paths, reduce allocations, improve cache behavior, or make the C++ code performant, this agent may read the PM workflow, add the needed performance tasks or bugs itself, set freshness, move its own performance tasks into active work, implement them, and move them to `In Review` when verified. It must still use `tools/project_board.py` with `--pm-authorized`, and that flag means "performance-dev authorized" only in this narrow workflow. It must not complete, archive, remove, reopen, push, bump versions, or update the public changelog without the normal user/PM closeout path.

Board completion archives instead of deleting. `tools/project_board.py complete --pm-authorized` moves the entry from `project_board.md` to `project_board_archive.md`, grouped by archive date and current internal dev version. Fixed bugs require `--resolution "<summary>"`; agents that fix bugs must include a concise `Bug resolution summary:` in their final report so the PM can use it when the user authorizes marking the bug fixed.

Archive search is read-only. Reopening an archived card is PM-gated and uses
`tools/project_board.py reopen-archive --pm-authorized`; reopened entries return
to the active board as provisional work with `Freshness: needs-refresh` and must
pass the normal task freshness gate before implementation.

Task freshness is start-gated. New tasks and bugs are created with
`Freshness: [status:needs-refresh][checked:never]`, and existing entries
without a `Freshness:` line are treated as `needs-refresh`. Before any Backlog
task moves to `In Progress`, revalidate its saved assumptions against the
current workspace, active plans, source refs, and active board metadata. Only
tasks marked `fresh` may start; `stale` tasks need PM/architect revalidation,
and `superseded` tasks are not implemented unless the user or PM replaces the
brief.

Executor goals are mandatory for implementation work. When a chat takes an
existing board task or PM implementation assignment, it must set a Codex `/goal`
before editing files or running implementation commands. The goal must name the
task id/title or assignment key and state that it is complete only when the full
executor scope is implemented, verified, documented when required, and handed
back through PM/authority review or moved to `In Review` when allowed.

Executors must not silently apply workarounds or plan deviations. If the
accepted plan, board brief, active architecture plan, or required verification
cannot be followed, the executor stops and sends a `Workaround/Deviation Report`
to the main/root chat. Main/root routes it through `quader-project-manager` and
the correct architect before any workaround is continued, accepted, or tracked as
a new task/bug.

If the user reports a bug or asks for a quick fix directly inside a running
executor/subagent chat, that executor must not independently implement the fix.
It stops affected work, sends a `Mid-work Bug Report` to the main/root chat, and
continues only after main/root returns PM/architect-approved instructions.

New task requests are intake-only by default. When the user asks for a new change, fix, rename, refactor, implementation, task, or bug that is not already represented by an active board entry, do not edit files or perform the work on the spot. First ask whether the request belongs to any architect domain. If yes, delegate to the relevant architect(s). If no, the main/root chat researches it. In both cases, one PM-ready `Final Task Intake Report` goes to `quader-project-manager` before the board changes. Imperative wording such as "rename X" or "change Y" is not consent to bypass intake. Reference/parity intake must include `Source refs:` with the source folder path and file-to-line mapping.

This intake rule is for Quader product work and project-board task creation. It does not block direct maintenance of the external Next.js control-board frontend when the user asks for that dashboard work in the current chat.

When an actionable master plan or master audit exists, route its ordered implementation batches through `quader-project-manager` before starting implementation. The PM checks duplicates and returns exact `tools/project_board.py add --pm-authorized --plans <master-plan-path>` commands for the main/root chat to apply. Keep the master plan itself in `agents/plans/` until the relevant work is approved or the plan is archived.

When asked to notify or message another Codex chat about a task, do not ask that chat to add or edit the board. Tell the target chat to run the PM-gated task intake workflow in this repository, or provide the task context as input for that workflow.

While `agents/plans/audit_20260704_full_architecture_master.md` is active, architecture-sensitive feature work must reference that master audit or a follow-up implementation plan before implementation starts. Architecture-sensitive work includes changes that touch UI/renderer/core boundaries or add real document mutation, topology editing, selection truth, import/export, material editing, picking-driven selection, PBR, render graph behavior, or debug panels. Until the owning batch or follow-up plan authorizes the work, preserve prototype behavior and do not add those capabilities directly to `MainWindow` or `BgfxWidget`.

## Architecture Authorities

Read the relevant architecture file before non-trivial code changes.

- Overall app, dependency direction, document/commands/tools/mesh boundaries: `agents/architecture/architecture.md`.
- Qt Widgets UI, actions, panels, item models, settings, viewport widget integration: `agents/architecture/ui.md`.
- Crimson renderer, render graph, PBR, materials, shaders, overlays, picking, render tests: `agents/architecture/renderer.md`.

Keep these boundaries intact:

- Mesh, geometry, document, and commands must not depend on Qt Widgets.
- UI dispatches commands for document mutation; it does not own document truth.
- Renderer consumes prepared render data and snapshots; it does not own modeling logic.
- The renderer is named `crimson`. Future renderer work should live under `src/crimson/`. Do not encode the graphics runtime name in renderer targets, folders, or planned class names.
- Overlays, grid, gizmos, selection, picking, and debug rendering must stay separate from lit/post-processed scene rendering.

## Specialized Architect Subagents

Use the three architects for task intake, architecture plans, and architecture review. They are scouts/planners/reviewers, not general implementation workers.

Spawn architects for the Task Intake Workflow, when the user asks for the architect workflow, when the user asks for subagents/delegation, or when the task explicitly needs architecture approval. During Task Intake Workflow, spawn only the classified intake architect roles, with at most one architect per relevant domain. Additional non-architect agents are allowed only after the PM has created or identified the board task and returned explicit assignment packets. Direct implementation is allowed only for an active board task, or when the user explicitly says to perform the work now and bypass task intake.

All specialized architects must read `agents/tests-policy.md` whenever their plan, audit, or review includes test strategy, test files, golden images, fixtures, sanitizer/fuzz/property checks, or verification substitutes.

Running subagents must be awaited. When the main/root chat spawns architects, PM agents, scouts, workers, or other subagents for the current request, it must wait for every spawned agent to return a terminal report before finalizing the turn, claiming the work is complete, or telling the user to wait. The main/root chat may not abandon open subagents with a handoff like "waiting for agents to finish." The only exception is an explicit user stop/cancel/redirect, in which case the main/root chat must state which agents are still open and what work is being discarded or superseded.

Beautify code is an explicit architect review mode. Prompts such as `beautify`,
`beautify code`, `API beautify`, and `audit and beautify` ask architects to
inspect public and cross-module APIs for elegance, concision, intent-revealing
call sites, ownership clarity, naming, parameter shape, and unnecessary
ceremony. This is not formatter/linter work and not whitespace cleanup.
Architects still do not implement directly; they return findings or a plan.
Normal `audit` requests report architecture drift, boundary flaws, ownership
problems, missing tests, and correctness risks. `Beautify` reports API/design
elegance improvements unless a real architecture flaw blocks the prettier API.
`Audit and beautify` must keep two separate sections:
`Architecture Audit Findings` and `Beautify Findings`.

Beautify findings should prefer intent-level APIs such as `mesh->bevel(options)`
over generic string-dispatch or procedural manager calls when the operation
belongs to the mesh/domain object; compact domain DTOs/options objects over long
parameter lists; project-owned typed IDs/results over raw strings, booleans, or
loosely typed handles at public boundaries; and APIs that read naturally at call
sites while preserving dependency direction and testability.

### Software Architect

Agent type:

```text
quader-software-architect
```

Use for:

- Cross-cutting app architecture.
- Module layout and dependency direction.
- Document, command, tool, mesh, I/O, app-service, and ownership decisions.
- Project-wide architecture audits.
- Public and cross-module API beautify reviews for app/core/document/commands/tools/mesh/build/workflow.
- Arbitration when UI and renderer decisions conflict at an app boundary.

Primary guide:

```text
agents/architecture/architecture.md
```

Expected plan name:

```text
agents/plans/implementation_{feature_name}_doc.md
```

### UI Architect

Agent type:

```text
quader-ui-architect
```

Use for:

- Qt Widgets shell, `MainWindow`, menus, toolbars, docks, status UI, and workspace state.
- `ActionRegistry`, action state, shortcuts, context menus.
- Qt model/view, item models, delegates, property editors, selection sync.
- Panels, dialogs, notification/settings services.
- Qt viewport host integration from the UI side.
- UI architecture audits.
- UI API beautify reviews for Qt-facing action/model/panel/service call shapes and widget boundaries.

For every task directly involving a Qt widget or Qt Widgets API surface, the UI architect must perform a quick external Qt-resource scan before returning intake findings, plans, or reviews. Official Qt documentation is the primary source; Stack Overflow answers and specialized Qt/C++ UI articles may be used as supporting sources. The UI architect must include a `Qt resources checked:` section with valid URLs and the concrete findings that affected the recommendation. If network access is unavailable, the report must say so explicitly instead of inventing sources.

Primary guide:

```text
agents/architecture/ui.md
```

Also reads:

```text
agents/architecture/architecture.md
```

Expected plan name:

```text
agents/plans/implementation_{feature_name}_ui_doc.md
```

### Renderer Architect

Agent type:

```text
quader-renderer-architect
```

Use for:

- Crimson graphics-runtime integration and boundary isolation.
- Render snapshots, render world, render graph, passes, frame resources.
- PBR materials, base shader system, shader pipeline, texture color space.
- IBL, lights, shadows, exposure, tone mapping, debug views.
- Overlays, grid, gizmos, selection rendering, picking.
- Renderer-facing asset import, glTF material mapping, render tests, golden images.
- Renderer architecture audits.
- Crimson API beautify reviews for render snapshots, materials, passes, handles, and renderer integration.

Primary guide:

```text
agents/architecture/renderer.md
```

Also reads:

```text
agents/architecture/architecture.md
```

For Qt viewport or renderer settings UI, it also reads:

```text
agents/architecture/ui.md
```

For Crimson or graphics-programming task intake, pending task revalidation,
implementation planning, deviation/bug review, or final implementation review,
the renderer architect must perform a quick external graphics-resource scan
before deciding. Use these authoritative references first when relevant:

- https://graphicscompendium.com/index.html
- https://learnopengl.com/Introduction

The renderer architect must browse/search for relevant article or HTML pages for
the task, read the selected page contents fully rather than relying on snippets,
and include `Renderer resources checked:` with valid URLs and the findings that
affected the recommendation. If network access fails, say so explicitly and
continue from local project docs/current code without inventing sources.

The renderer architect also has a local source-reference corpus at
`C:\Users\Drako\Desktop\_SOURCES`, currently including renderer codebases such
as `DiligentEngine-master` and `filament-main`. For renderer implementation
plans or implementation-facing renderer reviews, quickly scan this corpus with
targeted terms for relevant approaches, APIs, shader patterns, render-graph
structure, resource lifetime patterns, diagnostics, or tests. Include
`Local renderer references checked:` with the folders/files inspected and any
useful file-to-line references, or state that no relevant local reference was
found. Treat the corpus as reference material only: do not copy third-party code
verbatim unless its license and project policy make reuse explicitly safe.

Expected plan name:

```text
agents/plans/implementation_{feature_name}_renderer_doc.md
```

## Project Manager Subagent

Agent type:

```text
quader-project-manager
```

Use for:

- New task or bug intake after the finalizing architect has produced one PM-ready `Final Task Intake Report`.
- Board-backed tasks that the user asks to run "with PM" or "with the project manager".
- Multi-agent implementation or scouting work after a board task exists or the PM has identified an existing duplicate.
- Cross-domain tasks where software, UI, and Crimson responsibilities need coordination.
- Broad task-board execution where assignments, ordering, authority routing, and status updates need to be tracked.

The project-manager agent owns board decisions and coordination intent. It does not implement code, edit docs, mutate the board directly, or provide final domain approval. It returns assignment packets and exact board update requests for the main/root agent to apply. The main/root agent is only the mechanical executor of those PM commands and spawns the actual agents.

PM assignment packets must list `agents/tests-policy.md` as a required read for any scout, worker, architect, or future agent assignment that may plan, write, modify, delete, or review tests.

Final authority remains with the owning domain architect:

- software architect for app/core/build/workflow/board/process and cross-domain arbitration,
- UI architect for Qt Widgets/UI work,
- renderer architect for Crimson work.

If `quader-project-manager` is not listed by the multi-agent tool after the TOML file was added or edited, do not pretend the named role was used. Ask the user to reload the agent registry, or use a default agent only if the user explicitly accepts that fallback.

## Performance Developer Subagent

Agent type:

```text
quader-performance-dev
```

Use for:

- C++ performance audits and fix passes.
- Measured hot-path optimization, allocator pressure, copies, cache locality, vectorization, branch reduction, and concurrency overhead.
- Data-oriented redesign proposals when profiling or benchmarks show layout/traversal cost.
- Performance regression investigation for app/core, mesh, document, commands, tools, UI service internals, and Crimson CPU-side preparation paths.

`quader-performance-dev` must use the custom `cpp-performance` skill. If the skill is not available in the active tool/skill list, it must read `C:\Users\Drako\.codex\skills\cpp-performance\SKILL.md` directly before acting, or stop and report that the performance workflow cannot be run correctly.

This is an implementation-capable specialist, not an architect and not a general project manager. For explicit performance audit/fix requests, it is the only agent allowed to bypass the normal PM gate. It must first read `AGENTS.md`, `agents/workflow.md`, `agents/task_board.md`, `agents/tests-policy.md`, and `C:\Users\Drako\.codex\agents\quader-project-manager.toml`, then inspect current code, board state, and relevant active plans.

Performance workflow:

1. Establish the performance goal and likely hot path from profiling data, benchmarks, existing diagnostics, or a documented hypothesis.
2. Add one board task per coherent performance fix or measurement slice using `tools/project_board.py add --pm-authorized` or `add-bug --pm-authorized` itself. Use `area:performance` unless a more specific area is clearer, and include baseline/measurement context in `Brief:`.
3. Mark only its own performance tasks fresh after current-workspace revalidation, move them to `In Progress`, and set a Codex `/goal` before editing.
4. Implement the smallest high-confidence optimization first, preserving correctness and architecture boundaries.
5. Measure before and after under comparable conditions and report absolute and relative impact. Do not claim an improvement without measurement.
6. Run relevant tests/builds, and follow the Build Checkpoint Policy for runnable C++/CMake/shader/runtime changes.
7. Move verified performance tasks to `In Review` using `tools/project_board.py review --pm-authorized`.

Limits:

- Do not complete, archive, remove, reopen, push, bump `VERSION`, edit `DEV_VERSION` manually, or update user-facing `changelog.md`.
- Do not use the performance exception for feature work, architecture beautify, non-performance refactors, dashboard maintenance, or ordinary bug fixing.
- If an optimization requires public API redesign, dependency-direction changes, UI behavior changes, Crimson render semantics changes, or weaker verification, pause and request the relevant architect review. The performance agent may still own the performance task after that review, but it must not silently bypass architecture authority.
- Do not trade clarity for low-level tricks unless the code is proven hot and the gain is measured.

If `quader-performance-dev` is not listed by the multi-agent tool after its TOML file was added, do not pretend the named role was used. Ask the user to reload the agent registry, or use a default agent only if the user explicitly accepts that fallback and the fallback reads the performance TOML plus `cpp-performance` skill file.

## Task Intake Workflow

Use this deterministic workflow when the user asks to add a new task or bug, asks a fresh chat to create work, or provides any new change request that is not already represented by an active board entry. Do not perform a new task request on the spot unless the user explicitly consents to immediate implementation and skipping task intake.

Task intake rule:

- Does the request belong to any architect domain? If yes, delegate intake to the relevant architect(s). If no, the main/root chat researches it.
- Architect-scoped intake may involve multiple architects, but only one per domain and no duplicate generic/default/scout/worker agent for the same scope.
- Multi-domain architect intake produces domain findings first, then one finalizing architect produces the PM-ready `Final Task Intake Report`.
- The main/root chat sends the `Final Task Intake Report` to `quader-project-manager`; the PM checks duplicates and returns exact board commands or a no-op.
- Implementation starts only when the user explicitly asks to perform an existing board task or explicitly authorizes immediate implementation.
- If the request says to check a code reference, match the same behavior, or achieve `1:1`, `100%`, or parity, the final report and board task must include `Source refs:` with exact source folder path and file-to-line mapping. Match behavior and structure as closely as practical without unsafe verbatim third-party copying.

1. Decide whether the request belongs to any architect domain:
   - software architect: build, module layout, target naming, source-folder structure, workflow, board/process, app/core, cross-domain work, or unclear ownership.
   - UI architect: Qt Widgets, actions, panels, models, viewport host UI, settings, dialogs, and UI services.
   - renderer architect: Crimson, render graph, shaders, materials, overlays, picking, render tests, and renderer-facing assets.
2. If yes, spawn the relevant architect(s) in `TASK_INTAKE` mode. If no, research the code/docs yourself.
3. Produce one PM-ready `Final Task Intake Report`. For multi-domain intake, gather domain findings first and have the finalizing architect reconcile them.
4. Send the `Final Task Intake Report` to `quader-project-manager`.
5. The PM checks existing board entries for duplicates and returns either:
   - exact `tools/project_board.py add --pm-authorized` or `add-bug --pm-authorized` command(s), or
   - a clear duplicate/no-op decision.
6. Apply only the PM's exact command(s), then report the new task id or duplicate decision.

Do not treat imperative wording as consent to bypass intake. "Rename this folder", "change this module", and "fix this bug" are task requests unless the user explicitly consents to immediate implementation and skipping task intake.

If the user explicitly consents to immediate implementation and skipping task intake, run a conflict preflight before editing. Check active board entries, PM/Authority/Assignment/Coordination metadata, and available Codex thread coordination tools for overlapping work. If overlap is visible, stop and ask whether to coordinate, message the other thread, or proceed anyway. If no overlap is visible, record that the check is best-effort because thread tools are not a perfect live lock.

No preemptive task rule:

- Do not create "preflight", "PM prerequisite", or "setup" board tasks merely so the PM can assign work.
- The PM can create a board task directly from a `Final Task Intake Report`.
- Preflight tasks are valid only when explicitly requested by the user, an active master plan, or an architect plan.

Use `python tools/project_board.py edit-brief --pm-authorized --id <id> --brief "<text>"` when the PM returns that exact command for updating only the canonical `Brief:` line. Useful future board commands, not yet implemented, include `edit-summary`, `edit-title`, `edit-final-owner`, `edit-plans`, `edit-source-refs`, `set-priority`, `set-area`, `set-type`, `append-note`, `append-coordination`, `block`, `unblock`, `claim`, `release`, `duplicate`, and `split-task`.

## How To Spawn Architects

When using Codex multi-agent tools, first check the available roles in the `spawn_agent` tool. If the role is listed, spawn it directly and do not override its model or reasoning effort.

Example:

```json
{
  "agent_type": "quader-renderer-architect",
  "fork_context": false,
  "message": "Create a renderer architecture implementation plan for <feature>. Read agents/architecture/renderer.md, agents/architecture/architecture.md, and agents/tests-policy.md. Write agents/plans/implementation_<feature>_renderer_doc.md. Include classes, files, data flow, render graph passes, Crimson boundaries, shader/material changes, diagnostics, and tests. Do not implement code."
}
```

Use `fork_context: true` only when important context exists only in the current conversation. Otherwise pass a complete, self-contained message with paths, feature goals, constraints, and expected output file.

If `quader-ui-architect` or `quader-renderer-architect` is not listed by the tool after their TOML files were edited, do not pretend the named role was used. Either ask the user to reload the agent registry, or spawn a default agent only as an explicit fallback and instruct it to read the matching TOML profile plus the architecture docs before acting.

For Task Intake Workflow, fallback replaces only the unavailable or unsuited architect role for that domain. Do not spawn both the unavailable architect and a default/fallback agent for the same domain, and do not spawn any additional scout with the same task prompt.

Fallback prompt shape:

```text
Act as quader-ui-architect. First read C:\Users\Drako\.codex\agents\quader-ui-architect.toml, then follow its developer_instructions. Create the requested plan in C:\Users\Drako\Desktop\_quader-qt-base\agents\plans. Do not implement code.
```

## Project Manager Workflow

Use this loop for multi-agent board work:

1. Read the task entry in `project_board.md`, `agents/task_board.md`, and any active plan paths named by the task.
2. Run the freshness preflight from `agents/task_board.md`; if the task remains valid, apply the exact PM-issued `set-freshness --pm-authorized --status fresh` command.
3. Move the task from `Backlog` to `In Progress` only if that is its current section, the user asked to perform it, and `Freshness:` is `fresh`.
4. Set a Codex `/goal` for the board task or PM coordination scope before implementation begins.
5. Spawn `quader-project-manager` with the full task entry, relevant docs, constraints, current board state, and any active plans.
6. Apply only the PM's exact board update requests with `tools/project_board.py`; mutating commands must include `--pm-authorized`.
7. Spawn the agents requested by the PM. Do not let worker/scout/architect agents edit the board directly.
8. Forward agent results back to the PM for reconciliation.
9. When the PM says the work is ready for authority review, send the integrated implementation context to the owning architect.
10. If the architect approves, apply the PM/authority board update and move `In Progress` to `In Review`.
11. If a worker reports a workaround/deviation or mid-work bug, route the report back through the PM and correct architect before allowing continuation.
12. Complete the Codex goal only after the executor's full scope is implemented, verified, documented when required, and handed back through PM/authority review or moved to `In Review` when allowed.
13. Do not complete, remove, cancel, or archive the task unless the user explicitly asks for that board operation and the PM returns the exact `--pm-authorized` command. For fixed bugs, the PM command must include a concise `--resolution` summary.

`In Review` cards are closeout work, not implementation work. When the user
selects or asks to close an `In Review` card, read the task, authority notes,
verification notes, plans, and bug resolution summary if any. Archive it only
after explicit user acceptance plus an exact PM-issued
`tools/project_board.py complete --pm-authorized ...` command. If the user
rejects the review build or reports a new issue, route that through PM and the
authoritative architect instead of performing an ad hoc fix.

Spawned agents must report requested board changes as `Board update request:` blocks rather than editing `project_board.md`.

For PM-coordinated batches, revalidate all selected tasks before assignment.
Workers are spawned only for tasks marked `fresh`; stale or superseded tasks are
routed to the relevant/finalizing architect or reported back to the user. PM
assignment prompts for implementation work must require each executor to set and
complete its own Codex `/goal` for that assignment. They must also require
workers to stop and report workaround/deviation needs or direct mid-work
bug/quick-fix requests to the main/root chat before continuing.

## Architect Workflow

Use this loop for non-trivial architecture-sensitive work:

1. Identify the owning architect.
   - Cross-layer or unclear ownership: software architect.
   - Qt Widgets, actions, models, panels, dialogs, settings: UI architect.
   - Crimson, render graph, shaders, PBR, overlays, picking: renderer architect.
2. Spawn the architect with a narrow, self-contained request.
3. The architect writes a plan or audit file under `agents/plans/`. Follow-up implementation plans for work derived from `agents/plans/audit_20260704_full_architecture_master.md` must cite that master audit and the relevant batch or accepted finding.
4. Read the plan before editing code.
5. Implement the plan as written. If the implementation reveals a design gap, return to the architect instead of inventing a new architecture locally.
6. Run the relevant build/tests.
7. Return to the same architect with:
   - plan file path,
   - changed files,
   - important implementation notes,
   - build/test results,
   - any known deviations.
8. The architect reviews the implementation.
9. If approved, move the plan file to `agents/archive/`. Do not delete plan files automatically.
10. If not approved, follow the updated plan and repeat the review loop.

Archived plan rules:

- Do not automatically delete architect plans or audit reports.
- Move implemented, old, superseded, or outdated plans to `agents/archive/`.
- After a plan is moved to `agents/archive/`, do not reference it from active plans, audits, implementation notes, or architecture instructions.
- The user may delete files under `agents/archive/` at any time.

For work crossing UI and renderer boundaries, such as viewport hosting, material inspector UI, render debug panels, or picking-driven selection:

- Ask the renderer architect to define renderer data flow, handles, passes, shader/material details, and render tests.
- Ask the UI architect to define widgets, actions, models, settings, panel contracts, and user interaction flow.
- Reconcile both plans before implementation.
- Use the software architect only if the two plans conflict on ownership or app-wide dependency direction.

## Architect Beautify Workflow

Use this workflow when the user asks for `beautify`, `beautify code`,
`API beautify`, or `audit and beautify`:

1. Route to the owning architect(s) using the same domain split as the normal architect workflow.
2. Ask each architect to use `BEAUTIFY_REVIEW` mode for standalone beautify requests.
3. For `audit and beautify`, ask architects to keep `Architecture Audit Findings` separate from `Beautify Findings`.
4. Beautify reports must include the current awkward API or call pattern, why it is awkward, the proposed prettier API, affected call sites/modules, migration risk, and tests/build checks needed.
5. Do not treat beautify as permission to implement API changes directly. Route actionable work through the PM/task-board flow unless the user explicitly authorizes immediate implementation and the conflict preflight is clear.
6. For frozen parallel audits, individual architects include beautify findings only when the user explicitly requested `audit and beautify`; the software architect keeps accepted architecture findings separate from accepted beautify recommendations in the master report.

## Concurrent Architecture Audit Workflow

Use this workflow when the user asks to audit overall architecture, UI, and renderer architecture at the same time.

The concurrency model is **Frozen Parallel**:

- All architects audit the same frozen baseline.
- No implementation, architecture doc edits, formatting, codegen, or unrelated file changes should happen during the audit.
- Audit reports are the only allowed outputs while the audit is running.
- Individual audit reports are source material, not implementation authority.
- Only the consolidated master audit is actionable.

Main-agent steps:

1. Create an `AUDIT_RUN_ID`, for example `20260704_full_architecture`.
2. Capture the baseline summary that every architect receives:
   - current date/time,
   - repo root,
   - relevant architecture docs,
   - current source layout,
   - user request,
   - explicit statement that findings are against this frozen baseline only.
3. Spawn all three architects in parallel with the same `AUDIT_RUN_ID`.
4. Tell each architect to write only its own report:
   - software architect: `agents/plans/audit_{run_id}_software.md`
   - UI architect: `agents/plans/audit_{run_id}_ui.md`
   - renderer architect: `agents/plans/audit_{run_id}_renderer.md`
5. Tell each architect to record issues owned by another domain as `Cross-domain concern` instead of redesigning that other domain.
6. After all three reports exist, send their paths to the software architect for consolidation.
7. The software architect writes `agents/plans/audit_{run_id}_master.md`.
8. Send the master audit's ordered implementation batches to `quader-project-manager`; the PM returns exact Backlog task commands using `tools/project_board.py add --pm-authorized --plans agents/plans/audit_{run_id}_master.md`.
9. Implementation may start only from board tasks linked to `audit_{run_id}_master.md`.

The master audit must include:

- accepted findings,
- rejected or superseded findings,
- conflicts between reports,
- stale sections or required follow-up audits if the workspace changed during the audit,
- ordered implementation batches,
- board-ready task titles, priorities, areas, summaries, owners, verification notes, and the active master audit path each task should list in `Plans:`.

If the workspace changes during a concurrent audit, do not silently merge stale reports. Either rerun affected audits against the new baseline or ask the software architect to mark stale sections before producing the master report.

## Implementation Rules

- Keep edits scoped to the requested task.
- Do not perform broad rewrites to match the final architecture unless an architect plan explicitly calls for it.
- Do not implement architecture-sensitive feature work unless the current task references the active master audit or a follow-up implementation plan.
- Do not add new frameworks or dependencies without explicit approval.
- Do not implement topology operations in UI callbacks.
- Do not expose graphics-runtime handles outside the Crimson boundary.
- Do not add runtime shader compilation.
- Do not apply post effects, lighting, fog, bloom, or tone mapping to overlays.
- Prefer simple C++20, RAII, explicit ownership, and narrow public APIs.
- Update docs when commands, architecture, build steps, or public behavior changes.

## Build Checkpoint Policy

For C++/CMake/shader/runtime changes, keep the executable runnable throughout implementation:

- Run `cmake --build --preset qt-mingw-debug` after each coherent implementation slice and before starting another independent code task.
- In PM-coordinated batches, do not complete more than two implemented code tasks without a successful build.
- Before architect review or moving a task to `In Review`, run the relevant build/tests.
- If user-visible runtime behavior changed, also run `cmake --build --preset qt-mingw-debug --target deploy` so the control board can launch the current executable.
- Dev-build archives are heuristic stable checkpoints, not manual-only. After a successful build/deploy for a task or PM batch that changes runnable behavior, executable/runtime packaging, shaders/assets, CMake/runtime dependencies, or fixes a bug observed in the executable, create a preserved snapshot with `python tools/dev_builds.py archive --preset qt-mingw-debug`.
- In PM-coordinated batches, archive once per coherent stable checkpoint, normally after the ready tasks in that checkpoint have passed verification and authority review or when the user should test the runnable exe. Do not archive after every tiny code slice.
- Skip automatic dev-build archiving for docs-only, task-board-only, external control-board frontend-only, tests-only, planning-only, or pure internal refactor work with no runnable behavior/runtime risk unless the user asks for a snapshot.
- If known concurrent work would make the archive include unrelated unverified changes, do not auto-archive. Report `Dev build checkpoint deferred:` with the reason and the command to run once the workspace is stable.
- `DEV_VERSION` still increments only after `tools/dev_builds.py archive` confirms the runnable archive. Never edit `DEV_VERSION` manually.
- If the build fails, stop further code-task batching in that area and route the failure through the PM as a blocker or fix assignment.
