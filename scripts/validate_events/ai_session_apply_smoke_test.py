#!/usr/bin/env python3
"""Smoke-test AI GM response validation and application to session rewards."""

from __future__ import annotations

import json
import sys
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any


BLOCKED_TERMS = ("ハンター", "グリードアイランド", "念能力", "指定ポケット", "HxH", "G.I.")


@dataclass
class Session:
    known_cards: dict[str, dict[str, Any]]
    events: dict[str, dict[str, Any]]
    zone_id: str
    owned_card_ids: list[str] = field(default_factory=list)
    deck_card_ids: list[str] = field(default_factory=list)
    active_event_id: str = ""


def load_cards(path: Path) -> dict[str, dict[str, Any]]:
    data = json.loads(path.read_text(encoding="utf-8"))
    return {card["cardId"]: card for card in data["cards"]}


def load_zone(path: Path) -> tuple[dict[str, Any], dict[str, dict[str, Any]]]:
    data = json.loads(path.read_text(encoding="utf-8"))
    return data, {event["eventId"]: event for event in data["events"]}


def validate_response(response: dict[str, Any], allowed_reward_ids: set[str], known_card_ids: set[str]) -> list[str]:
    reasons: list[str] = []
    if not str(response.get("speakerName", "")).strip():
        reasons.append("speaker empty")
    if not str(response.get("dialogue", "")).strip():
        reasons.append("dialogue empty")
    if any(term.lower() in str(response.get("dialogue", "")).lower() for term in BLOCKED_TERMS):
        reasons.append("blocked wording")
    rewards = response.get("allowedRewardCardIds", [])
    if len(rewards) > 3:
        reasons.append("too many rewards")
    seen: set[str] = set()
    for reward_id in rewards:
        if reward_id in seen:
            reasons.append("duplicate reward")
        seen.add(reward_id)
        if reward_id not in allowed_reward_ids:
            reasons.append("reward not allowed")
        if reward_id not in known_card_ids:
            reasons.append("reward not defined")
    return reasons


def apply_ai_response(session: Session, response: dict[str, Any], player_choice: str) -> tuple[bool, list[str]]:
    del player_choice
    event = session.events[session.active_event_id]
    reasons = validate_response(
        response,
        set(event.get("allowedAiRewardCardIds", [])),
        set(session.known_cards.keys()),
    )
    if reasons:
        return False, reasons
    for reward_id in response["allowedRewardCardIds"]:
        if reward_id not in session.owned_card_ids:
            session.owned_card_ids.append(reward_id)
        if reward_id not in session.deck_card_ids:
            session.deck_card_ids.append(reward_id)
    return True, []


def assert_true(value: bool, label: str, detail: Any = None) -> None:
    if not value:
        raise AssertionError(f"{label} failed: {detail}")


def main() -> int:
    cards = load_cards(Path("data/cards/cards.mvp.json"))
    zone, events = load_zone(Path("data/events/events.mvp.json"))
    session = Session(
        known_cards=cards,
        events=events,
        zone_id=zone["zoneId"],
        owned_card_ids=["act_strike_001"],
        deck_card_ids=["act_strike_001"],
        active_event_id="event_contract_broker_001",
    )

    valid = {
        "speakerName": "契約屋リオ",
        "dialogue": "条件は満たした。仮契約書を渡そう。",
        "intent": "reward",
        "allowedRewardCardIds": ["item_contract_001"],
        "difficultyHint": "low",
    }
    ok, reasons = apply_ai_response(session, valid, "交渉する")
    assert_true(ok, "valid AI response should apply", reasons)
    assert_true("item_contract_001" in session.owned_card_ids, "reward added to owned", session.owned_card_ids)
    assert_true("item_contract_001" in session.deck_card_ids, "reward added to deck", session.deck_card_ids)

    invalid = dict(valid)
    invalid["allowedRewardCardIds"] = ["key_zone_core_001"]
    ok, reasons = apply_ai_response(session, invalid, "交渉する")
    assert_true(not ok, "disallowed AI reward should fail", reasons)

    blocked = dict(valid)
    blocked["dialogue"] = "グリードアイランドの指定ポケットだ。"
    ok, reasons = apply_ai_response(session, blocked, "交渉する")
    assert_true(not ok, "blocked wording should fail", reasons)

    print("OK: AI session apply smoke tests passed")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as exc:
        print(str(exc), file=sys.stderr)
        raise SystemExit(1)

