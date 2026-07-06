# Documentation Policy

This policy is the source of truth for Quader C++ source documentation,
copyright headers, and `$quader-workflow` documentation-maintenance mode.

The Doxygen style rules are based on the LSST C++ API documentation guidance,
adapted into Quader-owned policy. Useful reference sections:

- https://developer.lsst.io/cpp/api-docs.html#documentation-must-be-delimited-in-javadoc-style
- https://developer.lsst.io/cpp/api-docs.html#documentation-must-use-javadoc-style-tags
- https://developer.lsst.io/cpp/api-docs.html#documentation-should-use-markdown-for-formatting
- https://developer.lsst.io/cpp/api-docs.html#documentation-must-appear-where-a-component-is-first-declared
- https://developer.lsst.io/cpp/api-docs.html#common-structure-of-documentation-blocks

## Core Rule

Project-owned C++ files must have the Quader copyright block at the top.
Project-owned public/protected C++ APIs should be documented where they are
first declared using Javadoc-style Doxygen comments. Full documentation coverage
is maintained by `$quader-workflow` documentation-maintenance mode and
documentation-specific tasks; it is
not an automatic implementation-review blocker for newly added code.

Documentation maintenance is allowed to change comments and copyright headers.
It must not change behavior, public API shape, build semantics, tests, paused
board files/state, versions, or public release notes.

## Documentation Sync Rule

When an agent changes any project-owned C++ symbol that already has
documentation, it must update that documentation in the same edit.

This applies to documented:

- classes and structs,
- functions and methods,
- constructors and destructors,
- enum types and enum values,
- variables, constants, data members, aliases, templates, concepts, callbacks,
  handles, descriptors, options, results, and other named API elements,
- implementation comments that describe behavior, invariants, lifetime,
  ownership, threading, performance, side effects, or error handling.

Do not leave stale comments with a note to fix later. Do not create a separate
documentation cleanup task for symbol-level documentation that was made stale by
the current code change. Update the existing comment in place, near the
declaration or implementation it documents.

If the correct updated documentation cannot be stated confidently from the code,
tests, and active architecture docs, stop and report the ambiguity instead of
guessing.

Architecture plans and implementation prompts must call this out when the task
changes documented APIs. Reviews must reject work when code changed a documented
symbol but left the associated documentation stale, incomplete, or contradictory.

## Review Blocking Scope

Documentation review blockers are limited to stale or contradictory existing
documentation, documentation-specific tasks, and explicit documentation
requirements in the accepted task brief or active plan.

Architects and reviewers must not block an implementation solely because a newly
added file, class, function, variable, enum, or other previously undocumented
symbol lacks a new Doxygen note, when the change did not alter existing
documented code. Report that as:

```text
Docs maintainer follow-up:
```

Root may route that follow-up to `$quader-workflow` after the fix or feature is
accepted. Critical fixes must not be held back only for missing documentation on
new, previously undocumented code.

## File Scope

This policy applies to project-owned C++ files:

- `.h`
- `.hpp`
- `.hh`
- `.cpp`
- `.cc`
- `.cxx`
- `.ipp`
- `.inl`

It applies under project source, tests, benchmarks, and project-owned support
code. It does not apply to:

- Generated files.
- Build output.
- Vendored third-party source.
- Downloaded dependencies.
- Archived historical plans.
- Files whose ownership is unclear.

If ownership is unclear, report the file instead of editing it.

## Required Copyright Header

Every project-owned C++ source/header file must begin with this exact block:

```cpp
/*
 * This file is part of Quader.
 *
 * Copyright (c) 2026 Francesco Di Blasi.
 * All rights reserved.
 *
 * Unauthorized copying, modification, distribution, or use of this file,
 * in whole or in part, is prohibited without prior written permission.
 */
```

Placement rules:

- The block appears before `#pragma once`, includes, namespaces, comments, or
  any other content.
- Keep the text exact unless the user explicitly changes the legal text.
- Do not add this block to third-party code.
- Do not replace third-party license headers.

