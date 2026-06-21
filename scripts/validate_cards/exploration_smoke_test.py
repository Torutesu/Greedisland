#!/usr/bin/env python3
"""Smoke-test exploration event resolution and card rewards."""

from __future__ import annotations

import sys
from dataclasses import dataclass, field
from typing import Any


@dataclass
class Event:
    event_id: str
    reward_card_ids: list[str] = field(default_factory=list)
    required_card_ids: list[str] = field(default_factory=list)
    completed: bool = False


@dataclass
class ZoneProgress:
    completed_event_ids: list[str] = field(default_factory=list)
    acquired_key_card_ids: list[str] = field(default_factory=list)


def can_resolve(event: Event, owned_card_ids: list[str], progress: ZoneProgress) -> tuple[bool, list[str]]:
    reasons: list[str] = []
    if not event.event_id:
        reasons.append("Event id is missing.")
    if event.completed or event.event_id in progress.completed_event_ids:
        reasons.append(f"Event {event.event_id} is already completed.")
    for required_card_id in event.required_card_ids:
        if required_card_id not in owned_card_ids:
            reasons.append(f"Missing required card {required_card_id}.")
    return len(reasons) == 0, reasons


def resolve(event: Event, owned_card_ids: list[str], progress: ZoneProgress) -> dict[str, Any]:
    ok, reasons = can_resolve(event, owned_card_ids, progress)
    result = {
        "success": ok,
        "reasons": reasons,
        "granted": [],
        "log": [],
    }
    if not ok:
        return result

    for reward_card_id in event.reward_card_ids:
        if reward_card_id not in owned_card_ids:
            owned_card_ids.append(reward_card_id)
            result["granted"].append(reward_card_id)
        if reward_card_id.startswith("key_") and reward_card_id not in progress.acquired_key_card_ids:
            progress.acquired_key_card_ids.append(reward_card_id)

    if event.event_id not in progress.completed_event_ids:
        progress.completed_event_ids.append(event.event_id)
    return result


def assert_equal(actual: Any, expected: Any, label: str) -> None:
    if actual != expected:
        raise AssertionError(f"{label}: expected {expected!r}, got {actual!r}")


def main() -> int:
    owned = ["item_contract_001"]
    progress = ZoneProgress()

    locked_event = Event(
        event_id="event_gate_001",
        reward_card_ids=["key_zone_core_001"],
        required_card_ids=["con_four_party_001"],
    )
    result = resolve(locked_event, owned, progress)
    assert_equal(result["success"], False, "locked event should fail")
    assert_equal(owned, ["item_contract_001"], "locked event should not grant cards")

    owned.append("con_four_party_001")
    result = resolve(locked_event, owned, progress)
    assert_equal(result["success"], True, "unlocked event should succeed")
    assert_equal("key_zone_core_001" in owned, True, "key card granted")
    assert_equal(progress.acquired_key_card_ids, ["key_zone_core_001"], "key progress recorded")
    assert_equal(progress.completed_event_ids, ["event_gate_001"], "event completion recorded")

    result = resolve(locked_event, owned, progress)
    assert_equal(result["success"], False, "completed event should not resolve twice")

    duplicate_reward = Event(
        event_id="event_duplicate_reward_001",
        reward_card_ids=["item_contract_001"],
    )
    result = resolve(duplicate_reward, owned, progress)
    assert_equal(result["success"], True, "duplicate reward event should resolve")
    assert_equal(result["granted"], [], "duplicate reward should not be granted twice")

    print("OK: exploration smoke tests passed")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as exc:
        print(str(exc), file=sys.stderr)
        raise SystemExit(1)

