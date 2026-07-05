#!/usr/bin/env python3
"""Small format-preserving helper for Quader's project_board.md."""

from __future__ import annotations

import argparse
import re
import sys
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
BOARD_PATH = ROOT / "project_board.md"
ARCHIVE_PATH = ROOT / "project_board_archive.md"
VERSION_PATH = ROOT / "VERSION"
DEV_VERSION_PATH = ROOT / "DEV_VERSION"
SECTIONS = ("Backlog", "In Progress", "In Review", "Bugs")
PRIORITIES = {"critical", "high", "medium", "low"}
TYPES = {"enhancement", "documentation", "bug", "epic"}
PM_STATUSES = {"coordinating", "waiting-on-agents", "waiting-on-authority", "ready-for-review"}
AUTHORITY_OWNERS = {"software", "core", "build-workflow", "ui", "renderer", "performance", "docs", "multi"}
AUTHORITY_STATUSES = {"pending", "approved", "changes-requested"}
ASSIGNMENT_STATUSES = {"planned", "running", "reported", "blocked", "integrated"}
ASSIGNMENT_AUTHORITIES = {"none", "software", "core", "build-workflow", "ui", "renderer", "performance", "docs"}
ACTIVE_STATUSES = {"running", "integrating", "reviewing", "blocked"}
FRESHNESS_STATUSES = {"needs-refresh", "fresh", "stale", "superseded"}
ARCHIVE_STATUSES = {"completed", "fixed"}
MUTATING_COMMANDS = {
    "add",
    "add-bug",
    "pm-start",
    "set-authority",
    "assign",
    "assignment-status",
    "coordination-note",
    "set-active",
    "clear-active",
    "edit-brief",
    "set-freshness",
    "move",
    "start",
    "review",
    "remove",
    "complete",
    "reopen-archive",
}
AUTH_REQUIRED_MESSAGE = (
    "board mutation requires an exact software-architect-issued command, except for "
    "the narrow quader-performance-dev performance audit/fix override; use "
    "--architect-authorized only when the software architect returned this exact "
    "command or quader-performance-dev is acting inside that documented override"
)
ENTRY_RE = re.compile(
    r"^#(?P<id>\d+)\s+"
    r"\[priority:(?P<priority>[a-z]+)\]"
    r"\[type:(?P<type>[a-z]+)\]"
    r"\[area:(?P<area>[a-z0-9_-]+)\]\s+"
    r"(?P<title>.+?)\s+-\s+(?P<summary>.+)$"
)
PM_RE = re.compile(
    r"^  PM: "
    r"\[run:(?P<run>[A-Za-z0-9_.-]+)\]"
    r"\[status:(?P<status>[a-z-]+)\]"
    r"\[manager:(?P<manager>[A-Za-z0-9_.:-]+|-|[A-Za-z0-9_.:-]*-[A-Za-z0-9_.:-]*)\] "
    r"(?P<note>.*)$"
)
AUTHORITY_RE = re.compile(
    r"^  Authority: "
    r"\[owner:(?P<owner>[a-z-]+)\]"
    r"\[status:(?P<status>[a-z-]+)\]"
    r"\[agent:(?P<agent>[A-Za-z0-9_.:-]+|-|[A-Za-z0-9_.:-]*-[A-Za-z0-9_.:-]*)\] "
    r"(?P<note>.*)$"
)
ASSIGNMENT_RE = re.compile(
    r"^  Assignment: "
    r"\[key:(?P<key>[A-Za-z][A-Za-z0-9_-]*)\]"
    r"\[role:(?P<role>[A-Za-z0-9_.:-]+|-|[A-Za-z0-9_.:-]*-[A-Za-z0-9_.:-]*)\]"
    r"\[agent:(?P<agent>[A-Za-z0-9_.:-]+|-|[A-Za-z0-9_.:-]*-[A-Za-z0-9_.:-]*)\]"
    r"\[status:(?P<status>[a-z-]+)\]"
    r"\[authority:(?P<authority>[a-z-]+)\] "
    r"(?P<brief>.*)$"
)
COORDINATION_RE = re.compile(r"^  Coordination: (?P<note>.*)$")
ACTIVE_RE = re.compile(
    r"^  Active: "
    r"\[lead:(?P<lead>[A-Za-z0-9_.:-]+|-|[A-Za-z0-9_.:-]*-[A-Za-z0-9_.:-]*)\]"
    r"\[status:(?P<status>[a-z-]+)\]"
    r"\[workers:(?P<workers>\d+)\] "
    r"(?P<note>.*)$"
)
FRESHNESS_RE = re.compile(
    r"^  Freshness: "
    r"\[status:(?P<status>[a-z-]+)\]"
    r"\[checked:(?P<checked>[^\]]+)\] "
    r"(?P<note>.*)$"
)
CREATED_RE = re.compile(r"^  Created: (?P<at>.+)$")
ARCHIVE_SECTION_RE = re.compile(r"^## (?P<date>\d{4}-\d{2}-\d{2})$")
ARCHIVE_VERSION_RE = re.compile(r"^### (?P<dev_version>[A-Za-z0-9_.-]+)$")
ARCHIVED_RE = re.compile(
    r"^  Archived: "
    r"\[at:(?P<at>[^\]]+)\]"
    r"\[dev-version:(?P<dev_version>[^\]]+)\]"
    r"\[from:(?P<from_section>[^\]]+)\]"
    r"\[status:(?P<status>[a-z-]+)\] "
    r"(?P<note>.*)$"
)


