# Testing Policy

Lean first-read for Quader testing. Full detailed examples are preserved in `agents/reference/tests-policy-full.md`; read it for broad test audits, ambiguous test design, or major verification policy work.

## Core Rule

Every test must protect real user-visible behavior, data invariants, editor workflows, or critical failure handling. Do not add tests that only freeze implementation details, cosmetics, or mock call order.

Focus on geometry correctness, document/model state, tools, commands, undo/redo, import/export, selection, picking, snapping, renderer-facing semantics, and user-visible workflows.

## Validation Lock

Do not weaken tests, validation scripts, clang-format, clang-tidy, sanitizer settings, architecture checks, or quality gates so current code passes. Current code is migration evidence, not permission to lower the requested standard.

If a requested gate causes broad churn or needs staged adoption, stop and report a `Workaround/Deviation Report` with alternatives. Non-mutating checks and documented existing failures are allowed.

## Style/Static-Analysis Permission

Do not run clang-format, clang-tidy, `check_format`, `check_clang_tidy`, `check_static_analysis`, related Python tools, raw clang tools, or any CTest/preset command that includes them without immediate explicit user permission in the current turn.

Use `ctest --preset qt-mingw-debug-runtime` for broad agent-safe debug runtime coverage. If another broad preset includes those gates, use targeted test binaries or targeted CTest selections instead.

## What To Test

Use the smallest test that gives confidence:

- Unit: pure geometry/math, parsers, validators, command logic, serialization helpers.
- Integration: document + command stack + serializer, or multiple real systems.
- GUI/tool: critical editor workflows that cannot be tested below UI; assert semantic results, not layout.
- Smoke/E2E: very few high-value app workflows.

Must-test areas:

- geometry/mesh invariants, winding, bounds, invalid/degenerate input;
- commands and undo/redo: apply -> expected -> undo -> original -> redo -> expected;
- document state, selection, tool activation when behavior changes;
- import/export save-load equivalence and malformed input safety;
- picking, snapping, transform semantics, material/UV projection when touched;
- bug fixes: regression test that would fail before the fix when practical.

## What Not To Test

Avoid:

- exact widget pixels, margins, colors, fonts, layout hierarchy unless explicit product scope;
- private methods when behavior is testable through public APIs;
- internal call order unless order is required behavior;
- third-party library behavior;
- whole-window snapshots without a stable visual regression system;
- default-value tests with no invariant;
- sleeps, timing, network, wall-clock, machine-specific paths, random order;
- giant unreadable golden files.

## Quality Bar

Each test should:

- fail for a plausible regression;
- be deterministic and isolated;
- have a behavior-focused name;
- use minimal fixtures/data;
- assert outcomes with useful failure messages;
- use appropriate floating-point tolerances;
- clean up temporary files/resources;
- avoid human inspection.

Golden files must be small, intentional, readable, normalized, and justified. Prefer semantic comparisons.

Random/fuzz/property tests must be reproducible; print seeds on failure.

Performance tests should avoid tight timing thresholds. Use benchmarks or broad regression guards, not fragile pass/fail frame-time assertions.

## Existing Tests

Do not delete or weaken a failing test just because it fails. First determine whether behavior intentionally changed or a regression exists. If behavior changed, update the test to express the new behavior. If brittle, rewrite around meaningful outcomes.

## Agent Workflow

When changing code:

1. Identify affected behavior/invariant.
2. Choose the smallest useful test level.
3. Add/update tests before or alongside implementation when practical.
4. Run relevant local tests.
5. For geometry/parser/memory-sensitive changes, run sanitizer/property/fuzz checks when available and appropriate.
6. Report exactly what was tested.
7. If automated testing is not practical, state why and give concrete manual verification.
