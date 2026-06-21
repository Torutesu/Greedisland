#!/usr/bin/env python3
"""Validate the expected MVP walkthrough progress state machine."""

from __future__ import annotations

import sys
from dataclasses import dataclass, field


@dataclass
class Snapshot:
    has_initialized_session: bool = False
    combat_active: bool = False
    zone_cleared: bool = False
    active_event_id: str = ""
    available_event_ids: list[str] = field(default_factory=list)
    completed_quest_ids: list[str] = field(default_factory=list)
    owned_card_ids: list[str] = field(default_factory=list)


@dataclass
class Diagnostics:
    issues: list[str] = field(default_factory=list)
    save_exists: bool = False


def has_owned_card(snapshot: Snapshot, card_id: str) -> bool:
    return card_id in snapshot.owned_card_ids


def has_completed_event(completed_event_ids: list[str], event_id: str) -> bool:
    return event_id in completed_event_ids


def build_walkthrough_progress(
    snapshot: Snapshot,
    diagnostics: Diagnostics,
    completed_event_ids: list[str],
) -> list[dict[str, object]]:
    progress: list[dict[str, object]] = []
    assigned_current_focus = False

    def add_progress(
        order: int,
        label: str,
        event_id: str,
        completed: bool,
        event_available: bool,
        detail: str,
    ) -> None:
        nonlocal assigned_current_focus
        current_focus = not completed and not assigned_current_focus
        status = "Completed" if completed else ("Next Up" if current_focus else ("Available" if event_available else "Locked"))
        progress.append(
            {
                "order": order,
                "label": label,
                "event_id": event_id,
                "completed": completed,
                "current_focus": current_focus,
                "event_available": event_available,
                "status": status,
                "detail": detail,
            }
        )
        if current_focus:
            assigned_current_focus = True

    bootstrap_ok = snapshot.has_initialized_session and not diagnostics.issues
    add_progress(
        1,
        "Bootstrap Session",
        "",
        bootstrap_ok,
        snapshot.has_initialized_session,
        "bootstrap",
    )

    wake_cache_done = has_completed_event(completed_event_ids, "event_wake_cache_001") or all(
        has_owned_card(snapshot, card_id)
        for card_id in (
            "act_strike_001",
            "act_guard_001",
            "act_patch_heal_001",
            "act_draw_001",
        )
    )
    add_progress(
        2,
        "Wake Cache",
        "event_wake_cache_001",
        wake_cache_done,
        "event_wake_cache_001" in snapshot.available_event_ids,
        "wake",
    )

    contract_done = has_completed_event(completed_event_ids, "event_contract_broker_001") or (
        has_owned_card(snapshot, "item_contract_001") and has_owned_card(snapshot, "rule_party_proxy_001")
    )
    add_progress(
        3,
        "Contract Broker",
        "event_contract_broker_001",
        contract_done,
        "event_contract_broker_001" in snapshot.available_event_ids,
        "contract",
    )

    shrine_done = (
        has_completed_event(completed_event_ids, "event_silent_shrine_001")
        or "event_silent_shrine_001" in snapshot.completed_quest_ids
        or has_owned_card(snapshot, "con_silent_oath_001")
    )
    add_progress(
        4,
        "Silent Shrine",
        "event_silent_shrine_001",
        shrine_done,
        "event_silent_shrine_001" in snapshot.available_event_ids,
        "shrine",
    )

    ridge_done = has_completed_event(completed_event_ids, "event_ridge_scout_001") or has_owned_card(
        snapshot, "con_four_party_001"
    )
    add_progress(
        5,
        "Ridge Scout Battle",
        "event_ridge_scout_001",
        ridge_done,
        "event_ridge_scout_001" in snapshot.available_event_ids,
        "ridge-live" if snapshot.combat_active and snapshot.active_event_id == "event_ridge_scout_001" else "ridge",
    )

    proxy_done = (
        has_completed_event(completed_event_ids, "event_proxy_gate_001")
        or has_owned_card(snapshot, "key_zone_core_001")
        or snapshot.zone_cleared
    )
    add_progress(
        6,
        "Proxy Gate",
        "event_proxy_gate_001",
        proxy_done,
        "event_proxy_gate_001" in snapshot.available_event_ids,
        "proxy",
    )

    save_done = proxy_done and diagnostics.save_exists and snapshot.has_initialized_session
    add_progress(
        7,
        "Save And Restore",
        "",
        save_done,
        diagnostics.save_exists,
        "save",
    )

    return progress


def assert_equal(actual: object, expected: object, label: str) -> None:
    if actual != expected:
        raise AssertionError(f"{label}: expected {expected!r}, got {actual!r}")


def assert_focus(progress: list[dict[str, object]], expected_label: str, expected_status: str) -> None:
    focused = [item for item in progress if item["current_focus"]]
    assert_equal(len(focused), 1, "current focus count")
    assert_equal(focused[0]["label"], expected_label, "current focus label")
    assert_equal(focused[0]["status"], expected_status, "current focus status")