@dataclass
class Entry:
    section: str
    start: int
    end: int
    lines: list[str]
    task_id: int
    priority: str
    entry_type: str
    area: str
    title: str
    summary: str


@dataclass
class ArchiveEntry:
    archive_date: str
    dev_version: str
    start: int
    end: int
    lines: list[str]
    task_id: int
    priority: str
    entry_type: str
    area: str
    title: str
    summary: str


def read_board() -> list[str]:
    if not BOARD_PATH.exists():
        raise SystemExit(f"missing board: {BOARD_PATH}")
    return BOARD_PATH.read_text(encoding="utf-8").splitlines()


def read_archive() -> list[str]:
    if not ARCHIVE_PATH.exists():
        return archive_template()
    return ARCHIVE_PATH.read_text(encoding="utf-8").splitlines()


def write_board(lines: list[str]) -> None:
    BOARD_PATH.write_text("\n".join(lines).rstrip() + "\n", encoding="utf-8")


def write_archive(lines: list[str]) -> None:
    ARCHIVE_PATH.write_text("\n".join(lines).rstrip() + "\n", encoding="utf-8")


def archive_template() -> list[str]:
    return [
        "# Project Board Archive",
        "",
        "Completed tasks and fixed bugs are moved here from `project_board.md`.",
        "Entries are grouped newest-first by archive date and internal dev version.",
    ]


def section_ranges(lines: list[str]) -> dict[str, tuple[int, int]]:
    headings: list[tuple[str, int]] = []
    for index, line in enumerate(lines):
        if line.startswith("## "):
            name = line[3:].strip()
            if name in SECTIONS:
                headings.append((name, index))

    found = [name for name, _ in headings]
    missing = [name for name in SECTIONS if name not in found]
    if missing:
        raise SystemExit(f"missing board sections: {', '.join(missing)}")

    ranges: dict[str, tuple[int, int]] = {}
    for pos, (name, start) in enumerate(headings):
        end = headings[pos + 1][1] if pos + 1 < len(headings) else len(lines)
        ranges[name] = (start, end)
    return ranges


def parse_entries(lines: list[str]) -> list[Entry]:
    ranges = section_ranges(lines)
    entries: list[Entry] = []
    for section, (start, end) in ranges.items():
        index = start + 1
        while index < end:
            line = lines[index]
            if not line.startswith("#"):
                index += 1
                continue

            match = ENTRY_RE.match(line)
            if not match:
                raise SystemExit(f"invalid entry line {index + 1}: {line}")

            entry_end = index + 1
            while entry_end < end and not lines[entry_end].startswith("#"):
                entry_end += 1

            entries.append(
                Entry(
                    section=section,
                    start=index,
                    end=entry_end,
                    lines=lines[index:entry_end],
                    task_id=int(match.group("id")),
                    priority=match.group("priority"),
                    entry_type=match.group("type"),
                    area=match.group("area"),
                    title=match.group("title"),
                    summary=match.group("summary"),
                )
            )
            index = entry_end
    return entries


def parse_archive_entries(lines: list[str]) -> list[ArchiveEntry]:
    entries: list[ArchiveEntry] = []
    archive_date: str | None = None
    dev_version: str | None = None
    index = 0
    while index < len(lines):
        line = lines[index]
        date_match = ARCHIVE_SECTION_RE.match(line)
        if date_match:
            archive_date = date_match.group("date")
            dev_version = None
            index += 1
            continue

        version_match = ARCHIVE_VERSION_RE.match(line)
        if version_match and archive_date:
            dev_version = version_match.group("dev_version")
            index += 1
            continue

        if line.startswith("# "):
            index += 1
            continue

        if not line.startswith("#") or line.startswith("##"):
            index += 1
            continue

        if not archive_date or not dev_version:
            raise SystemExit(f"archived entry line {index + 1} must be under date and dev-version headings")

        match = ENTRY_RE.match(line)
        if not match:
            raise SystemExit(f"invalid archived entry line {index + 1}: {line}")

        entry_end = index + 1
        while entry_end < len(lines) and not lines[entry_end].startswith("#"):
            entry_end += 1

        entries.append(
            ArchiveEntry(
                archive_date=archive_date,
                dev_version=dev_version,
                start=index,
                end=entry_end,
                lines=lines[index:entry_end],
                task_id=int(match.group("id")),
                priority=match.group("priority"),
                entry_type=match.group("type"),
                area=match.group("area"),
                title=match.group("title"),
                summary=match.group("summary"),
            )
        )
        index = entry_end
    return entries


def validate_board(verbose: bool = True) -> set[int]:
    lines = read_board()
    entries = parse_entries(lines)
    ids: set[int] = set()
    for entry in entries:
        if entry.task_id in ids:
            raise SystemExit(f"duplicate task id: #{entry.task_id}")
        ids.add(entry.task_id)
        if entry.priority not in PRIORITIES:
            raise SystemExit(f"invalid priority for #{entry.task_id}: {entry.priority}")
        if entry.entry_type not in TYPES:
            raise SystemExit(f"invalid type for #{entry.task_id}: {entry.entry_type}")
        for continuation in entry.lines[1:]:
            if continuation and not continuation.startswith("  "):
                raise SystemExit(
                    f"entry #{entry.task_id} continuation line must be indented: {continuation}"
                )
            validate_metadata_line(entry.task_id, continuation)
    if verbose:
        print(f"validated {len(entries)} board entries")
    return ids


