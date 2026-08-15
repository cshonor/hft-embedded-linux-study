#!/usr/bin/env python3
"""Generate ER-style README.md hub for each chapter (Crafting Interpreters)."""
from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

PART_NAMES = {
    "part01_welcome": "Part I · Welcome",
    "part02_jlox": "Part II · jlox",
    "part03_clox": "Part III · clox",
}


def extract_meta(note: Path) -> tuple[str, str, str]:
    lines = note.read_text(encoding="utf-8").splitlines()
    title = lines[0].lstrip("# ").strip() if lines else note.stem
    online = ""
    zh = ""
    for line in lines[1:6]:
        if "craftinginterpreters.com" in line and not online:
            m = re.search(r"\((https://craftinginterpreters\.com/[^)]+)\)", line)
            if m:
                online = m.group(1)
        if ("craftinginterpreters-zh" in line or "中文" in line) and not zh:
            m = re.search(r"\((https://[^)]+)\)", line)
            if m and ("vercel" in m.group(1) or "zh" in m.group(1)):
                zh = m.group(1)
    return title, online, zh


def one_liner(note: Path) -> str:
    text = note.read_text(encoding="utf-8")
    m = re.search(r"## 本章定位\s*\n\n(.+?)(?:\n\n|\n\|)", text, re.DOTALL)
    if m:
        line = m.group(1).strip().split("\n")[0]
        line = re.sub(r"\*\*", "", line)
        return line[:200]
    return "见专项笔记。"


def build_readme(part: str, note: Path) -> str:
    title, online, zh = extract_meta(note)
    part_label = PART_NAMES.get(part, part)
    summary = one_liner(note)
    online_line = f"[craftinginterpreters.com]({online})" if online else "[craftinginterpreters.com](https://craftinginterpreters.com/contents.html)"
    zh_line = f" · [中文在线]({zh})" if zh else ""

    return f"""# {title}

> **Crafting Interpreters** · [{part_label}](../README.md) · [本书目录](../../本书目录.md)  
> 在线：{online_line}{zh_line}

## 状态

- [x] 已读（笔记整理）

---

## 一句话

{summary}

---

## 专项笔记

| 阅读 |
|------|
| [{note.name}](./{note.name}) |

---

## 逻辑脉络

见 [{note.name}](./{note.name}) 内 § 小节与速记。
"""


def main() -> None:
    count = 0
    for part_dir in ["part01_welcome", "part02_jlox", "part03_clox"]:
        part_path = ROOT / part_dir
        if not part_path.is_dir():
            continue
        for chapter in sorted(part_path.iterdir()):
            if not chapter.is_dir() or not chapter.name.startswith("chapter"):
                continue
            notes = sorted(chapter.glob("[0-9][0-9]-*.md"))
            if not notes:
                continue
            note = notes[0]
            readme = chapter / "README.md"
            readme.write_text(build_readme(part_dir, note), encoding="utf-8")
            count += 1
    print(f"Updated {count} chapter README hubs")


if __name__ == "__main__":
    main()
