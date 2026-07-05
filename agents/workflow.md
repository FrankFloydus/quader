# Workflow

## Documentation

- Documentation is part of the project contract.
- Keep documentation synchronized with accepted project changes that affect architecture, workflow, build commands, verification, file layout, naming, UI, renderer behavior, public behavior, or agent routing.
- Do not use `changelog.md` for agent workflow, internal tooling, build plumbing, documentation-only policy edits, scouting notes, repository maintenance, or generated artifact notes unless the change directly alters the public Quader application experience.
- Use `dev-changelog.md` for durable development changes, including internal workflow, tooling, architecture, documentation, build, task-board, and maintenance changes.
- A change is not complete until the matching documentation path is current or the final handoff states that no documentation path changed because the implementation stayed within already documented behavior.

## Testing Policy

- `agents/tests-policy.md` is the required authority for automated tests, test harnesses, fixtures, golden images, fuzz/property tests, sanitizer usage, validation checks, and manual verification substitutes.
- Any agent planning, writing, modifying, deleting, or reviewing tests must read `agents/tests-policy.md` first. This includes the main/root agent, specialized architects, PM-spawned workers or scouts, and future agent profiles that could affect tests.
- Project-manager assignment packets and architect prompts that include or imply test work must list `agents/tests-policy.md` in the required files/docs to read.
- If a meaningful automated test is not practical, record why and provide a concrete manual verification step.

## Changelog

- There are two changelogs:
  - `changelog.md`: user-facing release notes for the final "What's changed" surface.
  - `dev-changelog.md`: detailed engineering log for repository changes.
- Keep `changelog.md` updated during the push workflow only for public-facing Quader application changes.
- Keep `dev-changelog.md` updated during ordinary work for durable source, docs, workflow, build, architecture, tooling, task-board, verification, and maintenance changes.
- `changelog.md` is release notes for users, not an internal engineering diary.
- `dev-changelog.md` is intentionally technical and may mention files, tools, architecture decisions, verification commands, and internal process changes.
- The `changelog.md` format follows Keep a Changelog 1.1.0.
- Keep the `# Changelog` title, short format preamble, `## [Unreleased]` section, and newest-first version sections named `## [x.y.z] - YYYY-MM-DD`.
- Group release notes under `Added`, `Changed`, `Deprecated`, `Removed`, `Fixed`, and `Security`. Omit empty headings.
- Write for users. Record notable capabilities, visible behavior, compatibility, migration, and security changes.
- Do not record agent workflow, internal tooling, build scripts, docs policy, architecture-only notes, commit logs, noisy implementation steps, typo-only churn, or private debugging notes.
- In `dev-changelog.md`, record those internal changes when they are durable project changes.
- Temporary local experiments, smoke-test board entries that are removed before handoff, generated build output, and reverted edits do not need dev-changelog entries.
- If a push contains only internal tooling, docs, workflow, build, or maintenance changes, bump the version during push but do not add changelog bullets for those internal changes.
- Before push, inspect the outgoing diff and commit range, then write a structured entry for the confirmed version.
- Keep version headings linkable with reference links at the bottom of `changelog.md` whenever the official repository URL makes that practical.

## Task Board

