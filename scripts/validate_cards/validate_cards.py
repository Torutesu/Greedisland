#!/usr/bin/env python3
"""Validate Greeisland card JSON without requiring Unreal Engine."""

from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path
from typing import Any


CARD_ID_RE = re.compile(r"^[a-z]+_[a-z0-9_]+_[0-9]{3}$")
TAG_RE = re.compile(r"^[a-z][a-z0-9_]*$")

KINDS = {"Action", "Item", "Rule", "Constraint", "Key"}
RARITIES = {"Common", "Uncommon", "Rare", "Epic", "Legendary", "Key"}
PHASES = {"Exploration", "Combat", "Dialogue", "Reward"}
EFFECTS = {
    "Damage",
    "GainBlock",
    "Heal",
    "DrawCards",
    "ModifyRule",
    "GainCard",
    "AddStatus",
    "RemoveStatus",
    "ClaimReward",
    "PreventFailure",
}
CONSTRAINTS = {
    "HasTagInCollection",
    "EffectivePartySizeAtLeast",
    "HandCountAtMost",
    "HasStatus",
    "NotHasStatus",
}
RULE_CHANNELS = {
    "DrawCount",
    "HandLimit",
    "MoveCount",
    "EncounterDifficulty",
    "TradePrice",
    "RewardMultiplier",
    "CardPlayablePhase",
    "PartyRequirement",
    "TargetingRule",
}
OPERATIONS = {
    "AddValue",
    "SetValue",
    "MultiplyValue",
    "AddPhase",
    "AddMatchingTagCount",
    "SetFlag",
}
DURATIONS = {"Instant", "Turns", "Encounter", "Permanent"}


class ValidationError(Exception):
    pass


def require(condition: bool, path: str, message: str) -> None:
    if not condition:
        raise ValidationError(f"{path}: {message}")


def require_string(obj: dict[str, Any], key: str, path: str) -> str:
    value = obj.get(key)
    require(isinstance(value, str) and value, f"{path}.{key}", "must be a non-empty string")
    return value


def require_int(obj: dict[str, Any], key: str, path: str, minimum: int | None = None) -> int:
    value = obj.get(key)
    require(isinstance(value, int) and not isinstance(value, bool), f"{path}.{key}", "must be an integer")
    if minimum is not None:
        require(value >= minimum, f"{path}.{key}", f"must be >= {minimum}")
    return value


def require_string_list(obj: dict[str, Any], key: str, path: str, *, allow_empty: bool = False) -> list[str]:
    value = obj.get(key)
    require(isinstance(value, list), f"{path}.{key}", "must be an array")
    require(allow_empty or len(value) > 0, f"{path}.{key}", "must not be empty")
    for index, item in enumerate(value):
        require(isinstance(item, str) and item, f"{path}.{key}[{index}]", "must be a non-empty string")
    return value


def validate_duration(effect: dict[str, Any], path: str) -> None:
    duration = effect.get("duration")
    require(isinstance(duration, dict), f"{path}.duration", "must be an object")
    dtype = require_string(duration, "type", f"{path}.duration")
    require(dtype in DURATIONS, f"{path}.duration.type", f"unknown duration {dtype!r}")
    value = require_int(duration, "value", f"{path}.duration", minimum=0)
    if dtype == "Instant":
        require(value == 0, f"{path}.duration.value", "Instant duration must be 0")
    if dtype in {"Turns", "Encounter"}:
        require(value >= 1, f"{path}.duration.value", f"{dtype} duration must be >= 1")


def validate_constraint(constraint: dict[str, Any], path: str) -> None:
    ctype = require_string(constraint, "type", path)
    require(ctype in CONSTRAINTS, f"{path}.type", f"unknown constraint {ctype!r}")

    if ctype == "HasTagInCollection":
        tag = require_string(constraint, "tag", path)
        require(TAG_RE.match(tag) is not None, f"{path}.tag", "must be snake_case")
        require_int(constraint, "minCount", path, minimum=1)
    elif ctype == "EffectivePartySizeAtLeast":
        require_int(constraint, "minCount", path, minimum=1)
    elif ctype == "HandCountAtMost":
        require_int(constraint, "value", path, minimum=0)
    elif ctype in {"HasStatus", "NotHasStatus"}:
        status_id = require_string(constraint, "statusId", path)
        require(TAG_RE.match(status_id) is not None, f"{path}.statusId", "must be snake_case")


