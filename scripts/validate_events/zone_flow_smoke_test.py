#!/usr/bin/env python3
"""Smoke-test the MVP zone flow from first cache to clear key."""

from __future__ import annotations

import json
import sys
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any


@dataclass
class ZoneState:
    owned_card_ids: list[str] = field(default_factory=list)
    completed_event_ids: list[str] = field(default_factory=list)
    acquired_key_card_ids: list[str] = field(default_factory=list)
    available_event_ids: list[str] = field(default_factory=list)


def load_events(path: Path) -> tuple[dict[str, Any], dict[str, dict[str, Any]]]:
    data = json.loads(path.read_text(encoding="utf-8"))
    return data, {event["eventId"]: event for event in data["events"]}


def resolve_event(event: dict[str, Any], state: ZoneState) -> tuple[bool, list[str]]:
    reasons: list[str] = []
    if event["eventId"] not in state.available_event_ids:
        reasons.append(f"{event['eventId']} not available")
    if event["eventId"] in state.completed_event_ids:
        reasons.append(f"{event['eventId']} already completed")
    for required_card_id in event["requiredCardIds"]:
        if required_card_id not in state.owned_card_ids:
            reasons.append(f"missing {required_card_id}")
    if reasons:
        return False, reasons

    for reward_card_id in event["rewardCardIds"]:
        if reward_card_id not in state.owned_card_ids:
            state.owned_card_ids.append(reward_card_id)
        if reward_card_id.startswith("key_") and reward_card_id not in state.acquired_key_card_ids:
            state.acquired_key_card_ids.append(reward_card_id)
    state.completed_event_ids.append(event["eventId"])
    for next_event_id in event["nextEventIds"]:
        if next_event_id not in state.available_event_ids:
            state.available_event_ids.append(next_event_id)
    return True, []


def assert_true(value: bool, label: str, detail: Any = None) -> None:
    if not value:
        raise AssertionError(f"{label} failed: {detail}")


def main() -> int:
    zone, events = load_events(Path("data/events/events.mvp.json"))
    state = ZoneState(available_event_ids=[zone["events"][0]["eventId"]])

    gate_ok, gate_reasons = resolve_event(events["event_proxy_gate_001"], state)
    assert_true(not gate_ok, "final gate should be locked before setup", gate_reasons)

    for event_id in [
        "event_wake_cache_001",
        "event_contract_broker_001",
        "event_silent_shrine_001",
        "event_ridge_scout_001",
        "event_proxy_gate_001",
    ]:
        ok, reasons = resolve_event(events[event_id], state)
        assert_true(ok, f"{event_id} should resolve", reasons)

    for required_card_id in zone["clearRequiredCardIds"]:
        assert_true(required_card_id in state.owned_card_ids, "clear card should be owned", required_card_id)
        assert_true(required_card_id in state.acquired_key_card_ids, "clear key should be recorded", required_card_id)

    assert_true(
        state.completed_event_ids[-1] == "event_proxy_gate_001",
        "final event should complete last",
        state.completed_event_ids,
    )

    print("OK: zone flow smoke tests passed")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as exc:
        print(str(exc), file=sys.stderr)
        raise SystemExit(1)
