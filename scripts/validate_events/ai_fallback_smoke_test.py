#!/usr/bin/env python3
"""Smoke-test deterministic AI fallback response shaping for the MVP NPC event."""

from __future__ import annotations

import json
import sys
from pathlib import Path
from typing import Any


def load_zone(path: Path) -> dict[str, Any]:
    return json.loads(path.read_text(encoding="utf-8"))


def build_fallback_response(event: dict[str, Any], player_choice: str) -> dict[str, Any]:
    allowed_reward_ids = event.get("allowedAiRewardCardIds", [])
    if allowed_reward_ids:
        return {
            "speakerName": event.get("npcId", "").replace("npc_", "NPC ") or event["displayName"],
            "dialogue": f"{player_choice or '申し出'}を確認した。今回は定型手続きとして報酬を一つだけ渡す。",
            "intent": "reward",
            "allowedRewardCardIds": [allowed_reward_ids[0]],
            "difficultyHint": "low",
        }

    return {
        "speakerName": event["displayName"],
        "dialogue": f"{event['displayName']}での反応は記録した。いまは状況説明だけを返す。",
        "intent": "flavor",
        "allowedRewardCardIds": [],
        "difficultyHint": "medium",
    }


def assert_true(value: bool, label: str, detail: Any = None) -> None:
    if not value:
        raise AssertionError(f"{label} failed: {detail}")


def main() -> int:
    zone = load_zone(Path("data/events/events.mvp.json"))
    event_by_id = {event["eventId"]: event for event in zone["events"]}

    npc_response = build_fallback_response(event_by_id["event_contract_broker_001"], "交渉する")
    assert_true(npc_response["intent"] == "reward", "npc fallback intent", npc_response)
    assert_true(npc_response["allowedRewardCardIds"] == ["item_contract_001"], "npc fallback reward", npc_response)
    assert_true("交渉する" in npc_response["dialogue"], "player choice reflected", npc_response["dialogue"])

    treasure_response = build_fallback_response(event_by_id["event_wake_cache_001"], "")
    assert_true(treasure_response["intent"] == "flavor", "treasure fallback intent", treasure_response)
    assert_true(treasure_response["allowedRewardCardIds"] == [], "treasure fallback rewards empty", treasure_response)

    print("OK: AI fallback smoke tests passed")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as exc:
        print(str(exc), file=sys.stderr)
        raise SystemExit(1)