def validate_archive(verbose: bool = True) -> set[int]:
    lines = read_archive()
    entries = parse_archive_entries(lines)
    ids: set[int] = set()
    for entry in entries:
        if entry.task_id in ids:
            raise SystemExit(f"duplicate archived task id: #{entry.task_id}")
        ids.add(entry.task_id)
        if entry.priority not in PRIORITIES:
            raise SystemExit(f"invalid archived priority for #{entry.task_id}: {entry.priority}")
        if entry.entry_type not in TYPES:
            raise SystemExit(f"invalid archived type for #{entry.task_id}: {entry.entry_type}")

        has_archived = False
        has_resolution = False
        for continuation in entry.lines[1:]:
            if continuation and not continuation.startswith("  "):
                raise SystemExit(
                    f"archived entry #{entry.task_id} continuation line must be indented: {continuation}"
                )
            if continuation.startswith("  Archived: "):
                has_archived = True
                validate_archive_metadata_line(entry, continuation)
            elif continuation.startswith("  Resolution: "):
                has_resolution = bool(continuation.removeprefix("  Resolution: ").strip())
            else:
                validate_metadata_line(entry.task_id, continuation)

        if not has_archived:
            raise SystemExit(f"archived entry #{entry.task_id} is missing Archived metadata")
        if entry.entry_type == "bug" and not has_resolution:
            raise SystemExit(f"archived bug #{entry.task_id} is missing Resolution metadata")

    if verbose:
        print(f"validated {len(entries)} archived entries")
    return ids


def validate_metadata_line(task_id: int, line: str) -> None:
    if not line:
        return

    if line.startswith("  PM: "):
        match = PM_RE.match(line)
        if not match:
            raise SystemExit(f"invalid PM metadata for #{task_id}: {line}")
        status = match.group("status")
        if status not in PM_STATUSES:
            raise SystemExit(f"invalid PM status for #{task_id}: {status}")
        return

    if line.startswith("  Authority: "):
        match = AUTHORITY_RE.match(line)
        if not match:
            raise SystemExit(f"invalid authority metadata for #{task_id}: {line}")
        owner = match.group("owner")
        status = match.group("status")
        if owner not in AUTHORITY_OWNERS:
            raise SystemExit(f"invalid authority owner for #{task_id}: {owner}")
        if status not in AUTHORITY_STATUSES:
            raise SystemExit(f"invalid authority status for #{task_id}: {status}")
        return

    if line.startswith("  Assignment: "):
        match = ASSIGNMENT_RE.match(line)
        if not match:
            raise SystemExit(f"invalid assignment metadata for #{task_id}: {line}")
        status = match.group("status")
        authority = match.group("authority")
        if status not in ASSIGNMENT_STATUSES:
            raise SystemExit(f"invalid assignment status for #{task_id}: {status}")
        if authority not in ASSIGNMENT_AUTHORITIES:
            raise SystemExit(f"invalid assignment authority for #{task_id}: {authority}")
        return

    if line.startswith("  Coordination: "):
        if not COORDINATION_RE.match(line):
            raise SystemExit(f"invalid coordination metadata for #{task_id}: {line}")
        return

    if line.startswith("  Active: "):
        match = ACTIVE_RE.match(line)
        if not match:
            raise SystemExit(f"invalid active metadata for #{task_id}: {line}")
        status = match.group("status")
        workers = int(match.group("workers"))
        if status not in ACTIVE_STATUSES:
            raise SystemExit(f"invalid active status for #{task_id}: {status}")
        if workers < 0 or workers > 6:
            raise SystemExit(f"invalid active worker count for #{task_id}: {workers}")
        return

    if line.startswith("  Freshness: "):
        match = FRESHNESS_RE.match(line)
        if not match:
            raise SystemExit(f"invalid freshness metadata for #{task_id}: {line}")
        status = match.group("status")
        checked = match.group("checked")
        if status not in FRESHNESS_STATUSES:
            raise SystemExit(f"invalid freshness status for #{task_id}: {status}")
        validate_checked_timestamp(task_id, checked)
        return

    if line.startswith("  Created: "):
        match = CREATED_RE.match(line)
        if not match:
            raise SystemExit(f"invalid created metadata for #{task_id}: {line}")
        validate_checked_timestamp(task_id, match.group("at"))
        return


def validate_archive_metadata_line(entry: ArchiveEntry, line: str) -> None:
    match = ARCHIVED_RE.match(line)
    if not match:
        raise SystemExit(f"invalid archive metadata for #{entry.task_id}: {line}")
    archived_at = match.group("at")
    dev_version = match.group("dev_version")
    from_section = match.group("from_section")
    status = match.group("status")
    validate_checked_timestamp(entry.task_id, archived_at)
    if archived_at[:10] != entry.archive_date:
        raise SystemExit(
            f"archive date heading mismatch for #{entry.task_id}: {entry.archive_date} != {archived_at[:10]}"
        )
    if dev_version != entry.dev_version:
        raise SystemExit(
            f"archive dev-version heading mismatch for #{entry.task_id}: {entry.dev_version} != {dev_version}"
        )
    if from_section not in SECTIONS:
        raise SystemExit(f"invalid archive source section for #{entry.task_id}: {from_section}")
    if status not in ARCHIVE_STATUSES:
        raise SystemExit(f"invalid archive status for #{entry.task_id}: {status}")
    if entry.entry_type == "bug" and status != "fixed":
        raise SystemExit(f"archived bug #{entry.task_id} must use status:fixed")
    if entry.entry_type != "bug" and status != "completed":
        raise SystemExit(f"archived task #{entry.task_id} must use status:completed")


