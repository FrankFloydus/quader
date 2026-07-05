# Quader Task Board v2

The active board is `project_board.md`. The archive is `project_board_archive.md`.

## Authority

Board mutations are software-architect authorized. Root may run mutating `tools/project_board.py` commands only when applying exact commands returned by `quader-software-architect`.

Use `--architect-authorized` for new mutating commands. `--pm-authorized` is a legacy alias kept only so old commands do not break.

Other agents return reports to root; they do not edit the board directly. `quader-performance-dev` may use `--architect-authorized` for its own explicit performance audit/fix tasks only.

Validate after board changes:

```powershell
python tools/project_board.py validate
```

## Board Sections

- `Backlog`: accepted work not started.
- `In Progress`: actively executing or integrating work.
- `In Review`: implementation finished and awaiting architect decision.
- `Bugs`: accepted bugs not yet fixed or verified.

Completed tasks and fixed bugs are moved to `project_board_archive.md` with `complete`; they are not deleted.

## Entry Shape

Required task lines:

```text
#25 [priority:high][type:enhancement][area:crimson] Title - Summary.
  Brief: What must be done.
  Freshness: [status:needs-refresh][checked:never] New task; requires start-gate revalidation.
  Created: 2026-07-05T12:00:00+02:00
```

Common optional lines:

```text
  Final owner: src/crimson module and tests.
  Plans: agents/plans/implementation_feature_renderer_doc.md
  Source refs: C:\path\reference | src/foo.cpp:12-88; include/foo.h:4-31
  Authority: [owner:renderer][status:pending][agent:pending] Renderer architect owns final review.
  Active: [lead:root][status:running][workers:4] Parallel implementation slices are active.
  Coordination: Short coordination note.
```

Legacy `PM:` and `Assignment:` metadata remains valid for old entries, but v2 entries should avoid PM/assignment clutter unless a historical task already has it.

`Active:` is optional and should be used only when live coordination needs to be visible:

```text
Active: [lead:<root-or-thread-id>][status:running|integrating|reviewing|blocked][workers:0-6] Note.
```

## Commands

Add task:

```powershell
python tools/project_board.py add --architect-authorized --title "Title" --summary "Short summary." --brief "Detailed brief." --priority high --type enhancement --area core --final-owner "src/document and tests" --plans agents/plans/implementation_feature_doc.md
```

Add bug:

```powershell
python tools/project_board.py add-bug --architect-authorized --title "Bug title" --summary "Observed failure." --brief "Repro, expected behavior, likely area." --priority high --area ui --final-owner "src/ui and tests"
```

Edit the canonical brief line:

```powershell
python tools/project_board.py edit-brief --architect-authorized --id 25 --brief "Updated brief."
```

Set freshness:

```powershell
python tools/project_board.py set-freshness --architect-authorized --id 25 --status fresh --checked now --note "Revalidated against current files and active board metadata."
```

Start:

```powershell
python tools/project_board.py start --architect-authorized --id 25
```

Move:

```powershell
python tools/project_board.py move --architect-authorized --id 25 --to "In Progress"
```

Move to review:

```powershell
python tools/project_board.py review --architect-authorized --id 25
```

Set review authority:

```powershell
python tools/project_board.py set-authority --architect-authorized --id 25 --owner renderer --status pending --agent pending --note "Renderer architect owns final review."
```

Record or clear active coordination:

```powershell
python tools/project_board.py set-active --architect-authorized --id 25 --lead root --status running --workers 4 --note "Executing independent slices."
python tools/project_board.py clear-active --architect-authorized --id 25
```

Archive approved task:

```powershell
python tools/project_board.py complete --architect-authorized --id 25
```

Archive fixed bug:

```powershell
python tools/project_board.py complete --architect-authorized --id 19 --resolution "Fixed the runtime target binding and restored expected viewport behavior."
```

Remove an active board entry only when the software architect issued the exact command:

```powershell
python tools/project_board.py remove --architect-authorized --id 25
```

Search archive:

```powershell
python tools/project_board.py archive-search --query "diagnostics" --full
```

