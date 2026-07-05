# Quader Lean Workflow v2

This is the active daily workflow. Detailed policies remain in dedicated files, but normal routing should stay short and deterministic.

## Router

Use this order:

1. Docs-only request -> `quader-docs-mantainer`.
2. Explicit C++ performance audit/fix -> `quader-performance-dev`.
3. External Next.js control-board request -> use `$quader-board-developer` directly.
4. `audit`, `beautify`, or `audit and beautify` -> architect review workflow; root does not perform the analysis itself when any architect domain applies.
5. New product/build/workflow task or bug -> relevant architect scouts -> `quader-software-architect` finalization -> board entry.
6. Existing board task -> root revalidates freshness, coordinates workers directly, integrates, verifies, and sends to review.
7. Review task -> owning architect approves or rejects; approved items archive automatically through exact architect-issued commands.

The old manager role is not active workflow authority.

## Board Authority

`quader-software-architect` owns board authority and final architecture governance. It finalizes task intake, resolves cross-domain findings, arbitrates conflicts, and returns exact board commands for root to run.

Root is the mechanical board writer. It may run mutating `tools/project_board.py` commands only when applying exact software-architect commands. New mutating commands use `--architect-authorized`.

`--pm-authorized` exists only as a legacy compatibility alias. Do not use it for new tasks, prompts, docs, or examples.

Other agents must not mutate the board directly. They return findings, reports, reviews, implementation results, or exact requested context to root.

Exception: `quader-performance-dev` may add and move its own performance tasks with `--architect-authorized` only during explicit C++ performance audit/fix workflows. It still cannot complete, archive, remove, reopen, push, bump versions, or update the public changelog.

## Architect Domains

- `quader-core-architect`: document, commands, tools, mesh, I/O, app/core contracts, modeling-domain APIs.
- `quader-build-workflow-architect`: CMake, presets, tools, board/process, versioning, dev builds, agent workflow, support tooling.
- `quader-ui-architect`: Qt Widgets shell, actions, panels, item models, viewport host UI, settings, dialogs, UI services.
- `quader-renderer-architect`: Crimson, render graph, shaders, materials, overlays, picking, renderer assets, render tests.
- `quader-software-architect`: finalizes multi-domain findings, owns board commands, resolves conflicts, approves cross-domain architecture.

Active skill:

Use `$quader-board-developer` as a skill, not a spawned agent, for external Next.js project-board frontend/API work, copied prompts, archive/changelog views, dev-build controls, and dashboard styling.

When spawning agents, prefer clean explorer scouts with `fork_context: false`; pass the repository path, relevant files, and scoped instructions. Do not spawn full-history forks unless context only exists in the current conversation.

Root must wait for every spawned agent it created before finalizing the turn, unless the user explicitly cancels or redirects.

## Critical Hindsight

`agents/hindsight.md` is the durable register for reusable lessons future agents should not rediscover. It is separate from `dev-changelog.md`, board history, and active plans.

Before intake, task revalidation, implementation, audit/beautify, deviation review, bug review, final review, documentation maintenance, or performance work, search/read `agents/hindsight.md` for entries matching the relevant area. Reference matching hindsight IDs in reports, plans, task briefs, or review decisions only when they change or constrain the work. If no entry applies, do not add boilerplate.

If a worker, scout, architect, docs maintainer, or performance agent discovers reusable critical knowledge, it must add a `Hindsight Candidate:` block to its report with area/tags, lesson, evidence, and when it applies. Root routes candidates to the owning architect for acceptance. Root or `quader-docs-mantainer` records only accepted entries in `agents/hindsight.md`.

Record hidden build/tool traps, parity/reference mismatches, renderer math conventions, workflow holes, stale assumptions, non-obvious architecture constraints, verification pitfalls, licensing/reuse constraints, and rejected shortcuts. Do not record normal progress, generic best practices, one-off bugs, changelog items, active task todos, or plan content that belongs in `agents/plans/`.

## Audit And Beautify Routing Lock

