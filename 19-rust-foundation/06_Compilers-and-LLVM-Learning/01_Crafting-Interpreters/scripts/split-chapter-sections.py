#!/usr/bin/env python3
"""Split chapter notes into one .md per section (01-ER style)."""
from __future__ import annotations

import re
import unicodedata
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PART_NAMES = {
    "part01_welcome": "Part I · Welcome",
    "part02_jlox": "Part II · jlox",
    "part03_clox": "Part III · clox",
}

TAIL_SECTIONS = {"本章速记", "读后下一步", "阅读进度"}
TAIL_PREFIXES = ("自测", "Challenges", "读后")


def slugify(title: str) -> str:
    title = re.sub(r"^§[\d.～\-]+", "", title).strip()
    m = re.search(r"（([^）]+)）", title)
    base = m.group(1) if m else title.split("（")[0].strip()
    base = base.split("·")[0].strip()
    words = re.findall(r"[A-Za-z0-9]+", base)
    if words:
        s = "-".join(w.lower() for w in words[:6])
    else:
        s = re.sub(r"[-\s]+", "-", base.strip())[:40] or "section"
    return re.sub(r"[^a-z0-9-]", "", s.lower())[:48] or "section"


def parse_chapter(path: Path) -> tuple[str, list[tuple[str, str]]]:
    text = path.read_text(encoding="utf-8")
    lines = text.splitlines()
    title = lines[0].lstrip("# ").strip()
    body_start = 0
    for i, line in enumerate(lines):
        if line.strip() == "---" and i > 0 and lines[i - 1].startswith(">"):
            body_start = i + 1
            break
    body = "\n".join(lines[body_start:]).strip()
    sections: list[tuple[str, str]] = []
    for part in re.split(r"\n(?=## )", body):
        part = part.strip()
        if not part.startswith("## "):
            continue
        nl = part.find("\n")
        heading = part[3:nl].strip() if nl != -1 else part[3:].strip()
        content = part[nl + 1 :].strip() if nl != -1 else ""
        sections.append((heading, content))
    return title, sections


def is_tail_section(heading: str) -> bool:
    if heading in TAIL_SECTIONS:
        return True
    return any(heading.startswith(p) for p in TAIL_PREFIXES)


def section_file(ch_title: str, heading: str, content: str, prev_f: str | None, next_f: str | None) -> str:
    nav = "← [本章目录](./README.md)"
    if prev_f:
        nav += f" · 上一节：[{prev_f}](./{prev_f})"
    if next_f:
        nav += f" · 下一节：[{next_f}](./{next_f})"
    return f"# {ch_title} · {heading}\n\n{nav}\n\n---\n\n{content}\n"


def extract_online(title_path: Path) -> tuple[str, str]:
    if not title_path.exists():
        return "", ""
    header = title_path.read_text(encoding="utf-8").split("---")[0]
    online = zh = ""
    for line in header.splitlines():
        if "craftinginterpreters.com" in line:
            m = re.search(r"\((https://craftinginterpreters\.com/[^)]+)\)", line)
            if m:
                online = m.group(1)
        if "vercel" in line or ("中文" in line and "http" in line):
            m = re.search(r"\((https://[^)]+)\)", line)
            if m:
                zh = m.group(1)
    return online, zh


