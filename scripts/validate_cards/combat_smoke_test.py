#!/usr/bin/env python3
"""Smoke-test the minimal combat loop mirrored from UCombatEngine."""

from __future__ import annotations

import json
import sys
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any


@dataclass
class Combatant:
    max_hp: int
    current_hp: int
    block: int = 0


@dataclass
class CombatState:
    player: Combatant
    enemy: Combatant
    turn_number: int = 1
    energy: int = 3
    draw_pile: list[str] = field(default_factory=list)
    hand: list[str] = field(default_factory=list)
    discard_pile: list[str] = field(default_factory=list)
    claimed_reward_card_ids: list[str] = field(default_factory=list)
    outcome: str = "InProgress"


def load_cards(path: Path) -> dict[str, dict[str, Any]]:
    data = json.loads(path.read_text(encoding="utf-8"))
    return {card["cardId"]: card for card in data["cards"]}


def create_combat(deck_card_ids: list[str], enemy_hp: int, starting_energy: int = 3) -> CombatState:
    return CombatState(
        player=Combatant(max_hp=30, current_hp=30),
        enemy=Combatant(max_hp=enemy_hp, current_hp=enemy_hp),
        energy=max(0, starting_energy),
        draw_pile=list(deck_card_ids),
    )


def draw_cards(state: CombatState, count: int) -> list[str]:
    log: list[str] = []
    for _ in range(max(0, count)):
        if not state.draw_pile:
            if not state.discard_pile:
                log.append("No cards left to draw.")
                return log
            state.draw_pile = state.discard_pile
            state.discard_pile = []
            log.append("Discard pile recycled into draw pile.")

        card_id = state.draw_pile.pop(0)
        state.hand.append(card_id)
        log.append(f"Drew {card_id}.")
    return log


def apply_incoming_damage(target: Combatant, damage: int) -> int:
    blocked = min(target.block, damage)
    target.block -= blocked
    applied = damage - blocked
    target.current_hp = max(0, min(target.max_hp, target.current_hp - applied))
    return applied


def update_outcome(state: CombatState) -> None:
    if state.enemy.current_hp <= 0:
        state.outcome = "PlayerVictory"
    elif state.player.current_hp <= 0:
        state.outcome = "PlayerDefeat"
    else:
        state.outcome = "InProgress"


def play_card(state: CombatState, card: dict[str, Any]) -> tuple[bool, list[str]]:
    reasons: list[str] = []
    if state.outcome != "InProgress":
        return False, ["Combat is already finished."]
    if card["cardId"] not in state.hand:
        return False, [f"{card['cardId']} is not in hand."]
    if "Combat" not in card["playablePhases"]:
        reasons.append("Card is not playable in combat.")
    if card["cost"]["energy"] > state.energy:
        reasons.append("Not enough energy.")
    if reasons:
        return False, reasons

    state.energy -= card["cost"]["energy"]
    log = [f"Played {card['cardId']}."]

    for effect in card["effects"]:
        etype = effect["type"]
        if etype == "Damage":
            applied = apply_incoming_damage(state.enemy, effect["amount"])
            log.append(f"Damage {applied}.")
        elif etype == "GainBlock":
            state.player.block += effect["amount"]
            log.append(f"Block {effect['amount']}.")
        elif etype == "Heal":
            before = state.player.current_hp
            state.player.current_hp = min(state.player.max_hp, state.player.current_hp + effect["amount"])
            log.append(f"Heal {state.player.current_hp - before}.")
        elif etype == "DrawCards":
            log.extend(draw_cards(state, effect["amount"]))
        elif etype == "ClaimReward":
            state.claimed_reward_card_ids.append(card["cardId"])
            log.append("Reward claim reserved.")

    state.hand.remove(card["cardId"])
    state.discard_pile.append(card["cardId"])
    update_outcome(state)
    return True, log


def enemy_turn(state: CombatState, enemy_attack_damage: int, draw_count: int) -> tuple[bool, list[str]]:
    if state.outcome != "InProgress":
        return False, ["Combat is already finished."]

    applied = apply_incoming_damage(state.player, max(0, enemy_attack_damage))
    state.player.block = 0
    state.enemy.block = 0
    state.turn_number += 1
    state.energy = 3
    log = [f"Enemy damage {applied}."]
    log.extend(draw_cards(state, draw_count))
    update_outcome(state)
    return True, log


def assert_equal(actual: Any, expected: Any, label: str) -> None:
    if actual != expected:
        raise AssertionError(f"{label}: expected {expected!r}, got {actual!r}")


def main() -> int:
    cards = load_cards(Path("data/cards/cards.mvp.json"))

    state = create_combat(
        ["act_strike_001", "act_guard_001", "act_patch_heal_001"],
        enemy_hp=10,
        starting_energy=3,
    )
    draw_cards(state, 2)
    assert_equal(state.hand, ["act_strike_001", "act_guard_001"], "initial hand")

    ok, detail = play_card(state, cards["act_strike_001"])
    assert_equal(ok, True, f"strike play failed: {detail}")
    assert_equal(state.enemy.current_hp, 4, "enemy hp after strike")
    assert_equal(state.energy, 2, "energy after strike")

    ok, detail = play_card(state, cards["act_guard_001"])
    assert_equal(ok, True, f"guard play failed: {detail}")
    assert_equal(state.player.block, 5, "player block after guard")
    assert_equal(state.energy, 1, "energy after guard")

    ok, detail = enemy_turn(state, enemy_attack_damage=7, draw_count=1)
    assert_equal(ok, True, f"enemy turn failed: {detail}")
    assert_equal(state.player.current_hp, 28, "player hp after blocked attack")
    assert_equal(state.player.block, 0, "block reset after enemy turn")
    assert_equal(state.turn_number, 2, "turn number")
    assert_equal(state.energy, 3, "energy reset")
    assert_equal(state.hand, ["act_patch_heal_001"], "hand after enemy draw")

    victory = create_combat(["act_strike_001"], enemy_hp=5, starting_energy=3)
    draw_cards(victory, 1)
    ok, detail = play_card(victory, cards["act_strike_001"])
    assert_equal(ok, True, f"victory strike failed: {detail}")
    assert_equal(victory.outcome, "PlayerVictory", "victory outcome")

    print("OK: combat smoke tests passed")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as exc:
        print(str(exc), file=sys.stderr)
        raise SystemExit(1)

