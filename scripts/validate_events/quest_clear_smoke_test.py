#!/usr/bin/env python3
"""Smoke-test quest activation/completion and zone clear conditions."""

from __future__ import annotations

import json
import sys
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any


@dataclass
class Session:
    zone: dict[str, Any]
    events: dict[str, dict[str, Any]]
    owned_card_ids: list[str] = field(default_factory=list)
    completed_event_ids: list[str] = field(default_factory=list)
    available_event_ids: list[str] = field(default_factory=list)
    active_quest_ids: list[str] = field(default_factory=list)
    completed_quest_ids: list[str] = field(default_factory=list)
    zone_cleared: bool = False


def load_zone(path: Path) -> tuple[dict[str, Any], dict[str, dict[str, Any]]]:
    data = json.loads(path.read_text(encoding="utf-8"))
    return data, {event["eventId"]: event for event in data["events"]}


def refresh(session: Session) -> None:
    session.active_quest_ids = []
    for event in session.zone["events"]:
        if event["type"] != "Quest":
            continue
        event_id = event["eventId"]
        if event_id in session.completed_event_ids:
            if event_id not in session.completed_quest_ids:
                session.completed_quest_ids.append(event_id)
            continue
        if event_id in session.available_event_ids:
            session.active_quest_ids.append(event_id)
    session.zone_cleared = all(card_id in session.owned_card_ids for card_id in session.zone["clearRequiredCardIds"])


def resolve_event(session: Session, event_id: str) -> None:
    event = session.events[event_id]
    for required_card_id in event["requiredCardIds"]:
        if required_card_id not in session.owned_card_ids:
            raise AssertionError(f"missing required card {required_card_id}")
    for reward_card_id in event["rewardCardIds"]:
        if reward_card_id not in session.owned_card_ids:
            session.owned_card_ids.append(reward_card_id)
    if event_id not in session.completed_event_ids:
        session.completed_event_ids.append(event_id)
    for next_event_id in event["nextEventIds"]:
        if next_event_id not in session.available_event_ids:
            session.available_event_ids.append(next_event_id)
    refresh(session)


def assert_true(value: bool, label: str, detail: Any = None) -> None:
    if not value:
        raise AssertionError(f"{label} failed: {detail}")


def main() -> int:
    zone, events = load_zone(Path("data/events/events.mvp.json"))
    session = Session(zone=zone, events=events, available_event_ids=["event_wake_cache_001"])
    refresh(session)
    assert_true(session.active_quest_ids == [], "no quest active at start", session.active_quest_ids)

    resolve_event(session, "event_wake_cache_001")
    resolve_event(session, "event_contract_broker_001")
    assert_true("event_silent_shrine_001" in session.active_quest_ids, "quest should activate when unlocked", session.active_quest_ids)
    assert_true(not session.zone_cleared, "zone not cleared before key", session.zone_cleared)

    resolve_event(session, "event_silent_shrine_001")
    assert_true("event_silent_shrine_001" in session.completed_quest_ids, "quest should complete", session.completed_quest_ids)

    session.owned_card_ids.append("key_zone_core_001")
    refresh(session)
    assert_true(session.zone_cleared, "zone clears when required key is owned", session.owned_card_ids)

    print("OK: quest clear smoke tests passed")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as exc:
        print(str(exc), file=sys.stderr)
        raise SystemExit(1)