## Doxygen Delimiters

Use Javadoc-style Doxygen delimiters only:

- Multi-line documentation blocks begin with `/**` and end with `*/`.
- Single-line documentation blocks begin with `///`.
- Short inline member or enum-value comments may use `///<`.
- Do not use `/*!` or `//!`.
- Do not use backslash Doxygen commands such as `\param`, `\return`, or `\see`
  in new documentation.

Multi-line delimiters should be on their own lines:

```cpp
/**
 * Compute the normalized direction.
 *
 * @param value Vector to normalize.
 * @return Unit vector, or the zero vector when `value` is degenerate.
 */
Vec3 normalize(Vec3 value);
```

Avoid this style for public APIs:

```cpp
/** Compute the normalized direction. */
Vec3 normalize(Vec3 value);
```

One-line `///` comments are acceptable for very small APIs, enum values, aliases,
or constants, but public functions/classes usually deserve a multi-line block
when they have parameters, return values, invariants, or notable behavior.

## Tags And Formatting

Use Javadoc-style tags:

- `@brief`
- `@tparam`
- `@param`
- `@param[in]`
- `@param[out]`
- `@param[in, out]`
- `@return`
- `@throws`
- `@exceptsafe`
- `@overload`
- `@see`

Use Markdown inside documentation comments when formatting is useful:

- Backticks for code identifiers.
- Lists for short related points.
- Paragraphs for notes.
- Links only when a stable reference is genuinely useful.

Prefer Doxygen built-in formatting or HTML only when Markdown cannot express the
needed structure.

## Documentation Placement

Documentation must appear where the component is first declared.

Normal placement:

- Classes, structs, aliases, enums, functions, and public/protected methods are
  documented in headers.
- Source files document only file-local helpers, implementation-only types, or
  complex algorithms that are not declared in headers.

The block must appear immediately before the declaration and use the same
indentation as the declaration:

```cpp
/**
 * Sum values in a collection.
 *
 * @param values Values to add.
 * @return Arithmetic sum of `values`, or zero when empty.
 */
double sum(std::span<const double> values);
```

Do not put API documentation inside the function body:

```cpp
double sum(std::span<const double> values)
{
	/**
	 * Wrong: this is not where the API is declared.
	 */
}
```

## Required API Coverage

Document all project-owned public or protected API surface:

- Classes.
- Structs.
- Public and protected methods.
- Free functions.
- Function templates.
- Class templates.
- Concepts if introduced later.
- Type aliases and typedefs.
- Enumerations.
- Enum values.
- Non-private constants.
- Non-private variables.
- Non-private data members.
- Public callback types.
- Handles.
- Result types.
- Descriptor, options, settings, and configuration types.

Private members do not require documentation unless they encode non-obvious:

- Ownership.
- Lifetime.
- Invariants.
- Threading constraints.
- Performance constraints.
- Safety constraints.
- Cache/resource invalidation behavior.

## Common Documentation Block Order

When sections apply, keep this order:

1. Short summary.
2. Extended summary.
3. Template parameters with `@tparam`.
4. Function/method parameters with `@param`.
5. Return value with `@return`.
6. Exceptions with `@throws`.
7. Exception safety with `@exceptsafe`.
8. Related helper functions.
9. Initializer declaration for constants, when useful.
10. See also references with `@see`.
11. Notes.
12. References.
13. Examples.

Do not include empty sections. Keep documentation close to the declaration:
long design discussions belong in architecture docs, not in every API block.

## Short Summary

Every documented component starts with a short summary.

Rules:

- Keep it brief enough to fit on one line when practical.
- End a short summary with a period.
- For functions and methods, prefer imperative voice: describe what the caller
  asks the API to do.
- For getters, state-like methods, aliases, and value objects, value phrasing is
  acceptable when it reads naturally.
- Do not repeat the symbol name without adding meaning.

Good:

```cpp
/// Convert a document object into a render object payload.
RenderObject makeRenderObject(const SceneObject& object);
```

