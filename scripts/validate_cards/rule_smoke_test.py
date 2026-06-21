#!/usr/bin/env python3
"""Smoke-test the three MVP rule-hack examples from card JSON."""

from __future__ import annotations

import json
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Any


@dataclass(frozen=True)
class Context:
    phase: str
    energy: int
    base_party_size: int = 1
    collection_tags: tuple[str, ...] = ()
    active_statuses: tuple[str, ...] = ()
    active_rule_card_ids: tuple[str, ...] = ()


def load_cards(path: Path) -> dict[str, dict[str, Any]]:
    data = json.loads(path.read_text(encoding="utf-8"))
    return {card["cardId"]: card for card in data["cards"]}


def gather_rule_patches(cards: dict[str, dict[str, Any]], context: Context) -> list[dict[str, Any]]:
    patches: list[dict[str, Any]] = []
    for card_id in context.active_rule_card_ids:
        card = cards[card_id]
        for effect in card["effects"]:
            if effect["type"] == "ModifyRule":
                patch = dict(effect)
                patch["sourceCardId"] = card_id
                patches.append(patch)
    return sorted(patches, key=lambda patch: (patch.get("priority", 0), patch["sourceCardId"]))


def has_tag(card: dict[str, Any], tag: str) -> bool:
    return tag in card.get("tags", [])


def phase_allowed(cards: dict[str, dict[str, Any]], card_id: str, context: Context) -> bool:
    card = cards[card_id]
    if context.phase in card.get("playablePhases", []):
        return True

    for patch in gather_rule_patches(cards, context):
        if (
            patch.get("channel") == "CardPlayablePhase"
            and patch.get("operation") == "AddPhase"
            and patch.get("phase") == context.phase
            and has_tag(card, patch.get("targetTag", ""))
        ):
            return True

    return False


def resolve_int_rule(cards: dict[str, dict[str, Any]], channel: str, base_value: int, context: Context) -> int:
    value = base_value
    for patch in gather_rule_patches(cards, context):
        if patch.get("channel") != channel:
            continue
        operation = patch.get("operation")
        if operation == "AddValue":
            value += int(patch.get("value", 0))
        elif operation == "SetValue":
            value = int(patch.get("value", 0))
        elif operation == "MultiplyValue":
            value *= int(patch.get("value", 1))
        elif operation == "AddMatchingTagCount":
            value += context.collection_tags.count(patch.get("targetTag", "")) * int(patch.get("value", 1))
    return value


def can_play(cards: dict[str, dict[str, Any]], card_id: str, context: Context) -> tuple[bool, list[str]]:
    card = cards[card_id]
    reasons: list[str] = []

    if card["kind"] == "Key":
        reasons.append("key cards cannot be played")
    if card["cost"]["energy"] > context.energy:
        reasons.append("not enough energy")
    if not phase_allowed(cards, card_id, context):
        reasons.append("phase not allowed")

    effective_party_size = resolve_int_rule(cards, "PartyRequirement", context.base_party_size, context)

    for constraint in card.get("constraints", []):
        ctype = constraint["type"]
        if ctype == "HasTagInCollection":
            if context.collection_tags.count(constraint["tag"]) < constraint["minCount"]:
                reasons.append(f"missing tag {constraint['tag']}")
        elif ctype == "EffectivePartySizeAtLeast":
            if effective_party_size < constraint["minCount"]:
                reasons.append(f"party too small: {effective_party_size}")
        elif ctype == "HasStatus":
            if constraint["statusId"] not in context.active_statuses:
                reasons.append(f"missing status {constraint['statusId']}")
        elif ctype == "NotHasStatus":
            if constraint["statusId"] in context.active_statuses:
                reasons.append(f"blocked by status {constraint['statusId']}")

    return len(reasons) == 0, reasons


def assert_true(value: bool, label: str, detail: Any = None) -> None:
    if not value:
        raise AssertionError(f"{label} failed: {detail}")


def main() -> int:
    cards = load_cards(Path("data/cards/cards.mvp.json"))

    blocked, reasons = can_play(
        cards,
        "act_battle_claim_001",
        Context(phase="Exploration", energy=3),
    )
    assert_true(not blocked, "battle-only card should be blocked in exploration", reasons)

    bridged, reasons = can_play(
        cards,
        "act_battle_claim_001",
        Context(
            phase="Exploration",
            energy=3,
            collection_tags=("contract",),
            active_rule_card_ids=("rule_phase_bridge_001",),
        ),
    )
    assert_true(bridged, "phase bridge should allow battle-only card in exploration", reasons)

    blocked, reasons = can_play(
        cards,
        "con_four_party_001",
        Context(phase="Exploration", energy=3, base_party_size=3),
    )
    assert_true(not blocked, "four-party proof should be blocked at party size 3", reasons)

    proxied, reasons = can_play(
        cards,
        "con_four_party_001",
        Context(
            phase="Exploration",
            energy=3,
            base_party_size=3,
            collection_tags=("contract",),
            active_rule_card_ids=("rule_party_proxy_001",),
        ),
    )
    assert_true(proxied, "party proxy should count contract tag as one party member", reasons)

    reward_multiplier = resolve_int_rule(
        cards,
        "RewardMultiplier",
        1,
        Context(
            phase="Combat",
            energy=3,
            active_statuses=("silent_oath",),
            active_rule_card_ids=("con_silent_oath_001", "rule_reward_oath_001"),
        ),
    )
    assert_true(reward_multiplier == 4, "reward oath should raise multiplier to 4", reward_multiplier)

    print("OK: rule smoke tests passed")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as exc:
        print(str(exc), file=sys.stderr)
        raise SystemExit(1)

