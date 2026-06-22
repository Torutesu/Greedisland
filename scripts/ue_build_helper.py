#!/usr/bin/env python3
"""Prepare or run Unreal build/editor commands for the local Greeisland project."""

from __future__ import annotations

import argparse
import os
import subprocess
import sys
from dataclasses import dataclass
from datetime import datetime
from pathlib import Path


@dataclass(frozen=True)
class UnrealTooling:
    engine_root: Path | None
    editor_binary: Path | None
    projectfiles_script: Path | None
    build_script: Path | None
    uht_binary: Path | None


@dataclass(frozen=True)
class ReadinessItem:
    label: str
    ok: bool
    detail: str


def repo_root() -> Path:
    return Path(__file__).resolve().parents[1]


def uproject_path() -> Path:
    return repo_root() / "UnrealProject" / "Greeisland.uproject"


def common_engine_roots() -> list[Path]:
    env_candidates = []
    for env_var in ("UE5_ROOT", "UNREAL_ENGINE_ROOT", "UE_EDITOR", "UNREAL_EDITOR"):
        value = os.environ.get(env_var)
        if value:
            env_candidates.append(Path(value).expanduser())

    static_candidates = [
        Path("/Applications/UE_5.8"),
        Path("/Applications/Epic Games/UE_5.8"),
        Path("/Users/Shared/Epic Games/UE_5.8"),
    ]

    candidates: list[Path] = []
    seen: set[str] = set()
    for candidate in env_candidates + static_candidates:
        normalized = normalize_engine_root(candidate)
        key = str(normalized)
        if key in seen:
            continue
        seen.add(key)
        candidates.append(normalized)
    return candidates


def normalize_engine_root(candidate: Path) -> Path:
    candidate_str = str(candidate)
    if candidate_str.endswith("UnrealEditor.app"):
        return candidate.parents[4]
    if candidate_str.endswith("UnrealEditor"):
        return candidate.parents[3]
    if candidate.name == "Engine":
        return candidate.parent
    return candidate


def detect_tooling() -> UnrealTooling:
    for engine_root in common_engine_roots():
        editor_app = engine_root / "Engine" / "Binaries" / "Mac" / "UnrealEditor.app"
        editor_bin = engine_root / "Engine" / "Binaries" / "Mac" / "UnrealEditor"
        build_script = engine_root / "Engine" / "Build" / "BatchFiles" / "Mac" / "Build.sh"
        projectfiles_script = engine_root / "Engine" / "Build" / "BatchFiles" / "Mac" / "GenerateProjectFiles.sh"
        uht_binary = engine_root / "Engine" / "Binaries" / "Mac" / "UnrealHeaderTool"

        editor_candidate = editor_bin if editor_bin.exists() else editor_app
        if any(path.exists() for path in (editor_app, editor_bin, build_script, projectfiles_script, uht_binary)):
            return UnrealTooling(
                engine_root=engine_root,
                editor_binary=editor_candidate if editor_candidate.exists() else None,
                projectfiles_script=projectfiles_script if projectfiles_script.exists() else None,
                build_script=build_script if build_script.exists() else None,
                uht_binary=uht_binary if uht_binary.exists() else None,
            )

    return UnrealTooling(
        engine_root=None,
        editor_binary=None,
        projectfiles_script=None,
        build_script=None,
        uht_binary=None,
    )


def build_commands(tooling: UnrealTooling) -> dict[str, list[str] | None]:
    project = str(uproject_path())
    commands: dict[str, list[str] | None] = {
        "projectfiles": None,
        "build_editor": None,
        "build_game": None,
        "open_editor": None,
    }

    if tooling.projectfiles_script:
        commands["projectfiles"] = [
            str(tooling.projectfiles_script),
            f"-project={project}",
            "-game",
            "-engine",
        ]

    if tooling.build_script:
        commands["build_editor"] = [
            str(tooling.build_script),
            "GreeislandEditor",
            "Mac",
            "Development",
            f"-Project={project}",
            "-Progress",
        ]
        commands["build_game"] = [
            str(tooling.build_script),
            "Greeisland",
            "Mac",
            "Development",
            f"-Project={project}",
            "-Progress",
        ]

    if tooling.editor_binary:
        commands["open_editor"] = [str(tooling.editor_binary), project]

    return commands


