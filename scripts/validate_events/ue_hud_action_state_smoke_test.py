#!/usr/bin/env python3
"""Validate the expected HUD action enablement rules."""

from __future__ import annotations

import sys
from dataclasses import dataclass


@dataclass
class Snapshot:
    has_initialized_session: bool = False
    combat_active: bool = False
    active_event_id: str = ""


@dataclass
class Diagnostics:
    save_exists: bool = False


@dataclass
class EventView:
    event_id: str
    event_type: str


def build_action_states(
    snapshot: Snapshot,
    diagnostics: Diagnostics,
    has_bootstrap_actor: bool,
    has_focused_event: bool,
    event_views: list[EventView],
    has_last_ai_response: bool = False,
    last_ai_response_valid: bool = False,
) -> dict[str, dict[str, object]]:
    event_by_id = {event.event_id: event for event in event_views}

    def is_battle_active_event() -> bool:
        event = event_by_id.get(snapshot.active_event_id)
        return event is not None and event.event_type == "Battle"

    def is_non_battle_active_event() -> bool:
        event = event_by_id.get(snapshot.active_event_id)
        return event is not None and event.event_type != "Battle"

    return {
        "bootstrap_session": {"enabled": has_bootstrap_actor},
        "initialize_new_session": {"enabled": True},
        "restore_session": {"enabled": diagnostics.save_exists},
        "save_session": {"enabled": snapshot.has_initialized_session},
        "interact_focused_event": {"enabled": has_focused_event},
        "resolve_active_event": {
            "enabled": snapshot.has_initialized_session and is_non_battle_active_event()
        },
        "start_active_combat": {
            "enabled": snapshot.has_initialized_session and not snapshot.combat_active and is_battle_active_event()
        },
        "run_enemy_turn": {"enabled": snapshot.combat_active},
        "grant_developer_card": {"enabled": snapshot.has_initialized_session},
        "build_ai_request": {"enabled": snapshot.has_initialized_session and bool(snapshot.active_event_id)},
        "build_fallback_ai": {"enabled": snapshot.has_initialized_session and bool(snapshot.active_event_id)},
        "apply_last_ai_response": {
            "enabled": snapshot.has_initialized_session and has_last_ai_response and last_ai_response_valid
        },
    }


def assert_equal(actual: object, expected: object, label: str) -> None:
    if actual != expected:
        raise AssertionError(f"{label}: expected {expected!r}, got {actual!r}")


def main() -> int:
    base_events = [
        EventView("event_contract_broker_001", "Npc"),
        EventView("event_ridge_scout_001", "Battle"),
    ]

    states = build_action_states(Snapshot(), Diagnostics(), False, False, base_events)
    assert_equal(states["bootstrap_session"]["enabled"], False, "bootstrap disabled without actor")
    assert_equal(states["initialize_new_session"]["enabled"], True, "initialize always enabled")
    assert_equal(states["restore_session"]["enabled"], False, "restore disabled without save")
    assert_equal(states["save_session"]["enabled"], False, "save disabled before init")
    assert_equal(states["interact_focused_event"]["enabled"], False, "interact disabled without focus")
    assert_equal(states["apply_last_ai_response"]["enabled"], False, "apply ai disabled without response")

    states = build_action_states(
        Snapshot(has_initialized_session=True, active_event_id="event_contract_broker_001"),
        Diagnostics(save_exists=True),
        True,
        True,
        base_events,
        has_last_ai_response=True,
        last_ai_response_valid=True,
    )
    assert_equal(states["bootstrap_session"]["enabled"], True, "bootstrap enabled with actor")
    assert_equal(states["restore_session"]["enabled"], True, "restore enabled with save")
    assert_equal(states["save_session"]["enabled"], True, "save enabled after init")
    assert_equal(states["interact_focused_event"]["enabled"], True, "interact enabled with focus")
    assert_equal(states["resolve_active_event"]["enabled"], True, "resolve enabled for non-battle active event")
    assert_equal(states["start_active_combat"]["enabled"], False, "start combat disabled for non-battle active event")
    assert_equal(states["build_ai_request"]["enabled"], True, "AI request enabled with active event")
    assert_equal(states["apply_last_ai_response"]["enabled"], True, "apply ai enabled with valid response")

    states = build_action_states(
        Snapshot(has_initialized_session=True, active_event_id="event_ridge_scout_001"),
        Diagnostics(save_exists=False),
        True,
        False,
        base_events,
        has_last_ai_response=True,
        last_ai_response_valid=False,
    )
    assert_equal(states["resolve_active_event"]["enabled"], False, "resolve disabled for battle active event")
    assert_equal(states["start_active_combat"]["enabled"], True, "start combat enabled for battle active event")
    assert_equal(states["run_enemy_turn"]["enabled"], False, "enemy turn disabled before combat starts")
    assert_equal(states["apply_last_ai_response"]["enabled"], False, "apply ai disabled for invalid response")

    states = build_action_states(
        Snapshot(has_initialized_session=True, combat_active=True, active_event_id="event_ridge_scout_001"),
        Diagnostics(save_exists=False),
        True,
        False,
        base_events,
    )
    assert_equal(states["start_active_combat"]["enabled"], False, "start combat disabled during combat")
    assert_equal(states["run_enemy_turn"]["enabled"], True, "enemy turn enabled during combat")
    assert_equal(states["grant_developer_card"]["enabled"], True, "grant card stays enabled after init")

    print("OK: UE HUD action state smoke tests passed")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as exc:
        print(str(exc), file=sys.stderr)
        raise SystemExit(1)
