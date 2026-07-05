# Quader Task Board

Active board: `project_board.md`. Archive: `project_board_archive.md`. `$quader-workflow` is the board authority; do not hand-edit either file.

## Authority

Use `--workflow-authorized` for new mutating commands. `--architect-authorized` and `--pm-authorized` are legacy aliases only. Validate after board changes:

```powershell
python tools/project_board.py validate
```

## Sections

- `Backlog`: accepted work not started.
- `In Progress`: executing/integrating work.
- `In Review`: implementation finished, awaiting closeout.
- `Bugs`: accepted unfixed bugs.

Completed/fixed items move to archive with `complete`; they are not deleted unless explicitly removed.

## Entry Metadata

Required:

```text
#25 [priority:high][type:enhancement][area:crimson] Title - Summary.
  Brief: What must be done.
  Freshness: [status:needs-refresh][checked:never] New task; requires start-gate revalidation.
  Created: 2026-07-05T12:00:00+02:00
```

Optional/common:

```text
  Final owner: src/crimson module and tests.
  Plans: agents/plans/implementation_task25_slug.md
  Source refs: C:\path\reference | src/foo.cpp:12-88; include/foo.h:4-31
  Authority: [owner:workflow][status:pending][agent:quader-workflow] ...
  Rework: [owner:workflow][agent:quader-workflow][checked:...] Current requested corrections.
  Active: [lead:quader-workflow][status:running][workers:0] ...
  Coordination: Context note only.
```

Legacy `PM:`/`Assignment:` lines remain valid for old entries but should not be added to new v2 entries.

## Commands

```powershell
python tools/project_board.py list
python tools/project_board.py add --workflow-authorized --title "Title" --summary "Summary." --brief "Brief." --priority high --type enhancement --area core --final-owner "src/document and tests"
python tools/project_board.py add-bug --workflow-authorized --title "Bug" --summary "Observed failure." --brief "Repro, expected, likely fix." --priority high --area ui
python tools/project_board.py edit-brief --workflow-authorized --id 25 --brief "Updated brief."
python tools/project_board.py set-freshness --workflow-authorized --id 25 --status fresh --checked now --note "Revalidated."
python tools/project_board.py start --workflow-authorized --id 25
python tools/project_board.py move --workflow-authorized --id 25 --to "In Progress"
python tools/project_board.py request-changes --workflow-authorized --id 25 --owner workflow --agent quader-workflow --summary "Why rejected." --rework "Actionable corrections."
python tools/project_board.py review --workflow-authorized --id 25
python tools/project_board.py set-authority --workflow-authorized --id 25 --owner workflow --status pending --agent quader-workflow --note "Workflow owns final review."
python tools/project_board.py set-active --workflow-authorized --id 25 --lead quader-workflow --status running --workers 0 --note "Executing."
python tools/project_board.py clear-active --workflow-authorized --id 25
python tools/project_board.py complete --workflow-authorized --id 25
python tools/project_board.py complete --workflow-authorized --id 19 --resolution "Fixed by ..."
python tools/project_board.py remove --workflow-authorized --id 25
python tools/project_board.py archive-search --query "diagnostics" --full
python tools/project_board.py reopen-archive --workflow-authorized --id 25 --reason "Follow-up requested."
```

Rejected review closeout must use `request-changes`; `Coordination:` is not authoritative rework.

## Freshness Gate

New/missing freshness means `needs-refresh`. Before start, revalidate against current workspace, source refs, active board metadata, linked plans, and likely touched files.

- `fresh`: mark fresh, then start.
- `stale`: mark stale; do not implement until brief is revised.
- `superseded`: mark superseded; do not implement unless replaced.

`start` and `move --to "In Progress"` fail unless freshness is `fresh`.

## Intake

Fresh requests create board entries, not direct implementation. Exception: if the user explicitly asks to implement/fix/do now without a task id, add the entry first, capture the id, then execute it.

Briefs must include behavior, likely files/modules, initial direction, risks, source refs for parity/reference tasks, and likely verification. Quader Windows parity briefs must name `C:\Users\Drako\Desktop\quader-windows\quader-app` as source of truth and list inspected reference files/folders.

## Execution And Review

Execution creates/updates exactly one plan: `agents/plans/implementation_task{id}_{slug}.md`.

In Review cards are closeout prompts, not perform-task prompts. Decisions: approve/archive, request changes with `Rework:`, mark stale, or mark superseded. Returned In Progress prompts must prioritize `Rework:` over the original brief.

Fixed bugs require `Bug resolution summary:` and archive `--resolution`.

## Archive

`complete` moves entries to `project_board_archive.md`, grouped by archive date and internal dev version. Reopening resets active freshness to `needs-refresh`.

Dashboard actions may read/display board files but must call `tools/project_board.py` for mutations. Copied prompts route through `$quader-workflow`.
