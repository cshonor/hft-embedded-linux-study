#!/usr/bin/env python3
"""Replace book .nas suffix with .asm in 02-30days-os notes (not NASM/nask)."""

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1] / "07-system-low-level-hands-on" / "02-30days-os"

# Order matters: specific filenames first
REPLACEMENTS = [
    ("helloos.nas", "helloos.asm"),
    ("asmhead.nas", "asmhead.asm"),
    ("naskfunc.nas", "naskfunc.asm"),
    ("a_nask.nas", "a_nask.asm"),
    ("api.nas", "api.asm"),
    ("`*.nas`", "`*.asm`"),
    ('"*.nas"', '"*.asm"'),
    ("`.nas`", "`.asm`"),
    (".nas /", ".asm /"),  # rare
    ("/ `.nas`", "/ `.asm`"),
    ("（.nas ", "（.asm "),
    ("`.nas、", "`.asm、"),
    ("`.nas`、", "`.asm`、"),
    ("`.nas` /", "`.asm` /"),
    ("`.nas` / `.nasm` / `.asm`", "`.asm`"),
    ("`.asm` / `.nas` / `.nasm`", "`.asm`"),
    ("`.asm / .nas / .nasm`", "`.asm`"),
    (".nas / `.nasm` / `.asm`", ".asm"),
    ("后缀 `.nas`", "后缀 `.asm`"),
    ("不改 `.nas`", "统一 `.asm`"),
    ("不用改 `.nas`", "使用 `.asm`"),
    ("不用改后缀", "使用 `.asm` 后缀"),
    ("后缀不用改", "后缀为 `.asm`"),
    ("文件名照旧", "文件名为 `helloos.asm`"),
    ("保持 `.nas` 即可", "使用 `.asm`"),
    ("仍是 `helloos.nas`", "仍是 `helloos.asm`"),
    ("默认文件名仍是 `helloos.nas`", "默认文件名为 `helloos.asm`"),
    ("含 `Makefile` 与 `helloos.asm`", "含 `Makefile` 与 `helloos.asm`"),  # idempotent
]

# Phrases to rewrite after bulk replace
POST_FIXES = [
    (
        "| **`.asm` / `.asm`** | **源码后缀**",
        "| **`.asm`** | **源码后缀**",
    ),
    (
        "（**.asm / .nas / .nasm** 均可，内容一样）",
        "（**`.asm`** 源码）",
    ),
    (
        "**.asm / .nasm / .asm** NASM 都能读",
        "**`.asm`** — NASM 直接编译",
    ),
    (
        "`.asm` / `.nasm` / `.asm` NASM **都能读**",
        "`.asm` — NASM **直接编译**",
    ),
    (
        "只换 nask → NASM，不改 `.asm`：",
        "用 NASM 编译 `.asm`：",
    ),
    (
        "只换 nask → NASM，不改 `.asm`",
        "用 NASM 编译 `.asm`",
    ),
    (
        "## 只换汇编器，不改 `.asm` 后缀（最重要）",
        "## 汇编器：nask → NASM，源码统一 `.asm`",
    ),
    (
        "### 大白话 · NASM、`.asm` / `.asm`、nask 各是什么？",
        "### 大白话 · NASM、`.asm`、nask 各是什么？",
    ),
    (
        "原书作者习惯 `.asm`，完全等价",
        "原书作者写 `.nas`，本仓库一律 `.asm`",
    ),
    (
        "   后缀 .asm 或 .nas 都行",
        "   后缀统一 `.asm`",
    ),
]


def main() -> None:
    for path in ROOT.rglob("*.md"):
        text = path.read_text(encoding="utf-8")
        original = text
        for old, new in REPLACEMENTS:
            text = text.replace(old, new)
        for old, new in POST_FIXES:
            text = text.replace(old, new)
        # leftover standalone .nas in paths like section-8.3-揭秘-asmheadnas - content only
        if text != original:
            path.write_text(text, encoding="utf-8")
            print(f"updated {path.relative_to(ROOT)}")


if __name__ == "__main__":
    main()
