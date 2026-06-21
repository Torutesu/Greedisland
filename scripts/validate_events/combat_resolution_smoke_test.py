#!/usr/bin/env python3
"""Smoke-test battle event progression from combat start to reward resolution."""

from __future__ import annotations

import json
import sys
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any


@dataclass
class CombatState:
    enemy_hp: int = 0
    hand: list[str] = field(default_factory=list)
    outcome: str = "InProgress"


@dataclass
class Session:
    events: dict[str, dict[str, Any]]
    known_cards: dict[str, dict[str, Any]]
    owned_card_ids: list[str] = field(default_factory=list)
    deck_card_ids: list[str] = field(default_factory=list)
    completed_event_ids: list[str] = field(default_factory=list)
    available_event_ids: list[str] = field(default_factory=list)
    active_event_id: str = ""
    combat_active: bool = False
    combat_state: CombatState = field(default_factory=CombatState)


def load_cards(path: Path) -> dict[str, dict[str, Any]]:
    data = json.loads(path.read_text(encoding="utf-8"))
    return {card["cardId"]: card for card in data["cards"]}


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


def resolve_non_battle_event(session: Session, event_id: str) -> tuple[bool, list[str]]:
    event = session.events[event_id]
    if event["type"] == "Battle":
        return False, [f"{event_id} is battle-only"]
    for required_card_id in event["requiredCardIds"]:
        if required_card_id not in session.owned_card_ids:
            return False, [f"missing {required_card_id}"]
    grant_event_rewards(session, event)
    session.active_event_id = event_id
    return True, []


def start_battle(session: Session, event_id: str, opening_draw_count: int) -> tuple[bool, list[str]]:
    event = session.events[event_id]
    if event["type"] != "Battle":
        return False, [f"{event_id} is not battle"]
    if event_id not in session.available_event_ids:
        return False, [f"{event_id} not available"]
    session.combat_active = True
    session.active_event_id = event_id
    session.combat_state = CombatState(
        enemy_hp=event["enemyHp"],
        hand=session.deck_card_ids[:opening_draw_count],
        outcome="InProgress",
    )
    return True, []


def play_card(session: Session, card_id: str) -> tuple[bool, list[str]]:
    if not session.combat_active:
        return False, ["no active combat"]
    if card_id not in session.combat_state.hand:
        return False, [f"{card_id} not in hand"]
    card = session.known_cards[card_id]
    for effect in card["effects"]:
        if effect["type"] == "Damage":
            session.combat_state.enemy_hp = max(0, session.combat_state.enemy_hp - effect["amount"])
    session.combat_state.hand.remove(card_id)
    if session.combat_state.enemy_hp <= 0:
        session.combat_state.outcome = "PlayerVictory"
        session.combat_active = False
        grant_event_rewards(session, session.events[session.active_event_id])
    return True, []


def assert_true(value: bool, label: str, detail: Any = None) -> None:
    if not value:
        raise AssertionError(f"{label} failed: {detail}")


def main() -> int:
    cards = load_cards(Path("data/cards/cards.mvp.json"))
    events = load_events(Path("data/events/events.mvp.json"))
    session = Session(events=events, known_cards=cards, available_event_ids=["event_wake_cache_001"], active_event_id="event_wake_cache_001")

    ok, reasons = resolve_non_battle_event(session, "event_wake_cache_001")
    assert_true(ok, "starter event should resolve", reasons)
    ok, reasons = resolve_non_battle_event(session, "event_contract_broker_001")
    assert_true(ok, "contract broker should resolve", reasons)

    ok, reasons = resolve_non_battle_event(session, "event_ridge_scout_001")
    assert_true(not ok, "battle event should not resolve directly", reasons)

    ok, reasons = start_battle(session, "event_ridge_scout_001", opening_draw_count=4)
    assert_true(ok, "battle should start", reasons)
    assert_true(session.combat_active, "combat active", session.combat_active)

    session.combat_state.hand = ["act_strike_001", "act_strike_001"]
    ok, reasons = play_card(session, "act_strike_001")
    assert_true(ok, "first strike should play", reasons)
    assert_true(session.combat_state.outcome == "InProgress", "battle still running", session.combat_state.enemy_hp)
    session.combat_state.hand.append("act_strike_001")
    ok, reasons = play_card(session, "act_strike_001")
    assert_true(ok, "second strike should play", reasons)
    assert_true(session.combat_state.outcome == "PlayerVictory", "battle victory", session.combat_state.enemy_hp)
    assert_true(not session.combat_active, "combat closed after victory", session.combat_active)
    assert_true("con_four_party_001" in session.owned_card_ids, "battle reward granted", session.owned_card_ids)
    assert_true("event_proxy_gate_001" in session.available_event_ids, "next gate unlocked", session.available_event_ids)
    assert_true("event_ridge_scout_001" in session.completed_event_ids, "battle event completed", session.completed_event_ids)

    print("OK: combat resolution smoke tests passed")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as exc:
        print(str(exc), file=sys.stderr)
        raise SystemExit(1)

