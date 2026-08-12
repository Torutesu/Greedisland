#!/usr/bin/env python3
"""Catch Blueprint/UHT declaration shapes that are invalid or fragile before UE is installed."""

import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
UFUNCTION_RE = re.compile(
    r"UFUNCTION\((?P<spec>[^)]*)\)\s*\n\s*(?P<decl>[^;{]+(?:\([^;{]*\))?)",
    re.MULTILINE,
)


def main() -> int:
    checked = 0
    for header in (ROOT / "UnrealProject/Source/Greeisland").rglob("*.h"):
        text = header.read_text(encoding="utf-8")
        for match in UFUNCTION_RE.finditer(text):
            checked += 1
            spec = match.group("spec")
            declaration = " ".join(match.group("decl").split())
            if "BlueprintPure" in spec:
                assert not re.search(r"\bvoid\s+\w+\s*\(", declaration), (
                    f"BlueprintPure void function: {header}:{match.start()}"
                )
            if "BlueprintPure" in spec or "BlueprintCallable" in spec:
                assert not re.search(r"\bconst\s+[^()]+&\s+\w+\s*\(", declaration), (
                    f"Blueprint function returns a reference: {header}:{match.start()}"
                )

    assert checked > 0, "no UFUNCTION declarations were checked"
    print(f"OK: UE UHT surface smoke tests passed ({checked} UFUNCTION declarations)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
