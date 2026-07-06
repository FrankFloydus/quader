# Quader Architect-Led Workflow

The active workflow is root-coordinated and architect-led. `$quader-workflow` is disabled and is not an active routing authority. Root may spawn the active `quader-*.toml` agents as clean scoped agents; do not spawn full-history forks.

Active profiles:

- `quader-software-architect`: final architecture governor, final plan/review reconciler, board authority.
- `quader-core-architect`: document, commands, tools, mesh, I/O, app/core.
- `quader-build-workflow-architect`: CMake, tools, board/API, versioning, workflow.
- `quader-ui-architect`: Qt Widgets, actions, models, panels, viewport host UI.
- `quader-renderer-architect`: Crimson renderer, shaders, overlays, picking, render tests.
- `quader-docs-mantainer`: documentation-only maintenance.
- `quader-performance-dev`: explicit C++ performance audit/fix work using `cpp-performance`.

Root coordinates all spawned agents, waits for every spawned agent, verifies required artifacts exist, applies database-board API actions, integrates implementation, and runs the required checks. Architects scout, plan, and review; they do not implement production code unless their profile explicitly allows it.

## Artifact Policy

Use Windows-safe timestamps: `MM_DD_YYYY_HHMM`.

Architect plan mode:

- Trigger: an architect scouts/analyzes code for a new requested task or a stale task re-plan.
- Output: a detailed markdown plan under `agents/plans/`.
- Default name: `agents/plans/{architect-name}-plan_{MM_DD_YYYY_HHMM}.md`.
- Root may assign an exact path; if no exact path is provided, the architect uses the default.
- The file must be implementation-ready: exact files/symbols, ordered edits, APIs/data shapes, invariants, do-not-do constraints, tests/build/dev-build checks, docs updates, acceptance criteria, and unresolved blockers.

Root final plan:

- Output after architect plans are complete: `agents/plans/{task_id}-final-plan_{MM_DD_YYYY_HHMM}.md`.
- This is not a summary. It reconciles all architect plans, removes only true duplicates/useless material, preserves unique findings, and gives the sequential implementation recipe future workers should follow literally.
- After the final plan is written and linked into the board task, root archives the source architect plans into `agents/archive/`.

Architect review mode:

- Trigger: implementation is thought complete, or user reports review failure.
- Output: `agents/reviews/{task_id}_{architect-name}_review_{MM_DD_YYYY_HHMM}.md`.
- Each review file must include `Decision: approved` or `Decision: changes-requested`.
- `approved` reviews may be concise but must state reviewed scope and checks expected.
- `changes-requested` reviews must be implementation-ready with exact corrections.

Root final review:

- If any review has `Decision: changes-requested`, root writes `agents/reviews/{task_id}-final-review_{MM_DD_YYYY_HHMM}.md`, archives individual architect reviews, archives or supersedes the original final plan, links the final review into the board task, and implements the requested changes.
- This loop recurses until all relevant architects return `Decision: approved`.
- When all reviews approve, root archives review artifacts and moves the board task to `In Review`.

## Workflow A: Task Only

Trigger: user asks to add one or more tasks/bugs without immediate implementation.

1. Root classifies required architect domains.
2. Root spawns the needed architects as clean scoped agents with repository path, exact artifact path expectations, and scoped instructions.
3. Each architect runs architect plan mode.
4. Root waits for all architects. If any required plan file is missing or too generic, root asks only the missing/insufficient architect to repair it and waits again.
5. Root asks `quader-software-architect` to reconcile the architect plans and issue the exact database board add action payload.
6. Root creates the task/bug through `POST /api/board/action` using `add-task` or `add-bug` to lock a task id.
7. Root writes the final plan as `agents/plans/{task_id}-final-plan_{MM_DD_YYYY_HHMM}.md`.
8. Root updates the board task through `edit` so the task metadata/description includes the final plan path.
9. Root archives the source architect plans into `agents/archive/`.

## Workflow B: Implementation Only

Trigger: user pastes a board prompt, asks to implement task ids, or asks root to check the board for specific tasks.

1. Root reads the board task through the database API and fetches the linked final plan.
2. Root sends the task, current final plan path, and current workspace status to `quader-software-architect` for freshness evaluation.
3. If fresh, root implements the final plan as-is. Root should not re-scout or redesign.
4. If stale or needing re-evaluation, root runs architect plan mode for the affected domains, updates/replaces the final plan, updates the board task with the new plan path, and then implements the plan as-is.

## Workflow C: Task And Implementation

Trigger: user asks to add/plan a task and implement it immediately.

Run Workflow A, then immediately run Workflow B without a separate freshness gate because the final plan was just produced.

