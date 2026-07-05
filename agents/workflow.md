# Quader Lean Workflow v2

This is the active daily workflow. Detailed policies remain in dedicated files, but normal routing should stay short and deterministic.

## Router

Use this order:

1. Docs-only request -> `quader-docs-mantainer`.
2. Explicit C++ performance audit/fix -> `quader-performance-dev`.
3. External Next.js control-board request -> use `$quader-board-developer` directly.
4. New product/build/workflow task or bug -> relevant architect scouts -> `quader-software-architect` finalization -> board entry.
5. Existing board task -> root revalidates freshness, coordinates workers directly, integrates, verifies, and sends to review.
6. Review task -> owning architect approves or rejects; approved items archive automatically through exact architect-issued commands.

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

The architect reviews the implementation against the board brief, active plan, current code, verification results, architecture policy, tests policy, code style policy, and documentation policy where applicable.

For a batch of review tasks, the architect returns a per-task decision:

- `approved`: root applies the exact `complete --architect-authorized` command and archives it.
- `changes-requested`: root applies the exact `move --architect-authorized --to "In Progress"` command and records `Authority: [status:changes-requested]` or a coordination note if useful.
- `stale`: root applies `set-freshness --architect-authorized --status stale`, routes the report to the software architect, and does not continue implementation.
- `superseded`: root applies `set-freshness --architect-authorized --status superseded`; implementation stops unless the user or software architect replaces the brief.

Approved tasks and fixed bugs archive automatically after architect approval. No extra PM or user closeout is required. Fixed bugs require a concise `Bug resolution summary:` and the archive command must include `--resolution "<summary>"`.

## Build Checkpoints

For C++/CMake/shader/runtime changes:

- Run `cmake --build --preset qt-mingw-debug` after each coherent implementation slice and before another independent code task starts. This is the fast app/shader runtime build; use `qt-mingw-debug-tests`, `ctest --preset qt-mingw-debug`, and `qt-mingw-debug-full` when verification requires test executables, the registered CTest suite, or Ninja's full `all` target.
- Do not go more than two implemented code tasks without a successful build.
- Before review or `In Review`, run relevant build/tests.
- If user-visible runtime behavior changed or the executable must launch from Explorer, run `cmake --build --preset qt-mingw-debug-deploy` for debug or `cmake --build --preset qt-mingw-release` for release. The release preset deploys runtime DLLs by default; use `qt-mingw-release-runtime` only for a compile-only release app/shader build.
- Create a dev-build archive after a stable runnable change, executable/runtime packaging change, shader/asset change, CMake/runtime dependency change, or executable-observed bug fix unless the work is docs/task-board/control-board frontend/tests/planning only or concurrent unverified work would contaminate the snapshot.
- If skipped, report `Dev build checkpoint deferred:` with the reason.

Use preset build commands as written; the debug and release build presets set `jobs: 20` as the default local parallelism cap.

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

Read `agents/code-style.md` before non-trivial C++ source/style/static-analysis decisions. Do not run clang-format or clang-tidy checks unless the user requests them.

Keep unrelated user changes. Do not use destructive git commands unless explicitly requested. Do not push unless the user explicitly asks.

## External Board Developer

Use `$quader-board-developer` for non-trivial work in `C:\Users\Drako\Desktop\quader-project-board`.

That skill workflow may directly edit the external Next.js app for dashboard/UI/API/prompt behavior, run `npm run typecheck` and `npm run build`, and update `dev-changelog.md` when workflow behavior changes. It is not authority for board content: actual `project_board.md` and `project_board_archive.md` mutations still require exact `quader-software-architect` commands with `--architect-authorized`.

## Immediate Work Bypass

If the user explicitly says to implement immediately and skip intake, run a conflict preflight first: active board entries, `Active:`, `Authority:`, `Assignment:`, `Coordination:`, current git changes, and available Codex thread tools. If overlap is visible, ask whether to coordinate, message the other thread, or proceed anyway. If no overlap is visible, record that the check is best-effort.
