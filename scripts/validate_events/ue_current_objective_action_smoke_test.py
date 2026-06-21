#!/usr/bin/env python3
"""Validate the expected current-objective action routing rules."""

from __future__ import annotations

import sys


def choose_objective_action(
    step_order: int,
    save_exists: bool = False,
    bootstrap_enabled: bool = True,
    init_enabled: bool = True,
    combat_active: bool = False,
    has_playable_hand: bool = False,
    focused_matches: bool = False,
    active_matches: bool = False,
    event_primary_action: str = "",
) -> str:
    if step_order == 1:
        if bootstrap_enabled:
            return "bootstrap_session"
        if init_enabled:
            return "initialize_new_session"
        return ""

    if step_order == 7:
        return "restore_session" if save_exists else "save_session"

    if step_order == 5 and combat_active:
        return "play_combat_card" if has_playable_hand else "run_enemy_turn"

    if focused_matches:
        return "interact_focused_event"

    if active_matches and event_primary_action:
        return event_primary_action

    return "interact_focused_event"


def assert_equal(actual: object, expected: object, label: str) -> None:
    if actual != expected:
        raise AssertionError(f"{label}: expected {expected!r}, got {actual!r}")


def main() -> int:
    assert_equal(choose_objective_action(1, bootstrap_enabled=True), "bootstrap_session", "bootstrap objective")
    assert_equal(choose_objective_action(1, bootstrap_enabled=False, init_enabled=True), "initialize_new_session", "fallback init objective")
    assert_equal(choose_objective_action(7, save_exists=False), "save_session", "save objective")
    assert_equal(choose_objective_action(7, save_exists=True), "restore_session", "restore objective")
    assert_equal(choose_objective_action(5, combat_active=True, has_playable_hand=True), "play_combat_card", "combat hand objective")
    assert_equal(choose_objective_action(5, combat_active=True, has_playable_hand=False), "run_enemy_turn", "enemy turn fallback objective")
    assert_equal(choose_objective_action(3, focused_matches=True), "interact_focused_event", "focused event objective")
    assert_equal(choose_objective_action(6, active_matches=True, event_primary_action="resolve_active_event"), "resolve_active_event", "active non-battle objective")
    assert_equal(choose_objective_action(5, active_matches=True, event_primary_action="start_active_combat"), "start_active_combat", "active battle objective")

    print("OK: UE current objective action smoke tests passed")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as exc:
        print(str(exc), file=sys.stderr)
        raise SystemExit(1)
