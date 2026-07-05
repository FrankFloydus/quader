#!/usr/bin/env python3
"""Compile Crimson shaders from the checked-in manifest."""

from __future__ import annotations

import argparse
import json
import subprocess
import sys
from pathlib import Path


TARGETS = {
    "vulkan": {"profile": "spirv"},
    "dx11": {"platform": "windows", "profile": "s_5_0"},
    "dx12": {"platform": "windows", "profile": "s_5_0"},
    "metal": {"platform": "osx", "profile": "metal"},
}

STAGE_TYPES = {
    "vertex": "vertex",
    "fragment": "fragment",
}


def load_manifest(path: Path) -> dict:
    with path.open("r", encoding="utf-8") as handle:
        return json.load(handle)


def compile_stage(
    *,
    shaderc: Path,
    source_root: Path,
    bgfx_include: Path,
    varyingdef: Path,
    output_root: Path,
    host_platform: str,
    target: str,
    stage: str,
    source: Path,
    output_name: str,
) -> None:
    target_config = TARGETS[target]
    target_platform = target_config.get("platform", host_platform)
    output_path = output_root / target / output_name
    output_path.parent.mkdir(parents=True, exist_ok=True)

    command = [
        str(shaderc),
        "-f",
        str(source),
        "-i",
        str(source_root),
        "-i",
        str(bgfx_include),
        "--type",
        STAGE_TYPES[stage],
        "--platform",
        target_platform,
        "--profile",
        target_config["profile"],
        "--varyingdef",
        str(varyingdef),
        "-o",
        str(output_path),
    ]
    subprocess.run(command, check=True)


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", required=True, type=Path)
    parser.add_argument("--shaderc", required=True, type=Path)
    parser.add_argument("--source-root", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--include", required=True, type=Path)
    parser.add_argument("--varyingdef", required=True, type=Path)
    parser.add_argument("--host-platform", required=True)
    args = parser.parse_args(argv)

    manifest = load_manifest(args.manifest)
    targets = manifest.get("targets", [])
    programs = manifest.get("programs", [])

    for target in targets:
        if target not in TARGETS:
            raise SystemExit(f"unknown shader target in manifest: {target}")

    for program in programs:
        stages = program.get("stages", {})
        for stage, stage_data in stages.items():
            if stage not in STAGE_TYPES:
                raise SystemExit(f"unknown shader stage in manifest: {program.get('id')}:{stage}")
            source = args.source_root / stage_data["source"]
            output_name = stage_data["output"]
            for target in targets:
                compile_stage(
                    shaderc=args.shaderc,
                    source_root=args.source_root,
                    bgfx_include=args.include,
                    varyingdef=args.varyingdef,
                    output_root=args.output,
                    host_platform=args.host_platform,
                    target=target,
                    stage=stage,
                    source=source,
                    output_name=output_name,
                )

    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