- Root `project_board.md` is the source of truth for active tasks. Root `project_board_archive.md` is the chronological history for completed tasks and fixed bugs, grouped by archive date and internal dev version.
- Read `agents/task_board.md` before using board commands, changing the board, saving task clue images, or marking tasks complete.
- Board authority is PM-gated: `quader-project-manager` is the sole authority for board content, task creation, assignment metadata, and allowed status-transition decisions.
- The main/root chat may physically run `tools/project_board.py`, but only to apply exact PM-issued commands. Every mutating board command must include `--pm-authorized`, and main/root may use that flag only when the PM returned that exact command. Ordinary chats, architects, workers, scouts, and fallback agents must never add, edit, or move board entries directly.
- Performance exception: `quader-performance-dev` is the only non-PM role that may supersede the PM, and only for explicit C++ performance audit/fix workflows. In that workflow it reads the PM workflow first, adds needed performance tasks or bugs itself, implements them, and moves verified work to `In Review`. It must not complete, archive, remove, reopen, push, bump versions, or update public release notes.
- New task requests are intake-only by default. For a new change, fix, rename, refactor, implementation, task, or bug that is not already represented by an active board entry, do not edit files or perform the work on the spot. If it belongs to any architect domain, delegate intake to the relevant architect(s). If it belongs to no architect domain, the main/root chat researches it. Send one PM-ready `Final Task Intake Report` to `quader-project-manager` before board changes.
- Directly requested maintenance of the external Next.js control-board frontend at `C:\Users\Drako\Desktop\quader-project-board` is support tooling, not Quader product UI work. Dashboard, board-view, changelog-view, frontend API, and control-board styling changes may be implemented directly unless the user asks to create a board task for them.
- Reference/parity task intake must include `Source refs:` with the source folder path and file-to-line mapping. Treat `check this code reference`, `same`, `1:1`, `100%`, and `parity` as a request to match behavior and structure as closely as practical without unsafe verbatim third-party copying.
- When an actionable master plan or master audit exists, send its ordered implementation batches to `quader-project-manager` before implementation starts. The PM checks duplicates and returns exact `tools/project_board.py add --pm-authorized --plans <master-plan-path>` commands for the main/root chat to apply.
- When messaging another Codex chat about a task, instruct that chat to use the PM-gated task intake workflow; do not ask it to write the board.
- Before implementing architecture-sensitive feature work, confirm the task or prompt references the active master audit or a follow-up implementation plan. For the current freeze, use `agents/plans/audit_20260704_full_architecture_master.md` unless a later approved plan supersedes it.
- Before moving any Backlog task into active work, run the task freshness preflight in `agents/task_board.md`. New tasks and existing tasks without `Freshness:` are treated as `needs-refresh`; only tasks marked `fresh` may start.
- When an executor takes a board task or PM assignment for implementation, it must create a Codex `/goal` before editing. The goal states the task id/title or assignment scope and says completion means the implementation scope is fully done, verified, documented where needed, and handed back through PM/authority review.
- Executors must not silently apply workarounds, plan deviations, temporary bypasses, weakened tests, or partial substitutes. If a workaround or deviation seems necessary, stop and report it to the main/root chat so the PM can route it through the correct architect and add or update board tracking.
- If the user reports a bug or asks for a quick fix directly inside a running executor/subagent chat, the executor must stop affected work, capture the bug context, and ask the main/root chat to route architect review through the PM before continuing.
- Agent-initiated board status changes are limited to `Backlog` -> `In Progress` and `In Progress` -> `In Review`.
- Do not complete, remove, cancel, archive, or otherwise move a task or bug unless the user explicitly authorizes that board operation and the PM returns the exact `--pm-authorized` command. `complete` archives the entry; it does not delete history.
- When a bug fix is implemented, the executor's final report must include `Bug resolution summary:` with a concise explanation of how the bug was fixed. If the user later authorizes marking the bug fixed, the PM must include that summary in `python tools/project_board.py complete --pm-authorized --id <id> --resolution "<summary>"`.
- Passing tests, building successfully, or implementing a fix means the task is ready for user review; it does not authorize completing or removing the task.
- The board does not define release history. Version and user-facing `changelog.md` updates happen during push. Durable board process changes should be recorded in `dev-changelog.md`.

## Performance Developer Workflow

- Use `quader-performance-dev` when the user asks to audit and fix performance, optimize hot paths, reduce allocation pressure, improve cache behavior, improve vectorization/branch behavior, investigate concurrency overhead, or make the C++ project code performant.
- The performance developer must use the custom `cpp-performance` skill. If it is not exposed in the active skill list, it must read `C:\Users\Drako\.codex\skills\cpp-performance\SKILL.md` directly before acting.
- Before board mutation or implementation, it reads `AGENTS.md`, this workflow, `agents/task_board.md`, `agents/tests-policy.md`, `C:\Users\Drako\.codex\agents\quader-project-manager.toml`, current board entries, and active plans relevant to the hot path.
- For explicit performance audit/fix requests, it may create its own board entries with `tools/project_board.py add --pm-authorized` or `add-bug --pm-authorized`, using `area:performance` unless a specific area is clearer. The brief must include the suspected or measured hot path, baseline evidence or measurement plan, optimization hypothesis, owned files, risks, and verification commands.
- It may set freshness, move its own performance tasks to `In Progress`, implement them, and move verified tasks to `In Review` without PM assignment packets. It still sets a Codex `/goal` before editing and follows the no-silent-workaround, mid-work bug, tests-policy, and build checkpoint rules.
- It must measure before and after under comparable conditions and report absolute and relative impact. If measurement is not yet possible, the first task should add measurement or benchmarking support rather than claiming an optimization.
- If a performance change requires public API redesign, dependency-boundary changes, UI behavior changes, Crimson render semantics changes, or weaker verification, it pauses and requests the owning architect review before continuing.
- It cannot complete/archive/remove/reopen tasks, push, bump versions, manually edit `DEV_VERSION`, or write public `changelog.md` entries.

