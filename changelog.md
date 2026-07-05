# Changelog

All notable public-facing changes to Quader will be documented in this file.
This file feeds future user-visible "What's changed" release notes.

The format is based on Keep a Changelog 1.1.0, and this project uses an
unpadded `x.y.z` version format.

Do not record internal workflow, architecture-only, tooling, build, task-board,
or maintenance changes here unless they directly affect the user-facing
application. Use `dev-changelog.md` for the detailed engineering log.

## [Unreleased]

## [0.1.0] - 2026-07-05

### Added

- Initial Quader desktop prototype with a Qt Widgets shell, menu and toolbar
  actions, dockable editor panels, status messaging, and workspace persistence.
- bgfx-powered 3D viewport through the Crimson renderer boundary, including the
  prototype scene, grid/overlay presentation, camera navigation, and renderer
  diagnostics.
- Core editor foundations for documents, selection, command history, mesh
  validation, import/export services, and automated architecture/test checks.

[Unreleased]: https://github.com/FrankFloydus/quader/compare/v0.1.0...HEAD
[0.1.0]: https://github.com/FrankFloydus/quader/releases/tag/v0.1.0
