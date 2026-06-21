#!/usr/bin/env python3
"""Validate runtime card/event view data expose Blueprint-friendly summaries."""

from __future__ import annotations

import sys
from pathlib import Path


def assert_true(value: bool, label: str, detail: object = None) -> None:
    if not value:
        raise AssertionError(f"{label} failed: {detail}")


def main() -> int:
    repo_root = Path(__file__).resolve().parents[2]
    runtime_header = repo_root / "UnrealProject" / "Source" / "Greeisland" / "Runtime" / "GreeislandGameSubsystem.h"
    runtime_cpp = repo_root / "UnrealProject" / "Source" / "Greeisland" / "Runtime" / "GreeislandGameSubsystem.cpp"
    header_text = runtime_header.read_text(encoding="utf-8")
    cpp_text = runtime_cpp.read_text(encoding="utf-8")

    assert_true("KindLabel" in header_text, "card view exposes kind label")
    assert_true("StateSummary" in header_text, "card view exposes state summary")
    assert_true("DetailSummary" in header_text, "card view exposes detail summary")
    assert_true("TypeLabel" in header_text, "event view exposes type label")
    assert_true("StatusSummary" in header_text, "event view exposes status summary")
    assert_true("DetailSummary" in header_text, "event view exposes detail summary")
    assert_true("CardKindToLabel" in cpp_text, "runtime implements card kind label helper")
    assert_true("EventTypeToLabel" in cpp_text, "runtime implements event type label helper")
    assert_true("ViewData.StateSummary" in cpp_text, "runtime populates card state summary")
    assert_true("ViewData.DetailSummary" in cpp_text, "runtime populates card detail summary")
    assert_true("ViewData.TypeLabel" in cpp_text, "runtime populates event type label")
    assert_true("ViewData.StatusSummary" in cpp_text, "runtime populates event status summary")

    print("OK: UE runtime viewdata surface smoke tests passed")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as exc:
        print(str(exc), file=sys.stderr)
        raise SystemExit(1)
