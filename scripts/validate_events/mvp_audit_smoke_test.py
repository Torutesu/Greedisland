#!/usr/bin/env python3
"""Smoke-test MVP audit output so the completion matrix stays useful."""

from __future__ import annotations

import subprocess
import sys
import tempfile
from pathlib import Path
import importlib.util


def assert_true(value: bool, label: str, detail: object = None) -> None:
    if not value:
        raise AssertionError(f"{label} failed: {detail}")


def main() -> int:
    repo_root = Path(__file__).resolve().parents[2]
    audit_script = repo_root / "scripts" / "mvp_audit.py"

    spec = importlib.util.spec_from_file_location("mvp_audit_under_test", audit_script)
    assert spec is not None and spec.loader is not None
    audit_module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = audit_module
    spec.loader.exec_module(audit_module)
    with tempfile.TemporaryDirectory() as temporary_directory:
        audit_module.RUNTIME_LOG_DIR = Path(temporary_directory)
        contact_marker = "[Greeisland][EVENT_CONTACT]"
        pass_marker = "[Greeisland][MVP_SMOKE] PASS: one-zone MVP completed and restored."
        failed_log = audit_module.RUNTIME_LOG_DIR / "mvp-smoke-20260812-120000.log"
        failed_log.write_text(contact_marker, encoding="utf-8")
        assert_true(
            audit_module.latest_mvp_smoke_log_containing_all((contact_marker, pass_marker)) is None,
            "failed contact-only log is not sufficient",
        )
        failed_log.write_text(f"{contact_marker}\n{pass_marker}\n", encoding="utf-8")
        assert_true(
            audit_module.latest_mvp_smoke_log_containing_all((contact_marker, pass_marker)) == failed_log,
            "complete contact log is accepted",
        )

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
