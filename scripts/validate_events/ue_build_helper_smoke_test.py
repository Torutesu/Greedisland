#!/usr/bin/env python3
"""Validate Unreal build helper command resolution."""

from __future__ import annotations

import importlib.util
import sys
import tempfile
from pathlib import Path


def load_helper_module():
    repo_root = Path(__file__).resolve().parents[2]
    module_path = repo_root / "scripts" / "ue_build_helper.py"
    spec = importlib.util.spec_from_file_location("ue_build_helper", module_path)
    module = importlib.util.module_from_spec(spec)
    assert spec is not None and spec.loader is not None
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


def assert_equal(actual: object, expected: object, label: str) -> None:
    if actual != expected:
        raise AssertionError(f"{label}: expected {expected!r}, got {actual!r}")


def assert_true(value: bool, label: str) -> None:
    if not value:
        raise AssertionError(label)


def main() -> int:
    helper = load_helper_module()

    with tempfile.TemporaryDirectory() as tmpdir:
        engine_root = Path(tmpdir) / "UE_5.8"
        editor_bin = engine_root / "Engine" / "Binaries" / "Mac" / "UnrealEditor"
        build_script = engine_root / "Engine" / "Build" / "BatchFiles" / "Mac" / "Build.sh"
        projectfiles_script = engine_root / "Engine" / "Build" / "BatchFiles" / "Mac" / "GenerateProjectFiles.sh"
        uht_binary = engine_root / "Engine" / "Binaries" / "Mac" / "UnrealHeaderTool"
        for path in (editor_bin, build_script, projectfiles_script, uht_binary):
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text("", encoding="utf-8")

        tooling = helper.UnrealTooling(
            engine_root=engine_root,
            editor_binary=editor_bin,
            projectfiles_script=projectfiles_script,
            build_script=build_script,
            uht_binary=uht_binary,
        )
        commands = helper.build_commands(tooling)
        bootstrap_sequence = helper.build_sequence(commands, "bootstrap_editor")
        readiness = helper.collect_readiness(tooling)

        assert_true(commands["projectfiles"] is not None, "projectfiles command exists")
        assert_true(commands["build_editor"] is not None, "editor build command exists")
        assert_true(commands["build_game"] is not None, "game build command exists")
        assert_true(commands["open_editor"] is not None, "open editor command exists")
        assert_equal(commands["build_editor"][1], "GreeislandEditor", "editor target name")
        assert_equal(commands["build_game"][1], "Greeisland", "game target name")
        assert_true(any("Greeisland.uproject" in token for token in commands["projectfiles"]), "projectfiles includes uproject")
        assert_equal(commands["open_editor"][0], str(editor_bin), "open editor uses binary")
        assert_true(bootstrap_sequence is not None, "bootstrap editor sequence exists")
        assert_equal(len(bootstrap_sequence), 3, "bootstrap sequence step count")
        assert_equal(bootstrap_sequence[0][0], str(projectfiles_script), "bootstrap starts with projectfiles")
        assert_equal(bootstrap_sequence[1][0], str(build_script), "bootstrap continues with build")
        assert_equal(bootstrap_sequence[2][0], str(editor_bin), "bootstrap ends by opening editor")
        assert_true(any(item.label == "Build.sh available" and item.ok for item in readiness), "readiness sees Build.sh")
        assert_true(any(item.label == "UnrealEditor detected" and item.ok for item in readiness), "readiness sees editor")

    print("OK: UE build helper smoke tests passed")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as exc:
        print(str(exc), file=sys.stderr)
        raise SystemExit(1)
