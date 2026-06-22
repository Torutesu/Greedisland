#!/usr/bin/env python3
"""Smoke-test the conservative AI GM response validation rules."""

from __future__ import annotations

import json
import sys
from pathlib import Path
from typing import Any


BLOCKED_TERMS = ("ハンター", "グリードアイランド", "念能力", "指定ポケット", "HxH", "G.I.")


def load_card_ids(path: Path) -> set[str]:
    data = json.loads(path.read_text(encoding="utf-8"))
    return {card["cardId"] for card in data["cards"]}


def validate_response(
    response: dict[str, Any],
    request_allowed_reward_card_ids: set[str],
    request_allowed_quest_event_ids: set[str],
    known_card_ids: set[str],
) -> list[str]:
    reasons: list[str] = []
    speaker_name = str(response.get("speakerName", ""))
    dialogue = str(response.get("dialogue", ""))
    rewards = response.get("allowedRewardCardIds", [])

    if not speaker_name.strip():
        reasons.append("Speaker name is empty.")
    if len(speaker_name) > 48:
        reasons.append("Speaker name is too long.")
    if not dialogue.strip():
        reasons.append("Dialogue is empty.")
    if len(dialogue) > 420:
        reasons.append("Dialogue is too long.")
    if any(term.lower() in speaker_name.lower() or term.lower() in dialogue.lower() for term in BLOCKED_TERMS):
        reasons.append("Response contains blocked IP-adjacent wording.")

    if not isinstance(rewards, list):
        reasons.append("Reward card ids must be a list.")
        rewards = []
    if len(rewards) > 3:
        reasons.append("Too many reward card ids.")

    proposed_quest_id = response.get("proposedQuestId", "")
    intent = str(response.get("intent", ""))
    if intent == "quest_offer":
        if not isinstance(proposed_quest_id, str) or not proposed_quest_id:
            reasons.append("Quest-offer intent requires a proposed quest event id.")
        elif proposed_quest_id not in request_allowed_quest_event_ids:
            reasons.append(f"Proposed quest event {proposed_quest_id} was not allowed by the request.")
    elif isinstance(proposed_quest_id, str) and proposed_quest_id and proposed_quest_id not in request_allowed_quest_event_ids:
        reasons.append(f"Proposed quest event {proposed_quest_id} was not allowed by the request.")

    seen: set[str] = set()
    for reward_card_id in rewards:
        if not isinstance(reward_card_id, str) or not reward_card_id:
            reasons.append("Reward card id is empty.")
            continue
        if reward_card_id in seen:
            reasons.append(f"Duplicate reward card id {reward_card_id}.")
        seen.add(reward_card_id)
        if reward_card_id not in request_allowed_reward_card_ids:
            reasons.append(f"Reward card {reward_card_id} was not allowed by the request.")
        if reward_card_id not in known_card_ids:
            reasons.append(f"Reward card {reward_card_id} is not defined.")

    return reasons


def assert_equal(actual: Any, expected: Any, label: str) -> None:
    if actual != expected:
        raise AssertionError(f"{label}: expected {expected!r}, got {actual!r}")


def main() -> int:
    known_card_ids = load_card_ids(Path("data/cards/cards.mvp.json"))
    request_allowed = {"key_zone_core_001", "item_contract_001"}
    request_allowed_quests = {"event_silent_shrine_001"}

    valid = {
        "speakerName": "門番リオ",
        "dialogue": "証明を見せてくれたなら、この先の道を開けよう。",
        "intent": "negotiation",
        "allowedRewardCardIds": ["key_zone_core_001"],
        "difficultyHint": "medium",
    }
    assert_equal(validate_response(valid, request_allowed, request_allowed_quests, known_card_ids), [], "valid response")

    unknown_reward = dict(valid)
    unknown_reward["allowedRewardCardIds"] = ["key_missing_999"]
    reasons = validate_response(unknown_reward, request_allowed, request_allowed_quests, known_card_ids)
    assert_equal(any("not allowed" in reason for reason in reasons), True, "unknown reward allowed check")
    assert_equal(any("not defined" in reason for reason in reasons), True, "unknown reward defined check")

    blocked = dict(valid)
    blocked["dialogue"] = "グリードアイランドの指定ポケットを思い出せ。"
    reasons = validate_response(blocked, request_allowed, request_allowed_quests, known_card_ids)
    assert_equal(any("blocked IP" in reason for reason in reasons), True, "blocked wording")

    duplicate = dict(valid)
    duplicate["allowedRewardCardIds"] = ["item_contract_001", "item_contract_001"]
    reasons = validate_response(duplicate, request_allowed, request_allowed_quests, known_card_ids)
    assert_equal(any("Duplicate" in reason for reason in reasons), True, "duplicate rewards")

    quest_offer = dict(valid)
    quest_offer["intent"] = "quest_offer"
    quest_offer["proposedQuestId"] = "event_silent_shrine_001"
    quest_offer["allowedRewardCardIds"] = []
    assert_equal(validate_response(quest_offer, request_allowed, request_allowed_quests, known_card_ids), [], "quest offer response")

    invalid_quest_offer = dict(quest_offer)
    invalid_quest_offer["proposedQuestId"] = "event_proxy_gate_001"
    reasons = validate_response(invalid_quest_offer, request_allowed, request_allowed_quests, known_card_ids)
    assert_equal(any("Proposed quest event" in reason for reason in reasons), True, "quest offer allowlist")

    print("OK: AI GM smoke tests passed")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as exc:
        print(str(exc), file=sys.stderr)
        raise SystemExit(1)
