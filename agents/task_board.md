# Task Board

`project_board.md` is the source of truth for active tasks.
`project_board_archive.md` is the chronological archive for completed tasks and
fixed bugs, grouped by archive date and internal dev version. `changelog.md` is
release history for public-facing Quader application changes only.
`dev-changelog.md` is the detailed engineering log for durable repository
changes. A task leaves the board when it is no longer active work and the user
explicitly authorizes that board operation.

Use `tools/project_board.py` for mechanical board reads and writes. Manual edits
must preserve the format validated by:

```powershell
python tools/project_board.py validate
```

Board authority is PM-gated. `quader-project-manager` is the sole authority for
board content, task creation, assignment metadata, duplicate/no-op decisions,
and allowed status-transition decisions. The main/root chat may physically run
`tools/project_board.py`, but only to apply exact commands returned by the PM.
Every mutating `tools/project_board.py` command requires `--pm-authorized`.
The main/root chat must not add that flag unless `quader-project-manager`
returned the exact command containing it.
Ordinary chats, architects, workers, scouts, and fallback agents must never add,
edit, or move board entries directly.

Performance exception: `quader-performance-dev` is the only non-PM role that may
supersede the PM, and only for explicit C++ performance audit/fix workflows. In
that workflow it may create needed performance tasks or bugs itself, set
freshness, move its own performance tasks to `In Progress`, implement them, and
move verified work to `In Review`. Its mutating commands still use
`--pm-authorized`, but the authorization source is the performance-dev exception,
not a PM-issued command. It must not complete, archive, remove, reopen, push,
bump versions, or update public release notes.

New task requests are intake-only by default. For a new change, fix, rename,
refactor, implementation, task, or bug that is not already represented by an
active board entry, do not edit files or perform the work on the spot. If it
belongs to any architect domain, delegate intake to the relevant architect(s). If
it belongs to no architect domain, the main/root chat researches it. In both
cases, send one PM-ready `Final Task Intake Report` to the PM before board
changes. Imperative wording such as "rename X" is not consent to bypass intake.

Directly requested maintenance of the external Next.js control-board frontend at
`C:\Users\Drako\Desktop\quader-project-board` is support tooling, not Quader
product UI task intake. Implement dashboard, board-view, changelog-view,
frontend API, and control-board styling requests directly unless the user
explicitly asks to create a board task.

If a board task includes or implies test planning, test writing, test
modification, test deletion, test review, fixtures, golden images,
fuzz/property tests, sanitizer checks, validation checks, or manual verification
substitutes, assign agents with `agents/tests-policy.md` as a required read.

When an actionable master plan or master audit exists, the main/root agent must
send its ordered implementation batches to the PM before implementation starts.
The PM checks for duplicates and returns exact Backlog task commands. Add one
board task per coherent implementation batch only through those PM-issued
commands, unless the master plan explicitly says a batch is inseparable. Each
generated task must reference the active master plan with a `Plans:` line.

Architecture-sensitive board work must also carry a `Plans:` line that
references the active master audit or a follow-up implementation plan before
implementation starts. For the current Batch 0 freeze, preserve prototype
behavior and do not add real document mutation, topology editing, selection
truth, import/export, material editing, picking-driven selection, PBR, render
graph behavior, or debug panels directly to `MainWindow` or `BgfxWidget`.

## Board Sections

The only active board sections are:

- `Backlog`
- `In Progress`
- `In Review`
- `Bugs`

There is no `Completed` section on the active board. Completed tasks and fixed
bugs move to `project_board_archive.md` only after explicit user authorization.
Completion/removal still needs a PM-issued exact `--pm-authorized` command; the
PM does not initiate those operations by itself. Hard removal is reserved for
cleanup of entries that should not remain in history.
Version and user-facing `changelog.md` updates happen during the push workflow
in `agents/workflow.md`. Durable task-board policy or tooling changes should be
recorded in `dev-changelog.md`.

## Status Movement Lock

Agent-initiated board status changes are limited to exactly:

- `Backlog` -> `In Progress`
- `In Progress` -> `In Review`

Do not complete, remove, cancel, archive, or otherwise move task or bug entries
unless the user explicitly asks for that board operation and the PM returns the
exact `--pm-authorized` command. Finishing an implementation, passing tests, or
producing a build only means the task is ready for user review; it does not
authorize marking the task complete or deleting it from the board.

