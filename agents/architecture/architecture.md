# Core Architecture Brief

Lean first-read for Quader core architecture. The preserved full reference is `agents/architecture/reference/architecture-full.md`; read it for broad architecture audits, major subsystem redesign, or unclear boundary decisions.

## Core Stance

Quader is a C++20 Qt Widgets polygon modeling app.

Architecture rule:

```text
Core owns geometry/topology.
Commands mutate documents.
Tools convert input into commands.
UI and Crimson render views of state.
```

Keep the dependency graph boring:

```text
app
  -> ui / tools / crimson / io / commands
commands -> document
document -> mesh
mesh -> geometry -> math -> foundation
```

Lower layers must not include higher layers. Core mesh/document/commands/tools must not depend on Qt Widgets or graphics-runtime headers.

## Module Ownership

- `foundation`: results, diagnostics, logging, low-level utilities.
- `math`: Quader-owned math wrappers/types; do not leak third-party math types at public boundaries unless already accepted.
- `geometry`: stateless geometric algorithms and predicates.
- `mesh/core`: mesh storage, IDs, topology, validation, attributes.
- `mesh/ops`: mesh operations; transactional where they mutate topology.
- `document`: scene/document state, selection, dirty state, document events.
- `commands`: undoable user-visible mutations.
- `tools`: state machines that interpret editor input and emit commands/previews.
- `crimson`: renderer-facing snapshots, render data, GPU boundary, overlays, picking.
- `io`: import/export adapters; no UI dialogs.
- `ui`: Qt Widgets adapters, actions, models, panels, viewport host.
- `app`: composition root, startup, service wiring.

## Hard Invariants

- One mutation path: user/script/importer -> command -> document mutation API -> mesh/scene op -> event/dirty flags -> UI/renderer update.
- Use stable typed IDs/handles across systems; do not store raw mesh pointers in selection, commands, tools, UI, renderer caches, or long-lived state.
- Mesh operations must preserve topology invariants and expose validation.
- Renderer consumes prepared render data/snapshots; it never owns modeling logic.
- UI owns widgets/actions/presentation only; it does not implement topology or mutate meshes directly.
- Importers create documents/fragments through core APIs; file dialogs and UX live outside I/O.
- Avoid hidden global mutable state, service locators, and singleton current-document/current-renderer patterns.
- Prefer Quader-owned result/error types at public boundaries over raw booleans or stringly-typed dispatch.

## Beautify Standard

For `beautify` or API-elegance review, prefer APIs that read naturally at call sites while preserving dependency direction and testability:

- intent methods over generic string dispatch;
- compact options/results DTOs over long parameter lists;
- typed IDs/results over raw strings, booleans, or loosely typed handles;
- clear ownership/lifetime in names and return types.

This is not formatting work.

## Testing Expectations

Core changes should normally have focused tests for:

- mesh/topology invariants;
- command apply/undo/redo;
- document state and selection semantics;
- geometry edge cases and invalid input;
- import/export round trips and malformed input.

Read `agents/tests-policy.md` before writing or changing tests.

## Review Checklist

- Dependency direction remains valid.
- Public API belongs to the module that owns the concept.
- Mutation path preserves undo/redo, validation, dirty events, and selection remapping.
- IDs/handles remain stable and typed.
- No Qt Widgets or graphics-runtime leakage into core layers.
- User-visible behavior has verification.
- Existing symbol documentation changed by the edit was updated in place.

## Read Full Reference When

Read `agents/architecture/reference/architecture-full.md` when:

- doing a broad/current-code architecture audit;
- changing mesh storage, command architecture, document ownership, I/O architecture, app composition, or cross-module dependency rules;
- resolving an architecture conflict not covered by this brief;
- writing a master architecture plan.