Requests containing `audit`, `beautify`, `beautify code`, `API beautify`, or `audit and beautify` are architect-review requests, not root self-analysis.

Root may only classify scope, spawn the correct architect scouts, pass paths/context, wait for reports, and route final consolidation. Root must not inspect the codebase and produce its own architecture/beautify findings when an architect domain applies.

For `audit and beautify the current code` or similarly broad requests:

1. Spawn clean scoped scouts for relevant domains: `quader-core-architect`, `quader-build-workflow-architect`, `quader-ui-architect`, and `quader-renderer-architect`.
2. Root assigns one `AUDIT_RUN_ID` and an explicit report path for each domain:
   - `agents/plans/audit_{run_id}_core.md`
   - `agents/plans/audit_{run_id}_build_workflow.md`
   - `agents/plans/audit_{run_id}_ui.md`
   - `agents/plans/audit_{run_id}_renderer.md`
3. Each scout writes a full markdown report to its assigned path. For `audit and beautify`, that file must contain separated `Architecture Audit Findings` and `Beautify Findings`.
4. Each scout returns only the report path, coverage summary, and blockers. Root must not summarize, compress, rewrite, or filter the findings for the software architect.
5. Root waits for all report files, then sends the file paths to `quader-software-architect`.
6. `quader-software-architect` reads the report files directly, writes `agents/plans/audit_{run_id}_master.md`, arbitrates conflicts, and keeps architecture findings separate from beautify recommendations.
7. After the master audit is successfully written, the transient domain report files for that run are deleted from `agents/plans/`. They are not archived and must not be referenced by later plans.
8. No board task is created unless the user explicitly asks to turn findings into work.

This deletion rule is a narrow exception to the normal plan archival rule. It applies only to source reports consumed by a same-run master audit. Standalone single-domain audit/beautify reports remain active plan documents until archived normally.

For a single-domain audit/beautify request, spawn only the owning architect, require it to write its full report to `agents/plans/`, and route board-ready follow-up through `quader-software-architect` only if the user asks for tasks.

## New Task Intake

Fresh task requests are intake-only by default. Imperative wording is not consent to implement.

Flow:

1. Classify the architect domain(s).
2. Spawn at most one scout per relevant domain. Do not spawn duplicate default/scout/worker agents for the same scope.
3. Scouts return `Task Intake Domain Findings`.
4. `quader-software-architect` reconciles findings and returns one `Final Task Intake Report`.
5. The software architect checks duplicates and returns exact `tools/project_board.py add --architect-authorized` or `add-bug --architect-authorized` commands, or a duplicate/no-op decision.
6. Root applies only those exact commands and reports the new task id or no-op.

If no architect domain applies, root researches the current workspace and sends that report to `quader-software-architect` for board authority.

No preflight or setup task may be invented just so work can be assigned. Preflight tasks require an explicit user request or active architect plan.

Reference/parity intake must include:

```text
Source refs: C:\path\reference | src/foo.cpp:12-88; include/foo.h:4-31
```

Phrases such as `check this code reference`, `same`, `1:1`, `100%`, or `parity` mean match behavior and structure as closely as practical without unsafe verbatim third-party copying.

For renderer parity against `C:\Users\Drako\Desktop\quader-windows\quader-app`, route to `quader-renderer-architect` and explicitly require inspection of that app's renderer internals. Do not assume its renderer math, projection, picking, shader/material, or coordinate-space conventions match Crimson/current runtime behavior.

## Existing Task Execution

Before starting:

1. Read the task, active plans, source refs, current board metadata, and likely touched files.
2. Run the freshness gate. Missing freshness counts as `needs-refresh`.
3. If the task is still valid, root applies the exact architect-issued `set-freshness --architect-authorized --status fresh` command.
4. If stale or superseded, root routes the revalidation report to the software architect and does not implement.

Execution:

