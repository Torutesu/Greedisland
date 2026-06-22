#!/usr/bin/env python3
"""Summarize current MVP completion evidence and remaining runtime gaps."""

from __future__ import annotations

import argparse
from dataclasses import dataclass
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[1]


@dataclass(frozen=True)
class AuditItem:
    requirement: str
    status: str
    evidence: tuple[str, ...]
    note: str


def collect_items() -> list[AuditItem]:
    return [
        AuditItem(
            requirement="エディタまたはローカルビルドで1ゾーンを起動できる",
            status="needs_runtime_evidence",
            evidence=(
                "scripts/ue_build_helper.py",
                "scripts/validate_events/ue_editor_preflight_smoke_test.py",
                "docs/07_ue_bringup_sheet.md",
            ),
            note="起動補助と事前チェックはあるが、この環境では UE5.8 実機起動証跡がまだない。",
        ),
        AuditItem(
            requirement="探索地点に接触するとイベントが発火する",
            status="code_ready_needs_runtime",
            evidence=(
                "UnrealProject/Source/Greeisland/Actors/GreeislandEventActor.cpp",
                "docs/06_ue_debug_hud_setup.md",
                "scripts/validate_events/session_flow_smoke_test.py",
            ),
            note="EventActor とセッション進行は実装済み。接触発火の最終証跡は UE 実機確認待ち。",
        ),
        AuditItem(
            requirement="カードを獲得し、カード一覧UIで確認できる",
            status="code_ready_needs_runtime",
            evidence=(
                "UnrealProject/Source/Greeisland/Runtime/GreeislandGameSubsystem.cpp",
                "UnrealProject/Source/Greeisland/UI/GreeislandDebugHudWidget.cpp",
                "scripts/validate_events/ue_runtime_viewdata_surface_smoke_test.py",
            ),
            note="取得と UI 向け viewdata は実装済み。UMG 表示の実機証跡がまだない。",
        ),
        AuditItem(
            requirement="バトルでカードを使用できる",
            status="proven_in_logic_needs_runtime",
            evidence=(
                "UnrealProject/Source/Greeisland/Combat/CombatEngine.cpp",
                "scripts/validate_cards/combat_smoke_test.py",
                "scripts/validate_events/combat_resolution_smoke_test.py",
            ),
            note="ロジックと報酬解決はスモーク済み。UE 画面上での操作証跡は未取得。",
        ),
        AuditItem(
            requirement="制約を満たさないカードは使用できない",
            status="proven",
            evidence=(
                "UnrealProject/Source/Greeisland/Rules/RuleResolver.cpp",
                "scripts/validate_cards/playability_reason_smoke_test.py",
                "scripts/validate_cards/rule_smoke_test.py",
            ),
            note="制約拒否と理由表示のロジックは検証済み。",
        ),
        AuditItem(
            requirement="ルール変更カードによって、少なくとも1つの通常ルールが変化する",
            status="proven",
            evidence=(
                "UnrealProject/Source/Greeisland/Rules/RuleResolver.cpp",
                "scripts/validate_cards/rule_smoke_test.py",
                "data/cards/cards.mvp.json",
            ),
            note="MVP ルールハック例はスモークで通っている。",
        ),
        AuditItem(
            requirement="AI GMの応答がUIに表示される",
            status="code_ready_needs_runtime",
            evidence=(
                "UnrealProject/Source/Greeisland/UI/GreeislandDebugHudWidget.cpp",
                "scripts/validate_events/ai_fallback_smoke_test.py",
                "docs/06_ue_debug_hud_setup.md",
            ),
            note="HUD の request/fallback/apply 導線は実装済み。UMG 表示の実機証跡は未取得。",
        ),
        AuditItem(
            requirement="AI GMの応答が不正な報酬・不正なカードID・未許可イベントを返した場合、ゲーム側が拒否できる",
            status="proven",
            evidence=(
                "UnrealProject/Source/Greeisland/AiGm/AiGmValidator.cpp",
                "scripts/validate_cards/ai_gm_smoke_test.py",
                "scripts/validate_events/ai_session_apply_smoke_test.py",
            ),
            note="報酬 ID、IP 類似語、未許可クエスト提案は検証と拒否が通っている。",
        ),
        AuditItem(
            requirement="ゲーム終了後に所持カードと進行状態を復元できる",
            status="proven_in_logic_needs_runtime",
            evidence=(
                "UnrealProject/Source/Greeisland/Save/SaveGameMapper.cpp",
                "scripts/validate_events/save_restore_smoke_test.py",
                "scripts/validate_events/mvp_full_run_smoke_test.py",
            ),
            note="保存復元ロジックは検証済み。UE 実機での save slot 実証は未取得。",
        ),
    ]


def render_markdown(items: list[AuditItem]) -> str:
    proven = sum(item.status == "proven" for item in items)
    total = len(items)
    lines = [
        "# MVP Completion Audit",
        "",
        f"- RepoRoot: `{REPO_ROOT}`",
        f"- Proven requirements: {proven}/{total}",
        "",
        "| Requirement | Status | Evidence | Note |",
        "| --- | --- | --- | --- |",
    ]

    for item in items:
        evidence = "<br>".join(f"`{path}`" for path in item.evidence)
        lines.append(f"| {item.requirement} | `{item.status}` | {evidence} | {item.note} |")

    lines.extend(
        [
            "",
            "## Remaining Runtime Evidence",
            "",
            "- UE5.8 editor launch",
            "- UnrealHeaderTool / C++ compile in editor toolchain",
            "- UMG rendering of debug HUD",
            "- EventActor contact flow in a live level",
            "- Save/restore and AI loop on a real UE session",
        ]
    )
    return "\n".join(lines) + "\n"


def render_text(items: list[AuditItem]) -> str:
    lines = ["MVP Completion Audit", ""]
    for item in items:
        lines.append(f"- [{item.status}] {item.requirement}")
        lines.append(f"  evidence: {', '.join(item.evidence)}")
        lines.append(f"  note: {item.note}")
    lines.extend(
        [
            "",
            "Remaining runtime evidence:",
            "- UE5.8 editor launch",
            "- UnrealHeaderTool / C++ compile",
            "- UMG rendering",
            "- Live event contact flow",
            "- Live save/restore and AI loop",
        ]
    )
    return "\n".join(lines) + "\n"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--format",
        choices=("text", "markdown"),
        default="text",
        help="Output format.",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    items = collect_items()
    if args.format == "markdown":
        print(render_markdown(items), end="")
    else:
        print(render_text(items), end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