def main() -> int:
    progress = build_walkthrough_progress(Snapshot(), Diagnostics(), [])
    assert_focus(progress, "Bootstrap Session", "Next Up")

    progress = build_walkthrough_progress(
        Snapshot(has_initialized_session=True, available_event_ids=["event_wake_cache_001"]),
        Diagnostics(issues=["json missing"]),
        [],
    )
    assert_focus(progress, "Bootstrap Session", "Next Up")

    progress = build_walkthrough_progress(
        Snapshot(has_initialized_session=True, available_event_ids=["event_wake_cache_001"]),
        Diagnostics(),
        [],
    )
    assert_equal(progress[0]["completed"], True, "bootstrap completion")
    assert_focus(progress, "Wake Cache", "Next Up")

    progress = build_walkthrough_progress(
        Snapshot(
            has_initialized_session=True,
            available_event_ids=["event_wake_cache_001", "event_contract_broker_001"],
            owned_card_ids=["act_strike_001", "act_guard_001", "act_patch_heal_001", "act_draw_001"],
        ),
        Diagnostics(),
        ["event_wake_cache_001"],
    )
    assert_equal(progress[1]["completed"], True, "wake completion")
    assert_focus(progress, "Contract Broker", "Next Up")

    progress = build_walkthrough_progress(
        Snapshot(
            has_initialized_session=True,
            available_event_ids=[
                "event_wake_cache_001",
                "event_contract_broker_001",
                "event_silent_shrine_001",
                "event_ridge_scout_001",
            ],
            owned_card_ids=[
                "act_strike_001",
                "act_guard_001",
                "act_patch_heal_001",
                "act_draw_001",
                "item_contract_001",
                "rule_party_proxy_001",
            ],
        ),
        Diagnostics(),
        ["event_wake_cache_001", "event_contract_broker_001"],
    )
    assert_focus(progress, "Silent Shrine", "Next Up")
    assert_equal(progress[4]["status"], "Available", "ridge available after broker")

    progress = build_walkthrough_progress(
        Snapshot(
            has_initialized_session=True,
            combat_active=True,
            active_event_id="event_ridge_scout_001",
            available_event_ids=[
                "event_wake_cache_001",
                "event_contract_broker_001",
                "event_silent_shrine_001",
                "event_ridge_scout_001",
            ],
            completed_quest_ids=["event_silent_shrine_001"],
            owned_card_ids=[
                "act_strike_001",
                "act_guard_001",
                "act_patch_heal_001",
                "act_draw_001",
                "item_contract_001",
                "rule_party_proxy_001",
                "con_silent_oath_001",
            ],
        ),
        Diagnostics(),
        ["event_wake_cache_001", "event_contract_broker_001", "event_silent_shrine_001"],
    )
    assert_focus(progress, "Ridge Scout Battle", "Next Up")
    assert_equal(progress[4]["detail"], "ridge-live", "ridge live detail")

    progress = build_walkthrough_progress(
        Snapshot(
            has_initialized_session=True,
            available_event_ids=[
                "event_wake_cache_001",
                "event_contract_broker_001",
                "event_silent_shrine_001",
                "event_ridge_scout_001",
                "event_proxy_gate_001",
            ],
            completed_quest_ids=["event_silent_shrine_001"],
            owned_card_ids=[
                "act_strike_001",
                "act_guard_001",
                "act_patch_heal_001",
                "act_draw_001",
                "item_contract_001",
                "rule_party_proxy_001",
                "con_silent_oath_001",
                "con_four_party_001",
            ],
        ),
        Diagnostics(),
        [
            "event_wake_cache_001",
            "event_contract_broker_001",
            "event_silent_shrine_001",
            "event_ridge_scout_001",
        ],
    )
    assert_focus(progress, "Proxy Gate", "Next Up")

    progress = build_walkthrough_progress(
        Snapshot(
            has_initialized_session=True,
            zone_cleared=True,
            available_event_ids=[
                "event_wake_cache_001",
                "event_contract_broker_001",
                "event_silent_shrine_001",
                "event_ridge_scout_001",
                "event_proxy_gate_001",
            ],
            completed_quest_ids=["event_silent_shrine_001"],
            owned_card_ids=[
                "act_strike_001",
                "act_guard_001",
                "act_patch_heal_001",
                "act_draw_001",
                "item_contract_001",
                "rule_party_proxy_001",
                "con_silent_oath_001",
                "con_four_party_001",
                "key_zone_core_001",
            ],
        ),
        Diagnostics(),
        [
            "event_wake_cache_001",
            "event_contract_broker_001",
            "event_silent_shrine_001",
            "event_ridge_scout_001",
            "event_proxy_gate_001",
        ],
    )
    assert_focus(progress, "Save And Restore", "Next Up")

    progress = build_walkthrough_progress(
        Snapshot(
            has_initialized_session=True,
            zone_cleared=True,
            available_event_ids=[
                "event_wake_cache_001",
                "event_contract_broker_001",
                "event_silent_shrine_001",
                "event_ridge_scout_001",
                "event_proxy_gate_001",
            ],
            completed_quest_ids=["event_silent_shrine_001"],
            owned_card_ids=[
                "act_strike_001",
                "act_guard_001",
                "act_patch_heal_001",
                "act_draw_001",
                "item_contract_001",
                "rule_party_proxy_001",
                "con_silent_oath_001",
                "con_four_party_001",
                "key_zone_core_001",
            ],
        ),
        Diagnostics(save_exists=True),
        [
            "event_wake_cache_001",
            "event_contract_broker_001",
            "event_silent_shrine_001",
            "event_ridge_scout_001",
            "event_proxy_gate_001",
        ],
    )
    assert_equal(all(item["completed"] for item in progress), True, "all steps completed after save")
    assert_equal(any(item["current_focus"] for item in progress), False, "no current focus after full completion")

    print("OK: UE walkthrough progress smoke tests passed")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as exc:
        print(str(exc), file=sys.stderr)
        raise SystemExit(1)
