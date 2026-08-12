#!/usr/bin/env python3
"""Smoke-test session initialization, event resolution, combat start, and AI request creation."""

from __future__ import annotations

import json
import sys
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any


@dataclass
class Session:
    zone_id: str
    events: dict[str, dict[str, Any]]
    known_cards: dict[str, dict[str, Any]]
    owned_card_ids: list[str] = field(default_factory=list)
    deck_card_ids: list[str] = field(default_factory=list)
    completed_event_ids: list[str] = field(default_factory=list)
    acquired_key_card_ids: list[str] = field(default_factory=list)
    available_event_ids: list[str] = field(default_factory=list)
    active_event_id: str = ""
    combat_active: bool = False
    combat_hand: list[str] = field(default_factory=list)


def load_cards(path: Path) -> dict[str, dict[str, Any]]:
    data = json.loads(path.read_text(encoding="utf-8"))
    return {card["cardId"]: card for card in data["cards"]}


def load_events(path: Path) -> tuple[dict[str, Any], dict[str, dict[str, Any]]]:
    data = json.loads(path.read_text(encoding="utf-8"))
    return data, {event["eventId"]: event for event in data["events"]}


def initialize_session(zone: dict[str, Any], events: dict[str, dict[str, Any]], cards: dict[str, dict[str, Any]]) -> Session:
    session = Session(zone_id=zone["zoneId"], events=events, known_cards=cards)
    first_event_id = zone["events"][0]["eventId"]
    session.available_event_ids.append(first_event_id)
    session.active_event_id = first_event_id
    return session


def resolve_event(session: Session, event_id: str) -> tuple[bool, list[str]]:
    if session.combat_active:
        return False, ["cannot resolve an exploration event while combat is active"]
    if event_id not in session.events:
        return False, [f"unknown event {event_id}"]
    if event_id not in session.available_event_ids:
        return False, [f"event {event_id} not available"]

    event = session.events[event_id]
    reasons: list[str] = []
    if event_id in session.completed_event_ids:
        reasons.append(f"{event_id} already completed")
    for required_card_id in event["requiredCardIds"]:
        if required_card_id not in session.owned_card_ids:
            reasons.append(f"missing {required_card_id}")
    if reasons:
        return False, reasons

    for reward_card_id in event["rewardCardIds"]:
        if reward_card_id not in session.owned_card_ids:
            session.owned_card_ids.append(reward_card_id)
        if reward_card_id not in session.deck_card_ids:
            session.deck_card_ids.append(reward_card_id)
        if reward_card_id.startswith("key_") and reward_card_id not in session.acquired_key_card_ids:
            session.acquired_key_card_ids.append(reward_card_id)

    session.completed_event_ids.append(event_id)
    for next_event_id in event["nextEventIds"]:
        if next_event_id not in session.available_event_ids:
            session.available_event_ids.append(next_event_id)
    session.active_event_id = event_id
    return True, []


def start_combat(session: Session, event_id: str, opening_draw_count: int) -> tuple[bool, list[str]]:
    if session.combat_active:
        return False, ["combat is already active"]
    if event_id not in session.available_event_ids:
        return False, [f"event {event_id} not available"]
    event = session.events[event_id]
    if event["type"] != "Battle":
        return False, [f"event {event_id} is not battle"]

    session.combat_active = True
    session.active_event_id = event_id
    session.combat_hand = session.deck_card_ids[:opening_draw_count]
    return True, []


def build_ai_request(session: Session, event_id: str, player_choice: str) -> dict[str, Any]:
    event = session.events[event_id]
    collection_tags: list[str] = []
    for card_id in session.owned_card_ids:
        collection_tags.extend(session.known_cards[card_id]["tags"])
    return {
        "zoneId": session.zone_id,
        "eventId": event_id,
        "npcId": event.get("npcId", ""),
        "playerCollectionTags": collection_tags,
        "allowedRewardCardIds": event.get("allowedAiRewardCardIds", []),
        "playerChoice": player_choice,
    }


def assert_true(value: bool, label: str, detail: Any = None) -> None:
    if not value:
        raise AssertionError(f"{label} failed: {detail}")


def main() -> int:
    cards = load_cards(Path("data/cards/cards.mvp.json"))
    zone, events = load_events(Path("data/events/events.mvp.json"))
    session = initialize_session(zone, events, cards)

    assert_true(session.active_event_id == "event_wake_cache_001", "first event availability", session.active_event_id)

    ok, reasons = resolve_event(session, "event_wake_cache_001")
    assert_true(ok, "wake cache should resolve", reasons)
    assert_true("act_strike_001" in session.owned_card_ids, "starter cards granted", session.owned_card_ids)
    assert_true("event_contract_broker_001" in session.available_event_ids, "contract broker unlocked", session.available_event_ids)

    ai_request = build_ai_request(session, "event_contract_broker_001", "交渉する")
    assert_true(ai_request["allowedRewardCardIds"] == ["item_contract_001"], "allowed AI reward ids", ai_request)
    assert_true("attack" in ai_request["playerCollectionTags"], "collection tags populated", ai_request["playerCollectionTags"])

    ok, reasons = resolve_event(session, "event_contract_broker_001")
    assert_true(ok, "contract broker should resolve", reasons)
    assert_true("item_contract_001" in session.owned_card_ids, "contract item granted", session.owned_card_ids)
    assert_true("event_ridge_scout_001" in session.available_event_ids, "battle unlocked", session.available_event_ids)

    ok, reasons = start_combat(session, "event_ridge_scout_001", opening_draw_count=3)
    assert_true(ok, "battle should start", reasons)
    assert_true(session.combat_active, "combat active flag", session.combat_active)
    assert_true(len(session.combat_hand) == 3, "opening hand size", session.combat_hand)

    ok, reasons = resolve_event(session, "event_silent_shrine_001")
    assert_true(not ok, "exploration event blocked during combat", reasons)
    ok, reasons = start_combat(session, "event_ridge_scout_001", opening_draw_count=3)
    assert_true(not ok, "second battle blocked during combat", reasons)

    print("OK: session flow smoke tests passed")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as exc:
        print(str(exc), file=sys.stderr)
        raise SystemExit(1)
