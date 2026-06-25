#!/usr/bin/env python3
"""Renumber 09-system-low-level-hands-on subdirs: 08-x → 01/02/03; swap 2↔3 (Mikan before CPU)."""
from __future__ import annotations

import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SUB = "09-system-low-level-hands-on"

GIT_MV = [
    (f"{SUB}/08-1-30days-os", f"{SUB}/01-30days-os"),
    (f"{SUB}/08-3-mikan-os", f"{SUB}/02-mikan-os"),
    (f"{SUB}/08-2-30days-cpu", f"{SUB}/03-30days-cpu"),
]

TEXT_REPLACEMENTS = [
    # paths via placeholders (avoid partial overlap)
    ("08-1-30days-os", "__TMP01__"),
    ("08-3-mikan-os", "__TMP02__"),
    ("08-2-30days-cpu", "__TMP03__"),
    ("__TMP01__", "01-30days-os"),
    ("__TMP02__", "02-mikan-os"),
    ("__TMP03__", "03-30days-cpu"),
    # prose labels
    ("08-1 Day", "01 Day"),
    ("08-1 ·", "01 ·"),
    ("08-1 导读", "01 导读"),
    ("08-1 30天", "01 30天"),
    ("08-1 30 天", "01 30 天"),
    ("08-1 川合", "01 川合"),
    ("08-1 自制", "01 自制"),
    ("08-1 启蒙", "01 启蒙"),
    ("08-1 通读", "01 通读"),
    ("08-1 `switch_task`", "01 `switch_task`"),
    ("对照 08-1：", "对照 01："),
    ("对照 08-1 ", "对照 01 "),
    ("[08-1 ", "[01 "),
    ("**08-1 ", "**01 "),
    ("| **08-1 ", "| **01 "),
    ("至少完成 **08-1", "至少完成 **01"),
    ("08-3 MikanOS", "02 MikanOS"),
    ("08-3 ·", "02 ·"),
    ("08-3-mikan-os/", "02-mikan-os/"),
    ("[08-3 ", "[02 "),
    ("**08-3 ", "**02 "),
    ("| **08-3 ", "| **02 "),
    ("与 08-3 ", "与 02 "),
    ("→ 08-3 ", "→ 02 "),
    ("08-2 ·", "03 ·"),
    ("08-2 自制", "03 自制"),
    ("08-2 可与", "03 可与"),
    ("[08-2 ", "[03 "),
    ("**08-2 ", "**03 "),
    ("08-1 / 08-2", "01 / 02 / 03"),
    ("01 启蒙 → 08-3 现代 OS", "01 启蒙 → 02 现代 OS"),
    ("08-1 启蒙 → 08-3 现代 OS", "01 启蒙 → 02 现代 OS"),
    ("03 可与 08-1 并行或后补", "03 可与 01 并行或后补"),
    ("与 08-1 的分工", "与 01 的分工"),
    ("与 08-1 衔接", "与 01 衔接"),
    ("与 08-1 对齐", "与 01 对齐"),
    ("与 08-1 差异", "与 01 差异"),
    ("08-1 用", "01 用"),
    ("跳过 08-1", "跳过 01"),
    ("08-1 30天 → 08-3 MikanOS", "01 30天 → 02 MikanOS"),
    # parent module README / chain fixes
    ("# 08 · 系统底层动手", "# 09 · 系统底层动手"),
    ("08 自制 OS / CPU（本文件夹）", "09 自制 OS / CPU（本文件夹）"),
    (
        "→ `09` PNP → `10` UNP → `11`–`13` 网络 → `14` HFT",
        "→ `10` PNP → `11` UNP → `12`–`14` 网络 → `15` HFT",
    ),
    ("09 PNP → 10 UNP → 11–13 网络栈", "10 PNP → 11 UNP → 12–14 网络栈"),
    ("14 HFT · 15 Rust", "15 HFT · 16 Rust"),
    ("子目录用 `08-1` / `08-2` / `08-3`", "子目录用 `01` / `02` / `03`"),
    ("## 补充：自制系统动手（`08` 文件夹）", "## 补充：自制系统动手（`09` 文件夹）"),
    ("`07` TLPI 之后、`09` PNP 之前", "`07` TLPI 之后、`10` PNP 之前"),
    ("[08 自制 CPU]", "[03 自制 CPU]"),
    ("[08 PNP]", "[10 PNP]"),
    ("08-1 自制 OS（", "01 自制 OS（"),
    ("↓\n14 HFT", "↓\n15 HFT"),
]

SKIP_DIRS = {".git", "node_modules", ".cursor", "__pycache__"}
SKIP_FILES = {"renumber-09-submodules.py"}


def git_mv(src: str, dst: str) -> None:
    r = subprocess.run(["git", "mv", src, dst], cwd=ROOT, capture_output=True, text=True)
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
        if path.suffix.lower() not in {".md", ".py", ".txt"}:
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
        if (ROOT / src).exists():
            git_mv(src, dst)
        elif (ROOT / dst).exists():
            print(f"skip (already): {dst}")
        else:
            print(f"missing: {src}", file=sys.stderr)
            sys.exit(1)
    count = patch_text_files()
    print(f"done: {count} text files patched")


if __name__ == "__main__":
    main()
