#!/usr/bin/env python3
"""Swap 09 subdirs: 02-mikan-os -> 01-mikan-os, 01-30days-os -> 02-30days-os; patch links."""
from __future__ import annotations

import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SUB = ROOT / "07-system-low-level-hands-on"

GIT_MV = [
    ("01-30days-os", "_swap-tmp-30days"),
    ("02-mikan-os", "01-mikan-os"),
    ("_swap-tmp-30days", "02-30days-os"),
]

TEXT_REPLACEMENTS = [
    ("01-30days-os", "__SWAP_30__"),
    ("02-mikan-os", "01-mikan-os"),
    ("__SWAP_30__", "02-30days-os"),
    # prose in 09 module (order matters: longer first)
    ("02 MikanOS（UEFI/64 位）", "01 MikanOS（UEFI/64 位）"),
    ("02 MikanOS（UEFI · 64 位）", "01 MikanOS（UEFI · 64 位）"),
    ("直接 [02 MikanOS]", "直接 [01 MikanOS]"),
    ("**02 MikanOS**", "**01 MikanOS**"),
    ("→ 02 MikanOS", "→ 01 MikanOS"),
    ("再 **02 MikanOS**", "再 **01 MikanOS**"),
    ("C → 02 MikanOS", "C → 01 MikanOS"),
    ("C 达标后 **直接 [02 MikanOS]", "C 达标后 **直接 [01 MikanOS]"),
    ("跳过本书，C 达标后 **直接 [02 MikanOS]", "跳过本书，C 达标后 **直接 [01 MikanOS]"),
    ("HFT 走 02，不走 01", "HFT 走 01-mikan-os，不走 02-30days-os"),
    ("07/02` MikanOS", "07/01` MikanOS"),
    ("| **01 川合 30 天** | **02 MikanOS** |", "| **02 川合 30 天** | **01 MikanOS** |"),
    ("| **01 川合 haribote** | **02 MikanOS** |", "| **02 川合 haribote** | **01 MikanOS** |"),
    ("01 启蒙 → 02 现代 OS", "02 30天启蒙 → 01 MikanOS"),
    ("01 30天（启蒙）→ 02 MikanOS", "02 30天（启蒙）→ 01 MikanOS"),
    ("01 30天 → 02 MikanOS", "02 30天 → 01 MikanOS"),
    ("01 自制 OS（可选）→ 02 MikanOS", "02 30天 OS（可选）→ 01 MikanOS"),
    ("01 通读或 Day 1–15 后** 再开", "02 通读或 Day 1–15 后** 再开"),
    ("至少完成 **01 Day 1–15**", "至少完成 **02 Day 1–15**"),
    ("不必先做完 [01 30 天]", "不必先做完 [02 30 天]"),
    ("[01 30 天](../01-30days-os/)", "[02 30 天](../02-30days-os/)"),
    ("[01 30 天 OS]", "[02 30 天 OS]"),
    ("01 30 天 OS", "02 30 天 OS"),
    ("01 30天", "02 30天"),
    ("01 川合", "02 川合"),
    ("01 haribote", "02 haribote"),
    ("01 启蒙", "02 启蒙"),
    ("01 自制 OS", "02 30天 OS"),
    ("01 自制", "02 30天"),
    ("跳过 01", "跳过 02-30days-os"),
    ("与 01 的分工", "与 02-30days-os 的分工"),
    ("与 [01 30 天 OS]", "与 [02 30 天 OS]"),
    ("对照 01：", "对照 02-30days-os："),
    ("自制 OS（01 / 02 / 03）", "自制 OS（01 MikanOS / 02 30天）"),
    ("30 天 OS / MikanOS", "MikanOS / 30 天 OS"),
]

SKIP_DIRS = {".git", "node_modules", ".cursor", "__pycache__"}
SKIP_FILES = {"swap-09-mikan-30days.py", "renumber-09-submodules.py"}


def git_mv(src: str, dst: str) -> None:
    r = subprocess.run(
        ["git", "mv", src, dst],
        cwd=ROOT,
        capture_output=True,
        text=True,
    )
    if r.returncode != 0:
        print(f"git mv failed: {src} -> {dst}\n{r.stderr}", file=sys.stderr)
        sys.exit(1)
    print(f"git mv {src} -> {dst}")


def patch_text_files() -> int:
    n = 0
    for path in ROOT.rglob("*"):
        if not path.is_file():
            continue
        if any(p in SKIP_DIRS for p in path.parts):
            continue
        if path.name in SKIP_FILES:
            continue
        if path.suffix.lower() not in {".md", ".py", ".txt", ".json", ".mdc"}:
            continue
        try:
            text = path.read_text(encoding="utf-8")
        except (UnicodeDecodeError, OSError):
            continue
        orig = text
        for old, new in TEXT_REPLACEMENTS:
            text = text.replace(old, new)
        if text != orig:
            path.write_text(text, encoding="utf-8")
            n += 1
            print(f"updated {path.relative_to(ROOT)}")
    return n


def main() -> None:
    for src, dst in GIT_MV:
        sp, dp = SUB / src, SUB / dst
        if sp.exists():
            git_mv(str(sp.relative_to(ROOT)), str(dp.relative_to(ROOT)))
        elif dp.exists():
            print(f"skip (already): {dp.relative_to(ROOT)}")
        else:
            print(f"missing: {sp}", file=sys.stderr)
            sys.exit(1)
    count = patch_text_files()
    print(f"done: {count} text files patched")


if __name__ == "__main__":
    main()
