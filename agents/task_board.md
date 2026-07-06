# Quader Database Board

The active project board is the SQLite-backed external app/API, not the historical markdown files.

## Storage

- external board app: `C:\Users\Drako\Desktop\quader-project-board`
- default SQLite database: `C:\Users\Drako\Desktop\quader-project-board\data\quader-board.sqlite`
- override database path: `QUADER_BOARD_DB`
- historical active board import source: `project_board.md`
- historical archive import source: `project_board_archive.md`

The dashboard imports historical markdown files only when the SQLite database is first created. After that, board content belongs to the database.

## Rule

Use the database board for workflows that add, execute, review, rework, complete, archive, or reopen board tasks.

Never hand-edit `project_board.md` or `project_board_archive.md`. Do not run `tools/project_board.py` for active workflow. If the database API lacks a needed action, report the missing action and stop instead of falling back to legacy markdown tooling.

## Public API

Reads:

- `GET /api/board` lists active tasks with filters for `section`, `status`, `priority`, `type`, `area`, `freshness`, `rework`, and `query`/`q`.
- `GET /api/board/task?id=...&state=active|archived|all` shows one task; add `diagnostics=true` only when raw/original text is needed.
- `GET /api/board/search?q=...&state=active|archived|all` searches active and archive rows.
- `GET /api/archive` lists archived rows grouped by date and structured dev version.

Mutations:

- `POST /api/board/action`
- Required authorization flag: `workflowAuthorized`, `architectAuthorized`, or `pmAuthorized`.
- Supported actions: `add-task`, `add-bug`, `edit`, `delete`, `move`, `start`, `review`, `request-changes`, `complete`, `archive`, `reopen`, `append-note`, `bulk-edit-meta`.

## Archive Metadata

Archived database entries must carry explicit dev-version metadata. `archive` and `complete` actions must include `devVersion` and must store:

- `dev_version`
- `archive_date`
- `archived_at`
- `from_section`
- `archive_status`

Dashboard views must group history as date -> dev version -> changes without guessing from changelog text, notes, or historical markdown headings.

## Workflow Links

- Task-only flow creates the board task first to lock the task id, then writes `agents/plans/{task_id}-final-plan_{MM_DD_YYYY_HHMM}.md`, then updates the task metadata with that path.
- Implementation flow reads the linked final plan and implements it as-is after freshness approval.
- Review flow links final review files when changes are requested.
- Completion flow archives through `POST /api/board/action` with `action:"complete"` or `action:"archive"` and `devVersion`.

See `agents/workflow.md` for the full A-F workflow.
