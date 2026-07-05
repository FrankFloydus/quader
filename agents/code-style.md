# Code Style

Human guide for Quader C++ style. Executable authorities are root `.clang-format` and `.clang-tidy`.

For copyright/Doxygen, read `agents/documentation-policy.md`.

## Permission Lock

Do not run clang-format, clang-tidy, `check_format`, `check_clang_tidy`, `check_static_analysis`, related Python tools, raw clang tools, or any CTest/preset containing those gates unless the user gives immediate explicit permission in the current turn. A plan/checklist is not permission unless it says `run now without asking`.

## Formatting

- C++20.
- Tabs, width 4; continuation indent 8.
- No hard column limit.
- Preserve include blocks: project includes, C `.h`, other system headers.
- Do not hand-align comments/operands against clang-format.
- Do not format generated files, build output, fetched dependencies, or third-party code.

Authorized format check:

```powershell
cmake --build --preset qt-mingw-debug --target check_format
```

## Naming

Follow `.clang-tidy`:

- namespaces: `lower_case`
- classes/structs/enums: `CamelCase`
- functions/methods/variables/parameters: `lower_case`
- private/protected members: `lower_case_`
- constants: `kCamelCase`
- macros: `UPPER_CASE`
- Qt overrides keep Qt names, e.g. `resizeEvent`.

Authorized tidy/static checks:

```powershell
cmake --build --preset qt-mingw-debug --target check_clang_tidy
cmake --build --preset qt-mingw-debug --target check_static_analysis
```

## Scope

Do not weaken style, validation, file scope, naming, or warning rules so current code passes. If requested style enforcement causes broad churn or conflicts with an accepted plan, stop and report a `Workaround/Deviation Report`.
