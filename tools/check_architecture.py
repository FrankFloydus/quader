#!/usr/bin/env python3
"""Validate Quader's current source include boundaries."""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path


SOURCE_EXTENSIONS = {".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".hxx", ".inl"}

INCLUDE_PATTERN = re.compile(r"^\s*#\s*include\s*[<\"]([^>\"]+)[>\"]")

QT_WIDGETS_HEADERS = {
    "QAbstractButton",
    "QAbstractItemDelegate",
    "QAbstractItemView",
    "QAction",
    "QApplication",
    "QBoxLayout",
    "QCheckBox",
    "QComboBox",
    "QDialog",
    "QDockWidget",
    "QFormLayout",
    "QFrame",
    "QFileDialog",
    "QGroupBox",
    "QLabel",
    "QLayout",
    "QLineEdit",
    "QMainWindow",
    "QMenu",
    "QMenuBar",
    "QMessageBox",
    "QPushButton",
    "QScrollArea",
    "QSplitter",
    "QStatusBar",
    "QStyle",
    "QStyleFactory",
    "QStyleOption",
    "QToolBar",
    "QToolButton",
    "QTreeView",
    "QTreeWidget",
    "QVBoxLayout",
    "QWidget",
}

QT_WIDGETS_ALLOWED_PREFIXES = (
    "src/app/",
    "src/ui/",
)

QT_WIDGETS_ALLOWED_FILES = {
    "src/main.cpp",
}

GRAPHICS_RUNTIME_PREFIXES = (
    "bgfx/",
    "bimg/",
    "bx/",
)

GRAPHICS_RUNTIME_ALLOWED_PREFIXES = (
    "src/crimson/gpu/",
)

GRAPHICS_RUNTIME_TEMPORARY_ALLOWLIST = {}

IO_PREFIXES = (
    "src/io/",
)

IO_GLTF_PREFIXES = (
    "src/io/gltf/",
)

IO_CRIMSON_PUBLIC_INCLUDE_PREFIXES = (
    "crimson/material/",
    "src/crimson/material/",
)

IO_CRIMSON_PUBLIC_INCLUDE_FILES = {
    "crimson/renderer_types.hpp",
    "src/crimson/renderer_types.hpp",
}

GLM_ALLOWED_FILES = {
    "src/math/detail/glm_adapter.hpp",
    "src/math/detail/glm_config.hpp",
}

OPENMESH_ALLOWED_PREFIXES = (
    "src/mesh/core/detail/",
)

OPENMESH_ALLOWED_FILES = {
    "src/mesh/core/polyhedron.cpp",
    "src/mesh/core/mesh_validation.cpp",
}

GLM_NAMESPACE_PATTERN = re.compile(r"\bglm::")
GLM_MACRO_PATTERN = re.compile(r"\bGLM_[A-Za-z0-9_]*\b")


def normalized_relative(path: Path, root: Path) -> str:
    return path.relative_to(root).as_posix()


def is_allowed(relative_path: str, allowed_prefixes: tuple[str, ...], allowed_files: set[str]) -> bool:
    return relative_path in allowed_files or relative_path.startswith(allowed_prefixes)


def is_qt_widgets_include(header: str) -> bool:
    normalized = header.replace("\\", "/")
    if normalized.startswith("QtWidgets/"):
        return True
    return normalized.split("/")[-1] in QT_WIDGETS_HEADERS


def is_graphics_runtime_include(header: str) -> bool:
    normalized = header.replace("\\", "/")
    return normalized.startswith(GRAPHICS_RUNTIME_PREFIXES)


def is_glm_include(header: str) -> bool:
    normalized = header.replace("\\", "/")
    return normalized.startswith("glm/")


def is_openmesh_include(header: str) -> bool:
    normalized = header.replace("\\", "/")
    return normalized.startswith("OpenMesh/")


def is_glm_allowed(relative_path: str) -> bool:
    return relative_path in GLM_ALLOWED_FILES


def is_ui_project_include(header: str) -> bool:
    normalized = header.replace("\\", "/")
    return normalized.startswith("ui/") or normalized.startswith("src/ui/")


def is_crimson_gpu_project_include(header: str) -> bool:
    normalized = header.replace("\\", "/")
    return normalized.startswith("crimson/gpu/") or normalized.startswith("src/crimson/gpu/")


def is_crimson_project_include(header: str) -> bool:
    normalized = header.replace("\\", "/")
    return normalized.startswith("crimson/") or normalized.startswith("src/crimson/")


def is_allowed_io_crimson_include(header: str) -> bool:
    normalized = header.replace("\\", "/")
    return normalized in IO_CRIMSON_PUBLIC_INCLUDE_FILES or normalized.startswith(IO_CRIMSON_PUBLIC_INCLUDE_PREFIXES)


def iter_source_files(root: Path):
    src_root = root / "src"
    for path in src_root.rglob("*"):
        if path.is_file() and path.suffix in SOURCE_EXTENSIONS:
            yield path


def iter_glm_checked_files(root: Path):
    for source_root in (root / "src", root / "tests"):
        if not source_root.exists():
            continue
        for path in source_root.rglob("*"):
            if path.is_file() and path.suffix in SOURCE_EXTENSIONS:
                yield path


def check_file(path: Path, root: Path):
    relative_path = normalized_relative(path, root)
    violations: list[str] = []
    temporary_allowlist_hits: list[str] = []

    for line_number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), start=1):
        match = INCLUDE_PATTERN.match(line)
        if not match:
            continue

        header = match.group(1)
        location = f"{relative_path}:{line_number}"

        if is_qt_widgets_include(header) and not is_allowed(
            relative_path, QT_WIDGETS_ALLOWED_PREFIXES, QT_WIDGETS_ALLOWED_FILES
        ):
            violations.append(
                f"{location}: Qt Widgets include '{header}' is only allowed in src/ui, src/app, "
                "or the current prototype UI files"
            )

        if relative_path.startswith(IO_PREFIXES) and is_ui_project_include(header):
            violations.append(f"{location}: src/io must not include UI header '{header}'")

        if relative_path.startswith(IO_PREFIXES) and is_crimson_gpu_project_include(header):
            violations.append(f"{location}: src/io must not include Crimson GPU header '{header}'")

        if relative_path.startswith(IO_PREFIXES) and is_crimson_project_include(header):
            if not relative_path.startswith(IO_GLTF_PREFIXES) or not is_allowed_io_crimson_include(header):
                violations.append(
                    f"{location}: src/io may only include Crimson public material schemas through "
                    f"src/io/gltf adapters, not '{header}'"
                )

        if is_graphics_runtime_include(header):
            allowed_by_prefix = relative_path.startswith(GRAPHICS_RUNTIME_ALLOWED_PREFIXES)
            allowlist_reason = GRAPHICS_RUNTIME_TEMPORARY_ALLOWLIST.get(relative_path)
            if allowed_by_prefix:
                continue
            if allowlist_reason:
                temporary_allowlist_hits.append(f"{location}: {header} ({allowlist_reason})")
                continue
            violations.append(
                f"{location}: graphics-runtime include '{header}' is only allowed under src/crimson/gpu"
            )

        if is_openmesh_include(header) and not is_allowed(
            relative_path, OPENMESH_ALLOWED_PREFIXES, OPENMESH_ALLOWED_FILES
        ):
            violations.append(
                f"{location}: OpenMesh include '{header}' is only allowed in mesh-core private "
                "detail files or approved mesh-core implementation files"
            )

    return violations, temporary_allowlist_hits


