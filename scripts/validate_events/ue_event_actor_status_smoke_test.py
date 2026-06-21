#!/usr/bin/env python3
"""Validate the expected event-actor/status reconciliation rules."""

from __future__ import annotations

import sys
from dataclasses import dataclass


@dataclass
class EventView:
    event_id: str
    display_name: str
    event_type: str
    available: bool
    completed: bool
    is_active: bool


@dataclass
class ActorPlacement:
    event_id: str
    distance: float


def build_status_summary(status: dict[str, object]) -> str:
    parts: list[str] = []
    parts.append(
        f"placed x{status['placement_count']}" if status["has_actor_placement"] else "missing actor"
    )
    parts.append(
        "completed"
        if status["completed"]
        else ("available" if status["available"] else "locked")
    )
    if status["is_active"]:
        parts.append("active")
    if status["is_focused"]:
        parts.append("focused")
    if status["is_interactable_now"]:
        parts.append("interactable")
    if status["nearest_distance"] >= 0.0:
        parts.append(f"dist {status['nearest_distance']:.0f}")
    return " | ".join(parts)


def build_event_actor_status(
    event_views: list[EventView],
    placements: list[ActorPlacement],
    focused_event_id: str | None,
) -> list[dict[str, object]]:
    placement_counts: dict[str, int] = {}
    nearest_distances: dict[str, float] = {}

    for placement in placements:
        placement_counts[placement.event_id] = placement_counts.get(placement.event_id, 0) + 1
        nearest_distances[placement.event_id] = min(
            placement.distance,
            nearest_distances.get(placement.event_id, placement.distance),
        )

    statuses: list[dict[str, object]] = []
    for event_view in event_views:
        is_focused = focused_event_id == event_view.event_id
        status = {
            "event_id": event_view.event_id,
            "display_name": event_view.display_name,
            "event_type": event_view.event_type,
            "placement_count": placement_counts.get(event_view.event_id, 0),
            "has_actor_placement": placement_counts.get(event_view.event_id, 0) > 0,
            "available": event_view.available,
            "completed": event_view.completed,
            "is_active": event_view.is_active,
            "is_focused": is_focused,
            "is_interactable_now": event_view.available and is_focused,
            "nearest_distance": nearest_distances.get(event_view.event_id, -1.0),
        }
        status["status_summary"] = build_status_summary(status)
        statuses.append(status)

    return statuses


def assert_equal(actual: object, expected: object, label: str) -> None:
    if actual != expected:
        raise AssertionError(f"{label}: expected {expected!r}, got {actual!r}")


def main() -> int:
    event_views = [
        EventView("event_wake_cache_001", "Wake Cache", "Treasure", True, False, False),
        EventView("event_contract_broker_001", "Contract Broker", "Npc", True, True, False),
        EventView("event_silent_shrine_001", "Silent Shrine", "Quest", False, False, False),
        EventView("event_ridge_scout_001", "Ridge Scout", "Battle", True, False, True),
        EventView("event_proxy_gate_001", "Proxy Gate", "KeyGate", False, False, False),
    ]
    placements = [
        ActorPlacement("event_wake_cache_001", 120.0),
        ActorPlacement("event_contract_broker_001", 280.0),
        ActorPlacement("event_contract_broker_001", 150.0),
        ActorPlacement("event_ridge_scout_001", 60.0),
        ActorPlacement("event_proxy_gate_001", 640.0),
    ]

    statuses = build_event_actor_status(event_views, placements, focused_event_id="event_ridge_scout_001")
    by_id = {status["event_id"]: status for status in statuses}

    assert_equal(by_id["event_wake_cache_001"]["has_actor_placement"], True, "wake cache placed")
    assert_equal(by_id["event_wake_cache_001"]["placement_count"], 1, "wake cache placement count")
    assert_equal(by_id["event_wake_cache_001"]["is_interactable_now"], False, "wake cache not interactable when not focused")
    assert_equal(by_id["event_wake_cache_001"]["status_summary"], "placed x1 | available | dist 120", "wake cache summary")

    assert_equal(by_id["event_contract_broker_001"]["completed"], True, "broker completed")
    assert_equal(by_id["event_contract_broker_001"]["placement_count"], 2, "broker duplicate placement count")
    assert_equal(by_id["event_contract_broker_001"]["nearest_distance"], 150.0, "broker nearest distance")
    assert_equal(
        by_id["event_contract_broker_001"]["status_summary"],
        "placed x2 | completed | dist 150",
        "broker summary",
    )

    assert_equal(by_id["event_silent_shrine_001"]["has_actor_placement"], False, "shrine missing placement")
    assert_equal(by_id["event_silent_shrine_001"]["nearest_distance"], -1.0, "shrine missing distance")
    assert_equal(by_id["event_silent_shrine_001"]["status_summary"], "missing actor | locked", "shrine summary")

    assert_equal(by_id["event_ridge_scout_001"]["is_active"], True, "ridge active")
    assert_equal(by_id["event_ridge_scout_001"]["is_focused"], True, "ridge focused")
    assert_equal(by_id["event_ridge_scout_001"]["is_interactable_now"], True, "ridge interactable")
    assert_equal(
        by_id["event_ridge_scout_001"]["status_summary"],
        "placed x1 | available | active | focused | interactable | dist 60",
        "ridge summary",
    )

    assert_equal(by_id["event_proxy_gate_001"]["has_actor_placement"], True, "proxy placed")
    assert_equal(by_id["event_proxy_gate_001"]["available"], False, "proxy still locked")
    assert_equal(by_id["event_proxy_gate_001"]["status_summary"], "placed x1 | locked | dist 640", "proxy summary")

    print("OK: UE event actor status smoke tests passed")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as exc:
        print(str(exc), file=sys.stderr)
        raise SystemExit(1)
