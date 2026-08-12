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
        checklist_lines = helper.build_runtime_checklist_lines(tooling)
        audit_snapshot_lines = helper.build_mvp_audit_snapshot_lines()
        report_lines = helper.build_report_template_lines(tooling)
        readiness = helper.collect_readiness(tooling)

        assert_true(commands["projectfiles"] is not None, "projectfiles command exists")
        assert_true(commands["build_editor"] is not None, "editor build command exists")
        assert_true(commands["build_game"] is not None, "game build command exists")
        assert_true(commands["open_editor"] is not None, "open editor command exists")
        assert_true(commands["run_game"] is not None, "run game command exists")
        assert_true(commands["run_mvp_smoke"] is not None, "run MVP smoke command exists")
        assert_equal(commands["build_editor"][1], "GreeislandEditor", "editor target name")
        assert_equal(commands["build_game"][1], "Greeisland", "game target name")
        assert_true(any("Greeisland.uproject" in token for token in commands["projectfiles"]), "projectfiles includes uproject")
        assert_equal(commands["open_editor"][0], str(editor_bin), "open editor uses binary")
        assert_equal(commands["run_game"][0], str(editor_bin), "run game uses editor binary")
        assert_true("-game" in commands["run_game"], "run game uses game mode")
        assert_true("-log" in commands["run_game"], "run game enables log")
        assert_true("-GreeislandMvpSmoke" in commands["run_mvp_smoke"], "MVP smoke enables native runtime flag")
        assert_true("-nullrhi" in commands["run_mvp_smoke"], "MVP smoke uses headless rendering")
        assert_true(bootstrap_sequence is not None, "bootstrap editor sequence exists")
        assert_equal(len(bootstrap_sequence), 3, "bootstrap sequence step count")
        assert_equal(bootstrap_sequence[0][0], str(projectfiles_script), "bootstrap starts with projectfiles")
        assert_equal(bootstrap_sequence[1][0], str(build_script), "bootstrap continues with build")
        assert_equal(bootstrap_sequence[2][0], str(editor_bin), "bootstrap ends by opening editor")
        assert_true(any("UE Runtime Verification Checklist" in line for line in checklist_lines), "checklist title exists")
        assert_true(any("Wake Cache" in line for line in checklist_lines), "checklist references walkthrough flow")
        assert_true(any("run_mvp_smoke" in line for line in checklist_lines), "checklist references automated MVP smoke")
        assert_true(any("GenerateProjectFiles.sh" in line for line in checklist_lines), "checklist includes projectfiles command")
        assert_true(any("## MVP Audit Snapshot" in line for line in audit_snapshot_lines), "audit snapshot title exists")
        assert_true(any("Proven requirements" in line for line in audit_snapshot_lines), "audit snapshot includes proven count")
        assert_true(any("AI GMの応答が不正な報酬" in line for line in audit_snapshot_lines), "audit snapshot includes AI validation requirement")
        assert_true(any("# UE Runtime Verification Report" in line for line in report_lines), "report title exists")
        assert_true(any("## MVP Audit Snapshot" in line for line in report_lines), "report includes audit snapshot")
        assert_true(any("## Commands" in line for line in report_lines), "report includes commands section")
        assert_true(any("## Walkthrough Checks" in line for line in report_lines), "report includes walkthrough section")
        assert_true(any("Ridge Scout Battle" in line for line in report_lines), "report includes walkthrough step")
        assert_true(any("MVP Audit" in line for line in report_lines), "report includes audit command")
        assert_true(any("GenerateProjectFiles" in line for line in report_lines), "report includes projectfiles result row")
        assert_true(any("RunMvpSmoke" in line for line in report_lines), "report includes MVP smoke result row")
        assert_true(any("MVP smoke log" in line for line in report_lines), "report includes MVP smoke evidence slot")
        assert_true(any(item.label == "Build.sh available" and item.ok for item in readiness), "readiness sees Build.sh")
        assert_true(any(item.label == "UnrealEditor detected" and item.ok for item in readiness), "readiness sees editor")

        output_dir = Path(tmpdir) / "pack"
        result = helper.write_verification_pack(tooling, str(output_dir))
        assert_equal(result, 0, "verification pack exit code")
        assert_true((output_dir / "ue-runtime-verification-report.md").exists(), "verification report written")
        assert_true((output_dir / "mvp-completion-audit.md").exists(), "audit report written")
        assert_true((output_dir / "ue-runtime-checklist.txt").exists(), "checklist written")
        assert_true((output_dir / "ue-tooling-doctor.txt").exists(), "doctor written")
        assert_true((output_dir / "README.md").exists(), "readme written")
        assert_true((output_dir / "run-verification-commands.sh").exists(), "commands script written")
        commands_text = (output_dir / "run-verification-commands.sh").read_text(encoding="utf-8")
        assert_true("./scripts/run_local_checks.sh" in commands_text, "commands script runs local checks")
        assert_true((output_dir / "ue-editor-preflight-output.txt").exists(), "preflight output written")
        assert_true((output_dir / "mvp-audit-output.md").exists(), "audit output written")

    print("OK: UE build helper smoke tests passed")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as exc:
        print(str(exc), file=sys.stderr)
        raise SystemExit(1)
