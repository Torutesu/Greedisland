#!/usr/bin/env python3
"""End-to-end smoke test for the one-zone MVP loop."""

from __future__ import annotations

import json
import sys
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any


BLOCKED_TERMS = ("ハンター", "グリードアイランド", "念能力", "指定ポケット", "HxH", "G.I.")


@dataclass
class CombatState:
    enemy_hp: int = 0
    hand: list[str] = field(default_factory=list)
    outcome: str = "InProgress"


@dataclass
class Session:
    zone: dict[str, Any]
    known_cards: dict[str, dict[str, Any]]
    events: dict[str, dict[str, Any]]
    owned_card_ids: list[str] = field(default_factory=list)
    deck_card_ids: list[str] = field(default_factory=list)
    completed_event_ids: list[str] = field(default_factory=list)
    available_event_ids: list[str] = field(default_factory=list)
    active_event_id: str = ""
    active_quest_ids: list[str] = field(default_factory=list)
    completed_quest_ids: list[str] = field(default_factory=list)
    zone_cleared: bool = False
    combat_active: bool = False
    combat_state: CombatState = field(default_factory=CombatState)


def load_cards(path: Path) -> dict[str, dict[str, Any]]:
    data = json.loads(path.read_text(encoding="utf-8"))
    return {card["cardId"]: card for card in data["cards"]}


def load_zone(path: Path) -> tuple[dict[str, Any], dict[str, dict[str, Any]]]:
    data = json.loads(path.read_text(encoding="utf-8"))
    return data, {event["eventId"]: event for event in data["events"]}


def initialize_session(zone: dict[str, Any], known_cards: dict[str, dict[str, Any]], events: dict[str, dict[str, Any]]) -> Session:
    return Session(
        zone=zone,
        known_cards=known_cards,
        events=events,
        available_event_ids=[zone["events"][0]["eventId"]],
        active_event_id=zone["events"][0]["eventId"],
    )


def refresh_quest_and_clear_state(session: Session) -> None:
    session.active_quest_ids = []
    for event in session.zone["events"]:
        if event["type"] != "Quest":
            continue
        event_id = event["eventId"]
        if event_id in session.completed_event_ids:
            if event_id not in session.completed_quest_ids:
                session.completed_quest_ids.append(event_id)
            if event_id in session.active_quest_ids:
                session.active_quest_ids.remove(event_id)
            continue
        if event_id in session.available_event_ids and event_id not in session.active_quest_ids:
            session.active_quest_ids.append(event_id)

    session.zone_cleared = all(
        card_id in session.owned_card_ids for card_id in session.zone["clearRequiredCardIds"]
    )


def grant_event_rewards(session: Session, event: dict[str, Any]) -> None:
    for reward_card_id in event["rewardCardIds"]:
        if reward_card_id not in session.owned_card_ids:
            session.owned_card_ids.append(reward_card_id)
        if reward_card_id not in session.deck_card_ids:
            session.deck_card_ids.append(reward_card_id)
    if event["eventId"] not in session.completed_event_ids:
        session.completed_event_ids.append(event["eventId"])
    for next_event_id in event["nextEventIds"]:
        if next_event_id not in session.available_event_ids:
            session.available_event_ids.append(next_event_id)
    session.active_event_id = event["eventId"]
    refresh_quest_and_clear_state(session)


def resolve_event(session: Session, event_id: str) -> tuple[bool, list[str]]:
    event = session.events[event_id]
    if event["type"] == "Battle":
        return False, [f"Event {event_id} is battle-only."]
    if event_id not in session.available_event_ids:
        return False, [f"Event {event_id} is not available."]
    for required_card_id in event["requiredCardIds"]:
        if required_card_id not in session.owned_card_ids:
            return False, [f"Missing required card {required_card_id}."]
    grant_event_rewards(session, event)
    return True, []


