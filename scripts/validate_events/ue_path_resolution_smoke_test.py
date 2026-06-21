#!/usr/bin/env python3
"""Validate that default UE-facing JSON paths resolve from ProjectDir to repo data files."""

from __future__ import annotations

import configparser
import sys
from pathlib import Path


def assert_true(value: bool, label: str, detail: object = None) -> None:
    if not value:
        raise AssertionError(f"{label} failed: {detail}")


def main() -> int:
    repo_root = Path(__file__).resolve().parents[2]
    project_dir = repo_root / "UnrealProject"
    config_path = project_dir / "Config" / "DefaultGame.ini"

    parser = configparser.ConfigParser()
    parser.read(config_path, encoding="utf-8")
    section = "/Script/Greeisland.GreeislandProjectSettings"

    card_rel = parser[section]["CardJsonPath"]
    event_rel = parser[section]["EventJsonPath"]

    card_resolved = (project_dir / card_rel).resolve()
    event_resolved = (project_dir / event_rel).resolve()

    assert_true(card_resolved.exists(), "card path exists", card_resolved)
    assert_true(event_resolved.exists(), "event path exists", event_resolved)
    assert_true(card_resolved == repo_root / "data" / "cards" / "cards.mvp.json", "card path target", card_resolved)
    assert_true(event_resolved == repo_root / "data" / "events" / "events.mvp.json", "event path target", event_resolved)

    print("OK: UE path resolution smoke tests passed")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as exc:
        print(str(exc), file=sys.stderr)
        raise SystemExit(1)