Bug entries in `Bugs` remain active bug records unless the user explicitly
authorizes moving or removing them. If a bug fix is implemented from an existing
`Bugs` entry without explicit status-change authorization, update the brief with
implementation and verification notes but leave the bug entry active for user
validation. Any executor that fixes a bug must include `Bug resolution summary:`
in its final report. When the user authorizes marking the bug fixed, the PM must
include that summary in the `complete --resolution` command.

## Entry Format

Each task starts with one canonical first line:

```text
#25 [priority:critical][type:enhancement][area:modeling] Implement bevel operation - Short summary.
```

Indented lines under the entry hold the scouting brief:

```text
  Created: 2026-07-05T12:30:00+02:00
  Brief: Current findings, likely files, implementation path, edge cases, and verification.
  Freshness: [status:needs-refresh][checked:never] Awaiting start-gate revalidation.
  Source refs: C:\absolute\reference\root | src/foo.cpp:12-88; include/foo.h:4-31
  Final owner: Final files, classes, functions, and tests where the work lands.
  Plans: Active architecture plan paths, if any.
  PM: [run:20260704_task_25][status:coordinating][manager:pending] Splitting work.
  Authority: [owner:software][status:pending][agent:pending] Final approval owner.
  Assignment: [key:A1][role:worker][agent:pending][status:planned][authority:none] Implement scoped files.
  Coordination: Latest project-manager integration note.
  Images: C:\absolute\path\to\task_image.png
```

The first line remains compact for scanning. The indented brief must be detailed
enough that the next agent starts from saved findings instead of repeating the
full initial scan. `Source refs:` is optional for ordinary work and mandatory
when the request asks to check a code reference, match the same behavior, or
achieve `1:1`, `100%`, or parity with reference code.

`Created:` is added automatically for new tasks and bugs. The Next.js board uses
it for most-recent-first card ordering. Older entries without `Created:` are
treated as legacy entries and ordered by descending task id.

`Freshness:` records whether the saved task assumptions have been checked
against the current workspace before execution. New tasks and bugs default to
`needs-refresh`; existing entries without a `Freshness:` line are treated as
`needs-refresh`.

## Tags

Allowed priorities:

- `priority:critical`
- `priority:high`
- `priority:medium`
- `priority:low`

Allowed types:

- `type:enhancement`
- `type:documentation`
- `type:bug`
- `type:epic`

`type:enhancement` is the only valid spelling.

Area tags are lowercase reusable domains. Reuse existing area tags before adding
a new one. Preferred starting set:

- `area:architecture`
- `area:build`
- `area:commands`
- `area:crimson`
- `area:docs`
- `area:io`
- `area:mesh`
- `area:modeling`
- `area:performance`
- `area:tools`
- `area:ui`
- `area:viewport`

## Multi-Agent Board Workflow

Use the project-manager workflow for broad task-board execution, work that needs
multiple spawned agents, cross-domain implementation, or parallel scouting. The
project-manager agent coordinates work; it does not implement code and does not
finalize domain work.

This multi-agent workflow starts after a board task exists or after the PM has
identified an existing duplicate. It must not be used to duplicate Task Intake
Workflow scouting with an architect plus another agent for the same scope.

Board write ownership:

- The project-manager agent owns board decisions and returns exact board update
  requests for the main/root agent to apply.
- The main/root agent physically edits `project_board.md` through
  `tools/project_board.py` only when applying exact PM-issued commands.
- Worker, scout, and architect agents must not edit the board directly.
- Spawned agents report board changes as `Board update request:` blocks.
- The authoritative architect approves final work in its domain. The PM may only
  say the work is ready for authoritative review.
- The user is the only authority for completing, removing, canceling, or
  archiving task-board entries.

Structured multi-agent lines:

```text
  PM: [run:<run_id>][status:coordinating|waiting-on-agents|waiting-on-authority|ready-for-review][manager:<agent_id-or-pending>] Summary.
  Authority: [owner:software|ui|renderer|multi][status:pending|approved|changes-requested][agent:<agent_id-or-pending>] Notes.
  Assignment: [key:A1][role:<agent_type>][agent:<agent_id-or-pending>][status:planned|running|reported|blocked|integrated][authority:none|software|ui|renderer] Brief.
  Coordination: Latest PM integration note.
  Freshness: [status:needs-refresh|fresh|stale|superseded][checked:<iso-or-never>] Notes.
```

