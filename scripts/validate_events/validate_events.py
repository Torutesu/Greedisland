#!/usr/bin/env python3
"""Validate MVP exploration event JSON against the card set."""

from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path
from typing import Any


EVENT_ID_RE = re.compile(r"^event_[a-z0-9_]+_[0-9]{3}$")
ENTITY_ID_RE = re.compile(r"^[a-z]+_[a-z0-9_]+_[0-9]{3}$")
EVENT_TYPES = {"Battle", "Treasure", "Npc", "Quest", "KeyGate"}


class ValidationError(Exception):
    pass


def require(condition: bool, path: str, message: str) -> None:
    if not condition:
        raise ValidationError(f"{path}: {message}")


def require_string(obj: dict[str, Any], key: str, path: str) -> str:
    value = obj.get(key)
    require(isinstance(value, str) and value, f"{path}.{key}", "must be a non-empty string")
    return value


def optional_string(obj: dict[str, Any], key: str) -> str | None:
    value = obj.get(key)
    if value is None:
        return None
    if isinstance(value, str) and value:
        return value
    raise ValidationError(f"{key}: must be a non-empty string when provided")


def require_string_list(obj: dict[str, Any], key: str, path: str, *, allow_empty: bool = True) -> list[str]:
    value = obj.get(key)
    require(isinstance(value, list), f"{path}.{key}", "must be an array")
    require(allow_empty or len(value) > 0, f"{path}.{key}", "must not be empty")
    for index, item in enumerate(value):
        require(isinstance(item, str) and item, f"{path}.{key}[{index}]", "must be a non-empty string")
    require(len(value) == len(set(value)), f"{path}.{key}", "must be unique")
    return value


def load_card_ids(path: Path) -> set[str]:
    data = json.loads(path.read_text(encoding="utf-8"))
    cards = data.get("cards")
    if not isinstance(cards, list):
        raise ValidationError("card file: cards must be an array")
    return {card["cardId"] for card in cards if isinstance(card, dict) and isinstance(card.get("cardId"), str)}


def validate_card_refs(ids: list[str], known_card_ids: set[str], path: str, errors: list[str]) -> None:
    for index, card_id in enumerate(ids):
        if card_id not in known_card_ids:
            errors.append(f"{path}[{index}]: unknown card id {card_id!r}")


def validate_events(data: dict[str, Any], known_card_ids: set[str]) -> list[str]:
    errors: list[str] = []

    zone_id = data.get("zoneId")
    if not isinstance(zone_id, str) or not zone_id:
        errors.append("zoneId: must be a non-empty string")

    clear_required = data.get("clearRequiredCardIds", [])
    if not isinstance(clear_required, list):
        errors.append("clearRequiredCardIds: must be an array")
        clear_required = []
    else:
        validate_card_refs(clear_required, known_card_ids, "clearRequiredCardIds", errors)

    events = data.get("events")
    if not isinstance(events, list):
        return errors + ["events: must be an array"]
    if len(events) < 4:
        errors.append("events: MVP zone should contain at least four events")

    event_ids: set[str] = set()
    for index, event in enumerate(events):
        path = f"events[{index}]"
        if not isinstance(event, dict):
            errors.append(f"{path}: must be an object")
            continue
        event_id = event.get("eventId")
        if not isinstance(event_id, str):
            errors.append(f"{path}.eventId: must be a string")
            continue
        if event_id in event_ids:
            errors.append(f"{path}.eventId: duplicate id {event_id!r}")
        event_ids.add(event_id)

    for index, event in enumerate(events):
        path = f"events[{index}]"
        if not isinstance(event, dict):
            continue

        try:
            event_id = require_string(event, "eventId", path)
            require(EVENT_ID_RE.match(event_id) is not None, f"{path}.eventId", "must match event id format")

            etype = require_string(event, "type", path)
            require(etype in EVENT_TYPES, f"{path}.type", f"unknown event type {etype!r}")

            require_string(event, "displayName", path)
            require_string(event, "description", path)

            reward_card_ids = require_string_list(event, "rewardCardIds", path)
            required_card_ids = require_string_list(event, "requiredCardIds", path)
            next_event_ids = require_string_list(event, "nextEventIds", path)
            validate_card_refs(reward_card_ids, known_card_ids, f"{path}.rewardCardIds", errors)
            validate_card_refs(required_card_ids, known_card_ids, f"{path}.requiredCardIds", errors)

            for next_index, next_event_id in enumerate(next_event_ids):
                if next_event_id not in event_ids:
                    errors.append(f"{path}.nextEventIds[{next_index}]: unknown event id {next_event_id!r}")

            allowed_ai_rewards = event.get("allowedAiRewardCardIds", [])
            if not isinstance(allowed_ai_rewards, list):
                errors.append(f"{path}.allowedAiRewardCardIds: must be an array when provided")
            else:
                validate_card_refs(allowed_ai_rewards, known_card_ids, f"{path}.allowedAiRewardCardIds", errors)

            npc_id = optional_string(event, "npcId")
            if npc_id is not None:
                require(ENTITY_ID_RE.match(npc_id) is not None, f"{path}.npcId", "must match entity id format")

            enemy_id = optional_string(event, "enemyId")
            if etype == "Battle":
                require(enemy_id is not None, f"{path}.enemyId", "Battle events need an enemyId")
                enemy_hp = event.get("enemyHp")
                enemy_attack = event.get("enemyAttack")
                require(isinstance(enemy_hp, int) and enemy_hp > 0, f"{path}.enemyHp", "must be a positive integer")
                require(isinstance(enemy_attack, int) and enemy_attack >= 0, f"{path}.enemyAttack", "must be a non-negative integer")
            elif enemy_id is not None:
                errors.append(f"{path}.enemyId: only Battle events should define enemyId")
        except ValidationError as exc:
            errors.append(str(exc))

    if not any(isinstance(event, dict) and event.get("type") == "Npc" for event in events):
        errors.append("events: MVP zone should include at least one Npc event")
    if not any(isinstance(event, dict) and event.get("type") == "Battle" for event in events):
        errors.append("events: MVP zone should include at least one Battle event")
    if not any(isinstance(event, dict) and event.get("type") == "KeyGate" for event in events):
        errors.append("events: MVP zone should include at least one KeyGate event")

    return errors


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("events_path", type=Path)
    parser.add_argument("--cards", type=Path, default=Path("data/cards/cards.mvp.json"))
    args = parser.parse_args()

    try:
        known_card_ids = load_card_ids(args.cards)
        data = json.loads(args.events_path.read_text(encoding="utf-8"))
    except Exception as exc:
        print(f"Failed to read inputs: {exc}", file=sys.stderr)
        return 2

    if not isinstance(data, dict):
        print("Root JSON must be an object", file=sys.stderr)
        return 2

    errors = validate_events(data, known_card_ids)
    if errors:
        print("Event validation failed:")
        for error in errors:
            print(f"- {error}")
        return 1

    print(f"OK: {len(data['events'])} events validated")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

