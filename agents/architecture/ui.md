# Qt Widgets UI Architecture Brief

Lean first-read for Quader UI work. The preserved full reference is `agents/architecture/reference/ui-full.md`; read it for broad UI audits, major UI subsystem design, or Qt model/view/action architecture work.

## UI Stance

Qt owns widgets, events, actions, layouts, dialogs, and presentation adapters. The document owns editable state. Commands mutate the document. Tools interpret high-frequency viewport input. Crimson renders snapshots.

V1 uses Qt Fusion as the baseline. Do not build a broad custom theme, custom `QStyle`, or scattered style-sheet system unless explicitly planned.

## Allowed In UI

- Own widgets, `QAction`s, menus, toolbars, docks, dialogs, status UI.
- Translate Qt events into editor input events.
- Adapt document state into Qt models/delegates.
- Reflect command history/action state.
- Present diagnostics, settings, notifications, import/export UI.
- Host the viewport and hand render/input data across explicit boundaries.

## Forbidden In UI

- Topology algorithms or direct mesh mutation for user-visible edits.
- Source of truth for selection, transforms, object state, materials, or document data.
- Concrete Crimson GPU internals in widgets.
- Qt Widgets dependencies in mesh/document/commands/tools/core renderer headers.
- Long-lived raw mesh pointers or `QModelIndex` values as application truth.
- Signals/slots as an untyped global event bus.

## Module Map

- `ui/qt_app`: startup, `QApplication`, main window shell.
- `ui/actions`: action ids, registry, state updater, command dispatch.
- `ui/models`: Qt item models; adapters only, not domain truth.
- `ui/delegates`: item painting/editing, not business logic.
- `ui/panels`: dock/panel contracts and hosts.
- `ui/dialogs`: file/confirm/error UI services.
- `ui/style`: Fusion metrics, icons, targeted style policy.
- `ui/viewport`: widget host, controller, render host, event translation.
- `ui/services`: settings, notifications, presentation services.

## Hard Rules

- `QMainWindow` is a shell, not the application brain.
- Actions are canonical command/menu/shortcut state. Context-overloaded viewport keys must be resolved by viewport/tool context before triggering global actions.
- Slots stay thin: collect UI data, call controller/service/command, update presentation.
- Qt models expose stable project IDs in roles; do not treat row numbers or raw pointers as durable identity.
- Editable models dispatch commands; they do not mutate documents directly.
- Selection synchronization has one policy and one owner.
- Viewport widgets translate high-frequency input into editor input events and tool/controller calls.
- UI state is not document state unless it must survive save/load.
- UI updates happen on the UI thread.

## Qt Resource Scan

For any task involving a Qt widget, model/view, action/shortcut, event handling, dock/dialog, or style behavior, do a quick Qt docs/resource scan. Official Qt docs are primary; Stack Overflow or specialized Qt/C++ UI sources may support when relevant. Cite resources that materially affect the plan/review.

## Review Checklist

- No core layer gained Qt Widgets dependency.
- UI code dispatches through actions/commands/tools instead of mutating domain state.
- `QAction` state, shortcut context, and viewport input context do not conflict.
- Models/delegates keep domain behavior outside presentation.
- Widget lifetime/ownership is explicit and Qt-safe.
- Tests verify semantic behavior, not cosmetic layout, unless UI appearance is explicit scope.

## Read Full Reference When

Read `agents/architecture/reference/ui-full.md` when:

- designing or auditing actions, model/view, panels, shortcuts, viewport host, settings, dialogs, or UI service architecture;
- fixing non-trivial Qt event/key/focus behavior;
- changing UI module boundaries;
- writing a broad UI audit or master UI plan.