Use one board task ID for the user-visible work. Do not create child board tasks
for each spawned agent unless the user explicitly asks to split the work into
separate active tasks.

`quader-performance-dev` is not a PM-coordinated worker for explicit performance
audit/fix workflows. It may add and execute performance tasks directly under the
performance exception, but only within that narrow performance scope.

Subagent wait rule:

- When main/root spawns agents for a board workflow, all spawned agents must be
  awaited until they return terminal reports.
- Do not finish the main/root turn by saying subagents are still running or that
  the user should wait for them.
- If the user explicitly cancels or redirects the work, record which spawned
  agents were still open and whether their results are discarded, superseded, or
  should still be reconciled later.

UI architect Qt resource rule:

- For any board task directly involving Qt widgets or Qt Widgets APIs, the UI
  architect must do a quick external resource scan before returning intake
  findings, an implementation plan, or a review.
- Official Qt documentation is primary. Stack Overflow and specialized Qt/C++
  UI articles may supplement it when they are relevant and current enough.
- The UI architect must include `Qt resources checked:` with valid URLs and the
  findings that affected the recommendation.
- If network access fails, say so explicitly and do not invent sources.

Renderer architect graphics resource rule:

- For any board task involving Crimson or graphics-programming behavior, the
  renderer architect must do a quick external graphics-resource scan before
  returning intake findings, a pending-task revalidation decision, an
  implementation plan, a deviation/bug review, or final implementation review.
- Use https://graphicscompendium.com/index.html and
  https://learnopengl.com/Introduction as authoritative starting references when
  relevant to the task.
- The renderer architect must browse/search for relevant article or HTML pages
  for the task and read the selected page contents fully, not just snippets.
- The renderer architect must include `Renderer resources checked:` with valid
  URLs and the findings that affected the recommendation.
- If network access fails, say so explicitly and do not invent sources.

## Archive Format

`project_board_archive.md` stores archived entries under date and internal dev
version headings:

```text
## 2026-07-05

### 0.1.0-dev.2

#19 [priority:critical][type:bug][area:crimson] Bug title - Bug summary.
  Archived: [at:2026-07-05T10:30:00+02:00][dev-version:0.1.0-dev.2][from:Bugs][status:fixed] PM-authorized archive.
  Resolution: Concise explanation of how the bug was fixed.
  Brief: Original bug description and context.
```

The archive keeps the original task or bug first line and metadata so the
dashboard can show a compact card and a drawer with the initial description,
resolution, and raw record. Archived bug entries must have `Resolution:`.

## Master Plans And Audits

Master plans and master audits are planning authority, not a substitute for the
active task board. When a master plan becomes actionable:

1. Read the full master plan and its ordered implementation batches.
2. Send the batches to `quader-project-manager`.
3. The PM searches `project_board.md` for equivalent active tasks and returns duplicate/no-op decisions or exact `tools/project_board.py add --pm-authorized` commands.
4. The main/root agent applies only those exact commands. The PM should use the master plan path in `--plans`, for example:

```powershell
python tools/project_board.py add --pm-authorized --title "Extract Crimson module skeleton" --summary "Create the initial renderer module boundary from the master audit." --brief "Source: agents/plans/audit_20260704_full_architecture_master.md batch 1. Implement only the scoped module skeleton, preserve current prototype behavior, and run the documented build command." --priority high --type enhancement --area crimson --final-owner "src/crimson module files and matching CMake entries" --plans agents/plans/audit_20260704_full_architecture_master.md
```

5. Keep the master plan in `agents/plans/` while any linked board task is active.
6. Do not archive the master plan until linked tasks are approved, superseded, or
   explicitly moved to a newer plan.
7. Implementation should proceed through the generated board tasks, not directly
   from the master plan.

If a board entry asks for architecture-sensitive feature work but has no active
`Plans:` reference, stop before implementation and request board or planning
cleanup. A board brief alone is not authority to bypass the active master audit,
the relevant follow-up plan, or the prototype-growth freeze.

Project-manager command examples:

```powershell
python tools/project_board.py set-freshness --pm-authorized --id 25 --status fresh --checked now --note "Revalidated against current workspace; assumptions still hold."
python tools/project_board.py pm-start --pm-authorized --id 25 --run 20260704_task_25 --manager pending --status coordinating --note "Splitting assignments."
python tools/project_board.py set-authority --pm-authorized --id 25 --owner software --status pending --agent pending --note "Software architect owns final approval."
python tools/project_board.py assign --pm-authorized --id 25 --key A1 --role worker --agent pending --status planned --authority none --brief "Implement scoped files."
python tools/project_board.py assignment-status --pm-authorized --id 25 --key A1 --status reported --agent 019f... --note "Worker report received."
python tools/project_board.py coordination-note --pm-authorized --id 25 --note "Ready for software architect review."
```

## Commands

### add task

When the user makes a new task request, use the deterministic Task Intake
Workflow. Do not search and add directly from the main/root chat, do not perform
the work on the spot, and do not create a preflight or PM-prerequisite task.

Task intake sequence:

1. Decide whether the request belongs to any architect domain:
   - software architect for build, module layout, target naming, source-folder structure, workflow, board/process, app/core, cross-domain work, or unclear ownership.
   - UI architect for Qt Widgets, actions, panels, models, viewport host UI, settings, dialogs, and UI services.
   - renderer architect for Crimson, render graph, shaders, materials, overlays, picking, render tests, and renderer-facing assets.
2. If yes, spawn the relevant architect(s) in `TASK_INTAKE` mode, with at most one architect per domain and no duplicate generic/default/scout/worker agent for the same scope.
3. If no, research the code/docs from the main/root chat.
4. Produce one PM-ready `Final Task Intake Report`. For multi-domain architect intake, gather domain findings first and have one finalizing architect reconcile them.
5. Send the `Final Task Intake Report` to `quader-project-manager`.
6. The PM checks for duplicates and returns either an exact `tools/project_board.py add --pm-authorized` command or a duplicate/no-op decision.
7. The main/root chat applies only the PM's exact command and reports the new task id or duplicate decision.

The `Final Task Intake Report` and PM-generated brief should record:

- current findings
- relevant architecture docs or active plan paths
- likely files, classes, contracts, or docs
- final owner files, classes, functions, and tests
- intended implementation path
- edge cases or risks
- exact verification commands when they apply
- active master plan path for `--plans`, when the task comes from a master plan or master audit
- `Source refs:` with source folder path and file-to-line mapping when the request mentions code reference, same, `1:1`, `100%`, or parity
- image references

For reference/parity tasks, treat `same`, `1:1`, `100%`, and `parity` as a
request to match behavior and structure as closely as practical. Do not copy
third-party source verbatim unless it is project-owned or explicitly safe to
reuse. The board task must point pickup agents at the exact source folder and
file lines to inspect.

Use `Bugs` instead of `Backlog` only for `add bug`.

New `add` and `add-bug` commands automatically add
`Freshness: [status:needs-refresh][checked:never]`. Do not mark a freshly added
task `fresh` during intake; freshness is a start-time check against the
workspace state that exists when implementation actually begins.

Preflight tasks are valid only when explicitly requested by the user, an active
master plan, or an architect plan. Do not create "preflight", "PM prerequisite",
or "setup" tasks merely so the PM can assign work.

### add bug

Handle `add bug` with the same Task Intake Workflow, but the PM should return an
`add-bug --pm-authorized` command, place the entry in `Bugs`, and use
`type:bug`.

### perform task

Resolve the requested ids, titles, or tags. Process matching tasks sequentially.
For each task:

1. Read the full entry and referenced images.
2. Run a start-gate freshness preflight against the current workspace before moving it to `In Progress`.
3. Move it from `Backlog` to `In Progress` only when that is the current section and `Freshness:` is `fresh`.
4. Create a Codex `/goal` for the task before editing or running implementation commands.
5. Do not move tasks from any other section without explicit user authorization.
6. Read the routed architecture docs and active plans required by the task.
7. Confirm architecture-sensitive work references the active master audit or a follow-up implementation plan before implementation.
8. Implement and verify the task without silent workarounds or plan deviations.
9. If a workaround, plan gap, or deviation appears necessary, stop and send a `Workaround/Deviation Report` to the main/root chat for PM/architect routing.
10. If the user reports a bug or asks for a quick fix directly inside the executor chat, stop affected work and send a `Mid-work Bug Report` to the main/root chat before changing code.
11. Run the relevant build/tests required by the Build Checkpoint Policy.
12. Move it from `In Progress` to `In Review` only by applying the PM/authority board update after agent-side verification.
13. Complete the Codex goal only after the executor's full scope is implemented, verified, documented where required, and handed back through PM/authority review or moved to `In Review` when allowed.
14. Do not mark it completed or remove it from the board. User validation plus an exact PM-issued `--pm-authorized` command is required for completion/removal. If the task is a bug, include `Bug resolution summary:` in the final report.