def build_ai_request(session: Session, event_id: str, player_choice: str) -> dict[str, Any]:
    event = session.events[event_id]
    tags: list[str] = []
    for card_id in session.owned_card_ids:
        tags.extend(session.known_cards[card_id]["tags"])
    return {
        "zoneId": session.zone["zoneId"],
        "eventId": event_id,
        "npcId": event.get("npcId", ""),
        "playerCollectionTags": tags,
        "allowedRewardCardIds": event.get("allowedAiRewardCardIds", []),
        "playerChoice": player_choice,
    }


def build_fallback_ai_response(event: dict[str, Any], player_choice: str) -> dict[str, Any]:
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


def validate_ai_response(response: dict[str, Any], request: dict[str, Any], known_card_ids: set[str]) -> list[str]:
    reasons: list[str] = []
    if not str(response.get("speakerName", "")).strip():
        reasons.append("speaker empty")
    dialogue = str(response.get("dialogue", ""))
    if not dialogue.strip():
        reasons.append("dialogue empty")
    if any(term.lower() in dialogue.lower() for term in BLOCKED_TERMS):
        reasons.append("blocked wording")
    rewards = response.get("allowedRewardCardIds", [])
    if len(rewards) > 3:
        reasons.append("too many rewards")
    for reward_id in rewards:
        if reward_id not in request["allowedRewardCardIds"]:
            reasons.append("reward not allowed")
        if reward_id not in known_card_ids:
            reasons.append("reward not defined")
    return reasons


def apply_ai_response(session: Session, response: dict[str, Any], player_choice: str) -> tuple[bool, list[str]]:
    request = build_ai_request(session, session.active_event_id, player_choice)
    reasons = validate_ai_response(response, request, set(session.known_cards.keys()))
    if reasons:
        return False, reasons
    for reward_id in response["allowedRewardCardIds"]:
        if reward_id not in session.owned_card_ids:
            session.owned_card_ids.append(reward_id)
        if reward_id not in session.deck_card_ids:
            session.deck_card_ids.append(reward_id)
    refresh_quest_and_clear_state(session)
    return True, []


def start_battle(session: Session, event_id: str, opening_draw_count: int) -> tuple[bool, list[str]]:
    event = session.events[event_id]
    if event["type"] != "Battle":
        return False, [f"{event_id} is not battle."]
    if event_id not in session.available_event_ids:
        return False, [f"{event_id} not available."]
    session.combat_active = True
    session.active_event_id = event_id
    session.combat_state = CombatState(
        enemy_hp=event["enemyHp"],
        hand=session.deck_card_ids[:opening_draw_count],
        outcome="InProgress",
    )
    return True, []


def play_card(session: Session, card_id: str) -> tuple[bool, list[str]]:
    if not session.combat_active:
        return False, ["no active combat"]
    if card_id not in session.combat_state.hand:
        return False, [f"{card_id} not in hand"]

    card = session.known_cards[card_id]
    for effect in card["effects"]:
        if effect["type"] == "Damage":
            session.combat_state.enemy_hp = max(0, session.combat_state.enemy_hp - effect["amount"])

    session.combat_state.hand.remove(card_id)
    if session.combat_state.enemy_hp <= 0:
        session.combat_state.outcome = "PlayerVictory"
        session.combat_active = False
        grant_event_rewards(session, session.events[session.active_event_id])
    return True, []


def copy_session_to_save(session: Session) -> dict[str, Any]:
    return {
        "currentZoneId": session.zone["zoneId"],
        "ownedCardIds": list(session.owned_card_ids),
        "deckCardIds": list(session.deck_card_ids),
        "completedEventIds": list(session.completed_event_ids),
        "availableEventIds": list(session.available_event_ids),
        "activeQuestIds": list(session.active_quest_ids),
        "completedQuestIds": list(session.completed_quest_ids),
        "activeEventId": session.active_event_id,
        "zoneCleared": session.zone_cleared,
    }


