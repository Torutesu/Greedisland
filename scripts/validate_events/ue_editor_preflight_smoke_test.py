#!/usr/bin/env python3
"""Validate Unreal project preflight assumptions before editor/runtime bring-up."""

from __future__ import annotations

import json
import os
import sys
from pathlib import Path


def assert_true(value: bool, label: str, detail: object = None) -> None:
    if not value:
        raise AssertionError(f"{label} failed: {detail}")


def find_editor_candidates() -> list[Path]:
    candidates: list[Path] = []

    env_vars = [
        "UNREAL_EDITOR",
        "UE_EDITOR",
        "UE5_ROOT",
        "UNREAL_ENGINE_ROOT",
    ]
    for env_var in env_vars:
        value = os.environ.get(env_var)
        if not value:
            continue

        path = Path(value).expanduser()
        candidates.append(path)
        candidates.append(path / "Engine" / "Binaries" / "Mac" / "UnrealEditor.app")
        candidates.append(path / "Engine" / "Binaries" / "Mac" / "UnrealEditor")

    common_paths = [
        Path("/Applications/UE_5.8/Engine/Binaries/Mac/UnrealEditor.app"),
        Path("/Applications/Epic Games/UE_5.8/Engine/Binaries/Mac/UnrealEditor.app"),
        Path("/Users/Shared/Epic Games/UE_5.8/Engine/Binaries/Mac/UnrealEditor.app"),
        Path("/Applications/UE_5.8/Engine/Binaries/Mac/UnrealEditor"),
        Path("/Applications/Epic Games/UE_5.8/Engine/Binaries/Mac/UnrealEditor"),
        Path("/Users/Shared/Epic Games/UE_5.8/Engine/Binaries/Mac/UnrealEditor"),
    ]
    candidates.extend(common_paths)

    unique: list[Path] = []
    seen: set[str] = set()
    for candidate in candidates:
        key = str(candidate)
        if key in seen:
            continue
        seen.add(key)
        unique.append(candidate)
    return unique


def read_text(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def main() -> int:
    repo_root = Path(__file__).resolve().parents[2]
    unreal_root = repo_root / "UnrealProject"

    uproject_path = unreal_root / "Greeisland.uproject"
    build_cs_path = unreal_root / "Source" / "Greeisland" / "Greeisland.Build.cs"
    game_target_path = unreal_root / "Source" / "Greeisland.Target.cs"
    editor_target_path = unreal_root / "Source" / "GreeislandEditor.Target.cs"
    default_game_path = unreal_root / "Config" / "DefaultGame.ini"
    default_engine_path = unreal_root / "Config" / "DefaultEngine.ini"
    default_input_path = unreal_root / "Config" / "DefaultInput.ini"
    card_json_path = repo_root / "data" / "cards" / "cards.mvp.json"
    event_json_path = repo_root / "data" / "events" / "events.mvp.json"

    for required_path in [
        uproject_path,
        build_cs_path,
        game_target_path,
        editor_target_path,
        default_game_path,
        default_engine_path,
        default_input_path,
        card_json_path,
        event_json_path,
    ]:
        assert_true(required_path.exists(), "required path exists", required_path)

    uproject = json.loads(uproject_path.read_text(encoding="utf-8"))
    assert_true(uproject.get("EngineAssociation") == "5.8", "uproject targets UE5.8", uproject.get("EngineAssociation"))
    assert_true(
        any(module.get("Name") == "Greeisland" for module in uproject.get("Modules", [])),
        "uproject lists Greeisland module",
        uproject.get("Modules"),
    )
    assert_true(
        any(plugin.get("Name") == "EnhancedInput" and plugin.get("Enabled") for plugin in uproject.get("Plugins", [])),
        "uproject enables EnhancedInput",
        uproject.get("Plugins"),
    )

    build_cs_text = read_text(build_cs_path)
    for required_dependency in [
        '"Core"',
        '"CoreUObject"',
        '"DeveloperSettings"',
        '"Engine"',
        '"InputCore"',
        '"Json"',
        '"JsonUtilities"',
        '"UMG"',
        '"Slate"',
        '"SlateCore"',
    ]:
        assert_true(required_dependency in build_cs_text, "build dependency declared", required_dependency)

    game_target_text = read_text(game_target_path)
    editor_target_text = read_text(editor_target_path)
    for target_text, target_type in [(game_target_text, "Game"), (editor_target_text, "Editor")]:
        assert_true(f"Type = TargetType.{target_type};" in target_text, "target type set", target_type)
        assert_true("DefaultBuildSettings = BuildSettingsVersion.V5;" in target_text, "target uses V5 build settings", target_type)
        assert_true("IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;" in target_text, "target uses 5.8 include order", target_type)
        assert_true('ExtraModuleNames.Add("Greeisland");' in target_text, "target includes module", target_type)

    default_game_text = read_text(default_game_path)
    assert_true(
        "GlobalDefaultGameMode=/Script/Greeisland.GreeislandDebugGameMode" in default_game_text,
        "default game mode points at debug game mode",
    )
    default_engine_text = read_text(default_engine_path)
    assert_true("EditorStartupMap=/Engine/Maps/Entry" in default_engine_text, "editor startup map configured")
    assert_true("GameDefaultMap=/Engine/Maps/Entry" in default_engine_text, "game default map configured")
    assert_true("CardJsonPath=../data/cards/cards.mvp.json" in default_game_text, "card json path configured")
    assert_true("EventJsonPath=../data/events/events.mvp.json" in default_game_text, "event json path configured")
    assert_true("SaveSlotName=greeisland-dev-slot" in default_game_text, "save slot configured")

    default_input_text = read_text(default_input_path)
    for required_mapping in [
        'AxisName="MoveForward",Scale=1.000000,Key=W',
        'AxisName="MoveForward",Scale=-1.000000,Key=S',
        'AxisName="MoveRight",Scale=1.000000,Key=D',
        'AxisName="MoveRight",Scale=-1.000000,Key=A',
        'AxisName="Turn",Scale=1.000000,Key=MouseX',
        'AxisName="LookUp",Scale=-1.000000,Key=MouseY',
        'ActionName="Interact",Key=E',
        'ActionName="PlayHandCard1",Key=One',
        'ActionName="PlayHandCard5",Key=Five',
        'ActionName="RunEnemyTurn",Key=SpaceBar',
    ]:
        assert_true(required_mapping in default_input_text, "input mapping configured", required_mapping)

    editor_candidates = find_editor_candidates()
    found_editor = next((candidate for candidate in editor_candidates if candidate.exists()), None)

    if found_editor:
        print(f"NOTE: UnrealEditor candidate found at {found_editor}")
    else:
        print("NOTE: UnrealEditor candidate not found in common paths; project preflight still passed.")

    print("OK: UE editor preflight smoke tests passed")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as exc:
        print(str(exc), file=sys.stderr)
        raise SystemExit(1)
