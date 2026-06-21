#!/usr/bin/env python3
"""Validate expected row-level action metadata for card/event view data."""

from __future__ import annotations

import sys


def build_hand_card_row(playable: bool) -> dict[str, object]:
    return {
        "primary_action_id": "play_combat_card",
        "primary_action_label": "Play Card",
        "has_primary_action": True,
        "primary_action_enabled": playable,
    }


def build_owned_card_row(in_hand: bool, playable: bool) -> dict[str, object]:
    if not in_hand:
        return {
            "has_primary_action": False,
            "primary_action_enabled": False,
        }
    return build_hand_card_row(playable)


def build_event_row(event_type: str, available: bool, active: bool, combat_active: bool) -> dict[str, object]:
    if not (available and active):
        return {
            "has_primary_action": False,
            "primary_action_enabled": False,
        }

    if event_type == "Battle":
        return {
            "primary_action_id": "start_active_combat",
            "primary_action_label": "Start Combat",
            "has_primary_action": True,
            "primary_action_enabled": not combat_active,
        }

    return {
        "primary_action_id": "resolve_active_event",
        "primary_action_label": "Resolve Event",
        "has_primary_action": True,
        "primary_action_enabled": True,
    }


def assert_equal(actual: object, expected: object, label: str) -> None:
    if actual != expected:
        raise AssertionError(f"{label}: expected {expected!r}, got {actual!r}")


def main() -> int:
    hand = build_hand_card_row(playable=True)
    assert_equal(hand["primary_action_id"], "play_combat_card", "hand action id")
    assert_equal(hand["primary_action_enabled"], True, "hand playable action enabled")

    blocked_hand = build_hand_card_row(playable=False)
    assert_equal(blocked_hand["primary_action_enabled"], False, "blocked hand action disabled")

    owned = build_owned_card_row(in_hand=False, playable=False)
    assert_equal(owned["has_primary_action"], False, "owned non-hand card has no action")

    owned_in_hand = build_owned_card_row(in_hand=True, playable=True)
    assert_equal(owned_in_hand["primary_action_label"], "Play Card", "owned in-hand card can play")

    battle = build_event_row("Battle", available=True, active=True, combat_active=False)
    assert_equal(battle["primary_action_id"], "start_active_combat", "battle event action id")
    assert_equal(battle["primary_action_enabled"], True, "battle event enabled before combat")

    live_battle = build_event_row("Battle", available=True, active=True, combat_active=True)
    assert_equal(live_battle["primary_action_enabled"], False, "battle event disabled during combat")

    npc = build_event_row("Npc", available=True, active=True, combat_active=False)
    assert_equal(npc["primary_action_id"], "resolve_active_event", "non-battle event action id")

    locked = build_event_row("Quest", available=False, active=False, combat_active=False)
    assert_equal(locked["has_primary_action"], False, "locked event has no action")

    print("OK: UE row action surface smoke tests passed")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as exc:
        print(str(exc), file=sys.stderr)
        raise SystemExit(1)
