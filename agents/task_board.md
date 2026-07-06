# Quader Task Board Pause

The project board is temporarily disabled as workflow authority.

Historical files:

- active board file: `project_board.md`
- archive file: `project_board_archive.md`
- external board app: `C:\Users\Drako\Desktop\quader-project-board`

## Current Rule

Do not use the board for normal intake, execution, review, closeout, audit, beautify, docs, performance, parity, or dev-build work. Do not create, update, validate, move, archive, remove, reopen, or require board entries.

`project_board.md` and `project_board_archive.md` are read-only historical context during this pause. Never hand-edit either file.

## Allowed Access

Board access is allowed only when the current user request explicitly asks for one of these:

- inspect or summarize an existing board/archive entry;
- restore board-backed workflow;
- repair, migrate, or validate board metadata;
- maintain the external board/dashboard app without mutating board content;
- use an explicitly cited task id as historical context for direct work.

When a user cites a task id, read the cited entry only for context and then proceed from current code, current docs, hindsight, and the user's latest request. Do not run board state-transition commands.

## Replacement Workflow

- Add/intake requests: capture a concise brief from the user request and current repo context; write an `agents/plans/` artifact only when the user asks for one or the work needs durable planning.
- Execution requests: proceed directly after reading relevant docs, hindsight, and code.
- Review/rework: use the latest user/review feedback as the correction source and update the active plan when one exists.
- Audit/beautify: continue to write the full markdown artifact under `agents/plans/`.
- Closeout: report files changed, verification, docs/hindsight updates, dev-build status when relevant, and blockers. Report `Board: paused; no board commands run.`

## Dormant Tooling

`tools/project_board.py` is dormant while this pause is active. Do not run its mutating or validation commands unless the user explicitly asks for board restoration, repair, migration, validation, or inspection.
