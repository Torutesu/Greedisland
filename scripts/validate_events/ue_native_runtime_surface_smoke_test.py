#!/usr/bin/env python3
"""Validate the Blueprint-free native MVP runtime surface."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]


def assert_true(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def main() -> int:
    game_mode = (ROOT / "UnrealProject/Source/Greeisland/GameFramework/GreeislandDebugGameMode.cpp").read_text(encoding="utf-8")
    game_mode_header = (ROOT / "UnrealProject/Source/Greeisland/GameFramework/GreeislandDebugGameMode.h").read_text(encoding="utf-8")
    event_actor = (ROOT / "UnrealProject/Source/Greeisland/Actors/GreeislandEventActor.cpp").read_text(encoding="utf-8")
    hud = (ROOT / "UnrealProject/Source/Greeisland/UI/GreeislandDebugHud.cpp").read_text(encoding="utf-8")
    widget_header = (ROOT / "UnrealProject/Source/Greeisland/UI/GreeislandDebugHudWidget.h").read_text(encoding="utf-8")
    widget = (ROOT / "UnrealProject/Source/Greeisland/UI/GreeislandDebugHudWidget.cpp").read_text(encoding="utf-8")
    controller = (ROOT / "UnrealProject/Source/Greeisland/GameFramework/GreeislandDebugPlayerController.cpp").read_text(encoding="utf-8")
    input_config = (ROOT / "UnrealProject/Config/DefaultInput.ini").read_text(encoding="utf-8")
    engine_config = (ROOT / "UnrealProject/Config/DefaultEngine.ini").read_text(encoding="utf-8")

    for event_id in (
        "event_wake_cache_001",
        "event_contract_broker_001",
        "event_silent_shrine_001",
        "event_ridge_scout_001",
        "event_proxy_gate_001",
    ):
        assert_true(event_id in game_mode, f"native zone includes {event_id}")

    assert_true("BeginPlay() override" in game_mode_header, "game mode owns runtime bootstrap")
    assert_true("BuildMvpZoneIfNeeded" in game_mode, "game mode builds MVP zone")
    assert_true("SpawnActor<AGreeislandBootstrapActor>" in game_mode, "game mode spawns bootstrap actor")
    assert_true("SpawnActor<AGreeislandEventActor>" in game_mode, "game mode spawns event actors")
    assert_true("/Engine/BasicShapes/Cube.Cube" in game_mode, "game mode creates a collision floor")
    assert_true("EditorStartupMap=/Engine/Maps/Entry" in engine_config, "project has an editor startup map")
    assert_true("GameDefaultMap=/Engine/Maps/Entry" in engine_config, "project has a game default map")
    assert_true("SetEventId" in event_actor, "event actor supports runtime id binding")
    assert_true("GetOverlappingActors" in event_actor, "runtime event actor can trigger on contact")
    assert_true("MarkerMesh" in event_actor and "Cylinder.Cylinder" in event_actor, "runtime event actor has a visible marker")
    assert_true("MarkerLabel" in event_actor and "SetText(FText::FromName(EventId))" in event_actor, "runtime event actor labels its marker")
    assert_true("#if WITH_EDITOR" in event_actor, "editor-only actor label is guarded")
    assert_true("#if WITH_EDITOR" in game_mode, "editor-only ground label is guarded")

    assert_true("WidgetClass = UGreeislandDebugHudWidget::StaticClass()" in hud, "HUD has native widget fallback")
    assert_true("UCLASS(Blueprintable)" in widget_header and "UCLASS(Abstract" not in widget_header, "native widget can instantiate")
    for symbol in ("BuildNativeLayout", "NativeStatusText", "NativeCardsText", "NativeEventsText", "NativeAiText", "NativeActionText"):
        assert_true(symbol in widget_header or symbol in widget, f"native HUD exposes {symbol}")
    assert_true("LAST ACTION" in widget, "native HUD exposes latest action result")
    assert_true("BuildFallbackAiResponseForActiveEvent" in widget, "native HUD surfaces deterministic AI response")
    for symbol in ("PlayHandCardAtSlot", "RunEnemyTurnHotkey", "BuildFallbackAiHotkey", "SaveSessionHotkey"):
        assert_true(symbol in controller, f"native controller exposes {symbol}")
    for action_name in ("PlayHandCard1", "PlayHandCard5", "RunEnemyTurn", "BuildFallbackAi", "ApplyAi"):
        assert_true(f'ActionName=\"{action_name}\"' in input_config, f"input maps {action_name}")

    print("OK: UE native runtime surface smoke tests passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