def restore_session_from_save(
    save_data: dict[str, Any],
    zone: dict[str, Any],
    known_cards: dict[str, dict[str, Any]],
    events: dict[str, dict[str, Any]],
) -> Session:
    restored = initialize_session(zone, known_cards, events)
    restored.owned_card_ids = list(save_data["ownedCardIds"])
    restored.deck_card_ids = list(save_data["deckCardIds"])
    restored.completed_event_ids = list(save_data["completedEventIds"])
    restored.available_event_ids = list(save_data["availableEventIds"])
    restored.active_quest_ids = list(save_data["activeQuestIds"])
    restored.completed_quest_ids = list(save_data["completedQuestIds"])
    restored.active_event_id = save_data["activeEventId"]
    restored.zone_cleared = bool(save_data["zoneCleared"])
    return restored


def assert_true(value: bool, label: str, detail: Any = None) -> None:
    if not value:
        raise AssertionError(f"{label} failed: {detail}")


def main() -> int:
    known_cards = load_cards(Path("data/cards/cards.mvp.json"))
    zone, events = load_zone(Path("data/events/events.mvp.json"))
    session = initialize_session(zone, known_cards, events)
    refresh_quest_and_clear_state(session)

    ok, reasons = resolve_event(session, "event_wake_cache_001")
    assert_true(ok, "wake cache resolves", reasons)
    assert_true("act_strike_001" in session.owned_card_ids, "starter strike acquired", session.owned_card_ids)

    session.active_event_id = "event_contract_broker_001"
    request = build_ai_request(session, "event_contract_broker_001", "交渉する")
    assert_true(request["allowedRewardCardIds"] == ["item_contract_001"], "AI request rewards", request)

    fallback_response = build_fallback_ai_response(events["event_contract_broker_001"], "交渉する")
    ok, reasons = apply_ai_response(session, fallback_response, "交渉する")
    assert_true(ok, "fallback AI response applies", reasons)
    assert_true("item_contract_001" in session.owned_card_ids, "AI reward granted", session.owned_card_ids)

    ok, reasons = resolve_event(session, "event_contract_broker_001")
    assert_true(ok, "contract broker resolves", reasons)
    assert_true("rule_party_proxy_001" in session.owned_card_ids, "proxy rule acquired", session.owned_card_ids)
    assert_true("event_silent_shrine_001" in session.active_quest_ids, "quest unlocked", session.active_quest_ids)

    ok, reasons = resolve_event(session, "event_silent_shrine_001")
    assert_true(ok, "silent shrine resolves", reasons)
    assert_true("event_silent_shrine_001" in session.completed_quest_ids, "quest completed", session.completed_quest_ids)

    ok, reasons = start_battle(session, "event_ridge_scout_001", opening_draw_count=4)
    assert_true(ok, "ridge scout battle starts", reasons)
    session.combat_state.hand = ["act_strike_001", "act_strike_001"]
    ok, reasons = play_card(session, "act_strike_001")
    assert_true(ok, "first strike hits", reasons)
    session.combat_state.hand.append("act_strike_001")
    ok, reasons = play_card(session, "act_strike_001")
    assert_true(ok, "second strike wins", reasons)
    assert_true("con_four_party_001" in session.owned_card_ids, "battle reward acquired", session.owned_card_ids)

    ok, reasons = resolve_event(session, "event_proxy_gate_001")
    assert_true(ok, "proxy gate resolves", reasons)
    assert_true("key_zone_core_001" in session.owned_card_ids, "key acquired", session.owned_card_ids)
    assert_true(session.zone_cleared, "zone clears at end", session.zone_cleared)

    save_data = copy_session_to_save(session)
    restored = restore_session_from_save(save_data, zone, known_cards, events)
    assert_true(restored.zone_cleared, "zone clear survives restore", restored.zone_cleared)
    assert_true(restored.owned_card_ids == session.owned_card_ids, "owned cards survive restore", restored.owned_card_ids)
    assert_true(restored.completed_quest_ids == session.completed_quest_ids, "quests survive restore", restored.completed_quest_ids)

    print("OK: MVP full run smoke tests passed")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as exc:
        print(str(exc), file=sys.stderr)
        raise SystemExit(1)
