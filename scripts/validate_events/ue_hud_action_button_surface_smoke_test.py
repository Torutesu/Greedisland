#!/usr/bin/env python3
"""Validate the expected HUD action button aggregation rules."""

from __future__ import annotations

import sys


def build_button(action_id: str, enabled: bool) -> dict[str, object]:
    button = {
        "action_id": action_id,
        "enabled": enabled,
        "availability_label": "Enabled" if enabled else "Disabled",
        "default_name_argument": "",
        "default_string_argument": "",
        "default_flag": True,
    }

    if action_id == "grant_developer_card":
        button["default_name_argument"] = "con_four_party_001"
    elif action_id in ("build_ai_request", "build_fallback_ai"):
        button["default_string_argument"] = "inspect rewards"
    elif action_id == "apply_last_ai_response":
        button["default_string_argument"] = "inspect rewards"

    return button


def assert_equal(actual: object, expected: object, label: str) -> None:
    if actual != expected:
        raise AssertionError(f"{label}: expected {expected!r}, got {actual!r}")


def main() -> int:
    save_button = build_button("save_session", enabled=True)
    assert_equal(save_button["availability_label"], "Enabled", "enabled label")

    grant_button = build_button("grant_developer_card", enabled=True)
    assert_equal(grant_button["default_name_argument"], "con_four_party_001", "grant default card")

    ai_button = build_button("build_ai_request", enabled=False)
    assert_equal(ai_button["default_string_argument"], "inspect rewards", "ai default choice")
    assert_equal(ai_button["availability_label"], "Disabled", "disabled label")

    fallback_button = build_button("build_fallback_ai", enabled=True)
    assert_equal(fallback_button["default_string_argument"], "inspect rewards", "fallback default choice")

    apply_button = build_button("apply_last_ai_response", enabled=True)
    assert_equal(apply_button["default_string_argument"], "inspect rewards", "apply ai default choice")

    print("OK: UE HUD action button surface smoke tests passed")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as exc:
        print(str(exc), file=sys.stderr)
        raise SystemExit(1)
