# Quader Workflow

`$quader-workflow` is the same-thread workflow authority for direct implementation, planning, review/rework, docs, performance, parity, dev builds, audit/beautify, and explicit external board work. Do not spawn old `quader-*.toml` agents.

Temporary board pause: the project board is not workflow authority. Do not create, update, validate, move, archive, or require board entries unless the current user request explicitly asks to inspect, repair, restore, or maintain board tooling.

## Router

- Add task/bug/intake -> capture the requested work from the user request and current repo context; do not mutate the board.
- Implement/fix/do work with no task id -> proceed directly after reading relevant docs, hindsight, and code.
- Existing task id -> treat it as legacy context only if the user explicitly cites it; do not mutate board state.
- Review failure -> use the latest user/review feedback as authoritative rework, update the same plan when one exists, then fix and verify.
- `audit`, `beautify`, `audit and beautify` -> write full artifact in `agents/plans/`.
- External board/dashboard -> edit `C:\Users\Drako\Desktop\quader-project-board` only when explicitly requested; project-board content remains paused.

## Modes

### Intake

Use for `add task`, `add bug`, intake, or scope-capture requests while the board pause is active.

Read relevant docs/hindsight, duplicates in current plans when useful, and enough code/reference material to create a strong brief. Do not create or update board entries. If the user asks for a durable artifact, write an intake or planning note under `agents/plans/`; otherwise report the brief in chat.

Briefs should include requested/observed behavior, likely files/modules, initial fix/implementation direction, risks, source refs for parity/reference work, and likely verification.

### Execute

Execution can start directly from the current user request, an existing plan, or explicit legacy board context cited by the user.

1. Read relevant docs, matching hindsight, current code, source refs, review feedback, and any existing active plan.
2. Revalidate the request against the current workspace before editing.
3. Create or update a plan when the task is broad, risky, cross-domain, delegated, explicitly requested, or already plan-backed.
4. New board-free implementation plans use:

```text
agents/plans/implementation_YYYYMMDD_{slug}.md
```

The plan must be worker-ready when it exists: exact files/symbols, ordered edits, APIs/data shapes, invariants, do-not-do constraints, tests/build/dev-build checks, docs updates, and acceptance criteria.

Set a Codex `/goal` only for substantial work that benefits from explicit goal tracking. Implement, verify, and sync docs. Do not update board files or run board commands.

For complex work, split internally or use clean scoped workers with `fork_context:false`; wait for all workers.

For Quader Windows parity tasks, the plan or brief must map reference files/lines to current Quader files/symbols and state `Reference substance preserved:` acceptance checks. Do not invent different behavior unless the user asks for redesign.

### Review/Rework

Use latest user/review feedback as the authoritative correction list. If an implementation plan exists, update it with `Review Rework`, then fix against both original scope and current rework, verify again, and report status. Do not move, archive, complete, or otherwise mutate board entries.

### Audit/Beautify

Write full findings to `agents/plans/`; chat findings are not enough. Broad artifact pattern: `agents/plans/audit_YYYYMMDD_scope_master.md`.

`audit and beautify` separates `Architecture Audit Findings` from `Beautify Findings`. Audit covers boundaries, ownership, correctness, verification, and risks. Beautify covers API/call-site elegance, naming, ownership, parameter/result shape; not formatting.

## Board Pause

`project_board.md` and `project_board_archive.md` are historical/read-only during this pause. Do not use them as gating authority for new work. Do not run `tools/project_board.py` commands during normal execution, review, intake, closeout, or audit work.

Allowed board access during the pause:

- read-only context when the user explicitly cites a task id or board entry;
- explicit board restoration, repair, migration, validation, or dashboard maintenance requested by the user;
- external board app code changes that do not mutate board content.

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

`same`, `1:1`, `100%`, `parity`, and `check this code reference` require exact `Source refs:` in the brief or plan.

When `C:\Users\Drako\Desktop\quader-windows\quader-app` is cited, it is source of truth for behavior, interaction semantics, visual result, data meaning, and user-visible substance. Briefs/plans must name that path and inspected files/folders. Execution plans must map reference files/lines to current Quader files/symbols and include `Reference substance preserved:` checks.

For renderer parity with that app, inspect its renderer internals before projection/picking/material/overlay/coordinate-space decisions.

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

## Plans And Archives

Intake creates no board entry. Execute creates or updates one active implementation plan only when plan-backed execution is needed. Audit/beautify creates the requested artifact. Old/implemented/outdated plans move to `agents/archive/` and stop being active authority.

## Tests, Docs, Git

Read `agents/tests-policy.md` before test work. Read `agents/documentation-policy.md` when docs/comments/changelogs or documented symbols are involved. Read `agents/code-style.md` before non-trivial C++ style/static-analysis decisions.

Before commits inspect `git status --short` and `git diff --cached --name-status`; split unrelated staged changes. Do not push unless asked.
