#!/usr/bin/env python3
"""Smoke-test session save/restore mapping for MVP progress state."""

from __future__ import annotations

import json
import sys
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any


@dataclass
class Session:
    zone_id: str
    known_cards: dict[str, dict[str, Any]]
    events: dict[str, dict[str, Any]]
    owned_card_ids: list[str] = field(default_factory=list)
    deck_card_ids: list[str] = field(default_factory=list)
    completed_event_ids: list[str] = field(default_factory=list)
    acquired_key_card_ids: list[str] = field(default_factory=list)
    available_event_ids: list[str] = field(default_factory=list)
    active_quest_ids: list[str] = field(default_factory=list)
    completed_quest_ids: list[str] = field(default_factory=list)
    active_event_id: str = ""
    zone_cleared: bool = False


def load_cards(path: Path) -> dict[str, dict[str, Any]]:
    data = json.loads(path.read_text(encoding="utf-8"))
    return {card["cardId"]: card for card in data["cards"]}


def load_events(path: Path) -> tuple[dict[str, Any], dict[str, dict[str, Any]]]:
    data = json.loads(path.read_text(encoding="utf-8"))
    return data, {event["eventId"]: event for event in data["events"]}


def initialize_session(zone: dict[str, Any], cards: dict[str, dict[str, Any]], events: dict[str, dict[str, Any]]) -> Session:
    return Session(
        zone_id=zone["zoneId"],
        known_cards=cards,
        events=events,
        available_event_ids=[zone["events"][0]["eventId"]],
        active_event_id=zone["events"][0]["eventId"],
    )


def resolve_event(session: Session, event_id: str) -> None:
    event = session.events[event_id]
    for required_card_id in event["requiredCardIds"]:
        if required_card_id not in session.owned_card_ids:
            raise AssertionError(f"missing required card {required_card_id}")
    for reward_card_id in event["rewardCardIds"]:
        if reward_card_id not in session.owned_card_ids:
            session.owned_card_ids.append(reward_card_id)
        if reward_card_id not in session.deck_card_ids:
            session.deck_card_ids.append(reward_card_id)
        if reward_card_id.startswith("key_") and reward_card_id not in session.acquired_key_card_ids:
            session.acquired_key_card_ids.append(reward_card_id)
    if event_id not in session.completed_event_ids:
        session.completed_event_ids.append(event_id)
    event_type = event["type"]
    if event_type == "Quest":
        if event_id in session.active_quest_ids:
            session.active_quest_ids.remove(event_id)
        if event_id not in session.completed_quest_ids:
            session.completed_quest_ids.append(event_id)
    for next_event_id in event["nextEventIds"]:
        if next_event_id not in session.available_event_ids:
            session.available_event_ids.append(next_event_id)
            next_event = session.events[next_event_id]
            if next_event["type"] == "Quest" and next_event_id not in session.active_quest_ids:
                session.active_quest_ids.append(next_event_id)
    session.active_event_id = event_id
    session.zone_cleared = "key_zone_core_001" in session.owned_card_ids


def copy_session_to_save(session: Session) -> dict[str, Any]:
    return {
        "currentZoneId": session.zone_id,
        "ownedCardIds": list(session.owned_card_ids),
        "deckCardIds": list(session.deck_card_ids),
        "completedEventIds": list(session.completed_event_ids),
        "acquiredKeyCardIds": list(session.acquired_key_card_ids),
        "availableEventIds": list(session.available_event_ids),
        "activeQuestIds": list(session.active_quest_ids),
        "completedQuestIds": list(session.completed_quest_ids),
        "activeEventId": session.active_event_id,
        "zoneCleared": session.zone_cleared,
    }


def restore_session_from_save(
    save_data: dict[str, Any],
    zone: dict[str, Any],
    cards: dict[str, dict[str, Any]],
    events: dict[str, dict[str, Any]],
) -> Session:
    if save_data["currentZoneId"] != zone["zoneId"]:
        raise AssertionError("save zone mismatch")
    restored = initialize_session(zone, cards, events)
    restored.owned_card_ids = list(save_data["ownedCardIds"])
    restored.deck_card_ids = list(save_data["deckCardIds"])
    restored.completed_event_ids = list(save_data["completedEventIds"])
    restored.acquired_key_card_ids = list(save_data["acquiredKeyCardIds"])
    restored.available_event_ids = list(save_data["availableEventIds"])
    restored.active_quest_ids = list(save_data["activeQuestIds"])
    restored.completed_quest_ids = list(save_data["completedQuestIds"])
    restored.active_event_id = save_data["activeEventId"]
    restored.zone_cleared = bool(save_data["zoneCleared"])
    return restored


def assert_equal(actual: Any, expected: Any, label: str) -> None:
    if actual != expected:
        raise AssertionError(f"{label}: expected {expected!r}, got {actual!r}")


def main() -> int:
    cards = load_cards(Path("data/cards/cards.mvp.json"))
    zone, events = load_events(Path("data/events/events.mvp.json"))
    session = initialize_session(zone, cards, events)

    resolve_event(session, "event_wake_cache_001")
    resolve_event(session, "event_contract_broker_001")
    resolve_event(session, "event_silent_shrine_001")
    save_data = copy_session_to_save(session)
    restored = restore_session_from_save(save_data, zone, cards, events)

    assert_equal(restored.zone_id, session.zone_id, "zone id")
    assert_equal(restored.owned_card_ids, session.owned_card_ids, "owned cards")
    assert_equal(restored.deck_card_ids, session.deck_card_ids, "deck cards")
    assert_equal(restored.completed_event_ids, session.completed_event_ids, "completed events")
    assert_equal(restored.available_event_ids, session.available_event_ids, "available events")
    assert_equal(restored.active_quest_ids, session.active_quest_ids, "active quests")
    assert_equal(restored.completed_quest_ids, session.completed_quest_ids, "completed quests")
    assert_equal(restored.active_event_id, session.active_event_id, "active event")
    assert_equal(restored.acquired_key_card_ids, session.acquired_key_card_ids, "acquired key cards")
    assert_equal(restored.zone_cleared, session.zone_cleared, "zone cleared")

    print("OK: save restore smoke tests passed")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as exc:
        print(str(exc), file=sys.stderr)
        raise SystemExit(1)