Reopen archive:

```powershell
python tools/project_board.py reopen-archive --architect-authorized --id 25 --reason "Follow-up requested from archived task context."
```

List active entries:

```powershell
python tools/project_board.py list
```

## Freshness Gate

Every new task starts as:

```text
Freshness: [status:needs-refresh][checked:never]
```

Missing freshness metadata also counts as `needs-refresh`.

Before work starts, root revalidates the task against current workspace state, linked plans, source refs, active board metadata, and likely touched files. If the task is domain-specific or uncertain, ask the owning architect for `TASK_REVALIDATION`.

Outcomes:

- `fresh`: architect issues `set-freshness --architect-authorized --status fresh`; root may start.
- `stale`: architect issues `set-freshness --architect-authorized --status stale`; task returns to software architect for revised brief or replacement.
- `superseded`: architect issues `set-freshness --architect-authorized --status superseded`; do not implement unless replaced.

`start` and `move --to "In Progress"` fail unless freshness is `fresh`.

## Intake To Board

Fresh requests do not become direct edits. The v2 flow is:

1. Relevant architect scout(s) produce `Task Intake Domain Findings`.
2. `quader-software-architect` produces one `Final Task Intake Report`.
3. The software architect returns exact add/add-bug commands or duplicate/no-op.
4. Root applies exact commands.

Reference/parity tasks must include `Source refs:` with exact source folder path and file-to-line mapping.

## Execution Coordination

Root coordinates execution. Up to 6 workers may run in parallel when tasks or slices are independent.

Workers:

- receive clean scoped prompts with `fork_context: false`,
- set `/goal` before edits,
- implement only their assigned slice,
- run their assigned checks when feasible,
- report exact files changed and verification results,
- do not mutate the board.

Root:

- records `Active:` if helpful,
- integrates worker output,
- runs required build/tests,
- deploys for user-visible runtime changes,
- asks the owning architect for review,
- applies exact architect-issued board commands.

## Review And Rejection Handling

In Review cards are not perform-task prompts. They are closeout prompts.

For a single review card or a batch, the owning architect returns one decision per task:

- `approved`: archive with `complete --architect-authorized`.
- `changes-requested`: move back to `In Progress`, update `Authority:` or `Coordination:` if useful, then execute only the requested corrections.
- `stale`: mark freshness `stale` and return to software-architect finalization.
- `superseded`: mark freshness `superseded`; do not implement.

For a mixed batch, root archives only approved tasks and moves or marks only the rejected tasks. Do not rerun implementation for approved tasks, and do not archive rejected tasks.

Fixed bugs require a `Bug resolution summary:` from the executor before archival. The archive command must include `--resolution`.

## Archive

`complete` moves the entry to `project_board_archive.md`, grouped by archive date and current internal dev version. The archive keeps the original title/tags, brief, metadata, and resolution for bugs.

Archive search is read-only. Reopening an archived entry is software-architect authorized and returns the entry to the active board with `Freshness: needs-refresh`.

## Dashboard Behavior

The external Next.js board may read and display the board directly. Dashboard delete/remove/archive actions must still use software-architect authorized commands. Copied prompts must route:

- Backlog/In Progress work to root coordination with freshness gate.
- In Review cards to architect review and archive/reject split.
- Archived cards to historical reference or software-architect authorized reopen.

Use the `$quader-board-developer` skill for non-trivial changes to the external Next.js board app at `C:\Users\Drako\Desktop\quader-project-board`, including board/archive/changelog views, copied prompts, API routes, dev-build controls, and styling. That skill workflow may edit the external board app directly, but it must not mutate board markdown except by applying exact software-architect commands.

## Future Useful Commands

Useful but not required now:

- `edit-summary`
- `edit-title`
- `edit-final-owner`
- `edit-plans`
- `edit-source-refs`
- `set-priority`
- `set-area`
- `set-type`
- `append-note`
- `append-coordination`
- `block` / `unblock`
- `claim` / `release`
- `duplicate`
- `split-task`