## Workflow D: On Task/Goal Completed

Trigger: root believes the implementation task/goal is done, before ending the turn.

1. Root spawns the relevant reviewing architects in architect review mode.
2. Root waits for all review artifacts.
3. If every review says `Decision: approved`, root archives review files and moves the board task to `In Review`.
4. If any review says `Decision: changes-requested`, root writes a final review master file, archives the individual reviews and the superseded final plan, updates the board task with the final review path, implements the requested changes, and repeats Workflow D.

## Workflow E: Mark Review As Complete

Trigger: user explicitly asks to mark a reviewed task complete.

Root uses the database board API to archive/complete the task. The action must include `devVersion`; archive metadata must store `dev_version`, archive date, archived timestamp, source section, and archive status.

## Workflow F: Review Not Passed

Trigger: user manually tests the app and reports bugs, incorrect behavior, or missing expected behavior.

1. Root treats the user feedback as the authoritative correction list.
2. Root enters the review/rework loop using architect review mode and a final review master if needed.
3. After implementing corrections, root does not launch another architect review unless the user asks. Instead it reports what was fixed, what the user should see, and what the user should try.
4. If the user approves, root archives/completes the task with `devVersion`. If not, repeat from step 1.

## Database Board

The project board is active through the SQLite-backed external app/API:

- app: `C:\Users\Drako\Desktop\quader-project-board`
- default DB: `C:\Users\Drako\Desktop\quader-project-board\data\quader-board.sqlite`
- source module: `C:\Users\Drako\Desktop\quader-project-board\lib\board-database.ts`

Never hand-edit `project_board.md` or `project_board_archive.md`; never use `tools/project_board.py` for active workflow.

Reads:

- `GET /api/board`
- `GET /api/board/task?id=...&state=active|archived|all`
- `GET /api/board/search?q=...&state=active|archived|all`
- `GET /api/archive`

Mutations:

- `POST /api/board/action`
- Required authorization flag: `workflowAuthorized`, `architectAuthorized`, or `pmAuthorized`.
- Actions: `add-task`, `add-bug`, `edit`, `delete`, `move`, `start`, `review`, `request-changes`, `complete`, `archive`, `reopen`, `append-note`, `bulk-edit-meta`.

Archive/complete payloads must include `devVersion` matching `x.y.z-dev.n`. Do not infer dev versions from changelog text, archive notes, or historical markdown.

## Parity And Reference App

`same`, `1:1`, `100%`, `parity`, and `check this code reference` require exact `Source refs:` in the brief or plan.

When `C:\Users\Drako\Desktop\quader-windows\quader-app` is cited, it is source of truth for behavior, interaction semantics, visual result, data meaning, and user-visible substance. Plans must map reference files/lines to current Quader files/symbols and include `Reference substance preserved:` checks.

For parity work, architects should use the shared communication chat:

```text
codex://threads/019f365b-1ae4-7172-a3f1-d98d39834146
```

Root or the relevant architect asks the communication chat for targeted original-app details, waits for the response, and does no unrelated work while waiting. Poll/wait in 60 second intervals until the response arrives or a real blocker must be reported.

## External Resources

- Qt widget/API work: quick Qt docs scan; official Qt docs are primary.
- Renderer/graphics work: browse/read relevant Graphics Compendium or LearnOpenGL pages when useful.
- Renderer implementation work: scan targeted folders in `C:\Users\Drako\Desktop\_SOURCES` and cite file/line references.
- Third-party libraries: surface mature, commercially usable narrow dependencies as explicit decisions; do not replace central Quader systems.

## Build And Dev Builds

For C++/CMake/shader/runtime changes:

- build after coherent slices: `cmake --build --preset qt-mingw-debug --parallel 20`;
- run relevant tests before final handoff;
- deploy for user-visible runtime changes: `cmake --build --preset qt-mingw-debug-deploy --parallel 20`;
- archive a dev build before runtime-affecting final handoff unless a clear `Dev build checkpoint deferred:` blocker applies;
- never edit `DEV_VERSION` manually.

Do not run clang-format, clang-tidy, style/static-analysis targets, or CTest presets that include them without immediate explicit user permission.

## Hindsight, Docs, Tests, Git

Before intake, revalidation, execution, audit/beautify, review, docs, performance, or deviation handling, search/read relevant `agents/hindsight.md` entries.

Read `agents/tests-policy.md` before test work. Read `agents/documentation-policy.md` when docs/comments/changelogs or documented symbols are involved. Read `agents/code-style.md` before non-trivial C++ style/static-analysis decisions.

Before commits inspect `git status --short` and `git diff --cached --name-status`; split unrelated staged changes. Do not push unless asked.