def validate_checked_timestamp(task_id: int, checked: str) -> None:
    if checked == "never":
        return
    try:
        datetime.fromisoformat(checked.replace("Z", "+00:00"))
    except ValueError as exc:
        raise SystemExit(f"invalid freshness checked timestamp for #{task_id}: {checked}") from exc


def next_id(entries: list[Entry]) -> int:
    active_ids = [entry.task_id for entry in entries]
    archived_ids = [entry.task_id for entry in parse_archive_entries(read_archive())]
    return max(active_ids + archived_ids, default=0) + 1


def current_dev_version() -> str:
    public_version = VERSION_PATH.read_text(encoding="utf-8").strip()
    dev_counter = DEV_VERSION_PATH.read_text(encoding="utf-8").strip()
    if not public_version:
        raise SystemExit(f"missing public version in {VERSION_PATH}")
    if not dev_counter.isdigit():
        raise SystemExit(f"invalid dev version counter in {DEV_VERSION_PATH}: {dev_counter}")
    return f"{public_version}-dev.{dev_counter}"


def local_timestamp() -> str:
    return datetime.now().astimezone().isoformat(timespec="seconds")


def find_entry(entries: list[Entry], task_id: int) -> Entry:
    for entry in entries:
        if entry.task_id == task_id:
            return entry
    raise SystemExit(f"task not found: #{task_id}")


def find_archive_entry(entries: list[ArchiveEntry], task_id: int) -> ArchiveEntry:
    for entry in entries:
        if entry.task_id == task_id:
            return entry
    raise SystemExit(f"archived task not found: #{task_id}")


def insert_entry(
    lines: list[str],
    section: str,
    title: str,
    summary: str,
    priority: str,
    entry_type: str,
    area: str,
    brief: str,
    final_owner: str | None,
    plans: str | None,
) -> int:
    entries = parse_entries(lines)
    task_id = next_id(entries)
    ranges = section_ranges(lines)
    _, end = ranges[section]

    entry_lines = [
        f"#{task_id} [priority:{priority}][type:{entry_type}][area:{area}] {title} - {summary}",
        f"  Created: {local_timestamp()}",
        f"  Brief: {brief}",
        "  Freshness: [status:needs-refresh][checked:never] Awaiting start-gate revalidation.",
    ]
    if final_owner:
        entry_lines.append(f"  Final owner: {final_owner}")
    if plans:
        entry_lines.append(f"  Plans: {plans}")

    insert_at = end
    while insert_at > 0 and lines[insert_at - 1] == "":
        insert_at -= 1

    block = [""] + entry_lines + [""]
    lines[insert_at:insert_at] = block
    return task_id


def remove_entry(lines: list[str], entry: Entry) -> list[str]:
    start = entry.start
    end = entry.end
    previous_is_section_heading = start >= 2 and lines[start - 2].startswith("## ")
    if start > 0 and lines[start - 1] == "" and not previous_is_section_heading:
        start -= 1
    if end < len(lines) and lines[end] == "":
        end += 1
    del lines[start:end]
    return lines


def remove_archive_entry(lines: list[str], entry: ArchiveEntry) -> list[str]:
    start = entry.start
    end = entry.end
    previous_is_heading = start >= 2 and lines[start - 2].startswith("### ")
    if start > 0 and lines[start - 1] == "" and not previous_is_heading:
        start -= 1
    if end < len(lines) and lines[end] == "":
        end += 1
    del lines[start:end]
    return lines


def archive_entry_lines(entry: Entry, resolution: str | None) -> list[str]:
    if resolution is not None and ("\n" in resolution or "\r" in resolution):
        raise SystemExit("resolution must be a single line")
    is_bug = entry.entry_type == "bug" or entry.section == "Bugs"
    if is_bug and not (resolution and resolution.strip()):
        raise SystemExit("bug completion requires --resolution with a concise fix summary")

    archived_at = local_timestamp()
    dev_version = current_dev_version()
    status = "fixed" if is_bug else "completed"
    lines = [
        entry.lines[0],
        f"  Archived: [at:{archived_at}][dev-version:{dev_version}][from:{entry.section}][status:{status}] Architect-authorized archive.",
    ]
    if resolution and resolution.strip():
        lines.append(f"  Resolution: {resolution.strip()}")
    lines.extend(entry.lines[1:])
    return lines


def archive_metadata(entry: ArchiveEntry) -> dict[str, str]:
    archived_line = next((line for line in entry.lines if line.startswith("  Archived: ")), None)
    if not archived_line:
        raise SystemExit(f"archived entry #{entry.task_id} is missing Archived metadata")
    match = ARCHIVED_RE.match(archived_line)
    if not match:
        raise SystemExit(f"invalid archive metadata for #{entry.task_id}: {archived_line}")
    return match.groupdict()


def reopened_entry_lines(entry: ArchiveEntry, reason: str) -> list[str]:
    if "\n" in reason or "\r" in reason:
        raise SystemExit("reopen reason must be a single line")

    metadata = archive_metadata(entry)
    reopened_at = local_timestamp()
    result: list[str] = [entry.lines[0]]
    freshness_added = False

    result.append(
        f"  Reopened: [at:{reopened_at}][archive-date:{entry.archive_date}]"
        f"[dev-version:{entry.dev_version}][archive-status:{metadata['status']}] {reason}"
    )

    for line in entry.lines[1:]:
        if line.startswith("  Archived: "):
            continue
        if line.startswith("  Freshness: "):
            result.append("  Freshness: [status:needs-refresh][checked:never] Reopened from archive; requires start-gate revalidation.")
            freshness_added = True
            continue
        if line.startswith("  Resolution: "):
            result.append(f"  Previous resolution: {line.removeprefix('  Resolution: ')}")
            continue
        result.append(line)

    if not freshness_added:
        result.append("  Freshness: [status:needs-refresh][checked:never] Reopened from archive; requires start-gate revalidation.")

    return result


