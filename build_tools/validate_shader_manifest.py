#!/usr/bin/env python3
"""Validate Crimson shader manifest sources and compiled outputs."""

from __future__ import annotations

import argparse
import json
import sys
from collections import Counter
from pathlib import Path


ALLOWED_TARGETS = {"vulkan", "dx11", "dx12", "metal"}
REQUIRED_STAGES = {"vertex", "fragment"}


class ManifestError(RuntimeError):
    pass


def reject_duplicate_keys(pairs):
    keys = [key for key, _ in pairs]
    duplicates = [key for key, count in Counter(keys).items() if count > 1]
    if duplicates:
        raise ManifestError(f"duplicate manifest key(s): {', '.join(sorted(duplicates))}")
    return dict(pairs)


def load_manifest(path: Path) -> dict:
    try:
        with path.open("r", encoding="utf-8") as handle:
            return json.load(handle, object_pairs_hook=reject_duplicate_keys)
    except json.JSONDecodeError as error:
        raise ManifestError(f"invalid JSON: {error}") from error


def validate_manifest(manifest: dict, manifest_path: Path, compiled_root: Path | None) -> list[str]:
    errors: list[str] = []
    source_root = manifest_path.parent

    targets = manifest.get("targets")
    if not isinstance(targets, list) or not targets:
        errors.append("manifest must define a non-empty targets list")
        targets = []

    seen_targets: set[str] = set()
    for target in targets:
        if target in seen_targets:
            errors.append(f"duplicate shader target: {target}")
        seen_targets.add(target)
        if target not in ALLOWED_TARGETS:
            errors.append(f"unknown shader target: {target}")

    programs = manifest.get("programs")
    if not isinstance(programs, list) or not programs:
        errors.append("manifest must define a non-empty programs list")
        return errors

    seen_programs: set[str] = set()
    for program in programs:
        program_id = program.get("id")
        if not isinstance(program_id, str) or not program_id:
            errors.append("shader program is missing a string id")
            continue
        if program_id in seen_programs:
            errors.append(f"duplicate shader program id: {program_id}")
        seen_programs.add(program_id)

        stages = program.get("stages")
        if not isinstance(stages, dict):
            errors.append(f"{program_id}: stages must be an object")
            continue

        stage_names = set(stages.keys())
        missing = REQUIRED_STAGES - stage_names
        extra = stage_names - REQUIRED_STAGES
        for stage in sorted(missing):
            errors.append(f"{program_id}: missing {stage} stage")
        for stage in sorted(extra):
            errors.append(f"{program_id}: unknown {stage} stage")

        for stage in sorted(REQUIRED_STAGES & stage_names):
            stage_data = stages[stage]
            if not isinstance(stage_data, dict):
                errors.append(f"{program_id}:{stage}: stage entry must be an object")
                continue
            source = stage_data.get("source")
            output = stage_data.get("output")
            if not isinstance(source, str) or not source:
                errors.append(f"{program_id}:{stage}: missing source")
                continue
            if not isinstance(output, str) or not output:
                errors.append(f"{program_id}:{stage}: missing output")
                continue
            if Path(output).name != output:
                errors.append(f"{program_id}:{stage}: output must be a basename")
            if not (source_root / source).is_file():
                errors.append(f"{program_id}:{stage}: missing source {source}")
            if compiled_root is not None:
                for target in targets:
                    if target in ALLOWED_TARGETS and not (compiled_root / target / output).is_file():
                        errors.append(f"{program_id}:{stage}:{target}: missing compiled binary {output}")

    return errors


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", required=True, type=Path)
    parser.add_argument("--compiled", type=Path)
    args = parser.parse_args(argv)

    try:
        manifest = load_manifest(args.manifest)
        errors = validate_manifest(manifest, args.manifest, args.compiled)
    except ManifestError as error:
        print(f"shader manifest validation failed: {error}", file=sys.stderr)
        return 1

    if errors:
        print("shader manifest validation failed:", file=sys.stderr)
        for error in errors:
            print(f"  - {error}", file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