- Root coordinates implementation directly.
- Spawn up to 6 clean worker agents only when work is independent enough to parallelize.
- Each worker receives a scoped prompt, required docs, relevant paths, verification requirements, and a mandatory `/goal`.
- Workers implement only their slice and do not mutate the board.
- If a worker changes any documented C++ class, function, variable, enum, member, alias, template, option/result type, or behavior/invariant comment, it must update that documentation in place in the same edit.
- If a worker adds new previously undocumented code and does not add Doxygen notes, it reports `Docs maintainer follow-up:` instead of treating that as an implementation blocker, unless the task explicitly required new documentation.
- Root integrates, resolves mechanical conflicts, runs required checks, deploys if runtime behavior changed, and asks the owning architect for review.

Executor `/goal` is mandatory for implementation work. The goal must name the task id/title or assignment and state that it is complete only when the full scope is implemented, verified, documented when required, and handed back for architect review.

Executors must not silently apply workarounds, weaken validation, reinterpret parity, or change acceptance criteria. If the accepted brief or plan cannot be followed, stop and return a `Workaround/Deviation Report` to root.

If the user reports a bug or requests a quick fix inside a running executor chat, the executor stops and returns a `Mid-work Bug Report` to root. Root routes it to the correct architect before further work.

## Review Closeout

Review goes to the owning architect:

- Core/modeling/app/build/workflow/cross-domain: software architect, optionally with core or build/workflow input.
- UI: UI architect.
- Renderer: renderer architect.
- Performance: performance-dev may move performance tasks to review; final archive still uses software-architect authority unless the user explicitly says otherwise.

The architect reviews the implementation against the board brief, active plan, current code, verification results, architecture policy, tests policy, code style policy, and documentation policy where applicable. Review must reject code changes that leave existing symbol documentation stale, incomplete, or contradictory. Review must not reject solely because newly added, previously undocumented code lacks Doxygen notes when the task did not explicitly require documentation; return a `Docs maintainer follow-up:` instead.

For a batch of review tasks, the architect returns a per-task decision:

- `approved`: root applies the exact `complete --architect-authorized` command and archives it.
- `changes-requested`: root applies the exact `request-changes --architect-authorized` command with a mandatory actionable `--rework` correction list. Do not use a plain `move --to "In Progress"` for rejected review closeout.
- `stale`: root applies `set-freshness --architect-authorized --status stale`, routes the report to the software architect, and does not continue implementation.
- `superseded`: root applies `set-freshness --architect-authorized --status superseded`; implementation stops unless the user or software architect replaces the brief.

Rejected review corrections must be stored in the task's current `Rework:` metadata. Coordination notes are context only and are not the authoritative correction list for returned work.

Approved tasks and fixed bugs archive automatically after architect approval. No extra PM or user closeout is required. Fixed bugs require a concise `Bug resolution summary:` and the archive command must include `--resolution "<summary>"`.

## Build Checkpoints

For C++/CMake/shader/runtime changes:

- Run `cmake --build --preset qt-mingw-debug` after each coherent implementation slice and before another independent code task starts. This is the fast app/shader runtime build; use `qt-mingw-debug-tests`, targeted test binaries, targeted CTest selections, and `qt-mingw-debug-full` when verification requires them. Broad CTest presets are subject to the style/static-analysis permission lock below.
- Do not go more than two implemented code tasks without a successful build.
- Before review or `In Review`, run relevant build/tests.
- If user-visible runtime behavior changed or the executable must launch from Explorer, run `cmake --build --preset qt-mingw-debug-deploy` for debug or `cmake --build --preset qt-mingw-release` for release. The release preset deploys runtime DLLs by default; use `qt-mingw-release-runtime` only for a compile-only release app/shader build.
- Dev-build archives are a closeout gate for runnable work, not a manual-only convenience. Before moving a task to `In Review` or asking final architect review, create a dev-build archive with `python tools/dev_builds.py archive --preset qt-mingw-debug` when the task changed production C++ runtime code, CMake/deploy/runtime packaging, shaders/assets, UI/runtime behavior, renderer behavior, document/command/tool/mesh/I/O behavior that affects the executable, or fixed an executable-observed bug.
- Report the archived dev version and archive path in the executor/root closeout. `DEV_VERSION` increments only when the archive command succeeds; never edit it manually.
- Deferral is allowed only when the work is docs/task-board/control-board frontend/tests/planning only, the build/deploy needed for a runnable snapshot failed, the executable is intentionally unavailable, active concurrent unverified work in the same build tree would contaminate the snapshot, or the user explicitly says not to archive. If deferred, report `Dev build checkpoint deferred:` with the exact blocker and the next owner/action needed to produce the archive.
- Architects must treat a runnable task that reaches review without an archived dev version or a valid `Dev build checkpoint deferred:` line as `changes-requested`.

