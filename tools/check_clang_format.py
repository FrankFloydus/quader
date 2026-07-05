#!/usr/bin/env python3
"""Validate Quader-owned C and C++ files with clang-format."""

from __future__ import annotations

import argparse
import shutil
import subprocess
import sys
from pathlib import Path
from xml.etree import ElementTree


SOURCE_EXTENSIONS = {".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".hxx", ".ipp", ".inl"}
DEFAULT_SCAN_ROOTS = ("src", "tests", "benchmarks")
EXCLUDED_PARTS = {"_deps", "CMakeFiles", "build"}


def normalized_relative(path: Path, root: Path) -> str:
    return path.relative_to(root).as_posix()


def find_tool(name: str, override: str | None) -> str:
    if override:
        return override
    found = shutil.which(name)
    if found is None:
        raise RuntimeError(f"{name} was not found on PATH")
    return found


def is_owned_source_file(path: Path, root: Path) -> bool:
    if path.suffix not in SOURCE_EXTENSIONS:
        return False
    try:
        relative = path.resolve().relative_to(root)
    except ValueError:
        return False
    if not relative.parts:
        return False
    if relative.parts[0] not in DEFAULT_SCAN_ROOTS:
        return False
    return not any(part in EXCLUDED_PARTS for part in relative.parts)


def iter_source_files(root: Path, explicit_paths: list[Path]) -> list[Path]:
    if explicit_paths:
        files = []
        for path in explicit_paths:
            resolved = (path if path.is_absolute() else root / path).resolve()
            if resolved.is_file() and is_owned_source_file(resolved, root):
                files.append(resolved)
            elif resolved.suffix in SOURCE_EXTENSIONS:
                raise RuntimeError(f"refusing non-project source path: {path}")
        return sorted(files)

    files = []
    for relative_root in DEFAULT_SCAN_ROOTS:
        source_root = root / relative_root
        if not source_root.exists():
            continue
        for path in source_root.rglob("*"):
            if path.is_file() and is_owned_source_file(path, root):
                files.append(path)
    return sorted(files)


def has_replacements(xml_text: str) -> bool:
    root = ElementTree.fromstring(xml_text)
    return any(child.tag == "replacement" for child in root)


def check_file(clang_format: str, path: Path, fix: bool) -> bool:
    if fix:
        result = subprocess.run(
            [clang_format, "-i", str(path)],
            check=False,
            capture_output=True,
            text=True,
        )
        if result.returncode != 0:
            if result.stdout:
                print(result.stdout, end="")
            if result.stderr:
                print(result.stderr, end="", file=sys.stderr)
            return False
        return True

    result = subprocess.run(
        [clang_format, "--output-replacements-xml", str(path)],
        check=False,
        capture_output=True,
        text=True,
    )
    if result.returncode != 0:
        if result.stdout:
            print(result.stdout, end="")
        if result.stderr:
            print(result.stderr, end="", file=sys.stderr)
        return False
    return not has_replacements(result.stdout)


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path.cwd(), help="Repository root.")
    parser.add_argument("--clang-format", default=None, help="Path to clang-format.")
    parser.add_argument("--fix", action="store_true", help="Apply clang-format to Quader-owned files.")
    parser.add_argument("paths", nargs="*", type=Path, help="Optional files to check instead of the default roots.")
    return parser.parse_args(argv)


def main(argv: list[str]) -> int:
    args = parse_args(argv)
    root = args.root.resolve()
    config = root / ".clang-format"
    if not config.is_file():
        print(f"missing clang-format config: {config}", file=sys.stderr)
        return 2

    try:
        clang_format = find_tool("clang-format", args.clang_format)
    except RuntimeError as error:
        print(error, file=sys.stderr)
        return 2

    try:
        files = iter_source_files(root, args.paths)
    except RuntimeError as error:
        print(error, file=sys.stderr)
        return 2
    if not files:
        print("no C/C++ files found for clang-format validation")
        return 0

    failures = []
    for path in files:
        if not check_file(clang_format, path, args.fix):
            failures.append(normalized_relative(path, root))

    if failures:
        print("clang-format validation failed for:")
        for path in failures:
            print(f"  {path}")
        print("Run clang-format with the repository .clang-format before retrying.")
        return 1

    action = "updated" if args.fix else "validation passed for"
    print(f"clang-format {action} {len(files)} files")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