def build_sequence(commands: dict[str, list[str] | None], action: str) -> list[list[str]] | None:
    if action == "bootstrap_editor":
        ordered = [commands["projectfiles"], commands["build_editor"], commands["open_editor"]]
        if not all(ordered):
            return None
        return [command for command in ordered if command is not None]

    command = commands.get(action)
    if command is None:
        return None
    return [command]


def collect_readiness(tooling: UnrealTooling) -> list[ReadinessItem]:
    items: list[ReadinessItem] = []
    project = uproject_path()
    items.append(
        ReadinessItem(
            label="uproject exists",
            ok=project.exists(),
            detail=str(project),
        )
    )

    required_paths = [
        repo_root() / "UnrealProject" / "Source" / "Greeisland" / "Greeisland.Build.cs",
        repo_root() / "UnrealProject" / "Source" / "Greeisland.Target.cs",
        repo_root() / "UnrealProject" / "Source" / "GreeislandEditor.Target.cs",
        repo_root() / "UnrealProject" / "Config" / "DefaultGame.ini",
        repo_root() / "UnrealProject" / "Config" / "DefaultInput.ini",
        repo_root() / "data" / "cards" / "cards.mvp.json",
        repo_root() / "data" / "events" / "events.mvp.json",
    ]
    for required_path in required_paths:
        items.append(
            ReadinessItem(
                label=f"path exists: {required_path.name}",
                ok=required_path.exists(),
                detail=str(required_path),
            )
        )

    items.extend(
        [
            ReadinessItem(
                label="engine root detected",
                ok=tooling.engine_root is not None,
                detail=str(tooling.engine_root) if tooling.engine_root else "Set UE5_ROOT or install UE5.8",
            ),
            ReadinessItem(
                label="UnrealEditor detected",
                ok=tooling.editor_binary is not None,
                detail=str(tooling.editor_binary) if tooling.editor_binary else "UnrealEditor binary not found",
            ),
            ReadinessItem(
                label="GenerateProjectFiles available",
                ok=tooling.projectfiles_script is not None,
                detail=str(tooling.projectfiles_script) if tooling.projectfiles_script else "GenerateProjectFiles.sh not found",
            ),
            ReadinessItem(
                label="Build.sh available",
                ok=tooling.build_script is not None,
                detail=str(tooling.build_script) if tooling.build_script else "Build.sh not found",
            ),
            ReadinessItem(
                label="UnrealHeaderTool available",
                ok=tooling.uht_binary is not None,
                detail=str(tooling.uht_binary) if tooling.uht_binary else "UnrealHeaderTool not found",
            ),
        ]
    )

    return items


def print_plan(tooling: UnrealTooling) -> int:
    commands = build_commands(tooling)
    print(f"Repo: {repo_root()}")
    print(f"Project: {uproject_path()}")
    print(f"Engine root: {tooling.engine_root if tooling.engine_root else 'NOT FOUND'}")
    print(f"UnrealEditor: {tooling.editor_binary if tooling.editor_binary else 'NOT FOUND'}")
    print(f"Build.sh: {tooling.build_script if tooling.build_script else 'NOT FOUND'}")
    print(f"GenerateProjectFiles.sh: {tooling.projectfiles_script if tooling.projectfiles_script else 'NOT FOUND'}")
    print(f"UnrealHeaderTool: {tooling.uht_binary if tooling.uht_binary else 'NOT FOUND'}")
    print("")
    print("Available commands:")
    for key in ("projectfiles", "build_editor", "build_game", "open_editor"):
        command = commands[key]
        if command:
            print(f"- {key}: {' '.join(command)}")
        else:
            print(f"- {key}: unavailable")

    return 0


