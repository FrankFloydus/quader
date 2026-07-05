# Quader Agent Bootstrap

This file is intentionally short. Do not grow it with detailed workflows. Add new policy to the focused docs below and keep this file as the routing/index layer.

Quader is a C++20 Qt Widgets polygon modeling application. The renderer is named `crimson`; do not use graphics-runtime names in renderer folders, targets, or planned class names.

## Read First

Always read the focused doc for the kind of work being requested:

- Daily routing and authority: `agents/workflow.md`
- Board commands and metadata: `agents/task_board.md`
- Tests and verification policy: `agents/tests-policy.md`
- Documentation policy: `agents/documentation-policy.md`
- Critical hindsight register: `agents/hindsight.md`
- C++ style/static-analysis policy: `agents/code-style.md`
- Overall/core architecture: `agents/architecture/architecture.md`
- Qt Widgets UI architecture: `agents/architecture/ui.md`
- Crimson renderer architecture: `agents/architecture/renderer.md`

## Current Map

- Main repo: `C:\Users\Drako\Desktop\_quader-qt-base`
- External Next.js board: `C:\Users\Drako\Desktop\quader-project-board`
- Dev-build archives: `C:\Users\Drako\Desktop\quader-dev-builds`
- Active plans: `agents/plans/`
- Archived plans: `agents/archive/`
- Critical hindsight: `agents/hindsight.md`
- Active board: `project_board.md`
- Completed/fixed archive: `project_board_archive.md`

The app target is `quader_app`; the desktop executable is `quader.exe`.

## Fast Router

Use `agents/workflow.md` as the authority, but these triggers are critical:

- Docs-only maintenance -> `quader-docs-mantainer`.
- External Next.js board/dashboard work -> use `$quader-board-developer` directly.
- C++ performance audit/fix -> `quader-performance-dev`.
- `audit`, `beautify`, `audit and beautify` -> architect review workflow. Root must not perform the audit itself or summarize findings when an architect domain applies. Domain architects write full markdown reports in `agents/plans/`; the software architect reads those files directly for the master audit.
- New product/build/workflow/core/UI/renderer task or bug -> relevant architect scout(s), then `quader-software-architect` finalizes board entry.
- Existing board task -> root coordinates clean scoped workers only after freshness revalidation.
- Review task -> owning architect approves/rejects; approved items archive through exact architect-issued commands.

## Active Agents

- `quader-software-architect`: final architecture governor and board authority.
- `quader-core-architect`: document, commands, tools, mesh, I/O, app/core contracts.
- `quader-build-workflow-architect`: CMake, presets, tools, board/process, versioning, agent workflow.
- `quader-ui-architect`: Qt Widgets UI, actions, models, panels, viewport host UI.
- `quader-renderer-architect`: Crimson, render graph, shaders, materials, overlays, picking, render tests.
- `quader-docs-mantainer`: documentation-only maintenance.
- `quader-performance-dev`: measured C++ performance audit/fix specialist.

Active skill:

- `$quader-board-developer`: external Next.js board app, API routes, copied prompts, archive/changelog views, dev-build controls, and dashboard styling.

When spawning agents, use clean scoped prompts with `fork_context: false` unless current conversation context is required. Wait for every spawned agent before finalizing.

## Board Authority

`quader-software-architect` owns board authority. Root may physically run `tools/project_board.py`, but only with exact architect-issued commands. New mutating commands use `--architect-authorized`; `--pm-authorized` is legacy compatibility only.

Workers, scouts, architects except the software architect, docs agents, and skills must not mutate board content directly. The only exception is the narrow `quader-performance-dev` performance workflow described in `agents/workflow.md`.

## Standard Commands

```powershell
cmake --preset qt-mingw-debug
cmake --build --preset qt-mingw-debug
cmake --build --preset qt-mingw-debug-tests
ctest --preset qt-mingw-debug
cmake --build --preset qt-mingw-debug-deploy
.\build\qt-mingw-debug\quader.exe
python tools/project_board.py validate
python tools/dev_builds.py current
```

Run the external board:

```powershell
cd C:\Users\Drako\Desktop\quader-project-board
npm run dev
```

Open `http://127.0.0.1:3000`.

## Guardrails

- Keep unrelated user changes intact.
- Do not use destructive git commands unless explicitly requested.
- Do not silently weaken briefs, plans, validation strictness, parity requirements, style/tooling rules, or acceptance criteria.
- If code changes a documented class, function, variable, enum, member, or other symbol, update that documentation in place in the same edit; see `agents/documentation-policy.md`.
- Do not run clang-format, clang-tidy, style/static-analysis targets, or CTest presets that include them without immediate explicit user permission; plans/checklists are not permission.
- For C++/CMake/shader/runtime work, follow build checkpoint policy in `agents/workflow.md`.
- Record durable development/workflow changes in `dev-changelog.md`.
