# Documentation Policy

Lean first-read for Quader documentation work. Full detailed examples and extended Doxygen guidance are preserved in `agents/reference/documentation-policy-full.md`; read it for large documentation passes, ambiguous API docs, or documentation-only audits.

## Core Rule

Project-owned C++ files must start with the Quader copyright block. Project-owned public/protected C++ APIs should be documented where first declared with Javadoc-style Doxygen.

Documentation maintenance may change comments, copyright headers, docs, and dev-changelog entries. It must not change source behavior, public API shape, build/test behavior, paused board files/state, versions, or public release notes unless explicitly requested.

## Sync Rule

If code changes a documented class, function, method, enum/value, variable, member, alias, template, option/result type, behavior, invariant, ownership rule, lifetime, threading note, performance note, side effect, or error behavior, update the existing documentation in the same edit.

Do not leave stale comments for later. If the correct updated docs are unclear after reading code/tests/architecture, stop and report ambiguity.

## Review Blocking Scope

Block review for:

- stale/contradictory existing documentation caused or exposed by the change;
- documentation-specific tasks;
- explicit documentation requirements in the accepted brief/plan.

Do not block a critical fix solely because newly added, previously undocumented code lacks new Doxygen. Report:

```text
Docs maintainer follow-up:
```

## File Scope

Applies to project-owned `.h`, `.hpp`, `.hh`, `.cpp`, `.cc`, `.cxx`, `.ipp`, `.inl` under source, tests, benchmarks, and support code.

Does not apply to generated files, build output, vendored/fetched third-party code, archived plans, or unclear ownership. If ownership is unclear, report instead of editing.

## Copyright Header

Every project-owned C++ source/header file begins with this exact block before any other content:

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

Do not add this to third-party code or replace third-party license headers.

## Doxygen Style

- Use `/** ... */`, `///`, and short inline `///<`.
- Do not use `/*!`, `//!`, or backslash commands in new docs.
- Use Javadoc tags: `@brief`, `@tparam`, `@param`, `@param[in]`, `@param[out]`, `@param[in, out]`, `@return`, `@throws`, `@exceptsafe`, `@overload`, `@see`.
- Use Markdown sparingly inside comments: backticks, short lists, paragraphs, stable links.
- Place API docs immediately before the declaration where the component is first declared.
- Headers document public/protected APIs. Source files document file-local helpers or implementation-only algorithms.

## Coverage

Document project-owned public/protected API surface:

- classes/structs;
- public/protected methods, constructors, destructors;
- free functions and templates;
- type aliases, typedefs, concepts if introduced;
- enums and enum values;
- non-private constants, variables, and data members;
- callbacks, handles, descriptors, options, results, settings/config types.

Private members need docs only for non-obvious ownership, lifetime, invariants, threading, performance, safety, cache/resource invalidation, or platform constraints.

## Content Rules

- Start with a short meaningful summary ending with a period.
- For functions, document meaningful parameters, return values, exceptions/error behavior, mutation, ownership, lifetime, invalid input, and side effects.
- For classes, document role, scope, invariants, ownership/lifetime, and thread-safety assumptions when relevant.
- For aliases/enums/constants, document semantic meaning, units, ranges, sentinel values, or ownership when not obvious.
- Do not document implementation diary content as public contract.
- Do not repeat symbol names without adding meaning.
- Do not invent behavior not guaranteed by code.

Preferred block order when sections apply:

```text
summary, extended summary, @tparam, @param, @return, @throws, @exceptsafe, related helpers/@see, notes, references, examples
```

## Implementation Comments

Use ordinary comments sparingly for non-obvious algorithms, platform workarounds, ordering constraints, lifetime/resource handoff, safety, or performance-sensitive details. They do not replace public API Doxygen.

## Documentation Maintenance Mode

`$quader-workflow` can directly handle docs-only maintenance. Do not create or require board tasks while the board pause is active.

Allowed direct work: copyright headers, Doxygen repair/additions, scoped docs audits, docs policy edits, dev-changelog entries for durable docs changes.

Forbidden direct work: behavior changes, API redesign, build/test semantics, board mutation, version changes, public `changelog.md`, unrelated formatting/tidy cleanup.

## Audit Scope

Use the narrowest useful scope:

1. explicit user scope;
2. active plan/review scope;
3. changed files from git diff/status;
4. recent implementation scope;
5. full audit only when explicitly requested.

Use `rg` file and declaration scans before reading many full files. Report skipped or ambiguous scope.

## Report Format

Documentation reports include: `Scope`, `Files inspected`, `Files changed`, policy checks, ambiguous/skipped items, dev-changelog status, verification.