## Task Intake Workflow

- New task and bug requests must not be performed on the spot unless the user explicitly says to perform the work now and bypass task intake.
- First ask whether the request belongs to any architect domain:
  - software architect for build, module layout, target naming, source-folder structure, workflow, board/process, app/core, cross-domain work, or unclear ownership.
  - UI architect for Qt Widgets, actions, panels, models, viewport host UI, settings, dialogs, and UI services.
  - renderer architect for Crimson, render graph, shaders, materials, overlays, picking, render tests, and renderer-facing assets.
- If yes, spawn the relevant architect(s) in `TASK_INTAKE` mode. Use at most one architect per domain and do not spawn duplicate generic/default/scout/worker agents for the same scope.
- If no, the main/root chat researches the code/docs itself.
- Produce one PM-ready `Final Task Intake Report`. For multi-domain intake, gather architect domain findings first and have one finalizing architect reconcile them.
- Send the `Final Task Intake Report` to `quader-project-manager`. The PM checks duplicates and returns either exact `tools/project_board.py add --pm-authorized` or `add-bug --pm-authorized` commands, or a duplicate/no-op decision.
- The main/root chat applies only the PM's exact command and reports the new task id or duplicate decision.
- Imperative wording such as "rename X", "change Y", or "fix Z" is not consent to bypass intake.
- If the request mentions code reference, same, `1:1`, `100%`, or parity, include `Source refs:` in the final report and PM-generated board task.
- Do not create "preflight", "PM prerequisite", or "setup" board tasks merely so the PM can assign work. Preflight tasks are valid only when explicitly requested by the user, an active master plan, or an architect plan.

## Architecture Guardrails

- Preserve the current prototype behavior while the Batch 0 freeze from `agents/plans/audit_20260704_full_architecture_master.md` is active.
- Do not add real document mutation, topology editing, selection truth, import/export, material editing, picking-driven selection, PBR, render graph behavior, or debug panels directly to `MainWindow` or `BgfxWidget`.
- New work that touches UI/renderer/core boundaries must cite the active master audit or a follow-up implementation plan before implementation starts.
- If a task or prompt requests architecture-sensitive work without that reference, route it through the task-board or architect workflow first instead of implementing directly.

## Project Manager Workflow

- Use `quader-project-manager` for board-backed multi-agent work, cross-domain coordination, or tasks the user explicitly asks to run with the PM.
- The PM owns board decisions, assignments, and authority routing. It does not implement code, mutate the board directly, or provide final domain approval.
- The main/root agent applies only exact PM-issued board commands with `tools/project_board.py` and spawns agents requested by the PM.
- Spawned workers, scouts, and architects do not edit `project_board.md`; they return `Board update request:` blocks.
- PM batches must revalidate selected tasks before assignment. Spawn workers only for tasks whose `Freshness:` is `fresh`; route `stale` or `superseded` tasks through the relevant/finalizing architect or back to the user instead of assigning implementation.
- PM assignment packets for implementation work must instruct each executor to set a Codex `/goal` before editing and complete that goal only after the assigned scope is implemented, verified, and reported back.
- PM assignment packets must include the no-silent-workaround rule and the mid-work bug quick-fix lock. If a worker reports either condition, the PM routes the report to the correct architect, decides whether the current task scope changes or a new board entry is needed, and returns exact board update/assignment instructions.
- Final approval comes from the authoritative domain architect: software, UI, or renderer. Software architect arbitrates cross-domain conflicts.
- After authoritative approval, move `In Progress` -> `In Review` by applying the PM's exact `--pm-authorized` command. User authorization plus an exact PM-issued command is still required to complete, archive, or remove the task. Fixed-bug archival requires a `--resolution` summary.
- When the main/root chat spawns subagents for a workflow, it must wait for all spawned agents to finish and report before finalizing the turn or claiming completion. Do not end with "waiting for subagents" or similar. If the user explicitly cancels or redirects, state which spawned agents were still open and how their results are being discarded, superseded, or handled.

