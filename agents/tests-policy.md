## Testing policy for agents

The goal of tests in this project is to protect real modeling/editor behavior, not to inflate coverage or lock down irrelevant implementation details. This is a C++ desktop modeling/level-editing app inspired by tools like TrenchBroom and Source 2 Hammer, so tests must focus on geometry correctness, document/model state, tools, commands, undo/redo, import/export, selection, picking, snapping, and user-visible workflows.

### Core testing rule

Every test must answer: “Would this fail if a real user-visible behavior, data invariant, or editor workflow broke?”

Write the test if the answer is yes. Do not write the test if it only proves an implementation detail, a cosmetic detail, or a mock interaction that users do not care about.

### What agents MUST test

Prefer small, deterministic tests for pure logic and editor model behavior:

- Geometry and math:
  - brush/mesh creation
  - plane/face/winding validity
  - clipping, extrusion, CSG/boolean operations
  - transforms, pivots, bounding boxes, grid snapping
  - ray casting, picking, hit ordering
  - UV/material projection logic
  - degenerate and near-degenerate cases: tiny faces, coplanar faces, parallel planes, zero-length edges, invalid input

- Document/model commands:
  - create/delete/duplicate objects
  - move/rotate/scale selected objects
  - group/ungroup, parent/child changes
  - entity/property edits
  - selection changes
  - tool activation state when it affects behavior

- Undo/redo:
  - for every new command, test: apply -> expected state -> undo -> original state -> redo -> expected state
  - compare semantic document state, not pointer addresses or object allocation order
  - prefer normalized serialization/hash/state snapshots for comparing before/after editor document state

- Serialization/import/export:
  - save -> load -> equivalent document
  - load legacy/minimal files
  - preserve unknown or user-authored properties when required
  - reject malformed files safely with useful errors
  - use small, readable fixtures instead of huge golden files

- Regression tests:
  - every bug fix should include a test that would have failed before the fix
  - name the test after the behavior, not the bug number alone

- Parser and file-format robustness:
  - test empty, truncated, malformed, extreme, and valid inputs
  - add fuzz/property tests for map/model/material parsers when practical
  - fuzz targets must not crash, hang, call exit(), or write outside temporary test directories

- C++ safety:
  - run relevant tests with AddressSanitizer/UndefinedBehaviorSanitizer where available
  - use ThreadSanitizer for concurrency/job-system changes
  - do not hide sanitizer findings unless there is a documented false positive

### What agents MUST NOT test

Do not add useless or brittle tests such as:

- “the button has a 20px border”
- exact widget pixel positions, margins, fonts, colors, or layout hierarchy unless this is an explicit product requirement
- private methods directly when the behavior can be tested through public APIs
- internal call order between collaborators unless the order is the actual required behavior
- mocks that only verify “function X was called” without checking the resulting editor behavior
- tests of STL, Qt, OpenGL, ImGui, glm, Eigen, or other third-party library behavior
- snapshot tests of entire windows/screens unless there is a stable visual-regression system and the visual output is the product requirement
- tests that only instantiate a class and assert default values that do not encode a meaningful invariant
- tests that depend on timing, sleeps, local machine speed, network access, current time, random order, or external user files
- giant golden files where a failure produces an unreadable diff

### Test level selection

Use the smallest test that gives confidence:

1. Unit tests:
   - Use for pure geometry, math, serialization helpers, command logic, validators, and format parsers.
   - Must be fast, deterministic, isolated, and runnable frequently.

2. Integration tests:
   - Use when behavior requires multiple real project systems together, such as document + command stack + serializer.
   - Use local temporary files only.
   - Avoid real network, real user config, or machine-specific paths.

3. GUI/tool tests:
   - Use only for critical editor workflows that cannot be tested below the UI layer.
   - Simulate user input such as mouse/key/tool events through the app’s public event APIs.
   - Assert semantic results: selected object IDs, created brushes, transformed positions, enabled actions, document dirty state.
   - Do not assert cosmetic layout details.

4. End-to-end/smoke tests:
   - Keep very few.
   - Good examples: app starts, creates a minimal document, performs a basic edit, saves, reloads, exits cleanly.
   - These tests may be slower, so they must cover only high-value workflows.

### Quality bar for every test

Before adding or keeping a test, verify:

- It protects real behavior or an important invariant.
- It would fail for a plausible regression.
- It is deterministic and does not rely on sleeps, wall-clock timing, network, random seeds, or machine-specific state.
- It has a clear name describing behavior, for example:
  - `ClipBrush_WithCoplanarPlane_KeepsValidClosedBrush`
  - `MoveSelection_UndoRedo_RestoresDocumentState`
  - `MapParser_TruncatedEntity_ReturnsErrorWithoutCrash`
- It uses minimal fixtures and minimal data.
- It has readable assertions with useful failure messages.
- Floating-point comparisons use tolerances appropriate for the operation.
- It cleans up temporary files/resources.
- It does not require a human to inspect output.

### Preferred assertion style

Assert outcomes, not implementation.

Good:

- after clipping a brush, all faces are valid and the expected volume/bounds/topology are produced
- after dragging a selected vertex with grid snapping enabled, the vertex lands on the expected grid coordinate
- after undo, normalized document state equals the pre-command state
- after saving and loading, entities, brushes, materials, and properties are semantically equivalent
- malformed input returns a structured error and does not mutate the current document

Bad:

- assert that `ClipTool::buildTemporaryPreviewMesh()` was called
- assert that a private vector has size 3 because of the current implementation
- assert that a toolbar button is exactly 24px wide
- assert exact serialized whitespace unless the file format requires it
- assert the exact order of unordered containers

### Geometry-specific testing rules

For geometry code, test invariants in addition to examples:

- closed brush remains closed after valid operations
- face windings are consistently oriented
- no NaN/Inf coordinates are produced
- no zero-area faces remain unless explicitly allowed
- bounds are correct after transforms
- snapping is stable and idempotent
- applying identity transform changes nothing
- transform followed by inverse transform approximately restores original geometry
- serialization round-trip preserves geometry within tolerance

Use property-style tests where useful:

- generate valid simple brushes and random transforms
- apply operation
- assert invariants always hold
- record and minimize failing seeds/cases when the framework supports it

### Undo/redo command testing template

For any new editor command:

1. Build a minimal document fixture.
2. Capture normalized document state.
3. Execute the command.
4. Assert the expected semantic change.
5. Undo.
6. Assert normalized state equals the original state.
7. Redo.
8. Assert the expected semantic change again.
9. If command merging/coalescing exists, test merge and non-merge cases.

### Serialization/golden-file rules

Golden files are allowed only when they are small, intentional, and readable.

Prefer semantic comparisons over raw text comparison. If raw text comparison is necessary:

- normalize line endings
- normalize path separators where appropriate
- avoid timestamps/random IDs unless explicitly part of the test
- keep fixtures minimal
- include a clear reason why a golden file is better than object-level assertions

### Test data and fixtures

- Put test fixtures under a dedicated test data directory.
- Keep fixtures tiny and named by behavior.
- Do not use real user projects as fixtures unless reduced to the minimum repro.
- Do not rely on files outside the repository except temporary files created by the test.
- Temporary files must be written under the test framework’s temp directory and cleaned up.

### Randomness and fuzzing

- Normal unit tests must be deterministic.
- If randomness is used, fix the seed and print the seed on failure.
- Fuzz/property tests may generate broad inputs, but failing cases must be reproducible.
- Add fuzz tests for parsers, importers, geometry construction, and operations that consume complex user-authored data.

### Performance tests

Do not add fragile performance tests with tight timing thresholds.

Allowed:

- microbenchmarks that are not part of the normal pass/fail suite
- broad regression guards with generous thresholds for known expensive operations
- tests that assert algorithmic sanity, such as “does not hang on this previously pathological input”

Not allowed:

- tests that fail because a CI machine is temporarily slow
- exact frame-time assertions
- benchmarks mixed into normal correctness tests

### When modifying existing tests

- Do not delete or weaken a test just because it fails.
- First determine whether the behavior changed intentionally or a regression was introduced.
- If behavior changed intentionally, update the test to express the new behavior and explain why.
- If the test is brittle, rewrite it to assert the meaningful outcome instead of removing it.
- Keep tests readable; test code is production code.

### Agent workflow

When implementing a change:

1. Identify the user-visible behavior or invariant affected by the change.
2. Choose the smallest useful test level.
3. Add or update tests before or alongside the implementation.
4. Run the relevant local test target.
5. For geometry/parser/memory-sensitive changes, run sanitizer or fuzz/property tests when available.
6. Report exactly what was tested.
7. If no meaningful automated test is practical, state why and provide a concrete manual verification step.

A change is not better because it has more tests. It is better when the tests catch real regressions with minimal brittleness.