def print_doctor(tooling: UnrealTooling) -> int:
    print_plan(tooling)
    print("")
    print("Readiness:")
    for item in collect_readiness(tooling):
        status = "OK" if item.ok else "MISSING"
        print(f"- [{status}] {item.label}: {item.detail}")

    print("")
    print("Suggested next steps:")
    commands = build_commands(tooling)
    if tooling.engine_root is None:
        print("- Install UE5.8 or set UE5_ROOT / UNREAL_ENGINE_ROOT, then rerun this doctor.")
    else:
        bootstrap_sequence = build_sequence(commands, "bootstrap_editor")
        if bootstrap_sequence:
            print("- Bootstrap editor loop:")
            for command in bootstrap_sequence:
                print(f"  {' '.join(command)}")
        if commands["projectfiles"]:
            print(f"- Generate project files: {' '.join(commands['projectfiles'])}")
        if commands["build_editor"]:
            print(f"- Build editor target: {' '.join(commands['build_editor'])}")
        if commands["open_editor"]:
            print(f"- Open editor: {' '.join(commands['open_editor'])}")

    return 0


def build_runtime_checklist_lines(tooling: UnrealTooling) -> list[str]:
    commands = build_commands(tooling)
    lines: list[str] = [
        "UE Runtime Verification Checklist",
        "",
        "1. Run preflight:",
        "   python3 scripts/validate_events/ue_editor_preflight_smoke_test.py",
        "2. Run doctor:",
        "   python3 scripts/ue_build_helper.py --action doctor",
    ]

    bootstrap_sequence = build_sequence(commands, "bootstrap_editor")
    if bootstrap_sequence:
        lines.append("3. Bootstrap editor loop:")
        for command in bootstrap_sequence:
            lines.append(f"   {' '.join(command)}")
    else:
        lines.append("3. Bootstrap editor loop:")
        lines.append("   unavailable until UE5.8 / UE5_ROOT is available")

    lines.extend(
        [
            "4. In Unreal Editor, confirm World Settings -> GameMode Override uses BP_GreeislandDebugGameMode or the C++ debug game mode.",
            "5. Confirm BP_GreeislandBootstrapActor is placed exactly once and EventActors cover wake_cache / contract_broker / silent_shrine / ridge_scout / proxy_gate.",
            "6. Press Play and confirm the debug HUD appears.",
            "7. Verify SessionStatusRows and CurrentObjectiveAction update after bootstrap.",
            "8. Walk the Bring-up Sheet flow: Wake Cache -> Contract Broker -> Silent Shrine -> Ridge Scout Battle -> Proxy Gate -> Save/Restore.",
            "9. Capture evidence for missing items: editor launch, UHT/build output, HUD rendering, card load, walkthrough completion.",
        ]
    )
    return lines


def print_runtime_checklist(tooling: UnrealTooling) -> int:
    for line in build_runtime_checklist_lines(tooling):
        print(line)
    return 0


