#!/usr/bin/env python3
"""Archive and launch local Quader development build snapshots."""

from __future__ import annotations

import argparse
import json
import os
import shutil
import subprocess
import sys
from datetime import datetime, timezone
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
VERSION_PATH = ROOT / "VERSION"
DEV_VERSION_PATH = ROOT / "DEV_VERSION"
ARCHIVE_ROOT = Path(os.environ.get("QUADER_DEV_BUILDS_ROOT", ROOT.parent / "quader-dev-builds")).expanduser().resolve()
DEFAULT_PRESET = "qt-mingw-debug"
APP_EXE_CANDIDATES = ("quader.exe", "QuaderQtBgfx.exe")
RUNTIME_DIRS = (
    ".qt",
    "compiled_shaders",
    "resources",
    "shaders",
    "generic",
    "iconengines",
    "imageformats",
    "networkinformation",
    "platforms",
    "styles",
    "tls",
    "translations",
)


class DevBuildError(RuntimeError):
    pass


def read_public_version() -> str:
    version = VERSION_PATH.read_text(encoding="utf-8").strip()
    if not version:
        raise DevBuildError(f"{VERSION_PATH} is empty")
    return version


def read_dev_counter() -> int:
    if not DEV_VERSION_PATH.exists():
        return 0

    value = DEV_VERSION_PATH.read_text(encoding="utf-8").strip()
    if not value:
        return 0

    try:
        counter = int(value)
    except ValueError as exc:
        raise DevBuildError(f"{DEV_VERSION_PATH} must contain only a numeric counter") from exc

    if counter < 0:
        raise DevBuildError(f"{DEV_VERSION_PATH} must not be negative")
    return counter


def write_dev_counter(counter: int) -> None:
    DEV_VERSION_PATH.write_text(f"{counter}\n", encoding="utf-8")


def make_dev_version(public_version: str, counter: int) -> str:
    return f"{public_version}-dev.{counter}"


def build_dir_for_preset(preset: str) -> Path:
    return ROOT / "build" / preset


def find_app_exe(directory: Path) -> Path | None:
    for name in APP_EXE_CANDIDATES:
        candidate = directory / name
        if candidate.is_file():
            return candidate
    return None


def require_app_exe(directory: Path) -> Path:
    exe = find_app_exe(directory)
    if exe is None:
        names = ", ".join(APP_EXE_CANDIDATES)
        raise DevBuildError(f"No app executable found in {directory}. Expected one of: {names}")
    return exe


def resolve_app_exe(directory: Path, exe_name: Any = None) -> Path | None:
    if isinstance(exe_name, str) and exe_name:
        candidate = directory / exe_name
        if candidate.is_file():
            return candidate
    return find_app_exe(directory)


def metadata_path(build_dir: Path) -> Path:
    return build_dir / "build-info.json"


def load_build_info(build_dir: Path) -> dict[str, Any] | None:
    path = metadata_path(build_dir)
    if not path.is_file():
        return None

    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except json.JSONDecodeError as exc:
        raise DevBuildError(f"Invalid JSON in {path}") from exc

    if not isinstance(data, dict):
        raise DevBuildError(f"{path} must contain a JSON object")
    return data


def list_archives() -> list[dict[str, Any]]:
    if not ARCHIVE_ROOT.is_dir():
        return []

    archives: list[dict[str, Any]] = []
    for child in ARCHIVE_ROOT.iterdir():
        if not child.is_dir() or child.name.startswith("."):
            continue

        data = load_build_info(child)
        if data is None:
            public_version = read_public_version()
            counter = parse_dev_counter_from_version(child.name)
            exe = find_app_exe(child)
            data = {
                "publicVersion": public_version,
                "devCounter": counter,
                "devVersion": child.name,
                "archiveDir": str(child),
                "exe": exe.name if exe else None,
                "createdAt": None,
                "copiedFiles": [],
            }

        data["archiveDir"] = str(child)
        data.setdefault("devVersion", child.name)
        archives.append(data)

    return sorted(
        archives,
        key=lambda item: (
            int(item.get("devCounter") or parse_dev_counter_from_version(str(item.get("devVersion", ""))) or -1),
            str(item.get("createdAt") or ""),
        ),
        reverse=True,
    )


def parse_dev_counter_from_version(dev_version: str) -> int | None:
    marker = "-dev."
    if marker not in dev_version:
        return None
    suffix = dev_version.rsplit(marker, 1)[1]
    try:
        return int(suffix)
    except ValueError:
        return None


def latest_archive() -> dict[str, Any] | None:
    archives = list_archives()
    return archives[0] if archives else None


