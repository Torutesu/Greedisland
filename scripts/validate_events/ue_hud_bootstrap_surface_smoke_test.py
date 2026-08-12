#!/usr/bin/env python3
"""Validate that the debug HUD exposes bootstrap diagnostics hooks."""

from __future__ import annotations

import sys
from pathlib import Path


def assert_true(value: bool, label: str, detail: object = None) -> None:
    if not value:
        raise AssertionError(f"{label} failed: {detail}")


def main() -> int:
    repo_root = Path(__file__).resolve().parents[2]
    widget_header = repo_root / "UnrealProject" / "Source" / "Greeisland" / "UI" / "GreeislandDebugHudWidget.h"
    widget_cpp = repo_root / "UnrealProject" / "Source" / "Greeisland" / "UI" / "GreeislandDebugHudWidget.cpp"
    bootstrap_header = repo_root / "UnrealProject" / "Source" / "Greeisland" / "Actors" / "GreeislandBootstrapActor.h"
    bringup_sheet = repo_root / "docs" / "07_ue_bringup_sheet.md"
    text = widget_header.read_text(encoding="utf-8")
    widget_cpp_text = widget_cpp.read_text(encoding="utf-8")
    bootstrap_text = bootstrap_header.read_text(encoding="utf-8")

    assert_true("BootstrapSessionFromActor" in text, "widget exposes bootstrap action")
    assert_true("GetLastBootstrapDiagnostics" in text, "widget exposes bootstrap diagnostics getter")
    assert_true("LastBootstrapDiagnostics" in text, "widget stores bootstrap diagnostics")
    assert_true("FGreeislandHudChecklistItem" in text, "widget exposes HUD checklist struct")
    assert_true("GetRecommendedHudChecklist" in text, "widget exposes HUD checklist getter")
    assert_true("RecommendedHudChecklist" in text, "widget stores HUD checklist")
    assert_true("FGreeislandHudPanelDefinition" in text, "widget exposes HUD panel struct")
    assert_true("GetRecommendedHudPanels" in text, "widget exposes HUD panel getter")
    assert_true("RecommendedHudPanels" in text, "widget stores HUD panel definitions")
    assert_true("FGreeislandHudActionDefinition" in text, "widget exposes HUD action definition struct")
    assert_true("FGreeislandHudActionState" in text, "widget exposes HUD action state struct")
    assert_true("FGreeislandHudActionButtonViewData" in text, "widget exposes HUD action button viewdata struct")
    assert_true("FGreeislandCurrentObjectiveActionViewData" in text, "widget exposes current objective action struct")
    assert_true("FGreeislandVerificationCheckItem" in text, "widget exposes verification check struct")
    assert_true("FGreeislandSessionStatusRow" in text, "widget exposes session status row struct")
    assert_true("GetRecommendedHudActions" in text, "widget exposes HUD action definition getter")
    assert_true("GetHudActionStates" in text, "widget exposes HUD action state getter")
    assert_true("GetHudActionButtons" in text, "widget exposes HUD action button getter")
    assert_true("GetCurrentObjectiveAction" in text, "widget exposes current objective action getter")
    assert_true("GetVerificationChecks" in text, "widget exposes verification checks getter")
    assert_true("GetSessionStatusRows" in text, "widget exposes session status row getter")
    assert_true("ExecuteHudActionById" in text, "widget exposes action executor")
    assert_true("RecommendedHudActions" in text, "widget stores HUD action definitions")
    assert_true("HudActionStates" in text, "widget stores HUD action states")
    assert_true("HudActionButtons" in text, "widget stores HUD action buttons")
    assert_true("CurrentObjectiveAction" in text, "widget stores current objective action")
    assert_true("VerificationChecks" in text, "widget stores verification checks")
    assert_true("SessionStatusRows" in text, "widget stores session status rows")
    assert_true("bAutoRefreshPresentation" in text, "widget exposes auto refresh toggle")
    assert_true("AutoRefreshIntervalSeconds" in text, "widget exposes auto refresh interval")
    assert_true("FGreeislandWalkthroughStep" in text, "widget exposes walkthrough step struct")
    assert_true("GetRecommendedWalkthrough" in text, "widget exposes walkthrough getter")
    assert_true("RecommendedWalkthrough" in text, "widget stores walkthrough steps")
    assert_true("FGreeislandEventActorStatusViewData" in text, "widget exposes event actor status struct")
    assert_true("GetEventActorStatusViewData" in text, "widget exposes event actor status getter")
    assert_true("EventActorStatusViewData" in text, "widget stores event actor status data")
    assert_true("FGreeislandWalkthroughStepState" in text, "widget exposes walkthrough progress struct")
    assert_true("GetWalkthroughProgress" in text, "widget exposes walkthrough progress getter")
    assert_true("WalkthroughProgress" in text, "widget stores walkthrough progress")
    assert_true("BuildWalkthroughProgress" in text, "widget declares walkthrough progress builder")
    assert_true("BuildWalkthroughProgress()" in widget_cpp_text, "widget implements walkthrough progress builder")
    assert_true("BuildRecommendedHudPanels" in text, "widget declares HUD panel builder")
    assert_true("BuildRecommendedHudPanels()" in widget_cpp_text, "widget implements HUD panel builder")
    assert_true("BuildRecommendedHudActions" in text, "widget declares HUD action builder")
    assert_true("BuildRecommendedHudActions()" in widget_cpp_text, "widget implements HUD action builder")
    assert_true("BuildEventActorStatusViewData" in text, "widget declares event actor status builder")
    assert_true("BuildEventActorStatusViewData()" in widget_cpp_text, "widget implements event actor status builder")
    assert_true("BuildHudActionStates" in text, "widget declares HUD action state builder")
    assert_true("BuildHudActionStates()" in widget_cpp_text, "widget implements HUD action state builder")
    assert_true("BuildHudActionButtons" in text, "widget declares HUD action button builder")
    assert_true("BuildHudActionButtons()" in widget_cpp_text, "widget implements HUD action button builder")
    assert_true("BuildCurrentObjectiveAction" in text, "widget declares current objective action builder")
    assert_true("BuildCurrentObjectiveAction()" in widget_cpp_text, "widget implements current objective action builder")
    assert_true("BuildHudActionDetail" in text, "widget declares HUD action detail builder")
    assert_true("BuildHudActionDetail" in widget_cpp_text, "widget implements HUD action detail builder")
    assert_true("FindHudActionStateById" in text, "widget declares action state lookup helper")
    assert_true("FindHudActionStateById" in widget_cpp_text, "widget implements action state lookup helper")
    assert_true("BuildVerificationChecks" in text, "widget declares verification checks builder")
    assert_true("BuildVerificationChecks()" in widget_cpp_text, "widget implements verification checks builder")
    assert_true("BuildSessionStatusRows" in text, "widget declares session status row builder")
    assert_true("BuildSessionStatusRows()" in widget_cpp_text, "widget implements session status row builder")
    assert_true("NativeTick" in text, "widget declares native tick")
    assert_true("NativeTick(" in widget_cpp_text, "widget implements native tick")
    assert_true("UScrollBox" in widget_cpp_text, "native HUD wraps long MVP content in a scroll box")
    assert_true("SetAlwaysShowScrollbar" in widget_cpp_text, "native HUD keeps the content scrollbar visible")
    assert_true("BuildEventActorStatusSummary" in text, "widget declares event actor status summary builder")
    assert_true("BuildEventActorStatusSummary" in widget_cpp_text, "widget implements event actor status summary builder")
    assert_true("StatusLabel" in widget_cpp_text, "walkthrough progress sets status labels")
    assert_true("bCurrentFocus" in widget_cpp_text, "walkthrough progress sets focus state")
    assert_true("FGreeislandBlueprintAssetChecklistItem" in text, "widget exposes blueprint asset checklist struct")
    assert_true("GetRecommendedBlueprintAssets" in text, "widget exposes blueprint asset checklist getter")
    assert_true("RecommendedBlueprintAssets" in text, "widget stores blueprint asset checklist")
    assert_true("MissingEventActorIds" in bootstrap_text, "bootstrap diagnostics expose missing events")
    assert_true("DuplicateEventActorIds" in bootstrap_text, "bootstrap diagnostics expose duplicate events")
    assert_true("UnexpectedEventActorIds" in bootstrap_text, "bootstrap diagnostics expose unexpected events")
    assert_true("FGreeislandExpectedEventPlacement" in bootstrap_text, "bootstrap diagnostics expose placement checklist struct")
    assert_true("ExpectedEventPlacements" in bootstrap_text, "bootstrap diagnostics expose placement checklist array")
    assert_true("FGreeislandExpectedEventRouteNode" in bootstrap_text, "bootstrap diagnostics expose route checklist struct")
    assert_true("ExpectedEventRoute" in bootstrap_text, "bootstrap diagnostics expose route checklist array")
    assert_true(bringup_sheet.exists(), "bring-up sheet exists", bringup_sheet)

    print("OK: UE HUD bootstrap surface smoke tests passed")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as exc:
        print(str(exc), file=sys.stderr)
        raise SystemExit(1)
