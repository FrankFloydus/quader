# Quader Agent Bootstrap

Keep this file short. Put detailed policy in focused docs or `$quader-workflow`.

Quader is a C++20 Qt Widgets polygon modeling app. The renderer is `crimson`; do not use graphics-runtime names in renderer folders, targets, or planned public class names.

## Read First

- Workflow authority: `agents/workflow.md`
- Board pause/status: `agents/task_board.md`
- Tests: `agents/tests-policy.md`
- Documentation: `agents/documentation-policy.md`
- Critical hindsight: `agents/hindsight.md`
- C++ style/static analysis: `agents/code-style.md`
- Architecture briefs: `agents/architecture/architecture.md`, `agents/architecture/ui.md`, `agents/architecture/renderer.md`

Use full architecture references only for broad audits/major design work: `agents/architecture/reference/*-full.md`.

## Map

- Main repo: `C:\Users\Drako\Desktop\_quader-qt-base`
- External board app: `C:\Users\Drako\Desktop\quader-project-board`
- Dev builds: `C:\Users\Drako\Desktop\quader-dev-builds`
- Plans: `agents/plans/`; archived plans: `agents/archive/`
- Paused board files: `project_board.md`; archive: `project_board_archive.md`
- Target: `quader_app`; executable: `quader.exe`

## Active Workflow

Use `$quader-workflow` for Quader workflow requests: direct execution, review/rework, audit, beautify, docs, performance, parity/reference work, dev builds, changelogs, and explicit external board/dashboard maintenance.

No active `quader-*.toml` agents should be spawned.

Temporary board pause: do not create, update, validate, archive, or require project-board entries during normal work. Add/intake requests should be handled from the user request and current repository context without mutating `project_board.md`. Ad hoc implementation may proceed directly after reading the relevant docs, hindsight, and code.

Implementation plans are required only when the workflow docs, task risk, or user request call for them. Use `agents/plans/implementation_YYYYMMDD_{slug}.md` for new board-free work unless an existing plan is already the active authority.

Audit/beautify is incomplete until the full markdown artifact exists in `agents/plans/`.

## Board Pause

The project board is temporarily disabled as workflow authority. Treat `project_board.md` and `project_board_archive.md` as historical/read-only context only when the user explicitly cites them or asks for board restoration/inspection.

Never hand-edit `project_board.md` or `project_board_archive.md`. Do not run `tools/project_board.py` commands unless the current user request is explicitly about inspecting, repairing, restoring, or maintaining the board.

## Commands

```powershell
cmake --preset qt-mingw-debug
cmake --build --preset qt-mingw-debug --parallel 20
cmake --build --preset qt-mingw-debug-tests --parallel 20
ctest --preset qt-mingw-debug-runtime
cmake --build --preset qt-mingw-debug-deploy --parallel 20
.\build\qt-mingw-debug\quader.exe
python tools/dev_builds.py current
```

External board app, only when explicitly working on that app:

```powershell
cd C:\Users\Drako\Desktop\quader-project-board
npm run dev
```

## Guardrails

- Keep unrelated user changes intact.
- No destructive git commands unless explicitly requested.
- Before committing, inspect staged files and split unrelated work into coherent commits.
- Do not weaken briefs, plans, validation, parity, style/tooling rules, or acceptance criteria.
- If `C:\Users\Drako\Desktop\quader-windows\quader-app` is cited, briefs/plans must name it as source of truth and preserve the same user-visible substance with only necessary Quader/Crimson/Qt/licensing/test adaptations.
- If code changes documented symbols or documented behavior, update that documentation in place.
- Do not run clang-format, clang-tidy, style/static-analysis targets, or CTest presets that include them without immediate explicit user permission.
- For C++/CMake/shader/runtime work, follow build/dev-build checkpoint policy in `$quader-workflow`.
- Record durable development/workflow changes in `dev-changelog.md`.