## UI Architect Qt Resource Scan

- For tasks directly involving Qt widgets or Qt Widgets APIs, the UI architect must perform a quick external resource scan before returning intake findings, plans, or reviews.
- Official Qt documentation is the primary source. Stack Overflow and specialized Qt/C++ UI sites may supplement it when relevant.
- The UI architect output must include `Qt resources checked:` with valid URLs and concise findings that affected the recommendation.
- If network access fails, the UI architect must state that failure explicitly and proceed from local project docs/current code without inventing sources.

## Renderer Architect Graphics Resource Scan

- For Crimson or graphics-programming task intake, pending task revalidation, implementation planning, deviation/bug review, or final implementation review, the renderer architect must perform a quick external graphics-resource scan before deciding.
- Use https://graphicscompendium.com/index.html and https://learnopengl.com/Introduction as authoritative starting references when relevant to the task.
- The renderer architect must browse/search for relevant article or HTML pages for the task and read the selected page contents fully rather than relying on search snippets.
- Renderer architect output must include `Renderer resources checked:` with valid URLs and concise findings that affected the recommendation.
- If network access fails, the renderer architect must state that failure explicitly and proceed from local project docs/current code without inventing sources.
- For renderer implementation plans or implementation-facing renderer reviews, the renderer architect must also quickly scan the local source-reference corpus at `C:\Users\Drako\Desktop\_SOURCES` for useful renderer approaches or snippets. The output must include `Local renderer references checked:` with folders/files inspected and useful file-to-line references, or state that no relevant local reference was found. Treat local references as guidance only; do not copy third-party code verbatim unless reuse is explicitly safe.

## Architect Beautify Rule

- `beautify`, `beautify code`, `API beautify`, and `audit and beautify` are explicit architect-review triggers.
- Beautify means scanning public and cross-module APIs for elegance, concision, intent-revealing call sites, ownership clarity, naming, parameter shape, and unnecessary ceremony.
- Beautify is not formatter/linter work and not cosmetic whitespace cleanup. Architects return findings or plans; they do not implement directly.
- Normal `audit` reports architecture drift, boundary flaws, ownership problems, missing tests, and correctness risks.
- Standalone `beautify` reports API/design elegance improvements unless a real architecture flaw blocks the prettier API.
- `Audit and beautify` produces separate `Architecture Audit Findings` and `Beautify Findings` sections.
- Beautify findings should identify the current awkward API or call pattern, why it is awkward, the proposed prettier API, affected call sites/modules, migration risk, and tests/build checks needed.
- Prefer intent-level APIs such as `mesh->bevel(options)` when the operation belongs to the mesh/domain object, compact domain DTOs/options objects over long parameter lists, project-owned typed IDs/results over raw strings/booleans/loosely typed handles, and APIs that read naturally at call sites while preserving dependency direction and testability.
- In frozen parallel audit mode, architects include beautify findings only when explicitly asked for `audit and beautify`; the software architect keeps accepted architecture findings separate from accepted beautify recommendations in the master report.

## Task Freshness Gate

