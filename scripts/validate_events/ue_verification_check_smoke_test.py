#!/usr/bin/env python3
"""Validate the expected MVP verification-check rollup rules."""

from __future__ import annotations

import sys
from dataclasses import dataclass, field


@dataclass
class Snapshot:
    has_initialized_session: bool = False
    zone_cleared: bool = False
    active_event_id: str = ""


@dataclass
class Diagnostics:
    issues: list[str] = field(default_factory=list)
    save_exists: bool = False


@dataclass
class EventActorStatus:
    has_actor_placement: bool
    placement_count: int
    is_interactable_now: bool


@dataclass
class WalkthroughStep:
    completed: bool
    current_focus: bool


def build_verification_checks(
    snapshot: Snapshot,
    diagnostics: Diagnostics,
    statuses: list[EventActorStatus],
    walkthrough: list[WalkthroughStep],
) -> dict[str, bool]:
    missing = sum(1 for status in statuses if not status.has_actor_placement)
    duplicates = sum(1 for status in statuses if status.placement_count > 1)
    interactables = sum(1 for status in statuses if status.is_interactable_now)
    has_walkthrough_state = (
        len(walkthrough) == 0
        or any(step.current_focus or step.completed for step in walkthrough)
    )

    return {
        "session_initialized": snapshot.has_initialized_session,
        "bootstrap_issues_clear": len(diagnostics.issues) == 0,
        "event_placements_complete": missing == 0,
        "event_placements_unique": duplicates == 0,
        "walkthrough_has_focus": has_walkthrough_state,
        "interactable_event_visible": interactables > 0 or not snapshot.has_initialized_session,
        "save_path_ready": snapshot.has_initialized_session,
        "zone_clear_verified": snapshot.zone_cleared,
        "ai_debug_ready": snapshot.has_initialized_session and bool(snapshot.active_event_id),
    }


def assert_equal(actual: object, expected: object, label: str) -> None:
    if actual != expected:
        raise AssertionError(f"{label}: expected {expected!r}, got {actual!r}")


def main() -> int:
    checks = build_verification_checks(Snapshot(), Diagnostics(), [], [])
    assert_equal(checks["session_initialized"], False, "session init starts false")
    assert_equal(checks["bootstrap_issues_clear"], True, "issues clear when empty")
    assert_equal(checks["interactable_event_visible"], True, "interactable relaxed before init")
    assert_equal(checks["save_path_ready"], False, "save path not ready before init")
    assert_equal(checks["zone_clear_verified"], False, "zone clear starts false")

    checks = build_verification_checks(
        Snapshot(has_initialized_session=True, active_event_id="event_contract_broker_001"),
        Diagnostics(issues=["json missing"], save_exists=True),
        [
            EventActorStatus(has_actor_placement=True, placement_count=1, is_interactable_now=False),
            EventActorStatus(has_actor_placement=False, placement_count=0, is_interactable_now=False),
            EventActorStatus(has_actor_placement=True, placement_count=2, is_interactable_now=True),
        ],
        [WalkthroughStep(completed=True, current_focus=False), WalkthroughStep(completed=False, current_focus=True)],
    )
    assert_equal(checks["session_initialized"], True, "session init after bootstrap")
    assert_equal(checks["bootstrap_issues_clear"], False, "issues fail when present")
    assert_equal(checks["event_placements_complete"], False, "missing placements fail")
    assert_equal(checks["event_placements_unique"], False, "duplicates fail")
    assert_equal(checks["walkthrough_has_focus"], True, "walkthrough with focus passes")
    assert_equal(checks["interactable_event_visible"], True, "interactable event passes")
    assert_equal(checks["save_path_ready"], True, "save path ready after init")
    assert_equal(checks["ai_debug_ready"], True, "ai debug ready with active event")

    checks = build_verification_checks(
        Snapshot(has_initialized_session=True, zone_cleared=True),
        Diagnostics(),
        [EventActorStatus(has_actor_placement=True, placement_count=1, is_interactable_now=False)],
        [WalkthroughStep(completed=True, current_focus=False)],
    )
    assert_equal(checks["bootstrap_issues_clear"], True, "issues pass when empty")
    assert_equal(checks["event_placements_complete"], True, "placements pass when complete")
    assert_equal(checks["event_placements_unique"], True, "placements pass when unique")
    assert_equal(checks["walkthrough_has_focus"], True, "completed walkthrough still passes")
    assert_equal(checks["interactable_event_visible"], False, "no interactable after init fails")
    assert_equal(checks["zone_clear_verified"], True, "zone clear passes")
    assert_equal(checks["ai_debug_ready"], False, "ai debug false without active event")

    print("OK: UE verification check smoke tests passed")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as exc:
        print(str(exc), file=sys.stderr)
        raise SystemExit(1)
