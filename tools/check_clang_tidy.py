#!/usr/bin/env python3
"""Validate Quader-owned translation units with clang-tidy."""

from __future__ import annotations

import argparse
import concurrent.futures
import json
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path
from typing import NamedTuple


SOURCE_EXTENSIONS = {".c", ".cc", ".cpp", ".cxx"}
DEFAULT_SCAN_ROOTS = ("src", "tests", "benchmarks")
EXCLUDED_PARTS = {"_deps", "CMakeFiles", "build"}


class TidyResult(NamedTuple):
    relative: str
    returncode: int
    output: str


def normalized_relative(path: Path, root: Path) -> str:
    return path.relative_to(root).as_posix()


def find_tool(name: str, override: str | None) -> str:
    if override:
        return override
    found = shutil.which(name)
    if found is None:
        raise RuntimeError(f"{name} was not found on PATH")
    return found


def is_owned_translation_unit(path: Path, root: Path) -> bool:
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


def load_compile_commands(path: Path) -> list[Path]:
    with path.open("r", encoding="utf-8") as file:
        commands = json.load(file)

    files = []
    for entry in commands:
        file_value = entry.get("file")
        if not file_value:
            continue
        file_path = Path(file_value)
        if not file_path.is_absolute():
            directory = Path(entry.get("directory", path.parent))
            file_path = directory / file_path
        files.append(file_path.resolve())
    return files


def select_translation_units(all_files: set[Path], root: Path, explicit_paths: list[Path]) -> list[Path]:
    if not explicit_paths:
        return sorted(path for path in all_files if is_owned_translation_unit(path, root))

    selected = []
    for path in explicit_paths:
        resolved = (path if path.is_absolute() else root / path).resolve()
        if not is_owned_translation_unit(resolved, root):
            raise RuntimeError(f"refusing non-project translation unit: {path}")
        if resolved not in all_files:
            raise RuntimeError(f"translation unit is missing from compile_commands.json: {path}")
        selected.append(resolved)
    return sorted(set(selected))


def absolute_header_filter(root: Path) -> str:
    escaped_root = re.escape(root.as_posix().rstrip("/")).replace("/", r"[/\\]")
    allowed_roots = "|".join(re.escape(part) for part in DEFAULT_SCAN_ROOTS)
    return f"^{escaped_root}[/\\\\]({allowed_roots})[/\\\\]"


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path.cwd(), help="Repository root.")
    parser.add_argument("--build-dir", type=Path, required=True, help="CMake build directory containing compile_commands.json.")
    parser.add_argument("--clang-tidy", default=None, help="Path to clang-tidy.")
    parser.add_argument("--fix", action="store_true", help="Apply clang-tidy fixes to Quader-owned translation units.")
    parser.add_argument("--jobs", type=int, default=0, help="Parallel clang-tidy jobs. Use 0 for all logical cores.")
    parser.add_argument("paths", nargs="*", type=Path, help="Optional translation units to check instead of all owned files.")
    return parser.parse_args(argv)


def job_count(requested: int, file_count: int, fix: bool) -> int:
    if requested < 0:
        raise RuntimeError("--jobs must be 0 or greater")
    if fix:
        return 1
    available = os.cpu_count() or 1
    jobs = available if requested == 0 else requested
    return max(1, min(jobs, file_count))


def run_clang_tidy(clang_tidy: str, build_dir: Path, config: Path, header_filter: str, root: Path, path: Path, fix: bool) -> TidyResult:
    relative = normalized_relative(path, root)
    command = [
        clang_tidy,
        str(path),
        f"-p={build_dir}",
        f"--config-file={config}",
        f"--header-filter={header_filter}",
        "--quiet",
    ]
    if fix:
        command.extend(["-fix", "-fix-errors", "--format-style=file"])
    result = subprocess.run(
        command,
        cwd=root,
        env=os.environ.copy(),
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        check=False,
        text=True,
    )
    return TidyResult(relative=relative, returncode=result.returncode, output=result.stdout)


def main(argv: list[str]) -> int:
    args = parse_args(argv)
    root = args.root.resolve()
    build_dir = args.build_dir.resolve()
    config = root / ".clang-tidy"
    compile_commands = build_dir / "compile_commands.json"

    if not config.is_file():
        print(f"missing clang-tidy config: {config}", file=sys.stderr)
        return 2
    if not compile_commands.is_file():
        print(f"missing compile database: {compile_commands}", file=sys.stderr)
        print("Run cmake --preset qt-mingw-debug before clang-tidy validation.", file=sys.stderr)
        return 2

    try:
        clang_tidy = find_tool("clang-tidy", args.clang_tidy)
    except RuntimeError as error:
        print(error, file=sys.stderr)
        return 2

    all_files = {path.resolve() for path in load_compile_commands(compile_commands)}
    try:
        files = select_translation_units(all_files, root, args.paths)
    except RuntimeError as error:
        print(error, file=sys.stderr)
        return 2
    if not files:
        print("no Quader-owned translation units found in compile_commands.json", file=sys.stderr)
        return 2
    try:
        jobs = job_count(args.jobs, len(files), args.fix)
    except RuntimeError as error:
        print(error, file=sys.stderr)
        return 2
    if args.fix and args.jobs != 1:
        print("clang-tidy --fix is serialized to avoid concurrent edits to shared project headers.", flush=True)

    failures = []
    header_filter = absolute_header_filter(root)
    print(f"clang-tidy validation using {jobs} job(s) for {len(files)} translation units", flush=True)
    print(f"clang-tidy header filter: {header_filter}", flush=True)
    with concurrent.futures.ThreadPoolExecutor(max_workers=jobs) as executor:
        futures = [
            executor.submit(run_clang_tidy, clang_tidy, build_dir, config, header_filter, root, path, args.fix)
            for path in files
        ]
        for future in concurrent.futures.as_completed(futures):
            result = future.result()
            status = "ok" if result.returncode == 0 else "failed"
            print(f"clang-tidy [{status}]: {result.relative}", flush=True)
            if result.output:
                print(result.output, end="" if result.output.endswith("\n") else "\n", flush=True)
            if result.returncode != 0:
                failures.append(result.relative)

    if failures:
        print("clang-tidy validation failed for:")
        for path in failures:
            print(f"  {path}")
        return 1

    print(f"clang-tidy validation passed for {len(files)} translation units")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