### review and close task

`In Review` means implementation and authority review are complete enough for
user validation. It is not a request to perform implementation again.

Use this flow when the user selects an `In Review` card, asks whether a review
task can be closed, or asks to mark a reviewed task completed:

1. Read the full task entry, Authority/Coordination notes, active `Plans:`
   paths, verification notes, and any bug resolution summary.
2. Confirm the user explicitly accepts the task as completed or the bug as
   fixed. Passing tests, building, or being in `In Review` is not enough by
   itself.
3. Ask the PM for the exact completion/archive command.
4. For normal tasks, the PM command is
   `python tools/project_board.py complete --pm-authorized --id <id>`.
5. For fixed bugs, the PM command must include
   `--resolution "<summary>"`, sourced from `Bug resolution summary:` or an
   equivalent PM/architect-approved summary.
6. Apply only the PM-issued command and run
   `python tools/project_board.py validate`.
7. If the user rejects the review build or reports a new issue, do not implement
   an ad hoc fix from the closeout flow. Route the rejection through PM and the
   authoritative architect as a follow-up task, bug, or continuation decision.

Mixed selections are allowed, but `In Review` entries stay closeout-only while
Backlog/In Progress/Bugs entries follow their own task flow.

Freshness preflight means:

- Inspect the task brief, `Plans:`, `Source refs:`, final owner, active
  `PM:`, `Authority:`, `Assignment:`, and `Coordination:` metadata.
- Inspect the current files/modules likely touched by the task.
- If the task still matches the current workspace, ask the PM for and apply the
  exact command:
  `python tools/project_board.py set-freshness --pm-authorized --id <id> --status fresh --checked now --note "<reason>"`.
- If assumptions changed but the task is still useful, set `stale` and route a
  revalidation report through the PM and relevant/finalizing architect before
  implementation.
- If the task is obsolete or duplicated, set `superseded` and do not implement
  unless the user or PM replaces the brief.
- For architect-domain tasks, the relevant/finalizing architect owns the
  fresh/stale/superseded decision. For simple non-domain tasks, the PM or
  main/root scout may revalidate.

Use architect subagents only when `AGENTS.md` says the architect workflow is
required or the user explicitly asks for it.

For explicit C++ performance audit/fix work, use `quader-performance-dev`
instead of this normal PM-perform flow. That agent owns its own performance
board entries and movement to `In Review` under the performance exception.

### performance audit and fix

Use `quader-performance-dev` when the user asks to audit and fix performance,
optimize hot paths, reduce allocation pressure, improve cache locality,
vectorization, branch behavior, or concurrency overhead.

The performance developer must:

1. Use the custom `cpp-performance` skill, or read
   `C:\Users\Drako\.codex\skills\cpp-performance\SKILL.md` directly if the skill
   is not exposed in the active skill list.
2. Read `AGENTS.md`, `agents/workflow.md`, this file, `agents/tests-policy.md`,
   and `C:\Users\Drako\.codex\agents\quader-project-manager.toml` before board
   mutation or implementation.
3. Inspect current board entries and active plans for overlapping work.
4. Establish the performance goal and hot-path evidence from profiling data,
   benchmarks, existing diagnostics, or a clearly labeled hypothesis.
5. Add one task per coherent performance fix or measurement slice using
   `tools/project_board.py add --pm-authorized` or `add-bug --pm-authorized`.
   Use `area:performance` unless a more specific area is clearer.
6. Include baseline evidence or measurement plan, optimization hypothesis, final
   owner files/modules, risks, and verification commands in `Brief:`.
7. Mark only its own performance tasks fresh after current-workspace
   revalidation, move them to `In Progress`, set a Codex `/goal`, implement, and
   verify.
8. Measure before and after under comparable conditions. Do not claim a
   performance improvement without measurement.
9. Move verified performance work to `In Review` with
   `tools/project_board.py review --pm-authorized`.

