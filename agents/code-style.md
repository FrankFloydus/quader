# Code Style

This file is the human guide for Quader C++ style. The executable authorities
are the root `.clang-format` and `.clang-tidy` files.

For C++ copyright headers and Doxygen/API documentation, read
`agents/documentation-policy.md`; that policy is separate from clang-format and
clang-tidy style.

## Required Read

Read this file before writing, modifying, reviewing, or planning non-trivial
C++ source, tests, benchmarks, clang-format, clang-tidy, or style validation.
If this guide and the clang files disagree, follow the clang files and update
this guide in the same change.

## Formatting

- Use the repository `.clang-format`; do not hand-tune formatting by taste.
- C++ uses `Standard: c++20`.
- Indentation uses tabs with width 4. Continuation indentation is 8.
- There is no hard column limit. Prefer readable expressions over artificial
  wrapping.
- Include blocks are preserved. Keep quoted project includes before C `.h`
  headers, then other system headers.
- Do not align comments or operands manually when clang-format would undo it.

Check formatting with:

```powershell
cmake --build --preset qt-mingw-debug --target check_format
```

## Naming And Tidy Rules

Use the repository `.clang-tidy` profile. Important naming rules:

- namespaces: `lower_case`
- classes, structs, enums: `CamelCase`
- functions, methods, variables, parameters: `lower_case`
- private/protected members: `lower_case_`
- constants: `kCamelCase`
- macros: `UPPER_CASE`
- Qt event overrides keep Qt names, such as `resizeEvent` and `mousePressEvent`.

Check tidy diagnostics with:

```powershell
cmake --build --preset qt-mingw-debug --target check_clang_tidy
```

## Scope And Escalation

- Style checks cover Quader-owned code under `src`, `tests`, and `benchmarks`.
- Do not format, tidy, or rewrite generated files, build output, fetched
  dependencies, or third-party source trees.
- Do not weaken style, validation, file scope, naming, or warning rules merely
  so current code passes. Current code is migration evidence, not permission to
  lower the standard.
- If enforcing the requested style causes broad churn or conflicts with an
  accepted plan, stop and report a `Workaround/Deviation Report`.

Run the full static validation when style or quality-gate behavior changes:

```powershell
cmake --build --preset qt-mingw-debug --target check_static_analysis
```