- Every queued task is provisional until it is revalidated against the current workspace immediately before execution.
- `Freshness: [status:needs-refresh|fresh|stale|superseded][checked:<iso-or-never>] Note.` is the structured board metadata for this gate.
- New `add` and `add-bug` entries default to `needs-refresh`; existing entries without `Freshness:` are treated as `needs-refresh`.
- Freshness preflight reads the task entry, `Plans:`, `Source refs:`, final owner, active PM/authority/assignment/coordination metadata, and current files/modules likely touched.
- If the task still matches the current workspace, ask the PM for and apply `python tools/project_board.py set-freshness --pm-authorized --id <id> --status fresh --checked now --note "<reason>"`, then start it only through the PM-issued start/move command.
- If assumptions changed but the task is still useful, set `stale` and route a revalidation report through the PM and relevant/finalizing architect before implementation.
- If the task is obsolete or duplicated, set `superseded`; do not implement unless the user or PM replaces the brief.
- Architect-domain freshness decisions go through the relevant/finalizing architect. Simple non-domain freshness can be handled by the PM or main/root scout.
- `python tools/project_board.py start --pm-authorized --id <id>` and `move --pm-authorized --to "In Progress"` fail unless the task is `fresh`.

## Executor Goal Rule

- A chat that executes an existing board task or PM implementation assignment must set a Codex `/goal` before editing files or running implementation commands.
- The goal text must include the board task id/title or PM assignment key and the concrete completion condition.
- The goal is complete only when the executor's full scope is implemented, required build/tests or documented verification are done, docs/changelogs are updated when required, and the result has been handed back to the PM/authority path or moved to `In Review` when allowed.
- Do not mark the goal complete for planning, partial implementation, failed verification, or a handoff that still requires executor-side work.
- Intake-only, revalidation-only, PM coordination, and architect review chats do not need an executor goal unless they also perform implementation.

## No Silent Workaround Rule

- Executors must implement the accepted plan/task scope. They must not silently replace it with a workaround, shortcut, temporary fallback, bypass, weaker architecture, weaker verification, or partial substitute.
- Workaround examples include changing an interface contract to avoid planned integration, stubbing a planned feature, skipping a required test/build/check, moving logic into the wrong layer, accepting a known bug as "good enough", or adding temporary compatibility code without plan authority.
- When a workaround, deviation, or plan gap appears necessary, the executor stops before continuing that path and sends a `Workaround/Deviation Report` to the main/root chat.
- The report must include task id/assignment key, accepted plan or board brief being deviated from, files touched or proposed, exact reason the plan cannot be followed, risk, recommended correction, and whether any temporary code has already been written.
- The main/root chat sends the report to `quader-project-manager`. The PM routes to the correct architect and returns exact board update requests, such as a coordination note, revised assignment, blocker/fix assignment, or new task/bug intake command.
- The executor resumes only after main/root sends back the PM/architect-authorized path. If temporary workaround code was already written, it must stay visible in the report and cannot be treated as completed work until explicitly accepted or replaced.

## Mid-Work Bug Quick-Fix Lock

- If the user finds a bug in a mid-work build and reports it directly inside a running executor/subagent chat, that executor must not independently implement the quick fix.
- The executor acknowledges the report, stops affected work, preserves current context, and returns a `Mid-work Bug Report` to the main/root chat.
- The report must include task id/assignment key, user-visible symptom, repro steps if known, build/dev version or executable used if known, suspected files/area, overlap with current assignment, and whether continuing current work could hide or worsen the bug.
- The main/root chat routes the report to the PM, and the PM asks the correct architect for review when the bug touches an architect domain.
- The PM decides whether the bug is folded into the current task, becomes a new bug board entry, blocks the current assignment, or is deferred. Main/root then messages the executor with exact continuation instructions.
- Even if the user asks the subagent for a "quick fix", the executor continues only after that main/root + PM/architect path replies.

## Immediate Work Conflict Preflight

- If the user explicitly consents to immediate implementation and skipping task intake, run a conflict preflight before editing files.
- Check active board entries plus `PM:`, `Authority:`, `Assignment:`, and `Coordination:` metadata for overlapping files, domains, task ids, plans, or running assignments.
- When available, use Codex thread coordination tools to list/read/message likely active related threads.
- If overlap is visible, stop and ask whether to coordinate, message the other thread, or proceed anyway.
- If no overlap is visible, proceed only after recording that the check is best-effort because thread tools are not a perfect live lock.

## Build Checkpoint Policy

