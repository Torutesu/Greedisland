#!/usr/bin/env python3
"""Smoke-test defeat handling in the session combat flow."""

from __future__ import annotations

import json
import sys
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any


@dataclass
class CombatState:
    player_hp: int = 30
    enemy_hp: int = 0
    hand: list[str] = field(default_factory=list)
    outcome: str = "InProgress"


@dataclass
class Session:
    events: dict[str, dict[str, Any]]
    owned_card_ids: list[str] = field(default_factory=list)
    deck_card_ids: list[str] = field(default_factory=list)
    completed_event_ids: list[str] = field(default_factory=list)
    available_event_ids: list[str] = field(default_factory=list)
    active_event_id: str = ""
    combat_active: bool = False
    combat_state: CombatState = field(default_factory=CombatState)


def load_events(path: Path) -> dict[str, dict[str, Any]]:
    data = json.loads(path.read_text(encoding="utf-8"))
    return {event["eventId"]: event for event in data["events"]}


def grant_event_rewards(session: Session, event: dict[str, Any]) -> None:
    for reward_card_id in event["rewardCardIds"]:
        if reward_card_id not in session.owned_card_ids:
            session.owned_card_ids.append(reward_card_id)
        if reward_card_id not in session.deck_card_ids:
            session.deck_card_ids.append(reward_card_id)
    if event["eventId"] not in session.completed_event_ids:
        session.completed_event_ids.append(event["eventId"])
    for next_event_id in event["nextEventIds"]:
        if next_event_id not in session.available_event_ids:
            session.available_event_ids.append(next_event_id)


def resolve_event(session: Session, event_id: str) -> None:
    event = session.events[event_id]
    for required_card_id in event["requiredCardIds"]:
        if required_card_id not in session.owned_card_ids:
            raise AssertionError(f"missing required card {required_card_id}")
    if event["type"] != "Battle":
        grant_event_rewards(session, event)
        session.active_event_id = event_id


def start_battle(session: Session, event_id: str, opening_draw_count: int) -> None:
    event = session.events[event_id]
    session.combat_active = True
    session.active_event_id = event_id
    session.combat_state = CombatState(
        player_hp=30,
        enemy_hp=event["enemyHp"],
        hand=session.deck_card_ids[:opening_draw_count],
        outcome="InProgress",
    )


def run_enemy_turn(session: Session) -> None:
    event = session.events[session.active_event_id]
    session.combat_state.player_hp = max(0, session.combat_state.player_hp - event["enemyAttack"])
    if session.combat_state.player_hp <= 0:
        session.combat_state.outcome = "PlayerDefeat"
        session.combat_active = False
        if session.active_event_id not in session.available_event_ids:
            session.available_event_ids.append(session.active_event_id)
        session.combat_state = CombatState()


def assert_true(value: bool, label: str, detail: Any = None) -> None:
    if not value:
        raise AssertionError(f"{label} failed: {detail}")


def main() -> int:
    events = load_events(Path("data/events/events.mvp.json"))
    session = Session(events=events, available_event_ids=["event_wake_cache_001"], active_event_id="event_wake_cache_001")

    resolve_event(session, "event_wake_cache_001")
    session.available_event_ids.append("event_contract_broker_001")
    resolve_event(session, "event_contract_broker_001")
    session.available_event_ids.append("event_ridge_scout_001")

    start_battle(session, "event_ridge_scout_001", opening_draw_count=2)
    session.combat_state.player_hp = 4
    run_enemy_turn(session)

    assert_true(not session.combat_active, "combat should close after defeat", session.combat_active)
    assert_true("event_ridge_scout_001" in session.available_event_ids, "battle should stay retryable", session.available_event_ids)
    assert_true("event_ridge_scout_001" not in session.completed_event_ids, "defeat should not complete event", session.completed_event_ids)
    assert_true(session.combat_state.outcome == "InProgress", "combat state reset", session.combat_state.outcome)

    print("OK: defeat resolution smoke tests passed")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as exc:
        print(str(exc), file=sys.stderr)
        raise SystemExit(1)