Use preset build commands as written; the debug and release build presets set `jobs: 20` as the default local parallelism cap.

## Style And Static-Analysis Permission Lock

Do not run clang-format, clang-tidy, `check_format`, `check_clang_tidy`, `check_static_analysis`, `tools/check_clang_format.py`, `tools/check_clang_tidy.py`, raw `clang-format`, raw `clang-tidy`, or any preset/test command that includes those gates unless the user gives immediate explicit permission in the current turn.

An architect plan, board task, or verification checklist that merely lists style/static-analysis commands is not run-time permission. Treat those lines as gated final-validation candidates. They may be run only if the user explicitly says to run them now, or if the exact architect/board command says `run now without asking`.

Before running broad `ctest --preset ...`, inspect whether that preset includes style/static-analysis tests. If it does, use targeted test binaries or a targeted CTest selection instead, unless the user explicitly authorizes the full preset.

If a forbidden style/static-analysis gate starts accidentally, stop it, report the mistake, and continue with allowed targeted build/tests. Do not run formatting fixers as a reaction to an unauthorized style-gate failure.

## Plans

Active implementation and audit plans live in `agents/plans/`. Implemented, old, or outdated plans move to `agents/archive/` after the relevant work is approved. Archived plans are not active authority and must not be referenced by new plans, audits, or implementation notes.

Master plans or audits become board tasks through `quader-software-architect`, not through direct board edits.

## Changelogs And Versions

Root `VERSION` is public release state and changes only during push/release workflow.

Root `DEV_VERSION` is the internal dev-build counter and displays as `<VERSION>-dev.<DEV_VERSION>`. It increments only after `python tools/dev_builds.py archive --preset qt-mingw-debug` succeeds.

Record durable development changes in `dev-changelog.md`. The public `changelog.md` is user-facing and should contain only non-hyper-technical features, important behavior changes, and critical bug fixes.

## Tests, Docs, Style, Git

Read `agents/tests-policy.md` before planning, writing, modifying, deleting, or reviewing tests.

Read `agents/documentation-policy.md` before changing documentation, comments, API docs, changelogs, README, architecture docs, or agent instructions.

When changing code, also read `agents/documentation-policy.md` if the touched symbol or nearby behavior is documented. Existing documentation that becomes stale must be updated in the same edit.

Read `agents/code-style.md` before non-trivial C++ source/style/static-analysis decisions. Do not run clang-format or clang-tidy checks unless the user requests them.

Keep unrelated user changes. Do not use destructive git commands unless explicitly requested. Do not push unless the user explicitly asks.

## External Board Developer

Use `$quader-board-developer` for non-trivial work in `C:\Users\Drako\Desktop\quader-project-board`.

That skill workflow may directly edit the external Next.js app for dashboard/UI/API/prompt behavior, run `npm run typecheck` and `npm run build`, and update `dev-changelog.md` when workflow behavior changes. It is not authority for board content: actual `project_board.md` and `project_board_archive.md` mutations still require exact `quader-software-architect` commands with `--architect-authorized`.

## Immediate Work Bypass

If the user explicitly says to implement immediately and skip intake, run a conflict preflight first: active board entries, `Active:`, `Authority:`, `Assignment:`, `Coordination:`, current git changes, and available Codex thread tools. If overlap is visible, ask whether to coordinate, message the other thread, or proceed anyway. If no overlap is visible, record that the check is best-effort.