- For C++/CMake/shader/runtime changes, run `cmake --build --preset qt-mingw-debug` after each coherent implementation slice and before starting another independent code task.
- In PM-coordinated batches, do not complete more than two implemented code tasks without a successful build.
- Before architect review or moving a task to `In Review`, run the relevant build/tests.
- If user-visible runtime behavior changed, also run `cmake --build --preset qt-mingw-debug --target deploy` so the control board can launch the current executable.
- Dev-build archives are heuristic stable checkpoints, not manual-only. After a successful build/deploy for a task or PM batch that changes runnable behavior, executable/runtime packaging, shaders/assets, CMake/runtime dependencies, or fixes a bug observed in the executable, create a preserved snapshot with `python tools/dev_builds.py archive --preset qt-mingw-debug`.
- In PM-coordinated batches, archive once per coherent stable checkpoint, normally after the ready tasks in that checkpoint have passed verification and authority review or when the user should test the runnable exe. Do not archive after every tiny code slice.
- Skip automatic dev-build archiving for docs-only, task-board-only, external control-board frontend-only, tests-only, planning-only, or pure internal refactor work with no runnable behavior/runtime risk unless the user asks for a snapshot.
- If known concurrent work would make the archive include unrelated unverified changes, do not auto-archive. Report `Dev build checkpoint deferred:` with the reason and the command to run once the workspace is stable.
- `DEV_VERSION` still increments only after `tools/dev_builds.py archive` confirms the runnable archive. Never edit `DEV_VERSION` manually.
- If the build fails, stop further code-task batching in that area and route the failure through the PM as a blocker or fix assignment.

## App Versioning

- Root `VERSION` is the app version source of truth.
- Root `DEV_VERSION` is the internal development snapshot counter. It stores only a numeric counter.
- `CMakeLists.txt` must read or mirror `VERSION`; do not leave a contradictory hard-coded project version.
- Use unpadded `x.y.z` numeric format. The app may display it as `vX.Y.Z`.
- Internal dev versions display as `<VERSION>-dev.<DEV_VERSION>`, for example `0.1.0-dev.12`.
- `DEV_VERSION` increments only when a runnable dev build is successfully archived with `tools/dev_builds.py archive`.
- Do not edit `DEV_VERSION` manually during ordinary implementation, review, planning, task-board work, or push preparation.
- Do not bump `VERSION` when creating an internal dev build archive.
- Archived runtime bundles live outside the repository by default under `C:\Users\Drako\Desktop\quader-dev-builds\<VERSION>-dev.<N>\`; set `QUADER_DEV_BUILDS_ROOT` only for nonstandard layouts. `build-info.json` inside each archive records the exact bundle metadata.
- Patch pushes normally increment only the patch component.
- Patch must stay within `0..9999`; if it is already `9999`, stop and ask before rolling minor or major.
- Major or minor increments are release-line decisions and require explicit user confirmation during push.
- Version bumping is tied to push. Do not bump `VERSION` and do not create a new user-facing `changelog.md` version entry during ordinary implementation, review, inspection, planning, or task-board completion. Ordinary durable work may still update `dev-changelog.md`.

Internal dev-build commands:

```powershell
python tools/dev_builds.py current
python tools/dev_builds.py list
python tools/dev_builds.py archive --preset qt-mingw-debug
python tools/dev_builds.py launch --version 0.1.0-dev.N
```

When the user asks to push:

1. Inspect outgoing commits, staged changes, unstaged changes, and current `VERSION`.
2. Propose the next version number and ask the user to confirm or provide a replacement.
3. After confirmation, update `VERSION`, keep CMake version metadata in sync, and write a `changelog.md` entry only for public-facing application changes included in the push.
4. Commit release metadata changes when they are separate from already prepared work.
5. Push commits and matching version metadata together.

When a confirmed version starts a new major or minor line:

- Create an annotated git tag named `vX.Y.Z`.
- Create a zip snapshot of the tagged repository state excluding git internals and generated build/cache directories.
- Upload the zip as a GitHub release asset for that tag using the GitHub CLI `gh`.
- If `gh` is unavailable or unauthenticated, report the blocker before completing the push workflow.

## Git Repository Handling

- Never commit, push, tag, create releases, stage files for a requested commit, or otherwise alter remote repository state without explicit user authorization.
- When the user asks to `push`, execute the push-based versioning workflow before the actual push.
- Do not create new branches unless explicitly requested.
- Keep generated local build/check trees out of source review. Existing `build/` output is generated state, not architecture authority.
