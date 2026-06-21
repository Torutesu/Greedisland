#!/usr/bin/env python3
"""Smoke-test card playability reasons mirrored from URuleResolver combat checks."""

from __future__ import annotations

import json
import sys
from pathlib import Path
from typing import Any


def load_cards(path: Path) -> dict[str, dict[str, Any]]:
    data = json.loads(path.read_text(encoding="utf-8"))
    return {card["cardId"]: card for card in data["cards"]}


def can_play(card: dict[str, Any], *, energy: int, phase: str, hand_count: int, tags: list[str]) -> tuple[bool, int, list[str]]:
    reasons: list[str] = []
    effective_party_size = 1

    if card["kind"] == "Key":
        reasons.append("Key cards are progression state and cannot be played directly.")

    if card["cost"]["energy"] > energy:
        reasons.append(f"Not enough energy: need {card['cost']['energy']}, have {energy}.")

    if phase not in card["playablePhases"]:
        reasons.append("Card is not playable in the current phase.")

    for constraint in card.get("constraints", []):
        ctype = constraint["type"]
        if ctype == "HasTagInCollection":
            count = sum(1 for tag in tags if tag == constraint["tag"])
            if count < constraint["minCount"]:
                reasons.append(
                    f"Requires at least {constraint['minCount']} collected card(s) with tag '{constraint['tag']}'."
                )
        elif ctype == "EffectivePartySizeAtLeast":
            if effective_party_size < constraint["minCount"]:
                reasons.append(
                    f"Requires effective party size {constraint['minCount']}, currently {effective_party_size}."
                )
        elif ctype == "HandCountAtMost":
            if hand_count > constraint["value"]:
                reasons.append(f"Requires hand count at most {constraint['value']}, currently {hand_count}.")
        elif ctype == "HasStatus":
            reasons.append(f"Requires status '{constraint['statusId']}'.")

    return len(reasons) == 0, effective_party_size, reasons


def assert_true(value: bool, label: str, detail: Any = None) -> None:
    if not value:
        raise AssertionError(f"{label} failed: {detail}")


def main() -> int:
    cards = load_cards(Path("data/cards/cards.mvp.json"))

    four_party_ok, party_size, four_party_reasons = can_play(
        cards["con_four_party_001"],
        energy=3,
        phase="Combat",
        hand_count=1,
        tags=[],
    )
    assert_true(not four_party_ok, "four-party card should be blocked", four_party_reasons)
    assert_true(
        any("effective party size 4" in reason for reason in four_party_reasons),
        "party size reason present",
        four_party_reasons,
    )
    assert_true(
        any("current phase" in reason for reason in four_party_reasons),
        "phase reason present",
        four_party_reasons,
    )
    assert_true(party_size == 1, "effective party size mirrored", party_size)

    bridge_ok, _, bridge_reasons = can_play(
        cards["rule_phase_bridge_001"],
        energy=1,
        phase="Exploration",
        hand_count=1,
        tags=[],
    )
    assert_true(not bridge_ok, "phase bridge needs collection tag", bridge_reasons)
    assert_true(
        any("tag 'contract'" in reason for reason in bridge_reasons),
        "collection tag reason present",
        bridge_reasons,
    )

    strike_ok, _, strike_reasons = can_play(
        cards["act_strike_001"],
        energy=0,
        phase="Combat",
        hand_count=1,
        tags=[],
    )
    assert_true(not strike_ok, "strike should fail on energy", strike_reasons)
    assert_true(
        any("Not enough energy" in reason for reason in strike_reasons),
        "energy reason present",
        strike_reasons,
    )

    print("OK: playability reason smoke tests passed")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as exc:
        print(str(exc), file=sys.stderr)
        raise SystemExit(1)