def check_glm_leakage_file(path: Path, root: Path):
    relative_path = normalized_relative(path, root)
    violations: list[str] = []
    allowed = is_glm_allowed(relative_path)

    for line_number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), start=1):
        match = INCLUDE_PATTERN.match(line)
        location = f"{relative_path}:{line_number}"

        if match and is_glm_include(match.group(1)) and not allowed:
            violations.append(
                f"{location}: direct GLM include '{match.group(1)}' is only allowed in src/math/detail"
            )

        if allowed:
            continue

        if GLM_NAMESPACE_PATTERN.search(line):
            violations.append(f"{location}: glm:: usage is only allowed in src/math/detail")

        macro_match = GLM_MACRO_PATTERN.search(line)
        if macro_match:
            violations.append(
                f"{location}: GLM macro '{macro_match.group(0)}' is only allowed in src/math/detail"
            )

    return violations


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path.cwd(), help="Repository root")
    args = parser.parse_args()

    root = args.root.resolve()
    violations: list[str] = []
    temporary_allowlist_hits: list[str] = []

    for path in iter_source_files(root):
        file_violations, file_allowlist_hits = check_file(path, root)
        violations.extend(file_violations)
        temporary_allowlist_hits.extend(file_allowlist_hits)

    for path in iter_glm_checked_files(root):
        violations.extend(check_glm_leakage_file(path, root))

    if violations:
        print("Architecture boundary check failed:", file=sys.stderr)
        for violation in violations:
            print(f"  - {violation}", file=sys.stderr)
        if temporary_allowlist_hits:
            print("\nTemporary graphics-runtime allowlist entries still active:", file=sys.stderr)
            for hit in temporary_allowlist_hits:
                print(f"  - {hit}", file=sys.stderr)
        return 1

    print("Architecture boundary checks passed.")
    if temporary_allowlist_hits:
        print("Temporary graphics-runtime allowlist entries still active:")
        for hit in temporary_allowlist_hits:
            print(f"  - {hit}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
