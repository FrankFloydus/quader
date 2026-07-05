# Legacy Quader Custom-Agents Workflow

Status: archived reference only  
Archived on: 2026-07-05  
Superseded by: `$quader-workflow` plugin

This file parks the pre-plugin Quader custom-agent methodology in case the unified plugin workflow needs to be reverted or partially restored. It is not active workflow authority.

The active custom TOML profiles were removed from:

```text
C:\Users\Drako\.codex\agents\quader-*.toml
```

Disabled reconstructed copies are parked at:

```text
C:\Users\Drako\.codex\agents_disabled\quader_legacy_20260705\*.toml.disabled
```

The Windows Recycle Bin was checked after removal and did not contain the deleted
`quader-*.toml` files. The parked disabled copies are reconstructed from the
profile content read during migration and the folded `$quader-workflow` rules;
they are not byte-for-byte undeleted originals.

The removed active profiles were:

- `quader-software-architect.toml`
- `quader-core-architect.toml`
- `quader-build-workflow-architect.toml`
- `quader-ui-architect.toml`
- `quader-renderer-architect.toml`
- `quader-docs-mantainer.toml`
- `quader-performance-dev.toml`

`quader-board-developer` had already been converted from an agent to a skill before the plugin migration. It is now a redirect skill to `$quader-workflow`.

## Legacy Lean Workflow v2 Summary

The old model was architect-led and root-coordinated:

1. Root chat classified requests.
2. Root spawned clean scoped architect scouts with `fork_context:false`.
3. Domain architects produced findings, reviews, or plans.
4. `quader-software-architect` acted as final architecture governor and board authority.
5. Root physically ran `tools/project_board.py` commands returned by the software architect.
6. Existing board tasks were executed by root and optional worker agents.
7. Review went to the owning architect; approved work was archived.

The old model intentionally avoided a PM agent after Lean Workflow v2. The software architect replaced PM board authority.

## Legacy Routing

- Docs-only maintenance -> `quader-docs-mantainer`.
- External Next.js board/dashboard work -> `$quader-board-developer` skill.
- C++ performance audit/fix -> `quader-performance-dev`.
- `audit`, `beautify`, `audit and beautify` -> domain architect reports, then software-architect master consolidation.
- New product/build/workflow/core/UI/renderer task or bug -> relevant architect scout(s), then `quader-software-architect` final board entry.
- Existing board task -> root revalidates freshness, runs execution split gate, coordinates workers, integrates, verifies, sends to review.
- Review task -> owning architect approves/rejects; approved items archive through exact architect-issued commands.

## Legacy Agent Roles

### quader-software-architect

Final architecture governor and board authority.

Responsibilities:

- Finalize task intake.
- Check duplicates.
- Issue exact `tools/project_board.py ... --architect-authorized` commands.
- Arbitrate cross-domain conflicts.
- Own app-wide dependency direction.
- Review cross-domain/core/build/workflow work.
- Consolidate domain audit/beautify reports into master artifacts.
- Enforce worker-ready handoff quality.
- Reject missing/stale documentation only when existing documented symbols or explicit docs scope were affected.
- Enforce dev-build evidence for runtime-affecting closeout.

### quader-core-architect

First-pass scout/reviewer for:

- document state and mutation contracts;
- commands, undo/redo, command validation;
- tools and editor input contracts;
- mesh, topology, geometry, attributes, IDs, validation;
- I/O services and app/core service boundaries;
- modeling-domain API elegance.

### quader-build-workflow-architect

First-pass scout/reviewer for:

- CMake, presets, targets, deploy/runtime layout, executable naming;
- tools under `tools/`;
- task-board process and board command behavior;
- versioning, `DEV_VERSION`, dev-build archives, changelog/dev-changelog workflow;
- agent workflow docs and external profile wiring;
- support tooling that affects repository workflow.

### quader-ui-architect

First-pass scout/reviewer for:

- Qt Widgets shell, menus, toolbars, docks, status UI, dialogs;
- action registry, shortcuts, action state, context menus;
- Qt model/view, item models, delegates, property editors;
- settings, workspace state, notifications, UI services;
- viewport host UI and UI-side renderer integration;
- Qt-facing API elegance.

For Qt widget/API tasks, it was required to perform a quick Qt-resource scan and cite official Qt docs or relevant supporting resources.

### quader-renderer-architect

First-pass scout/reviewer for:

- Crimson renderer architecture and runtime isolation;
- render snapshots, render world, render graph, passes, frame resources;
- shaders, materials, texture/color-space handling;
- overlays, grid, gizmos, selection rendering, picking;
- renderer-facing asset import and material mapping;
- render tests, golden images, diagnostics, GPU/CPU boundaries;
- Crimson API elegance.

Renderer tasks used these reference sources when relevant:

- `https://graphicscompendium.com/index.html`
- `https://learnopengl.com/Introduction`
- `C:\Users\Drako\Desktop\_SOURCES`
- `C:\Users\Drako\Desktop\quader-windows\quader-app` for parity/source-of-truth tasks.

### quader-docs-mantainer

Documentation-only profile.

Allowed:

- copyright headers;
- Doxygen comments;
- documentation-policy updates;
- accepted hindsight entries;
- dev-changelog entries for durable documentation maintenance.

Forbidden:

- runtime behavior changes;
- public API redesign;
- build/test behavior changes;
- task-board mutations;
- version changes;
- public changelog changes.

### quader-performance-dev

Implementation-capable C++ performance specialist using the `cpp-performance` skill.

It had a narrow exception allowing it to create and move its own performance tasks with `--architect-authorized` during explicit performance audit/fix workflows. It could not complete/archive/remove/reopen/push/bump versions/update public changelog.

## Legacy Board Authority

New mutating commands used:

```powershell
python tools/project_board.py ... --architect-authorized
```

`--pm-authorized` existed only as a compatibility alias.

Only `quader-software-architect` could decide board content, task creation, status transitions, archive closeout, and rework commands. Root could physically run the command.

The narrow exception was `quader-performance-dev` for explicit performance audit/fix tasks.

## Legacy Audit And Beautify Flow

Broad `audit`, `beautify`, or `audit and beautify` requests used multiple domain artifacts:

```text
agents/plans/audit_{run_id}_core.md
agents/plans/audit_{run_id}_build_workflow.md
agents/plans/audit_{run_id}_ui.md
agents/plans/audit_{run_id}_renderer.md
agents/plans/audit_{run_id}_master.md
```

Rules:

- Root assigned exact lowercase snake_case report paths.
- Domain architects wrote full markdown reports.
- Root passed only paths to the software architect.
- Software architect read source reports directly and wrote the master artifact.
- Chat summaries were invalid.
- The master had to preserve the substance of source findings with a coverage ledger.
- Same-run transient domain reports could be deleted after a self-contained master was written.

## Legacy New Task Intake

Fresh task/bug requests were intake-only by default.

Flow:

1. Root classified domains.
2. Root spawned at most one scout per relevant domain.
3. Scouts returned `Task Intake Domain Findings`.
4. Software architect reconciled findings into one final intake.
5. Software architect returned exact add/add-bug command.
6. Root applied only that exact command.

No preflight task could be invented just so work could be assigned.

## Legacy Execution Flow

Before implementation:

- read task, plans, source refs, current board metadata, and likely touched files;
- run freshness gate;
- use exact `set-freshness --architect-authorized` command for fresh tasks;
- stop on stale/superseded tasks;
- run execution split gate.

Complex work was expected to use 2-6 clean scoped workers with `fork_context:false` when useful. Workers set `/goal`, implemented only their slice, reported checks, and did not mutate the board.

Root integrated, ran builds/tests, deployed for user-visible runtime changes, created dev-build archives for runtime-affecting work, and requested formal review.

## Legacy Review And Rework

Review decisions:

- `approved` -> `complete --architect-authorized`.
- `changes-requested` -> `request-changes --architect-authorized` with actionable `--rework`.
- `stale` -> `set-freshness --architect-authorized --status stale`.
- `superseded` -> `set-freshness --architect-authorized --status superseded`.

Rejected reviews were not moved back with plain `move --to "In Progress"`.

The board stored only current rework in:

```text
Rework: [owner:<owner>][agent:<agent>][checked:<iso>] <current requested corrections>
```

## Legacy Parity Rule

For `C:\Users\Drako\Desktop\quader-windows\quader-app`:

- Treat the reference app as source of truth for behavior, interaction semantics, visual result, data meaning, and user-visible substance.
- Adapt only necessary underlying implementation details for current Quader architecture, Crimson, Qt Widgets, licensing, and tests.
- Include `Source refs:` with exact folder/file/line mapping.
- Include `Reference substance preserved:` in implementation/review reports.
- Renderer parity required inspecting the reference app renderer internals before making renderer math/projection/picking/material recommendations.

## Legacy Revert Checklist

To revert from `$quader-workflow` back to the custom-agent methodology:

1. Copy the seven disabled TOML profiles from `C:\Users\Drako\.codex\agents_disabled\quader_legacy_20260705` to `C:\Users\Drako\.codex\agents`.
2. Rename them from `.toml.disabled` to `.toml`.
3. Restore active docs to the Lean Workflow v2 authority model:
   - `AGENTS.md`
   - `agents/workflow.md`
   - `agents/task_board.md`
   - `agents/documentation-policy.md`
   - `agents/hindsight.md`
4. Update external board copied prompts to route through architect-led intake/review again.
5. Use `--architect-authorized` for new board examples. The current board tool keeps it as a compatibility alias.
6. Either disable or ignore `$quader-workflow`, or reduce it to a wrapper that spawns/coordinates the restored agents.
7. Restore `$quader-board-developer` as the direct external-board skill if desired.
8. Validate:

```powershell
python tools/project_board.py validate
rg -n "quader-software-architect|quader-core-architect|quader-build-workflow-architect|quader-ui-architect|quader-renderer-architect|quader-docs-mantainer|quader-performance-dev" AGENTS.md agents C:\Users\Drako\.codex\agents
```

## Notes

The plugin migration folded the legacy rules into one skill rather than intentionally discarding them. If reverting, prefer regenerating TOMLs from the current `$quader-workflow` skill plus this file so any newer policy changes are not lost.
