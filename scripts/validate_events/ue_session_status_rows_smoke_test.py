#!/usr/bin/env python3
"""Validate the expected session-status row rollup rules."""

from __future__ import annotations

import sys
from dataclasses import dataclass, field


@dataclass
class Snapshot:
    has_initialized_session: bool = False
    combat_active: bool = False
    zone_cleared: bool = False
    zone_id: str = ""
    active_event_id: str = ""
    available_event_ids: list[str] = field(default_factory=list)
    owned_card_ids: list[str] = field(default_factory=list)
    completed_quest_ids: list[str] = field(default_factory=list)
    deck_card_ids: list[str] = field(default_factory=list)
    hand_card_ids: list[str] = field(default_factory=list)
    player_hp: int = 0
    enemy_hp: int = 0
    energy: int = 0


@dataclass
class Diagnostics:
    issues: list[str] = field(default_factory=list)
    save_exists: bool = False


@dataclass
class Step:
    order: int
    label: str
    status_label: str
    detail: str
    current_focus: bool


def build_rows(
    snapshot: Snapshot,
    diagnostics: Diagnostics,
    walkthrough: list[Step],
    has_focused_event: bool,
    missing_placements: int,
    duplicate_placements: int,
    interactable_events: int,
    playable_card_count: int,
) -> dict[str, dict[str, object]]:
    next_step = next((step for step in walkthrough if step.current_focus), None)

    rows = {
        "session": {
            "value": "Initialized" if snapshot.has_initialized_session else "Not Initialized",
            "healthy": snapshot.has_initialized_session,
        },
        "bootstrap": {
            "value": "Diagnostics Clean" if not diagnostics.issues else f"{len(diagnostics.issues)} Issues",
            "healthy": len(diagnostics.issues) == 0,
            "detail": f"Save={'Present' if diagnostics.save_exists else 'None'} | Missing={missing_placements} | Duplicate={duplicate_placements}"
            if not diagnostics.issues
            else " | ".join(diagnostics.issues),
        },
        "active_event": {
            "value": "No Active Event" if not snapshot.active_event_id else snapshot.active_event_id,
            "healthy": bool(snapshot.active_event_id),
        },
        "focus": {
            "value": "Focused" if has_focused_event else "No Focused Event",
            "healthy": has_focused_event,
        },
        "walkthrough": {
            "value": f"{next_step.order}. {next_step.label}"
            if next_step
            else ("Zone Clear Reached" if snapshot.zone_cleared else "No Walkthrough State"),
            "healthy": len(walkthrough) > 0,
        },
        "combat": {
            "value": f"HP {snapshot.player_hp} / Enemy {snapshot.enemy_hp} / Energy {snapshot.energy}"
            if snapshot.combat_active
            else "Exploration",
            "healthy": (not snapshot.combat_active) or snapshot.player_hp > 0,
            "detail": f"Hand {len(snapshot.hand_card_ids)} | Playable {playable_card_count}"
            if snapshot.combat_active
            else f"Available Events {len(snapshot.available_event_ids)} | Interactable {interactable_events}",
        },
        "progression": {
            "value": "Zone Cleared" if snapshot.zone_cleared else "In Progress",
            "healthy": snapshot.zone_cleared,
        },
    }
    return rows


def assert_equal(actual: object, expected: object, label: str) -> None:
    if actual != expected:
        raise AssertionError(f"{label}: expected {expected!r}, got {actual!r}")


def main() -> int:
    rows = build_rows(Snapshot(), Diagnostics(), [], False, 0, 0, 0, 0)
    assert_equal(rows["session"]["healthy"], False, "session starts unhealthy")
    assert_equal(rows["bootstrap"]["healthy"], True, "bootstrap starts clean without issues")
    assert_equal(rows["active_event"]["value"], "No Active Event", "active event starts empty")
    assert_equal(rows["walkthrough"]["value"], "No Walkthrough State", "walkthrough starts empty")
    assert_equal(rows["combat"]["value"], "Exploration", "combat defaults to exploration")

    rows = build_rows(
        Snapshot(
            has_initialized_session=True,
            combat_active=True,
            zone_id="zone_mvp_001",
            active_event_id="event_ridge_scout_001",
            available_event_ids=["event_ridge_scout_001", "event_proxy_gate_001"],
            owned_card_ids=["act_strike_001"],
            completed_quest_ids=["event_wake_cache_001"],
            deck_card_ids=["act_strike_001", "act_guard_001"],
            hand_card_ids=["act_strike_001"],
            player_hp=17,
            enemy_hp=9,
            energy=2,
        ),
        Diagnostics(issues=["json missing"], save_exists=True),
        [Step(5, "Ridge Scout Battle", "Next Up", "Finish battle", True)],
        True,
        1,
        0,
        1,
        1,
    )
    assert_equal(rows["session"]["healthy"], True, "session healthy after init")
    assert_equal(rows["bootstrap"]["value"], "1 Issues", "bootstrap reports issue count")
    assert_equal(rows["active_event"]["value"], "event_ridge_scout_001", "active event surfaces id")
    assert_equal(rows["focus"]["healthy"], True, "focus healthy in range")
    assert_equal(rows["walkthrough"]["value"], "5. Ridge Scout Battle", "walkthrough surfaces current step")
    assert_equal(rows["combat"]["value"], "HP 17 / Enemy 9 / Energy 2", "combat summary includes hp and energy")
    assert_equal(rows["combat"]["detail"], "Hand 1 | Playable 1", "combat detail includes hand/playable counts")

    rows = build_rows(
        Snapshot(
            has_initialized_session=True,
            zone_cleared=True,
            available_event_ids=["event_proxy_gate_001"],
            completed_quest_ids=["event_proxy_gate_001"],
        ),
        Diagnostics(save_exists=True),
        [],
        False,
        0,
        0,
        0,
        0,
    )
    assert_equal(rows["progression"]["value"], "Zone Cleared", "progression surfaces clear state")
    assert_equal(rows["progression"]["healthy"], True, "progression becomes healthy on clear")
    assert_equal(rows["walkthrough"]["value"], "Zone Clear Reached", "walkthrough falls back to clear state")

    print("OK: UE session status row smoke tests passed")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as exc:
        print(str(exc), file=sys.stderr)
        raise SystemExit(1)