def validate_effect(effect: dict[str, Any], path: str, card_ids: set[str]) -> None:
    etype = require_string(effect, "type", path)
    require(etype in EFFECTS, f"{path}.type", f"unknown effect {etype!r}")
    validate_duration(effect, path)

    priority = require_int(effect, "priority", path, minimum=0)
    require(priority <= 1000, f"{path}.priority", "must be <= 1000")

    if etype in {"Damage", "GainBlock", "Heal", "DrawCards", "ClaimReward", "PreventFailure"}:
        require_int(effect, "amount", path, minimum=1)
    elif etype == "GainCard":
        card_id = require_string(effect, "cardId", path)
        require(card_id in card_ids, f"{path}.cardId", f"unknown card id {card_id!r}")
    elif etype in {"AddStatus", "RemoveStatus"}:
        status_id = require_string(effect, "statusId", path)
        require(TAG_RE.match(status_id) is not None, f"{path}.statusId", "must be snake_case")
    elif etype == "ModifyRule":
        channel = require_string(effect, "channel", path)
        operation = require_string(effect, "operation", path)
        require(channel in RULE_CHANNELS, f"{path}.channel", f"unknown channel {channel!r}")
        require(operation in OPERATIONS, f"{path}.operation", f"unknown operation {operation!r}")
        if operation in {"AddValue", "SetValue", "MultiplyValue", "AddMatchingTagCount"}:
            require_int(effect, "value", path)
        if operation in {"AddPhase"}:
            phase = require_string(effect, "phase", path)
            require(phase in PHASES, f"{path}.phase", f"unknown phase {phase!r}")
        if operation in {"AddPhase", "AddMatchingTagCount"}:
            tag = require_string(effect, "targetTag", path)
            require(TAG_RE.match(tag) is not None, f"{path}.targetTag", "must be snake_case")


def validate_card(card: dict[str, Any], path: str, card_ids: set[str]) -> None:
    card_id = require_string(card, "cardId", path)
    require(CARD_ID_RE.match(card_id) is not None, f"{path}.cardId", "must match card id format")

    require_string(card, "displayName", path)

    kind = require_string(card, "kind", path)
    rarity = require_string(card, "rarity", path)
    require(kind in KINDS, f"{path}.kind", f"unknown kind {kind!r}")
    require(rarity in RARITIES, f"{path}.rarity", f"unknown rarity {rarity!r}")
    if kind == "Key":
        require(rarity == "Key", f"{path}.rarity", "Key cards must use Key rarity")

    tags = require_string_list(card, "tags", path)
    for index, tag in enumerate(tags):
        require(TAG_RE.match(tag) is not None, f"{path}.tags[{index}]", "must be snake_case")
    require(len(tags) == len(set(tags)), f"{path}.tags", "must be unique")

    phases = require_string_list(card, "playablePhases", path, allow_empty=(kind == "Key"))
    for index, phase in enumerate(phases):
        require(phase in PHASES, f"{path}.playablePhases[{index}]", f"unknown phase {phase!r}")
    require(len(phases) == len(set(phases)), f"{path}.playablePhases", "must be unique")

    cost = card.get("cost")
    require(isinstance(cost, dict), f"{path}.cost", "must be an object")
    require_int(cost, "energy", f"{path}.cost", minimum=0)

    constraints = card.get("constraints")
    require(isinstance(constraints, list), f"{path}.constraints", "must be an array")
    for index, constraint in enumerate(constraints):
        require(isinstance(constraint, dict), f"{path}.constraints[{index}]", "must be an object")
        validate_constraint(constraint, f"{path}.constraints[{index}]")

    effects = card.get("effects")
    require(isinstance(effects, list), f"{path}.effects", "must be an array")
    if kind != "Key":
        require(len(effects) > 0, f"{path}.effects", "non-Key cards need at least one effect")
    for index, effect in enumerate(effects):
        require(isinstance(effect, dict), f"{path}.effects[{index}]", "must be an object")
        validate_effect(effect, f"{path}.effects[{index}]", card_ids)

    require_string(card, "flavorText", path)


def validate_cards(data: dict[str, Any]) -> list[str]:
    errors: list[str] = []
    cards = data.get("cards")
    if not isinstance(cards, list):
        return ["cards: must be an array"]

    card_ids: set[str] = set()
    for index, card in enumerate(cards):
        if not isinstance(card, dict):
            errors.append(f"cards[{index}]: must be an object")
            continue
        card_id = card.get("cardId")
        if isinstance(card_id, str):
            if card_id in card_ids:
                errors.append(f"cards[{index}].cardId: duplicate id {card_id!r}")
            card_ids.add(card_id)

    for index, card in enumerate(cards):
        if not isinstance(card, dict):
            continue
        try:
            validate_card(card, f"cards[{index}]", card_ids)
        except ValidationError as exc:
            errors.append(str(exc))

    kinds = {card.get("kind") for card in cards if isinstance(card, dict)}
    for required_kind in {"Action", "Item", "Rule", "Constraint", "Key"}:
        if required_kind not in kinds:
            errors.append(f"cards: missing at least one {required_kind} card")

    if len(cards) < 15:
        errors.append("cards: MVP set should contain at least 15 cards")

    return errors


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("path", type=Path)
    args = parser.parse_args()

    try:
        data = json.loads(args.path.read_text(encoding="utf-8"))
    except Exception as exc:
        print(f"Failed to read JSON: {exc}", file=sys.stderr)
        return 2

    if not isinstance(data, dict):
        print("Root JSON must be an object", file=sys.stderr)
        return 2

    errors = validate_cards(data)
    if errors:
        print("Card validation failed:")
        for error in errors:
            print(f"- {error}")
        return 1

    print(f"OK: {len(data['cards'])} cards validated")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