The performance developer must pause and request the owning architect review
when an optimization requires public API redesign, dependency-boundary changes,
UI behavior changes, Crimson render semantics changes, or weaker verification.
It must not complete, archive, remove, reopen, push, bump versions, manually edit
`DEV_VERSION`, or update public `changelog.md` entries.

When the user explicitly consents to immediate implementation and skipping task
intake, run a conflict preflight before editing. Check active board entries,
PM/Authority/Assignment/Coordination metadata, and available Codex thread
coordination tools. If overlapping active work is visible, stop and ask whether
to coordinate, message the other thread, or proceed anyway. If no overlap is
visible, record that the check is best-effort because thread tools are not a
perfect live lock.

When the user says to perform a task "with PM", "with the project manager", or
as multi-agent work, first use the project-manager workflow above. The PM returns
assignment packets; the main/root agent spawns the requested agents and applies
board metadata updates. PM batches must revalidate selected tasks first and
spawn workers only for tasks marked `fresh`; stale or superseded tasks are
reported back instead of queued as implementation work. PM implementation
assignment packets must tell each executor to create a Codex `/goal` before
editing and complete it only after the assigned scope is fully implemented,
verified, and reported back. Assignment packets must also tell executors to stop
and report any workaround/deviation or direct mid-work bug/quick-fix request
back to the main/root chat before continuing.

### workaround or deviation during execution

Executors must not hide deviations from the accepted plan, board brief, active
architecture plan, required verification, or assigned scope. If following the
plan appears impossible or a workaround seems necessary, the executor stops and
returns:

```text
Workaround/Deviation Report:
- Task/assignment:
- Accepted plan or brief being deviated from:
- Files touched or proposed:
- Why the accepted path cannot be followed:
- Proposed workaround or correction:
- Risk:
- Temporary code already written:
```

The main/root chat sends the report to `quader-project-manager`. The PM routes
it to the correct architect and returns exact continuation instructions, board
metadata updates, or a new task/bug command. The executor resumes only after
those instructions are sent back.

### mid-work bug reported inside an executor chat

When the user reports a bug or asks for a quick fix directly in a running
executor/subagent chat, the executor must not independently fix it. The executor
stops affected work and returns:

```text
Mid-work Bug Report:
- Task/assignment:
- User-visible symptom:
- Repro steps:
- Build/dev version or executable:
- Suspected area/files:
- Overlap with current assignment:
- Risk if current work continues:
```

The main/root chat sends the report to the PM. The PM requests the correct
architect review when needed and decides whether to fold the fix into the active
task, create a new bug entry, block the assignment, or defer it. The executor
continues only after main/root sends the PM/architect-approved path back.

## Build Checkpoint Policy

For C++/CMake/shader/runtime changes, keep the runnable executable fresh:

- Run `cmake --build --preset qt-mingw-debug` after each coherent implementation slice and before starting another independent code task.
- In PM-coordinated batches, do not complete more than two implemented code tasks without a successful build.
- Before architect review or moving a task to `In Review`, run the relevant build/tests.
- If user-visible runtime behavior changed, also run `cmake --build --preset qt-mingw-debug --target deploy` so the control board can launch the current executable.
- Dev-build archives are heuristic stable checkpoints, not manual-only. After a successful build/deploy for a task or PM batch that changes runnable behavior, executable/runtime packaging, shaders/assets, CMake/runtime dependencies, or fixes a bug observed in the executable, create a preserved snapshot with `python tools/dev_builds.py archive --preset qt-mingw-debug`.
- In PM-coordinated batches, archive once per coherent stable checkpoint, normally after the ready tasks in that checkpoint have passed verification and authority review or when the user should test the runnable exe. Do not archive after every tiny code slice.
- Skip automatic dev-build archiving for docs-only, task-board-only, external control-board frontend-only, tests-only, planning-only, or pure internal refactor work with no runnable behavior/runtime risk unless the user asks for a snapshot.
- If known concurrent work would make the archive include unrelated unverified changes, do not auto-archive. Report `Dev build checkpoint deferred:` with the reason and the command to run once the workspace is stable.
- `DEV_VERSION` still increments only after `tools/dev_builds.py archive` confirms the runnable archive. Never edit `DEV_VERSION` manually.
- If the build fails, stop further code-task batching in that area and route the failure through the PM as a blocker or fix assignment.

### fix bug

