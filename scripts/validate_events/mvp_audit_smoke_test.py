#!/usr/bin/env python3
"""Smoke-test MVP audit output so the completion matrix stays useful."""

from __future__ import annotations

import subprocess
import sys
from pathlib import Path


def assert_true(value: bool, label: str, detail: object = None) -> None:
    if not value:
        raise AssertionError(f"{label} failed: {detail}")


def main() -> int:
    repo_root = Path(__file__).resolve().parents[2]
    audit_script = repo_root / "scripts" / "mvp_audit.py"

    text_output = subprocess.check_output(
        ["python3", str(audit_script)],
        cwd=repo_root,
        text=True,
    )
    markdown_output = subprocess.check_output(
        ["python3", str(audit_script), "--format", "markdown"],
        cwd=repo_root,
        text=True,
    )

    assert_true("MVP Completion Audit" in text_output, "text title")
    assert_true("needs_runtime_evidence" in text_output, "text includes runtime gap status")
    assert_true("AI GMの応答が不正な報酬" in text_output, "text includes AI validation requirement")
    assert_true("Remaining runtime evidence:" in text_output, "text includes remaining evidence section")

    assert_true("# MVP Completion Audit" in markdown_output, "markdown title")
    assert_true("| Requirement | Status | Evidence | Note |" in markdown_output, "markdown table header")
    assert_true("`proven`" in markdown_output, "markdown includes proven status")
    assert_true("## Remaining Runtime Evidence" in markdown_output, "markdown remaining evidence section")
    assert_true("mvp-smoke-" not in markdown_output or "MVP smoke" in markdown_output, "runtime log output remains readable")

    print("OK: MVP audit smoke tests passed")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as exc:
        print(str(exc), file=sys.stderr)
        raise SystemExit(1)