Weak:

```cpp
/// makeRenderObject function.
RenderObject makeRenderObject(const SceneObject& object);
```

If the summary must span multiple lines, use `@brief` and end the brief section
with a blank line:

```cpp
/**
 * @brief Build a render snapshot from the current document state and viewport
 * settings.
 *
 * The snapshot owns only renderer-facing data and does not expose document
 * mutation APIs.
 */
FrameSnapshot buildFrameSnapshot(const Document& document, ViewportSettings settings);
```

## Extended Summary

Use an extended summary when the short summary is not enough.

Rules:

- Keep it to a short paragraph, normally three sentences or fewer.
- Explain role, scope, ownership, lifetime, invariants, or major constraints.
- Do not duplicate detailed parameter or return descriptions.
- Put deep background, algorithms, and usage examples in Notes or Examples only
  when necessary.

## Template Parameters

Use `@tparam` for template parameters.

Rules:

- List template parameters in declaration order.
- Describe the expected type or callable contract.
- Do not document default values unless the default has a semantic reason that
  is not obvious.
- If a description wraps, indent continuation lines under the description.
- Consecutive template parameters with exactly the same role may be combined.

Example:

```cpp
/**
 * Store values with deterministic lookup order.
 *
 * @tparam T Value type stored by the table.
 * @tparam Comparator Callable that defines strict weak ordering for `T`.
 */
template <typename T, typename Comparator = std::less<T>>
class LookupTable;
```

For partial specializations:

- Do not repeat unchanged template-parameter docs if the full template already
  defines the contract.
- Add `@see` to the full template when useful.
- Redocument parameters when the specialization changes restrictions or meaning.

## Function And Method Parameters

Use `@param` for function and method parameters.

Rules:

- List parameters in signature order.
- Keep descriptions in sync with names and meaning.
- Use `@param[out]` for output parameters.
- Use `@param[in, out]` for input/output parameters.
- `@param[in]` is optional when all parameters are inputs.
- If several consecutive parameters have the same description, they may be
  combined.
- Do not write multi-paragraph parameter descriptions. Put deeper discussion in
  Notes and reference it from the parameter description.

Example:

```cpp
/**
 * Compute summary statistics for a sample.
 *
 * @param[out] mean Arithmetic mean, or `NaN` when `values` is empty.
 * @param[out] stddev Sample standard deviation, or `NaN` when fewer than two
 *     values are available.
 * @param values Values to analyze.
 */
void computeStatistics(double& mean, double& stddev, std::span<const double> values);
```

Combined parameters:

```cpp
/**
 * Create a point in viewport coordinates.
 *
 * @param x, y Viewport pixel coordinates.
 */
ViewportPoint makeViewportPoint(int x, int y);
```

## Return Values

Use `@return` when the return value is meaningful.

Rules:

- Explain what is returned.
- Include important empty, invalid, or fallback cases.
- Do not use `@return` for `void`.
- Mention ownership/lifetime when returning references, pointers, handles, or
  views.

Example:

```cpp
/**
 * Find an object by stable document id.
 *
 * @param id Stable object identifier.
 * @return Pointer to the object, or `nullptr` when the id is not present.
 */
const SceneObject* findObject(ObjectId id) const;
```

## Exceptions And Error Behavior

Use `@throws` only when the API throws documented exceptions.

Rules:

- Name the exception type.
- State the condition that triggers it.
- If the project API returns `Result`, `Expected`, diagnostics, or error enums
  instead of throwing, document that under `@return` or Notes.

Use `@exceptsafe` when exception-safety behavior is part of the contract:

```cpp
/**
 * Commit a command into history.
 *
 * @param command Command to execute and store.
 * @return Result describing success or failure.
 *
 * @exceptsafe Strong exception guarantee: failed commands do not change history.
 */
CommandResult commit(std::unique_ptr<ICommand> command);
```

## Classes And Structs

Document classes and structs immediately before the declaration.

Recommended sections:

1. Short summary.
2. Extended summary.
3. `@tparam` if templated.
4. `@see` when useful.
5. Notes when there are important invariants or usage constraints.
6. Examples when the API is not obvious.

Class docs describe the type as a whole, not every method.

Example:

```cpp
/**
 * Owns renderer-facing resources for one viewport frame.
 *
 * Frame resources are recreated when the swapchain generation changes. The
 * object is not thread-safe and must be used from the render thread.
 */
class GpuFrameResources
{
public:
	...
};
```

## Type Aliases And Typedefs

Document public aliases and typedefs.

Rules:

- A short `///` comment is usually enough.
- Explain semantic meaning, not just the underlying type.
- Mention units or ownership when the alias hides important meaning.

Example:

```cpp
/// Stable handle used to refer to a material instance.
using MaterialHandle = TypedHandle<MaterialTag>;
```

## Enumerations

Document the enum as a type and document each value.

Rules:

- Enum documentation explains what decision space the enum represents.
- Every value gets either a preceding `///` block/comment or a short inline
  `///<` comment.
- Use preceding comments when descriptions are long.
- Use inline comments only when they are short and fit naturally on one line.

Example:

```cpp
/**
 * Backend selected for the Crimson renderer.
 *
 * Values describe the requested graphics API. The runtime may fall back when
 * the selected backend is unavailable.
 */
enum class RendererBackend
{
	/// Let Crimson choose the best available backend.
	Auto,
	/// Use Vulkan when supported by the runtime.
	Vulkan,
	/// Use Direct3D 11 when supported by the runtime.
	Direct3D11,
};
```

Inline style for short descriptions:

```cpp
enum class AlphaMode
{
	Opaque,      ///< Ignore the alpha channel.
	Mask,        ///< Discard fragments below the cutoff.
	Blend,       ///< Blend fragments using alpha.
};
```

## Methods And Free Functions

Document all public/protected methods and all project-owned free functions.

Recommended sections:

1. Short summary.
2. Extended summary, when useful.
3. `@tparam`, if templated.
4. `@param`.
5. `@return`.
6. `@throws`, if applicable.
7. `@exceptsafe`, if relevant.
8. Related helpers, `@see`, Notes, References, Examples when useful.

Rules:

- Explain the behavior from the caller's perspective.
- Mention ownership, lifetime, mutation, threading, and invalidation when
  relevant.
- For command/document/tool APIs, mention whether the function mutates document
  state, records history, or only prepares data.
- For renderer APIs, mention whether data is CPU-side, GPU-owned, frame-local,
  or persistent.

## Overloads

Use `@overload` only when the overload is a convenience form whose behavior is
obvious from a fully documented overload.

Rules:

- Fully document the primary overload.
- Use `@overload` sparingly.
- Do not use `@overload` when the overload changes ownership, validation,
  lifetime, errors, or performance characteristics.

Example:

```cpp
/**
 * Submit an overlay line list.
 *
 * @param lines Lines to render in viewport space.
 * @return Submission result.
 */
OverlayResult submitLines(std::span<const OverlayLine> lines);

/**
 * Submit one overlay line.
 *
 * @overload
 */
OverlayResult submitLine(const OverlayLine& line);
```

## Constants, Variables, And Data Members

Document all non-private constants, variables, and data members.

Rules:

- A short summary is often enough.
- Include units, ranges, default meaning, or sentinel behavior when relevant.
- Use `///<` only for short inline descriptions.
- Prefer a full documentation block when the description is long.

Examples:

```cpp
/// Maximum number of diagnostic entries retained per viewport.
inline constexpr std::size_t MaxDiagnosticEntries = 256;
```

```cpp
struct ViewportSettings
{
	float exposure = 1.0F; ///< Linear exposure multiplier applied before tone mapping.
};
```

## Notes, References, And Examples

Use these sections only when they add real value:

- Notes explain non-obvious contracts, invariants, pitfalls, or rationale.
- References point to stable specs, architecture docs, algorithms, or papers.
- Examples show realistic usage when the API is difficult to understand from
  the signature and parameter docs alone.

