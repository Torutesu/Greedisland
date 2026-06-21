#!/usr/bin/env python3
"""Validate the expected MVP event route graph stays stable."""

from __future__ import annotations

import json
import sys
from collections import deque
from pathlib import Path


def assert_equal(actual: object, expected: object, label: str) -> None:
    if actual != expected:
        raise AssertionError(f"{label}: expected {expected!r}, got {actual!r}")


def main() -> int:
    repo_root = Path(__file__).resolve().parents[2]
    event_path = repo_root / "data" / "events" / "events.mvp.json"
    payload = json.loads(event_path.read_text(encoding="utf-8"))
    events = payload["events"]

    event_ids = [event["eventId"] for event in events]
    next_ids_by_event_id = {event["eventId"]: list(event.get("nextEventIds", [])) for event in events}
    incoming_ids_by_event_id = {event_id: [] for event_id in event_ids}

    for event_id, next_ids in next_ids_by_event_id.items():
        for next_event_id in next_ids:
            incoming_ids_by_event_id.setdefault(next_event_id, []).append(event_id)

    root_ids = [event_id for event_id in event_ids if not incoming_ids_by_event_id.get(event_id)]
    assert_equal(root_ids, ["event_wake_cache_001"], "route root ids")

    route_depth_by_event_id: dict[str, int] = {root_ids[0]: 0}
    frontier = deque(root_ids)
    while frontier:
        event_id = frontier.popleft()
        for next_event_id in next_ids_by_event_id.get(event_id, []):
            if next_event_id in route_depth_by_event_id:
                continue
            route_depth_by_event_id[next_event_id] = route_depth_by_event_id[event_id] + 1
            frontier.append(next_event_id)

    assert_equal(
        route_depth_by_event_id,
        {
            "event_wake_cache_001": 0,
            "event_contract_broker_001": 1,
            "event_silent_shrine_001": 2,
            "event_ridge_scout_001": 2,
            "event_proxy_gate_001": 3,
        },
        "route depths",
    )

    assert_equal(
        incoming_ids_by_event_id["event_proxy_gate_001"],
        ["event_silent_shrine_001", "event_ridge_scout_001"],
        "proxy gate incoming ids",
    )
    assert_equal(next_ids_by_event_id["event_proxy_gate_001"], [], "proxy gate terminal node")

    print("OK: UE route graph smoke tests passed")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as exc:
        print(str(exc), file=sys.stderr)
        raise SystemExit(1)