def process_chapter(chapter: Path, part: str) -> None:
    candidates = [p for p in chapter.glob("[0-9][0-9]-*.md") if p.name != "cheat-sheet.md"]
    if not candidates:
        return
    note = candidates[0]
    ch_title, sections = parse_chapter(note)
    online, zh = extract_online(note)

    overview = ""
    summary = ""
    main: list[tuple[str, str, str]] = []  # heading, content, fname
    tail: list[str] = []

    seq = 1
    for heading, content in sections:
        if heading == "本章定位":
            overview = content
            summary = re.sub(r"\*\*", "", content.split("\n")[0]).strip()[:200]
            continue
        if is_tail_section(heading):
            tail.append(f"## {heading}\n\n{content}")
            continue
        slug = slugify(heading)
        fname = f"{seq:02d}-{slug}.md"
        main.append((heading, content, fname))
        seq += 1

    files: list[tuple[str, str, str]] = []  # label, heading, fname

    if overview:
        fname = "00-overview.md"
        (chapter / fname).write_text(
            section_file(
                ch_title,
                "本章定位",
                overview,
                None,
                main[0][2] if main else ("cheat-sheet.md" if tail else None),
            ),
            encoding="utf-8",
        )
        files.append(("—", "本章定位", fname))

    prev_chain = ["00-overview.md"] if overview else []
    for i, (heading, content, fname) in enumerate(main):
        prev_f = prev_chain[-1] if prev_chain else None
        next_f = main[i + 1][2] if i + 1 < len(main) else ("cheat-sheet.md" if tail else None)
        (chapter / fname).write_text(
            section_file(ch_title, heading, content, prev_f, next_f),
            encoding="utf-8",
        )
        prev_chain.append(fname)
        m = re.match(r"^(§[\d.～\-]+)", heading)
        files.append((m.group(1) if m else f"·{i+1}", heading, fname))

    cheat_name = None
    if tail:
        cheat_name = "cheat-sheet.md"
        prev_f = prev_chain[-1] if prev_chain else None
        body = f"# {ch_title} · 速记与自测\n\n← [本章目录](./README.md)"
        if prev_f:
            body += f" · 上一节：[{prev_f}](./{prev_f})"
        body += "\n\n---\n\n" + "\n\n---\n\n".join(tail) + "\n"
        (chapter / cheat_name).write_text(body, encoding="utf-8")

    rows = "\n".join(f"| {a} | {b} | [{c}](./{c}) |" for a, b, c in files)
    if cheat_name:
        rows += f"\n| — | 速记 · 自测 · 进度 | [{cheat_name}](./{cheat_name}) |"

    part_label = PART_NAMES.get(part, part)
    online_line = f"[craftinginterpreters.com]({online})" if online else "[craftinginterpreters.com](https://craftinginterpreters.com/contents.html)"
    zh_line = f" · [中文在线]({zh})" if zh else ""

    readme = f"""# {ch_title}

> **Crafting Interpreters** · [{part_label}](../README.md) · [本书目录](../../本书目录.md)  
> 在线：{online_line}{zh_line}

## 状态

- [x] 已读（笔记整理）

---

## 一句话

{summary or "见各小节笔记。"}

---

## 专项笔记（一小节一文件）

| 小节 | 主题 | 阅读 |
|------|------|------|
{rows}

---

## 逻辑脉络

按上表 **§ 顺序** 阅读；`cheat-sheet.md` 含速记与自测。
"""
    (chapter / "README.md").write_text(readme, encoding="utf-8")
    note.unlink()


def fix_links(text: str) -> str:
    text = text.replace("/notes/", "/")
    # chapter monolithic note -> README hub
    text = re.sub(
        r"(chapter\d+_[a-z0-9-]+)/(\d{2}-[a-z0-9-]+\.md)",
        r"\1/README.md",
        text,
    )
    return text


def main() -> None:
    count = 0
    for part_dir in ["part01_welcome", "part02_jlox", "part03_clox"]:
        for chapter in sorted((ROOT / part_dir).iterdir()):
            if chapter.is_dir() and chapter.name.startswith("chapter"):
                process_chapter(chapter, part_dir)
                count += 1
    for md in ROOT.rglob("*.md"):
        new = fix_links(md.read_text(encoding="utf-8"))
        md.write_text(new, encoding="utf-8")
    idx = ROOT / "本书目录.md"
    idx.write_text(fix_links(idx.read_text(encoding="utf-8")), encoding="utf-8")
    print(f"Split {count} chapters")


if __name__ == "__main__":
    main()