def current_state() -> dict[str, Any]:
    public_version = read_public_version()
    dev_counter = read_dev_counter()
    latest = latest_archive()
    build_dir = build_dir_for_preset(DEFAULT_PRESET)
    current_exe = find_app_exe(build_dir)
    return {
        "publicVersion": public_version,
        "devCounter": dev_counter,
        "currentDevVersion": make_dev_version(public_version, dev_counter),
        "currentBuild": {
            "preset": DEFAULT_PRESET,
            "buildDir": str(build_dir),
            "exePath": str(current_exe) if current_exe else None,
            "exeName": current_exe.name if current_exe else None,
            "exists": current_exe is not None,
        },
        "latestArchive": latest,
        "archives": list_archives(),
    }


def copy_file(source: Path, target: Path, bundle_root: Path, copied_files: list[str]) -> None:
    target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(source, target)
    copied_files.append(target.relative_to(bundle_root).as_posix())


def copy_tree(source: Path, target: Path, bundle_root: Path, copied_files: list[str]) -> None:
    shutil.copytree(source, target)
    for copied in target.rglob("*"):
        if copied.is_file():
            copied_files.append(copied.relative_to(bundle_root).as_posix())


def latest_dev_changelog_section() -> str | None:
    path = ROOT / "dev-changelog.md"
    if not path.is_file():
        return None

    lines = path.read_text(encoding="utf-8").splitlines()
    start = None
    for index, line in enumerate(lines):
        if line.startswith("## "):
            start = index
            break
    if start is None:
        return None

    end = len(lines)
    for index in range(start + 1, len(lines)):
        if lines[index].startswith("## "):
            end = index
            break

    section = "\n".join(lines[start:end]).strip()
    return section or None


def write_notes(target_dir: Path, dev_version: str, copied_files: list[str]) -> str | None:
    section = latest_dev_changelog_section()
    if not section:
        return None

    notes_path = target_dir / "notes.md"
    notes = [
        f"# {dev_version} Notes",
        "",
        "Generated from the latest `dev-changelog.md` section at archive time.",
        "",
        section,
        "",
    ]
    notes_path.write_text("\n".join(notes), encoding="utf-8")
    copied_files.append(notes_path.relative_to(target_dir).as_posix())
    return notes_path.name


def archive_build(preset: str) -> dict[str, Any]:
    public_version = read_public_version()
    old_counter = read_dev_counter()
    next_counter = old_counter + 1
    dev_version = make_dev_version(public_version, next_counter)
    source_build_dir = build_dir_for_preset(preset)
    source_exe = require_app_exe(source_build_dir)

    final_dir = ARCHIVE_ROOT / dev_version
    temp_dir = ARCHIVE_ROOT / f".tmp-{dev_version}"
    if final_dir.exists():
        raise DevBuildError(f"Archive already exists: {final_dir}")
    if temp_dir.exists():
        shutil.rmtree(temp_dir)

    copied_files: list[str] = []
    ARCHIVE_ROOT.mkdir(parents=True, exist_ok=True)

    try:
        temp_dir.mkdir(parents=True)
        copy_file(source_exe, temp_dir / source_exe.name, temp_dir, copied_files)

        for dll in sorted(source_build_dir.glob("*.dll")):
            copy_file(dll, temp_dir / dll.name, temp_dir, copied_files)

        for dirname in RUNTIME_DIRS:
            source_dir = source_build_dir / dirname
            if source_dir.is_dir():
                copy_tree(source_dir, temp_dir / dirname, temp_dir, copied_files)

        notes_file = write_notes(temp_dir, dev_version, copied_files)
        info = {
            "publicVersion": public_version,
            "devCounter": next_counter,
            "devVersion": dev_version,
            "createdAt": datetime.now(timezone.utc).isoformat(),
            "preset": preset,
            "sourceBuildDir": str(source_build_dir),
            "sourceExe": str(source_exe),
            "archiveDir": str(final_dir),
            "exe": source_exe.name,
            "copiedFiles": sorted(copied_files),
        }
        if notes_file:
            info["notesFile"] = notes_file

        (temp_dir / "build-info.json").write_text(json.dumps(info, indent=2) + "\n", encoding="utf-8")
        temp_dir.replace(final_dir)
        write_dev_counter(next_counter)
        return info
    except Exception:
        if temp_dir.exists():
            shutil.rmtree(temp_dir, ignore_errors=True)
        raise