def insert_archive_entry(lines: list[str], entry_lines: list[str]) -> None:
    archived_line = next((line for line in entry_lines if line.startswith("  Archived: ")), None)
    if not archived_line:
        raise SystemExit("archive entry is missing Archived metadata")
    match = ARCHIVED_RE.match(archived_line)
    if not match:
        raise SystemExit(f"invalid archive metadata: {archived_line}")

    date = match.group("at")[:10]
    dev_version = match.group("dev_version")
    date_heading = f"## {date}"
    version_heading = f"### {dev_version}"

    date_index = next((index for index, line in enumerate(lines) if line == date_heading), None)
    if date_index is None:
        first_date_index = next(
            (index for index, line in enumerate(lines) if ARCHIVE_SECTION_RE.match(line)),
            len(lines),
        )
        section_block = ["", date_heading, "", version_heading, ""] + entry_lines + [""]
        lines[first_date_index:first_date_index] = section_block
        return

    next_date_index = next(
        (
            index
            for index in range(date_index + 1, len(lines))
            if ARCHIVE_SECTION_RE.match(lines[index])
        ),
        len(lines),
    )
    version_index = next(
        (
            index
            for index in range(date_index + 1, next_date_index)
            if lines[index] == version_heading
        ),
        None,
    )
    if version_index is None:
        insert_at = date_index + 1
        while insert_at < len(lines) and lines[insert_at] == "":
            insert_at += 1
        lines[insert_at:insert_at] = ["", version_heading, ""] + entry_lines + [""]
        return

    insert_at = version_index + 1
    while insert_at < len(lines) and lines[insert_at] == "":
        insert_at += 1
    lines[insert_at:insert_at] = [""] + entry_lines + [""]


def insert_existing_entry(lines: list[str], entry_lines: list[str], target_section: str) -> None:
    ranges = section_ranges(lines)
    _, end = ranges[target_section]
    insert_at = end
    while insert_at > 0 and lines[insert_at - 1] == "":
        insert_at -= 1
    lines[insert_at:insert_at] = [""] + entry_lines + [""]


def move_entry(lines: list[str], task_id: int, target_section: str) -> None:
    entries = parse_entries(lines)
    entry = find_entry(entries, task_id)
    entry_lines = entry.lines[:]
    lines = remove_entry(lines, entry)
    ranges = section_ranges(lines)
    _, end = ranges[target_section]
    insert_at = end
    while insert_at > 0 and lines[insert_at - 1] == "":
        insert_at -= 1
    lines[insert_at:insert_at] = [""] + entry_lines + [""]
    write_board(lines)
    print(f"moved #{task_id} to {target_section}")


def replace_entry(lines: list[str], entry: Entry, new_entry_lines: list[str]) -> None:
    lines[entry.start:entry.end] = new_entry_lines


def find_line_index(entry: Entry, prefix: str) -> int | None:
    for offset, line in enumerate(entry.lines):
        if line.startswith(prefix):
            return offset
    return None


def find_assignment_index(entry: Entry, key: str) -> int | None:
    for offset, line in enumerate(entry.lines):
        match = ASSIGNMENT_RE.match(line)
        if match and match.group("key") == key:
            return offset
    return None


def upsert_line(lines: list[str], task_id: int, prefix: str, new_line: str) -> None:
    entries = parse_entries(lines)
    entry = find_entry(entries, task_id)
    entry_lines = entry.lines[:]
    offset = find_line_index(entry, prefix)
    if offset is None:
        entry_lines.append(new_line)
    else:
        entry_lines[offset] = new_line
    replace_entry(lines, entry, entry_lines)


def remove_line(lines: list[str], task_id: int, prefix: str) -> bool:
    entries = parse_entries(lines)
    entry = find_entry(entries, task_id)
    entry_lines = entry.lines[:]
    offset = find_line_index(entry, prefix)
    if offset is None:
        return False
    del entry_lines[offset]
    replace_entry(lines, entry, entry_lines)
    return True


def edit_brief(lines: list[str], task_id: int, brief: str) -> None:
    if "\n" in brief or "\r" in brief:
        raise SystemExit("brief must be a single line")
    entries = parse_entries(lines)
    entry = find_entry(entries, task_id)
    entry_lines = entry.lines[:]
    offset = find_line_index(entry, "  Brief: ")
    if offset is None:
        raise SystemExit(f"brief line not found for #{task_id}")
    entry_lines[offset] = f"  Brief: {brief}"
    replace_entry(lines, entry, entry_lines)


def normalize_checked(checked: str) -> str:
    if checked == "now":
        return datetime.now(timezone.utc).isoformat(timespec="seconds").replace("+00:00", "Z")
    return checked


def set_freshness(lines: list[str], task_id: int, status: str, checked: str, note: str) -> None:
    if "\n" in note or "\r" in note:
        raise SystemExit("freshness note must be a single line")
    checked_value = normalize_checked(checked)
    validate_checked_timestamp(task_id, checked_value)
    upsert_line(
        lines,
        task_id,
        "  Freshness: ",
        f"  Freshness: [status:{status}][checked:{checked_value}] {note}",
    )


