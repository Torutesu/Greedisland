#!/usr/bin/env python3
"""Smoke-test developer card grant behavior for the MVP runtime model."""

from __future__ import annotations

import json
import sys
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any


@dataclass
class Session:
    zone_id: str
    known_cards: dict[str, dict[str, Any]]
    owned_card_ids: list[str] = field(default_factory=list)
    deck_card_ids: list[str] = field(default_factory=list)
    zone_cleared: bool = False


def load_cards(path: Path) -> dict[str, dict[str, Any]]:
    data = json.loads(path.read_text(encoding="utf-8"))
    return {card["cardId"]: card for card in data["cards"]}


def initialize_session(cards: dict[str, dict[str, Any]]) -> Session:
    return Session(zone_id="zone_mvp_island_001", known_cards=cards)


def refresh_zone_clear_state(session: Session) -> None:
    session.zone_cleared = "key_zone_core_001" in session.owned_card_ids


def grant_card(session: Session, card_id: str, add_to_deck: bool = True) -> tuple[bool, list[str]]:
    if card_id not in session.known_cards:
        return False, [f"unknown card {card_id}"]

    if card_id not in session.owned_card_ids:
        session.owned_card_ids.append(card_id)
    if add_to_deck and card_id not in session.deck_card_ids:
        session.deck_card_ids.append(card_id)

    refresh_zone_clear_state(session)
    return True, []


def assert_true(value: bool, label: str, detail: Any = None) -> None:
    if not value:
        raise AssertionError(f"{label} failed: {detail}")


def main() -> int:
    cards = load_cards(Path("data/cards/cards.mvp.json"))
    session = initialize_session(cards)

    ok, reasons = grant_card(session, "rule_reward_oath_001")
    assert_true(ok, "grant known card succeeds", reasons)
    assert_true("rule_reward_oath_001" in session.owned_card_ids, "card added to owned", session.owned_card_ids)
    assert_true("rule_reward_oath_001" in session.deck_card_ids, "card added to deck", session.deck_card_ids)

    ok, reasons = grant_card(session, "key_zone_core_001", add_to_deck=False)
    assert_true(ok, "grant key card succeeds", reasons)
    assert_true("key_zone_core_001" in session.owned_card_ids, "key card added to owned", session.owned_card_ids)
    assert_true("key_zone_core_001" not in session.deck_card_ids, "key card not added to deck", session.deck_card_ids)
    assert_true(session.zone_cleared, "zone clear refreshed from granted key card", session.zone_cleared)

    ok, reasons = grant_card(session, "card_missing_999")
    assert_true(not ok, "unknown card rejected", reasons)

    print("OK: developer grant smoke tests passed")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as exc:
        print(str(exc), file=sys.stderr)
        raise SystemExit(1)