def build_report_template_lines(tooling: UnrealTooling) -> list[str]:
    commands = build_commands(tooling)
    generated_at = datetime.now().astimezone().isoformat(timespec="seconds")
    lines = [
        "# UE Runtime Verification Report",
        "",
        f"- GeneratedAt: {generated_at}",
        f"- RepoRoot: {repo_root()}",
        f"- Project: {uproject_path()}",
        f"- EngineRoot: {tooling.engine_root if tooling.engine_root else 'NOT FOUND'}",
        f"- UnrealEditor: {tooling.editor_binary if tooling.editor_binary else 'NOT FOUND'}",
        f"- GenerateProjectFiles: {tooling.projectfiles_script if tooling.projectfiles_script else 'NOT FOUND'}",
        f"- BuildScript: {tooling.build_script if tooling.build_script else 'NOT FOUND'}",
        f"- UnrealHeaderTool: {tooling.uht_binary if tooling.uht_binary else 'NOT FOUND'}",
        "",
        "## Commands",
        "",
        f"- Preflight: `python3 scripts/validate_events/ue_editor_preflight_smoke_test.py`",
        f"- Doctor: `python3 scripts/ue_build_helper.py --action doctor`",
        f"- Checklist: `python3 scripts/ue_build_helper.py --action checklist`",
    ]

    for label, command_key in (
        ("ProjectFiles", "projectfiles"),
        ("BuildEditor", "build_editor"),
        ("BuildGame", "build_game"),
        ("OpenEditor", "open_editor"),
    ):
        command = commands[command_key]
        rendered = " ".join(command) if command else "UNAVAILABLE"
        lines.append(f"- {label}: `{rendered}`")

    lines.extend(
        [
            "",
            "## Command Results",
            "",
            "| Step | Status | Notes |",
            "| --- | --- | --- |",
            "| Preflight | TODO | |",
            "| Doctor | TODO | |",
            "| GenerateProjectFiles | TODO | |",
            "| BuildEditor | TODO | |",
            "| BuildGame | TODO | |",
            "| OpenEditor | TODO | |",
            "",
            "## Editor Checks",
            "",
            "| Check | Status | Evidence | Notes |",
            "| --- | --- | --- | --- |",
            "| World Settings uses debug GameMode | TODO | | |",
            "| BootstrapActor placed exactly once | TODO | | |",
            "| EventActors cover wake_cache / contract_broker / silent_shrine / ridge_scout / proxy_gate | TODO | | |",
            "| Debug HUD appears on Play | TODO | | |",
            "| SessionStatusRows updates after bootstrap | TODO | | |",
            "| CurrentObjectiveAction updates after bootstrap | TODO | | |",
            "| Card data loads from cards.mvp.json | TODO | | |",
            "| Event data loads from events.mvp.json | TODO | | |",
            "",
            "## Walkthrough Checks",
            "",
            "| Step | Status | Evidence | Notes |",
            "| --- | --- | --- | --- |",
            "| Wake Cache | TODO | | |",
            "| Contract Broker | TODO | | |",
            "| Silent Shrine | TODO | | |",
            "| Ridge Scout Battle | TODO | | |",
            "| Proxy Gate | TODO | | |",
            "| Save Session | TODO | | |",
            "| Restore Session | TODO | | |",
            "",
            "## Evidence Captured",
            "",
            "- Editor launch log:",
            "- UHT / build output:",
            "- HUD screenshot:",
            "- Walkthrough completion screenshot or note:",
            "",
            "## Open Issues",
            "",
            "- ",
            "",
            "## Next Actions",
            "",
            "- ",
        ]
    )

    return lines


def print_or_write_report_template(tooling: UnrealTooling, output_path: str | None) -> int:
    lines = build_report_template_lines(tooling)
    content = "\n".join(lines) + "\n"
    if output_path:
        destination = Path(output_path).expanduser()
        if not destination.is_absolute():
            destination = repo_root() / destination
        destination.parent.mkdir(parents=True, exist_ok=True)
        destination.write_text(content, encoding="utf-8")
        print(f"Wrote report template: {destination}")
        return 0

    print(content, end="")
    return 0


def run_named_command(tooling: UnrealTooling, action: str) -> int:
    commands = build_commands(tooling)
    sequence = build_sequence(commands, action)
    if not sequence:
        print(f"{action} is unavailable because Unreal tooling was not found.", file=sys.stderr)
        return 2

    for command in sequence:
        print(f"Running: {' '.join(command)}")
        completed = subprocess.run(command, cwd=repo_root())
        if completed.returncode != 0:
            return completed.returncode

    return 0


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--action",
        choices=("doctor", "print-plan", "checklist", "report-template", "projectfiles", "build-editor", "build-game", "open-editor", "bootstrap-editor"),
        default="print-plan",
        help="Action to run. Defaults to printing the resolved Unreal commands.",
    )
    parser.add_argument(
        "--output",
        help="Optional file path for actions that can write artifacts, such as report-template.",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    tooling = detect_tooling()

    if args.action == "doctor":
        return print_doctor(tooling)
    if args.action == "checklist":
        return print_runtime_checklist(tooling)
    if args.action == "report-template":
        return print_or_write_report_template(tooling, args.output)
    if args.action == "print-plan":
        return print_plan(tooling)
    if args.action == "projectfiles":
        return run_named_command(tooling, "projectfiles")
    if args.action == "build-editor":
        return run_named_command(tooling, "build_editor")
    if args.action == "build-game":
        return run_named_command(tooling, "build_game")
    if args.action == "open-editor":
        return run_named_command(tooling, "open_editor")
    if args.action == "bootstrap-editor":
        return run_named_command(tooling, "bootstrap_editor")

    print(f"Unknown action: {args.action}", file=sys.stderr)
    return 2


if __name__ == "__main__":
    raise SystemExit(main())