def set_active(lines: list[str], task_id: int, lead: str, status: str, workers: int, note: str) -> None:
    if "\n" in note or "\r" in note:
        raise SystemExit("active note must be a single line")
    if workers < 0 or workers > 6:
        raise SystemExit("active workers must be between 0 and 6")
    upsert_line(
        lines,
        task_id,
        "  Active: ",
        f"  Active: [lead:{lead}][status:{status}][workers:{workers}] {note}",
    )


def freshness_status(entry: Entry) -> str:
    offset = find_line_index(entry, "  Freshness: ")
    if offset is None:
        return "needs-refresh"
    match = FRESHNESS_RE.match(entry.lines[offset])
    if not match:
        raise SystemExit(f"invalid freshness metadata for #{entry.task_id}: {entry.lines[offset]}")
    return match.group("status")


def require_fresh_entry(entry: Entry) -> None:
    status = freshness_status(entry)
    if status != "fresh":
        raise SystemExit(
            f"task #{entry.task_id} is not fresh ({status}); run set-freshness after current-workspace revalidation before starting"
        )


def upsert_assignment(lines: list[str], task_id: int, key: str, new_line: str) -> None:
    entries = parse_entries(lines)
    entry = find_entry(entries, task_id)
    entry_lines = entry.lines[:]
    offset = find_assignment_index(entry, key)
    if offset is None:
        entry_lines.append(new_line)
    else:
        entry_lines[offset] = new_line
    replace_entry(lines, entry, entry_lines)


def list_entries(section: str | None, tag: str | None) -> None:
    entries = parse_entries(read_board())
    for entry in entries:
        if section and entry.section != section:
            continue
        if tag and tag not in entry.lines[0]:
            continue
        print(f"## {entry.section}")
        print("\n".join(entry.lines).rstrip())
        print()


def search_archive(query: str, task_id: int | None, limit: int, full: bool) -> None:
    entries = parse_archive_entries(read_archive())
    needle = query.strip().lower()
    matched: list[ArchiveEntry] = []
    for entry in entries:
        haystack = "\n".join(
            [
                str(entry.task_id),
                entry.archive_date,
                entry.dev_version,
                entry.priority,
                entry.entry_type,
                entry.area,
                entry.title,
                entry.summary,
                *entry.lines,
            ]
        ).lower()
        if task_id is not None and entry.task_id != task_id:
            continue
        if needle and needle not in haystack:
            continue
        matched.append(entry)

    if limit > 0:
        matched = matched[:limit]

    for entry in matched:
        metadata = archive_metadata(entry)
        print(
            f"#{entry.task_id} [{entry.archive_date}][{entry.dev_version}]"
            f"[status:{metadata['status']}][from:{metadata['from_section']}] {entry.title} - {entry.summary}"
        )
        if full:
            print("\n".join(entry.lines).rstrip())
        print()


def list_tags() -> None:
    entries = parse_entries(read_board())
    areas = sorted({entry.area for entry in entries})
    print("Priorities:")
    for priority in sorted(PRIORITIES):
        print(f"- priority:{priority}")
    print("Types:")
    for entry_type in sorted(TYPES):
        print(f"- type:{entry_type}")
    print("Areas:")
    for area in areas:
        print(f"- area:{area}")


