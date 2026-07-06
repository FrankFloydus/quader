# Quader Agent Bootstrap

Keep this file short. Detailed policy lives in focused docs and active `quader-*.toml` architects.

Quader is a C++20 Qt Widgets polygon modeling app. The renderer is `crimson`; do not use graphics-runtime names in renderer folders, targets, or planned public class names.

## Read First

- Workflow authority: `agents/workflow.md`
- Database board/status: `agents/task_board.md`
- Tests: `agents/tests-policy.md`
- Documentation: `agents/documentation-policy.md`
- Critical hindsight: `agents/hindsight.md`
- C++ style/static analysis: `agents/code-style.md`
- Architecture briefs: `agents/architecture/architecture.md`, `agents/architecture/ui.md`, `agents/architecture/renderer.md`

Use full architecture references only for broad audits/major design work: `agents/architecture/reference/*-full.md`.

## Map

- Main repo: `C:\Users\Drako\Desktop\_quader-qt-base`
- Active architect profiles: `C:\Users\Drako\.codex\agents\quader-*.toml`
- External board app: `C:\Users\Drako\Desktop\quader-project-board`
- Database board: `C:\Users\Drako\Desktop\quader-project-board\data\quader-board.sqlite` unless `QUADER_BOARD_DB` overrides it
- Dev builds: `C:\Users\Drako\Desktop\quader-dev-builds`
- Plans: `agents/plans/`; reviews: `agents/reviews/`; archived artifacts: `agents/archive/`
- Historical text board files: `project_board.md`; archive: `project_board_archive.md`
- Target: `quader_app`; executable: `quader.exe`

## Active Workflow

Use the restored active `quader-*.toml` architects. Do not route Quader work through `$quader-workflow`; that plugin is disabled.

Root coordinates the work:

- task-only requests -> spawn needed architects, require plan files, create the database board task, write the final plan, link it on the board;
- implementation requests -> fetch the board task/final plan, get software-architect freshness approval, implement the plan as-is;
- completed work -> spawn reviewing architects, require review files, recursively fix changes-requested reviews, then move the task to `In Review`;
- user review failure -> use user feedback as the correction source, run the rework loop, then ask the user to check again;
- complete/archive requests -> archive through the database board API with `devVersion`.

Architect plan/review artifacts are mandatory. Use Windows-safe timestamps: `MM_DD_YYYY_HHMM`.

## Database Board

The active board uses the SQLite-backed external app/API.

Never hand-edit `project_board.md` or `project_board_archive.md`. Do not run `tools/project_board.py`; board-content changes go through `POST /api/board/action`.

Public board API: `GET /api/board`, `GET /api/board/task?id=...`, `GET /api/board/search?q=...&state=active|archived|all`, `GET /api/archive`, and `POST /api/board/action` with typed JSON actions such as `add-task`, `add-bug`, `edit`, `delete`, `move`, `start`, `review`, `request-changes`, `complete`, `archive`, `reopen`, `append-note`, and `bulk-edit-meta`.

Archive/complete actions must store the exact internal dev version in structured metadata (`dev_version`, with archive date/time/status/source section). Do not infer archive grouping from changelog text, notes, or legacy markdown.

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
- For C++/CMake/shader/runtime work, follow build/dev-build checkpoint policy in `agents/workflow.md`.
- Record durable development/workflow changes in `dev-changelog.md`.