Do not overload API docs with implementation diary content. If a long
architecture explanation is needed, write or update an architecture doc and link
it with `@see`.

## Implementation Comments

Implementation comments are not API documentation.

Use ordinary comments sparingly for:

- Non-obvious algorithms.
- Counterintuitive ordering constraints.
- Platform workarounds.
- Lifetime/resource handoff.
- Safety or performance-sensitive details.

Implementation comments do not replace required Doxygen documentation for public
or protected API declarations.

## Quality Bar

Good documentation:

- Helps a caller use the API correctly.
- States ownership and lifetime when not obvious.
- States mutation and side effects when relevant.
- States invalid input behavior.
- Names result/error behavior.
- Stays synchronized with code and tests.
- Uses concise language.

Bad documentation:

- Repeats the symbol name without meaning.
- Invents behavior not guaranteed by code.
- Hides uncertainty.
- Documents implementation details as public contract.
- Adds noise to private obvious members.
- Uses obsolete Doxygen style.

If the code behavior is ambiguous, inspect the declaration, implementation,
tests, and active architecture docs enough to be accurate. If it is still
ambiguous, report it instead of guessing.

## Documentation Maintenance Mode

`$quader-workflow` handles documentation-only maintenance directly in the same
thread. It does not create or require board tasks while the board pause is
active.

Allowed direct work:

- Add or repair copyright headers in project-owned C++ files.
- Add or repair Doxygen comments according to this policy.
- Audit scoped files for missing copyright headers or API documentation.
- Update this policy or supporting documentation when the user requests policy
  changes.
- Update `dev-changelog.md` after durable documentation maintenance.

Forbidden direct work:

- Source behavior changes.
- Public API redesign.
- CMake/build/test behavior changes.
- Board mutations.
- Version changes.
- Public `changelog.md` changes.
- Formatting or clang-tidy cleanup unrelated to documentation.

If requested work exceeds documentation maintenance, stop and return a
deviation report to main/root.

## Audit Scope Heuristic

Avoid full codebase scans by default. Use the narrowest useful scope:

1. **Explicit scope**: files, folders, legacy task ids explicitly cited by the
   user, symbols, or active assignment
   scope named by the user.
2. **Active plan/review scope**: files listed in an active plan, architect
   review, current executor report, or explicitly cited legacy board context.
3. **Changed-file scope**: project-owned C++ files from `git status --short`,
   `git diff --name-only`, and `git diff --cached --name-only`.
4. **Recent implementation scope**: files mentioned in the latest relevant
   `dev-changelog.md` entries or agent report when no git diff is useful.
5. **Full audit**: only when the user explicitly asks for a full codebase
   documentation audit.

For audits, use file-list and pattern scans before reading full files:

```powershell
rg --files -g "*.h" -g "*.hpp" -g "*.hh" -g "*.cpp" -g "*.cc" -g "*.cxx" -g "*.ipp" -g "*.inl" src tests benchmarks
rg -n "^(class|struct|enum class|enum |using |typedef )" src tests benchmarks
```

Function discovery needs care because C++ declarations are complex. Prefer
candidate scans followed by local file reads rather than broad full-file
analysis:

```powershell
rg -n "^[A-Za-z_:<>~*&][A-Za-z0-9_:<>~*&\\s]+\\s+[A-Za-z_][A-Za-z0-9_]*\\s*\\(" src tests benchmarks
```

Read only candidate files and nearby declarations needed to decide or fix
documentation. Report skipped full-audit scope explicitly.

## Report Format

Documentation-maintainer reports must include:

- `Scope:` targeted, active-task, changed-files, recent, or full.
- `Files inspected:`.
- `Files changed:`.
- `Policy checks:` copyright, delimiter style, tag style, placement, public API
  coverage.
- `Ambiguous or skipped:`.
- `Dev changelog:` entry written or reason not needed.
- `Verification:`.
