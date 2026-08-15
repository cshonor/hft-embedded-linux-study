#!/usr/bin/env python3
"""Rebuild chapter README 专项笔记 table from on-disk section files."""
from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PART_NAMES = {
    "part01_welcome": "Part I · Welcome",
    "part02_jlox": "Part II · jlox",
    "part03_clox": "Part III · clox",
}


def display_heading(fname: Path) -> str:
    line = fname.read_text(encoding="utf-8").splitlines()[0].lstrip("# ").strip()
    if " · " in line:
        return line.split(" · ")[-1].strip()
    return fname.stem


def section_label(heading: str, fname: str, idx: int) -> str:
    if fname == "00-overview.md" or heading == "本章定位":
        return "—"
    m = re.match(r"^(§[\d.～\-]+)", heading)
    if m:
        return m.group(1)
    return f"·{idx}"


def refresh(chapter: Path, part: str) -> None:
    readme = chapter / "README.md"
    if not readme.exists():
        return
    old = readme.read_text(encoding="utf-8")
    title_m = re.match(r"^# (.+)\n", old)
    title = title_m.group(1) if title_m else chapter.name
    blockquote = re.search(r"(> .+\n(?:> .+\n)*)", old)
    blockquote_s = blockquote.group(1).rstrip() if blockquote else ""
    summary_m = re.search(r"## 一句话\n\n(.+?)\n\n---", old, re.DOTALL)
    summary = summary_m.group(1).strip() if summary_m else "见各小节笔记。"
    status_m = re.search(r"## 状态\n\n(.+?)\n\n---", old, re.DOTALL)
    status = status_m.group(1).strip() if status_m else "- [x] 已读（笔记整理）"

    rows: list[str] = []
    idx = 0
    for f in sorted(chapter.glob("*.md")):
        if f.name in ("README.md", "cheat-sheet.md"):
            continue
        if not re.match(r"\d{2}-", f.name):
            continue
        heading = display_heading(f)
        idx += 1
        label = section_label(heading, f.name, idx)
        disp = re.sub(r"^§[\d.～\-]+\s*", "", heading) if heading.startswith("§") else heading
        rows.append(f"| {label} | {disp} | [{f.name}](./{f.name}) |")
    if (chapter / "cheat-sheet.md").exists():
        rows.append("| — | 速记 · 自测 · 进度 | [cheat-sheet.md](./cheat-sheet.md) |")

    table = "\n".join(rows)
    part_label = PART_NAMES.get(part, part)
    if not blockquote_s:
        blockquote_s = f"> **Crafting Interpreters** · [{part_label}](../README.md) · [本书目录](../../本书目录.md)"

    new = f"""# {title}

{blockquote_s}

## 状态

{status}

---

## 一句话

{summary}

---

## 专项笔记（一小节一文件）

| 小节 | 主题 | 阅读 |
|------|------|------|
{table}

---

## 逻辑脉络

按上表 **§ 顺序** 阅读；`cheat-sheet.md` 含速记与自测。
"""
    readme.write_text(new, encoding="utf-8")


def main() -> None:
    for part in ["part01_welcome", "part02_jlox", "part03_clox"]:
        for ch in sorted((ROOT / part).iterdir()):
            if ch.is_dir() and ch.name.startswith("chapter"):
                refresh(ch, part)
    print("Refreshed chapter README tables")


if __name__ == "__main__":
    main()
