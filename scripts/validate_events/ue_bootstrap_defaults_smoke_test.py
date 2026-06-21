#!/usr/bin/env python3
"""Validate bootstrap defaults stay aligned with project settings defaults."""

from __future__ import annotations

import re
import sys
from pathlib import Path


def extract_string(path: Path, field_name: str) -> str:
    text = path.read_text(encoding="utf-8")
    match = re.search(rf"\b{re.escape(field_name)}\b\s*=\s*TEXT\(\"([^\"]+)\"\);", text)
    if not match:
        raise AssertionError(f"Could not find string default for {field_name} in {path}")
    return match.group(1)


def extract_int(path: Path, field_name: str) -> int:
    text = path.read_text(encoding="utf-8")
    match = re.search(rf"{re.escape(field_name)}\s*=\s*([0-9]+);", text)
    if not match:
        raise AssertionError(f"Could not find int default for {field_name} in {path}")
    return int(match.group(1))


def assert_equal(actual: object, expected: object, label: str) -> None:
    if actual != expected:
        raise AssertionError(f"{label}: expected {expected!r}, got {actual!r}")


def main() -> int:
    repo_root = Path(__file__).resolve().parents[2]
    settings_header = repo_root / "UnrealProject" / "Source" / "Greeisland" / "Runtime" / "GreeislandProjectSettings.h"
    bootstrap_header = repo_root / "UnrealProject" / "Source" / "Greeisland" / "Actors" / "GreeislandBootstrapActor.h"

    assert_equal(
        extract_string(bootstrap_header, "CardJsonPath"),
        extract_string(settings_header, "CardJsonPath"),
        "bootstrap card path default",
    )
    assert_equal(
        extract_string(bootstrap_header, "EventJsonPath"),
        extract_string(settings_header, "EventJsonPath"),
        "bootstrap event path default",
    )
    assert_equal(
        extract_string(bootstrap_header, "SaveSlotName"),
        extract_string(settings_header, "SaveSlotName"),
        "bootstrap save slot default",
    )
    assert_equal(
        extract_int(bootstrap_header, "SaveUserIndex"),
        extract_int(settings_header, "SaveUserIndex"),
        "bootstrap save user index default",
    )

    print("OK: UE bootstrap defaults smoke tests passed")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as exc:
        print(str(exc), file=sys.stderr)
        raise SystemExit(1)