Handle `fix bug` like `perform task`, but default filtering to the `Bugs`
section and `type:bug` entries. Do not remove the bug or mark it complete after
implementation unless the user explicitly asks for that board operation. The
executor's final report must include:

```text
Bug resolution summary: One concise sentence explaining how the bug was fixed.
```

### list tags

Print the fixed priority and type tags, then print all area tags currently used
in `project_board.md`.

### list tasks

Print full task entries. Honor requested section and tag filters.

### search archive

Search completed tasks and fixed bugs without mutating the board:

```powershell
python tools/project_board.py archive-search --query "diagnostics"
python tools/project_board.py archive-search --id 25 --full
```

Archive search is read-only and does not require `--pm-authorized`. PM may use it
to find historical context, duplicate/superseded work, bug resolutions, or a
candidate entry to reopen.

### edit brief

Update only the canonical `Brief:` line for an existing task or bug:

```powershell
python tools/project_board.py edit-brief --pm-authorized --id 25 --brief "Updated findings, implementation path, and verification."
```

The command preserves all other metadata and validates `project_board.md` after
the edit.

### set freshness

Update the task's start-gate freshness state:

```powershell
python tools/project_board.py set-freshness --pm-authorized --id 25 --status fresh --checked now --note "Revalidated current files and active assignments; task remains valid."
```

Allowed statuses are `needs-refresh`, `fresh`, `stale`, and `superseded`.
`--checked` defaults to `now`; use `never` only when resetting a task to
`needs-refresh`. `start` and `move --to "In Progress"` fail unless the task is
`fresh`.

### remove task

Resolve ids, titles, or explanations to board entries. Remove only the matched
entries. Delete referenced images only when the user explicitly asks for image
cleanup.

The Next.js board delete action is PM-gated and must not delete directly. It may
only call `tools/project_board.py remove --pm-authorized --id <id>` after a
PM-issued exact command has been supplied. Deletion removes only the board entry
and never deletes referenced images or files.

### future command candidates

Useful commands to consider later, but not part of the current command surface:
`edit-summary`, `edit-title`, `edit-final-owner`, `edit-plans`,
`edit-source-refs`, `set-priority`, `set-area`, `set-type`, `append-note`,
`append-coordination`, `block`, `unblock`, `claim`, `release`, `duplicate`, and
`split-task`.

### mark as completed

Resolve ids, titles, or explanations to board entries. Archive the matched
entries only when the user explicitly asks to mark the task completed or the bug
fixed:

```powershell
python tools/project_board.py complete --pm-authorized --id 25
python tools/project_board.py complete --pm-authorized --id 19 --resolution "Fixed the runtime target binding and restored single-cube viewport behavior."
```

This command moves the entry from `project_board.md` to
`project_board_archive.md` under the current date and current internal dev
version. It does not bump `VERSION` and does not create a user-facing
`changelog.md` release entry. Bug entries require `--resolution`; the summary
must come from the executor's bug-fix report or a PM/architect-approved
equivalent. Push is the release boundary, and `changelog.md` remains limited to
public-facing application changes. If task completion changes durable repository
state beyond board bookkeeping, record that work in `dev-changelog.md`.

### reopen archived task

Reopen an archived card only when the user explicitly asks for archived work to
become active again and the PM returns an exact command:

```powershell
python tools/project_board.py reopen-archive --pm-authorized --id 25 --reason "User requested follow-up work from archived task context."
python tools/project_board.py reopen-archive --pm-authorized --id 19 --to Bugs --reason "Bug reproduced after archive validation."
```

`reopen-archive` removes the entry from `project_board_archive.md` and restores
the same task id to `project_board.md`. Normal tasks default to `Backlog`; bugs
default to `Bugs`. The reopened entry receives
`Freshness: [status:needs-refresh][checked:never]`, so it cannot start until the
normal freshness preflight marks it fresh. The command preserves the original
entry text, records `Reopened:` metadata, and keeps previous bug resolutions as
historical context.

## Images

When the user provides visual clues for a task or bug, save each image under an
appropriate project-local task image folder such as:

```text
agents/task_images/
```

Use this filename form:

```text
{task_id}_{slugified_task_title}.extension
```

For multiple images, append `_2`, `_3`, and so on. Reference the image paths in
the task brief. Do not delete images unless the user explicitly authorizes image
cleanup.