def add_authorization(command: argparse.ArgumentParser) -> None:
    command.add_argument(
        "--architect-authorized",
        dest="authorized",
        action="store_true",
        help=(
            "Required for board mutations. Use only when this exact command was returned by "
            "quader-software-architect, or inside the documented performance-dev override."
        ),
    )
    command.add_argument(
        "--pm-authorized",
        dest="authorized",
        action="store_true",
        help=(
            "Deprecated compatibility alias for --architect-authorized. Do not use for new v2 flows."
        ),
    )


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="command", required=True)

    sub.add_parser("validate")

    add = sub.add_parser("add")
    add_authorization(add)
    add.add_argument("--title", required=True)
    add.add_argument("--summary", required=True)
    add.add_argument("--brief", required=True)
    add.add_argument("--priority", choices=sorted(PRIORITIES), default="medium")
    add.add_argument("--type", choices=sorted(TYPES), default="enhancement")
    add.add_argument("--area", required=True)
    add.add_argument("--section", choices=SECTIONS, default="Backlog")
    add.add_argument("--final-owner")
    add.add_argument("--plans")

    add_bug = sub.add_parser("add-bug")
    add_authorization(add_bug)
    add_bug.add_argument("--title", required=True)
    add_bug.add_argument("--summary", required=True)
    add_bug.add_argument("--brief", required=True)
    add_bug.add_argument("--priority", choices=sorted(PRIORITIES), default="high")
    add_bug.add_argument("--area", required=True)
    add_bug.add_argument("--final-owner")
    add_bug.add_argument("--plans")

    pm_start = sub.add_parser("pm-start")
    add_authorization(pm_start)
    pm_start.add_argument("--id", type=int, required=True)
    pm_start.add_argument("--run", required=True)
    pm_start.add_argument("--manager", required=True)
    pm_start.add_argument("--status", choices=sorted(PM_STATUSES), required=True)
    pm_start.add_argument("--note", required=True)

    authority = sub.add_parser("set-authority")
    add_authorization(authority)
    authority.add_argument("--id", type=int, required=True)
    authority.add_argument("--owner", choices=sorted(AUTHORITY_OWNERS), required=True)
    authority.add_argument("--status", choices=sorted(AUTHORITY_STATUSES), required=True)
    authority.add_argument("--agent", required=True)
    authority.add_argument("--note", required=True)

    assign = sub.add_parser("assign")
    add_authorization(assign)
    assign.add_argument("--id", type=int, required=True)
    assign.add_argument("--key", required=True)
    assign.add_argument("--role", required=True)
    assign.add_argument("--agent", required=True)
    assign.add_argument("--status", choices=sorted(ASSIGNMENT_STATUSES), required=True)
    assign.add_argument("--authority", choices=sorted(ASSIGNMENT_AUTHORITIES), required=True)
    assign.add_argument("--brief", required=True)

    assignment_status = sub.add_parser("assignment-status")
    add_authorization(assignment_status)
    assignment_status.add_argument("--id", type=int, required=True)
    assignment_status.add_argument("--key", required=True)
    assignment_status.add_argument("--status", choices=sorted(ASSIGNMENT_STATUSES), required=True)
    assignment_status.add_argument("--agent", required=True)
    assignment_status.add_argument("--note", required=True)

    coordination = sub.add_parser("coordination-note")
    add_authorization(coordination)
    coordination.add_argument("--id", type=int, required=True)
    coordination.add_argument("--note", required=True)

    active = sub.add_parser("set-active")
    add_authorization(active)
    active.add_argument("--id", type=int, required=True)
    active.add_argument("--lead", required=True)
    active.add_argument("--status", choices=sorted(ACTIVE_STATUSES), required=True)
    active.add_argument("--workers", type=int, required=True)
    active.add_argument("--note", required=True)

    clear_active = sub.add_parser("clear-active")
    add_authorization(clear_active)
    clear_active.add_argument("--id", type=int, required=True)

    edit_brief_cmd = sub.add_parser("edit-brief")
    add_authorization(edit_brief_cmd)
    edit_brief_cmd.add_argument("--id", type=int, required=True)
    edit_brief_cmd.add_argument("--brief", required=True)

    freshness = sub.add_parser("set-freshness")
    add_authorization(freshness)
    freshness.add_argument("--id", type=int, required=True)
    freshness.add_argument("--status", choices=sorted(FRESHNESS_STATUSES), required=True)
    freshness.add_argument("--note", required=True)
    freshness.add_argument("--checked", default="now")

    move = sub.add_parser("move")
    add_authorization(move)
    move.add_argument("--id", type=int, required=True)
    move.add_argument("--to", choices=SECTIONS, required=True)

    start = sub.add_parser("start")
    add_authorization(start)
    start.add_argument("--id", type=int, required=True)

    review = sub.add_parser("review")
    add_authorization(review)
    review.add_argument("--id", type=int, required=True)

    remove = sub.add_parser("remove")
    add_authorization(remove)
    remove.add_argument("--id", type=int, required=True)

    complete = sub.add_parser("complete")
    add_authorization(complete)
    complete.add_argument("--id", type=int, required=True)
    complete.add_argument("--resolution")

    archive_search = sub.add_parser("archive-search")
    archive_search.add_argument("--query", default="")
    archive_search.add_argument("--id", type=int)
    archive_search.add_argument("--limit", type=int, default=20)
    archive_search.add_argument("--full", action="store_true")

    reopen_archive = sub.add_parser("reopen-archive")
    add_authorization(reopen_archive)
    reopen_archive.add_argument("--id", type=int, required=True)
    reopen_archive.add_argument("--to", choices=SECTIONS)
    reopen_archive.add_argument("--reason", required=True)

    list_cmd = sub.add_parser("list")
    list_cmd.add_argument("--section", choices=SECTIONS)
    list_cmd.add_argument("--tag")

    sub.add_parser("list-tags")

    args = parser.parse_args(argv)

    if args.command in MUTATING_COMMANDS and not getattr(args, "authorized", False):
        raise SystemExit(AUTH_REQUIRED_MESSAGE)

    if args.command == "validate":
        board_ids = validate_board(verbose=False)
        archive_ids = validate_archive(verbose=False)
        overlap = sorted(board_ids & archive_ids)
        if overlap:
            raise SystemExit(
                f"task ids exist in both active board and archive: {', '.join(f'#{task_id}' for task_id in overlap)}"
            )
        print(f"validated {len(board_ids)} board entries and {len(archive_ids)} archived entries")
        return 0

    if args.command == "list":
        list_entries(args.section, args.tag)
        return 0

    if args.command == "archive-search":
        search_archive(args.query, args.id, args.limit, args.full)
        return 0

    if args.command == "list-tags":
        list_tags()
        return 0

    lines = read_board()
    if args.command == "add":
        task_id = insert_entry(
            lines,
            args.section,
            args.title,
            args.summary,
            args.priority,
            args.type,
            args.area,
            args.brief,
            args.final_owner,
            args.plans,
        )
        write_board(lines)
        print(f"added #{task_id}")
        return 0

    if args.command == "add-bug":
        task_id = insert_entry(
            lines,
            "Bugs",
            args.title,
            args.summary,
            args.priority,
            "bug",
            args.area,
            args.brief,
            args.final_owner,
            args.plans,
        )
        write_board(lines)
        print(f"added bug #{task_id}")
        return 0

    if args.command == "pm-start":
        upsert_line(
            lines,
            args.id,
            "  PM: ",
            f"  PM: [run:{args.run}][status:{args.status}][manager:{args.manager}] {args.note}",
        )
        write_board(lines)
        print(f"updated PM metadata for #{args.id}")
        return 0

    if args.command == "set-authority":
        upsert_line(
            lines,
            args.id,
            "  Authority: ",
            f"  Authority: [owner:{args.owner}][status:{args.status}][agent:{args.agent}] {args.note}",
        )
        write_board(lines)
        print(f"updated authority metadata for #{args.id}")
        return 0

    if args.command == "assign":
        upsert_assignment(
            lines,
            args.id,
            args.key,
            f"  Assignment: [key:{args.key}][role:{args.role}][agent:{args.agent}]"
            f"[status:{args.status}][authority:{args.authority}] {args.brief}",
        )
        write_board(lines)
        print(f"updated assignment {args.key} for #{args.id}")
        return 0

    if args.command == "assignment-status":
        entries = parse_entries(lines)
        entry = find_entry(entries, args.id)
        offset = find_assignment_index(entry, args.key)
        if offset is None:
            raise SystemExit(f"assignment not found for #{args.id}: {args.key}")
        match = ASSIGNMENT_RE.match(entry.lines[offset])
        if not match:
            raise SystemExit(f"invalid assignment metadata for #{args.id}: {entry.lines[offset]}")
        upsert_assignment(
            lines,
            args.id,
            args.key,
            f"  Assignment: [key:{args.key}][role:{match.group('role')}][agent:{args.agent}]"
            f"[status:{args.status}][authority:{match.group('authority')}] {args.note}",
        )
        write_board(lines)
        print(f"updated assignment {args.key} status for #{args.id}")
        return 0

    if args.command == "coordination-note":
        upsert_line(lines, args.id, "  Coordination: ", f"  Coordination: {args.note}")
        write_board(lines)
        print(f"updated coordination note for #{args.id}")
        return 0

    if args.command == "set-active":
        set_active(lines, args.id, args.lead, args.status, args.workers, args.note)
        write_board(lines)
        validate_board(verbose=False)
        print(f"updated active metadata for #{args.id}")
        return 0

    if args.command == "clear-active":
        removed = remove_line(lines, args.id, "  Active: ")
        write_board(lines)
        validate_board(verbose=False)
        if removed:
            print(f"cleared active metadata for #{args.id}")
        else:
            print(f"no active metadata for #{args.id}")
        return 0

    if args.command == "edit-brief":
        edit_brief(lines, args.id, args.brief)
        write_board(lines)
        validate_board(verbose=False)
        print(f"updated brief for #{args.id}")
        return 0

    if args.command == "set-freshness":
        set_freshness(lines, args.id, args.status, args.checked, args.note)
        write_board(lines)
        validate_board(verbose=False)
        print(f"updated freshness for #{args.id}")
        return 0

    if args.command == "move":
        if args.to == "In Progress":
            entry = find_entry(parse_entries(lines), args.id)
            require_fresh_entry(entry)
        move_entry(lines, args.id, args.to)
        return 0

    if args.command == "start":
        entry = find_entry(parse_entries(lines), args.id)
        if entry.section != "Backlog":
            raise SystemExit(f"start only moves Backlog -> In Progress; #{args.id} is in {entry.section}")
        require_fresh_entry(entry)
        move_entry(lines, args.id, "In Progress")
        return 0

    if args.command == "review":
        entry = find_entry(parse_entries(lines), args.id)
        if entry.section != "In Progress":
            raise SystemExit(
                f"review only moves In Progress -> In Review; #{args.id} is in {entry.section}"
            )
        move_entry(lines, args.id, "In Review")
        return 0

    if args.command == "remove":
        entry = find_entry(parse_entries(lines), args.id)
        lines = remove_entry(lines, entry)
        write_board(lines)
        print(f"removed #{args.id}")
        return 0

    if args.command == "complete":
        entry = find_entry(parse_entries(lines), args.id)
        archive_lines = read_archive()
        insert_archive_entry(archive_lines, archive_entry_lines(entry, args.resolution))
        lines = remove_entry(lines, entry)
        write_archive(archive_lines)
        write_board(lines)
        validate_board(verbose=False)
        validate_archive(verbose=False)
        status = "fixed" if entry.entry_type == "bug" or entry.section == "Bugs" else "completed"
        print(f"archived #{args.id} as {status}")
        return 0

    if args.command == "reopen-archive":
        archive_lines = read_archive()
        archive_entry = find_archive_entry(parse_archive_entries(archive_lines), args.id)
        existing_ids = {entry.task_id for entry in parse_entries(lines)}
        if archive_entry.task_id in existing_ids:
            raise SystemExit(f"task #{archive_entry.task_id} already exists on the active board")
        target_section = args.to or ("Bugs" if archive_entry.entry_type == "bug" else "Backlog")
        if archive_entry.entry_type == "bug" and target_section != "Bugs":
            raise SystemExit("archived bugs can only be reopened into Bugs")
        if archive_entry.entry_type != "bug" and target_section == "Bugs":
            raise SystemExit("non-bug archived tasks cannot be reopened into Bugs")

        entry_lines = reopened_entry_lines(archive_entry, args.reason)
        insert_existing_entry(lines, entry_lines, target_section)
        archive_lines = remove_archive_entry(archive_lines, archive_entry)
        write_board(lines)
        write_archive(archive_lines)
        validate_board(verbose=False)
        validate_archive(verbose=False)
        print(f"reopened archived #{args.id} to {target_section}")
        return 0

    return 1


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