def archive_for_version(dev_version: str | None, use_latest: bool) -> dict[str, Any]:
    if use_latest:
        latest = latest_archive()
        if latest is None:
            raise DevBuildError("No archived dev builds exist")
        return latest

    if not dev_version:
        raise DevBuildError("Pass --version <x.y.z-dev.N> or --latest")

    build_dir = ARCHIVE_ROOT / dev_version
    if not build_dir.is_dir():
        raise DevBuildError(f"Archived dev build not found: {build_dir}")

    data = load_build_info(build_dir)
    if data is None:
        raise DevBuildError(f"Archived dev build is missing build-info.json: {build_dir}")
    data.setdefault("archiveDir", str(build_dir))
    data.setdefault("devVersion", dev_version)
    return data


def launch_executable(exe_path: Path, dry_run: bool) -> dict[str, Any]:
    if not exe_path.is_file():
        raise DevBuildError(f"Executable not found: {exe_path}")

    if dry_run:
        return {"launched": False, "dryRun": True, "pid": None, "exePath": str(exe_path)}

    creationflags = 0
    if sys.platform == "win32":
        creationflags = getattr(subprocess, "DETACHED_PROCESS", 0) | getattr(subprocess, "CREATE_NEW_PROCESS_GROUP", 0)

    process = subprocess.Popen(
        [str(exe_path)],
        cwd=str(exe_path.parent),
        stdin=subprocess.DEVNULL,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
        close_fds=True,
        creationflags=creationflags,
    )
    return {"launched": True, "dryRun": False, "pid": process.pid, "exePath": str(exe_path)}


def launch_archive(dev_version: str | None, use_latest: bool, dry_run: bool) -> dict[str, Any]:
    archive = archive_for_version(dev_version, use_latest)
    archive_dir = Path(str(archive.get("archiveDir") or ARCHIVE_ROOT / str(archive["devVersion"])))
    exe_path = resolve_app_exe(archive_dir, archive.get("exe"))
    if exe_path is None:
        raise DevBuildError(f"No app executable found in archive: {archive_dir}")

    launch = launch_executable(exe_path, dry_run=dry_run)
    return {
        **launch,
        "devVersion": archive.get("devVersion"),
        "archiveDir": str(archive_dir),
    }


def emit(data: Any, as_json: bool) -> None:
    if as_json:
        print(json.dumps(data, indent=2))
        return

    if isinstance(data, dict):
        for key, value in data.items():
            if isinstance(value, (dict, list)):
                print(f"{key}: {json.dumps(value)}")
            else:
                print(f"{key}: {value}")
        return

    if isinstance(data, list):
        if not data:
            print("No archived dev builds.")
            return
        for item in data:
            print(f"{item.get('devVersion')}  {item.get('createdAt') or 'unknown time'}  {item.get('archiveDir')}")
        return

    print(data)


def handle_current(args: argparse.Namespace) -> int:
    emit(current_state(), args.json)
    return 0


def handle_list(args: argparse.Namespace) -> int:
    emit(list_archives(), args.json)
    return 0


def handle_archive(args: argparse.Namespace) -> int:
    emit(archive_build(args.preset), args.json)
    return 0


def handle_launch(args: argparse.Namespace) -> int:
    emit(launch_archive(args.version, args.latest, args.dry_run), args.json)
    return 0


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    subparsers = parser.add_subparsers(dest="command", required=True)

    current = subparsers.add_parser("current", help="Print current public/dev version and latest archive state.")
    current.add_argument("--json", action="store_true", help="Emit JSON.")
    current.set_defaults(func=handle_current)

    list_cmd = subparsers.add_parser("list", help="List archived dev builds, newest first.")
    list_cmd.add_argument("--json", action="store_true", help="Emit JSON.")
    list_cmd.set_defaults(func=handle_list)

    archive = subparsers.add_parser("archive", help="Create the next archived dev build.")
    archive.add_argument("--preset", default=DEFAULT_PRESET, help=f"CMake build preset, default: {DEFAULT_PRESET}.")
    archive.add_argument("--json", action="store_true", help="Emit JSON.")
    archive.set_defaults(func=handle_archive)

    launch = subparsers.add_parser("launch", help="Launch an archived dev build.")
    launch.add_argument("--version", help="Archived dev version, for example 0.1.0-dev.12.")
    launch.add_argument("--latest", action="store_true", help="Launch the newest archived dev build.")
    launch.add_argument("--dry-run", action="store_true", help="Validate launch metadata without starting the executable.")
    launch.add_argument("--json", action="store_true", help="Emit JSON.")
    launch.set_defaults(func=handle_launch)

    return parser


def main(argv: list[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)
    try:
        return args.func(args)
    except DevBuildError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
