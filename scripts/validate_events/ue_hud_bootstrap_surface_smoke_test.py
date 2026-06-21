#!/usr/bin/env python3
"""Validate that the debug HUD exposes bootstrap diagnostics hooks."""

from __future__ import annotations

import sys
from pathlib import Path


def assert_true(value: bool, label: str, detail: object = None) -> None:
    if not value:
        raise AssertionError(f"{label} failed: {detail}")


def main() -> int:
    repo_root = Path(__file__).resolve().parents[2]
    widget_header = repo_root / "UnrealProject" / "Source" / "Greeisland" / "UI" / "GreeislandDebugHudWidget.h"
    bootstrap_header = repo_root / "UnrealProject" / "Source" / "Greeisland" / "Actors" / "GreeislandBootstrapActor.h"
    text = widget_header.read_text(encoding="utf-8")
    bootstrap_text = bootstrap_header.read_text(encoding="utf-8")

    assert_true("BootstrapSessionFromActor" in text, "widget exposes bootstrap action")
    assert_true("GetLastBootstrapDiagnostics" in text, "widget exposes bootstrap diagnostics getter")
    assert_true("LastBootstrapDiagnostics" in text, "widget stores bootstrap diagnostics")
    assert_true("MissingEventActorIds" in bootstrap_text, "bootstrap diagnostics expose missing events")
    assert_true("DuplicateEventActorIds" in bootstrap_text, "bootstrap diagnostics expose duplicate events")
    assert_true("UnexpectedEventActorIds" in bootstrap_text, "bootstrap diagnostics expose unexpected events")
    assert_true("FGreeislandExpectedEventPlacement" in bootstrap_text, "bootstrap diagnostics expose placement checklist struct")
    assert_true("ExpectedEventPlacements" in bootstrap_text, "bootstrap diagnostics expose placement checklist array")

    print("OK: UE HUD bootstrap surface smoke tests passed")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as exc:
        print(str(exc), file=sys.stderr)
        raise SystemExit(1)
