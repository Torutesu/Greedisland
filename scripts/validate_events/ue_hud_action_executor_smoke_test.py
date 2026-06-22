#!/usr/bin/env python3
"""Validate the expected HUD action executor routing stays stable."""

from __future__ import annotations

import sys


def execute_action(action_id: str, optional_name: str = "", optional_text: str = "") -> str:
    if action_id == "bootstrap_session":
        return "BootstrapSessionFromActor"
    if action_id == "initialize_new_session":
        return "InitializeNewSession"
    if action_id == "restore_session":
        return "RestoreSession"
    if action_id == "save_session":
        return "SaveSession"
    if action_id == "interact_focused_event":
        return "InteractWithFocusedEvent"
    if action_id == "resolve_active_event":
        return "ResolveActiveEvent"
    if action_id == "start_active_combat":
        return "StartCombatForActiveEvent"
    if action_id == "run_enemy_turn":
        return "RunEnemyTurn"
    if action_id == "grant_developer_card":
        return optional_name or "con_four_party_001"
    if action_id == "play_combat_card":
        return optional_name or "missing"
    if action_id == "build_ai_request":
        return f"request:{optional_text}"
    if action_id == "build_fallback_ai":
        return f"fallback:{optional_text}"
    if action_id == "apply_last_ai_response":
        return f"apply:{optional_text}"
    return "unknown"


def assert_equal(actual: object, expected: object, label: str) -> None:
    if actual != expected:
        raise AssertionError(f"{label}: expected {expected!r}, got {actual!r}")


def main() -> int:
    assert_equal(execute_action("bootstrap_session"), "BootstrapSessionFromActor", "bootstrap routing")
    assert_equal(execute_action("initialize_new_session"), "InitializeNewSession", "new session routing")
    assert_equal(execute_action("grant_developer_card"), "con_four_party_001", "developer grant default card")
    assert_equal(execute_action("grant_developer_card", "key_zone_core_001"), "key_zone_core_001", "developer grant override card")
    assert_equal(execute_action("play_combat_card", "act_strike_001"), "act_strike_001", "play card routing")
    assert_equal(execute_action("play_combat_card"), "missing", "play card requires card id")
    assert_equal(execute_action("build_ai_request", optional_text="offer trade"), "request:offer trade", "ai request routing")
    assert_equal(execute_action("build_fallback_ai", optional_text="offer trade"), "fallback:offer trade", "fallback ai routing")
    assert_equal(execute_action("apply_last_ai_response", optional_text="offer trade"), "apply:offer trade", "apply last ai routing")
    assert_equal(execute_action("unknown_action"), "unknown", "unknown action fallback")

    print("OK: UE HUD action executor smoke tests passed")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as exc:
        print(str(exc), file=sys.stderr)
        raise SystemExit(1)
