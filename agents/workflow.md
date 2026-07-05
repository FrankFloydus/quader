# Quader Workflow

`$quader-workflow` is the same-thread workflow authority for board intake, planning, implementation, review/rework, docs, performance, parity, dev builds, and external board work. Do not spawn old `quader-*.toml` agents.

## Router

- Add task/bug -> intake only.
- Implement/fix/do work with no task id -> add board entry first, then execute it.
- Existing task id -> revalidate, plan, execute.
- Review failure -> `request-changes`, store `Rework:`, fix again.
- `audit`, `beautify`, `audit and beautify` -> write full artifact in `agents/plans/`.
- External board/dashboard -> edit `C:\Users\Drako\Desktop\quader-project-board`; board content still mutates via CLI.

## Modes

### Add Task/Bug

Read board, relevant docs/hindsight, duplicates, and enough code/reference material to create a strong board brief. Do not create a plan. Add through `tools/project_board.py add|add-bug --workflow-authorized`. New entries start `Freshness: needs-refresh`.

Brief must include behavior, likely files/modules, initial direction, risks, source refs for parity, and likely verification.

### Execute

Execution starts only from a board entry. Read task, `Rework:`, source refs, hindsight, current code, and any existing plan. Revalidate freshness before work; stale/superseded tasks stop.

Create/update exactly one plan:

```text
agents/plans/implementation_task{id}_{slug}.md
```

The plan must be worker-ready: exact files/symbols, ordered edits, APIs/data shapes, invariants, do-not-do constraints, tests/build/dev-build checks, docs updates, acceptance criteria.

Set a Codex `/goal`; finish it only after scope, verification, docs sync, and board-ready state are complete. For complex work, split internally or use clean scoped workers with `fork_context:false`; wait for all workers.

### Review/Rework

Rejected review must use `request-changes --workflow-authorized` with actionable `--rework`; do not plain-move rejected review cards. `Rework:` is the authoritative correction list and must be reflected in the same plan under `Review Rework`.

Approved tasks/fixed bugs archive with `complete --workflow-authorized`. Fixed bugs require `Bug resolution summary:` and `--resolution`.

### Audit/Beautify

Write full findings to `agents/plans/`; chat findings are not enough. Broad artifact pattern: `agents/plans/audit_YYYYMMDD_scope_master.md`.

`audit and beautify` separates `Architecture Audit Findings` from `Beautify Findings`. Audit covers boundaries, ownership, correctness, verification, and risks. Beautify covers API/call-site elegance, naming, ownership, parameter/result shape; not formatting.

## Authority

`$quader-workflow` owns board authority. Use `--workflow-authorized`; old `--architect-authorized`/`--pm-authorized` are compatibility only. Never hand-edit board markdown.

## Hindsight

Before intake, revalidation, execution, audit/beautify, review, docs, performance, or deviation handling, search/read relevant `agents/hindsight.md` entries. Reference matching IDs only when they affect the work.

If reusable critical knowledge appears, report:

```text
Hindsight Candidate:
Area/tags:
Lesson:
Evidence:
Applies when:
```

## Parity

`same`, `1:1`, `100%`, `parity`, and `check this code reference` require exact `Source refs:` in board brief/plan.

When `C:\Users\Drako\Desktop\quader-windows\quader-app` is cited, it is source of truth for behavior, interaction semantics, visual result, data meaning, and user-visible substance. Intake briefs must name that path and inspected files/folders. Execution plans must map reference files/lines to current Quader files/symbols and include `Reference substance preserved:` checks.

For renderer parity with that app, inspect its renderer internals before projection/picking/material/overlay/coordinate-space decisions.

## External Resources

- Qt widget/API work: quick Qt docs scan; official Qt docs are primary.
- Renderer/graphics work: browse/read relevant Graphics Compendium or LearnOpenGL pages when useful.
- Renderer implementation work: scan targeted folders in `C:\Users\Drako\Desktop\_SOURCES` and cite file/line references.
- Third-party libraries: surface mature, commercially usable narrow dependencies as explicit decisions; do not replace central Quader systems.

## Build And Dev Builds

For C++/CMake/shader/runtime changes:

- build after coherent slices: `cmake --build --preset qt-mingw-debug --parallel 20`;
- run relevant tests before review;
- deploy for user-visible runtime changes: `cmake --build --preset qt-mingw-debug-deploy --parallel 20`;
- archive a dev build before runtime-affecting review/final closeout unless a clear `Dev build checkpoint deferred:` blocker applies;
- never edit `DEV_VERSION` manually.

Do not run clang-format, clang-tidy, style/static-analysis targets, or CTest presets that include them without immediate explicit user permission.

## Plans And Archives

Add/intake creates no plan. Execute creates exactly one active implementation plan. Audit/beautify creates the requested artifact. Old/implemented/outdated plans move to `agents/archive/` and stop being active authority.

## Tests, Docs, Git

Read `agents/tests-policy.md` before test work. Read `agents/documentation-policy.md` when docs/comments/changelogs or documented symbols are involved. Read `agents/code-style.md` before non-trivial C++ style/static-analysis decisions.

Before commits inspect `git status --short` and `git diff --cached --name-status`; split unrelated staged changes. Do not push unless asked.
