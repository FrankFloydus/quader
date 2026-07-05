# Quader Agent Bootstrap

Keep this file short. Put detailed policy in focused docs or `$quader-workflow`.

Quader is a C++20 Qt Widgets polygon modeling app. The renderer is `crimson`; do not use graphics-runtime names in renderer folders, targets, or planned public class names.

## Read First

- Workflow authority: `agents/workflow.md`
- Board commands/metadata: `agents/task_board.md`
- Tests: `agents/tests-policy.md`
- Documentation: `agents/documentation-policy.md`
- Critical hindsight: `agents/hindsight.md`
- C++ style/static analysis: `agents/code-style.md`
- Architecture briefs: `agents/architecture/architecture.md`, `agents/architecture/ui.md`, `agents/architecture/renderer.md`

Use full architecture references only for broad audits/major design work: `agents/architecture/reference/*-full.md`.

## Map

- Main repo: `C:\Users\Drako\Desktop\_quader-qt-base`
- External board: `C:\Users\Drako\Desktop\quader-project-board`
- Dev builds: `C:\Users\Drako\Desktop\quader-dev-builds`
- Plans: `agents/plans/`; archived plans: `agents/archive/`
- Board: `project_board.md`; archive: `project_board_archive.md`
- Target: `quader_app`; executable: `quader.exe`

## Active Workflow

Use `$quader-workflow` for Quader workflow requests: task/bug intake, execution, review/rework, archive closeout, audit, beautify, docs, performance, parity/reference work, dev builds, changelogs, and external board/dashboard work.

No active `quader-*.toml` agents should be spawned.

Add/intake creates only a board entry. Execute creates/updates exactly one plan: `agents/plans/implementation_task{id}_{slug}.md`. Ad hoc implementation without a task id must add the board entry first, then execute it.

Audit/beautify is incomplete until the full markdown artifact exists in `agents/plans/`.

## Board Authority

`$quader-workflow` owns board authority. Mutating commands use `--workflow-authorized`; `--architect-authorized` and `--pm-authorized` are legacy only.

Never hand-edit `project_board.md` or `project_board_archive.md`.

## Commands

```powershell
cmake --preset qt-mingw-debug
cmake --build --preset qt-mingw-debug --parallel 20
cmake --build --preset qt-mingw-debug-tests --parallel 20
ctest --preset qt-mingw-debug
cmake --build --preset qt-mingw-debug-deploy --parallel 20
.\build\qt-mingw-debug\quader.exe
python tools/project_board.py validate
python tools/dev_builds.py current
```

External board:

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
