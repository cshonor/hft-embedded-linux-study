#!/usr/bin/env python3
"""Split flat chapter-XX.md into chapter-XX/README.md + notes/section-*.md"""
import re
import os
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1] / "02-Systems-Performance-2nd"

META_SECTIONS = {
    "大白话",
    "本章 Checklist",
    "本章学习目标",
    "HFT 精读捷径",
    "相关章节",
    "本章在全书中的位置",
    "全书知识地图",
    "推荐阅读顺序",
    "Gregg 本章金句",
    "HFT 工具与场景对照",
    "与 perf / Ftrace 的分工",
    "Ftrace vs perf vs BPF",
    "工具选型",
    "危机响应",
    "内核路径速查",
    "方法论速查",
}


def section_kind(title: str) -> str:
    t = title.strip()
    for prefix in META_SECTIONS:
        if prefix in t:
            return "meta"
    return "note"


def slugify(title: str) -> str:
    s = title.strip()
    s = re.sub(r"^[#\s]+", "", s)
    s = re.sub(r"[·\(\)（）/`'\"]", "", s)
    s = re.sub(r"\s+", "-", s)
    s = re.sub(r"[^\w\u4e00-\u9fff\-–—.]+", "", s)
    s = s.replace("–", "-").replace("—", "-")
    s = re.sub(r"-+", "-", s).strip("-")
    if not s.lower().startswith("section-"):
        # prefix with section- if starts with digit
        if re.match(r"[\d]", s):
            s = "section-" + s
        else:
            s = "section-" + s
    return s[:80] + ".md"


def split_chapter(path: Path) -> None:
    text = path.read_text(encoding="utf-8")
    lines = text.splitlines(keepends=True)

    # split on ## headings
    sections: list[tuple[str, str]] = []
    current_title = None
    current_lines: list[str] = []

    for line in lines:
        if line.startswith("## "):
            if current_title is not None:
                sections.append((current_title, "".join(current_lines)))
            current_title = line[3:].strip()
            current_lines = [line]
        elif current_title is None:
            current_lines.append(line)
        else:
            current_lines.append(line)
    if current_title is not None:
        sections.append((current_title, "".join(current_lines)))

    preamble = ""
    if sections and not sections[0][0].startswith("大白话"):
        # content before first ## (shouldn't happen often)
        pass
    else:
        # first block might be # title before ##
        idx = text.find("\n## ")
        if idx == -1:
            preamble = text
            sections = []
        else:
            preamble = text[: idx + 1]

    chapter_dir = path.parent / path.stem  # chapter-06-中央处理器
    notes_dir = chapter_dir / "notes"
    notes_dir.mkdir(parents=True, exist_ok=True)

    meta_parts: list[str] = []
    note_entries: list[tuple[str, str, str]] = []  # title, filename, kind

    for title, body in sections:
        kind = section_kind(title)
        if kind == "meta":
            meta_parts.append(body)
        else:
            fname = slugify(title)
            # avoid collisions
            base = fname
            n = 1
            while (notes_dir / fname).exists():
                fname = base.replace(".md", f"-{n}.md")
                n += 1
            note_path = notes_dir / fname
            note_content = f"# {title}\n\n" if not body.lstrip().startswith("#") else ""
            # body already has ## title line
            if body.lstrip().startswith("##"):
                note_content = body
            else:
                note_content += body
            # add back-link
            if "## 相关" not in note_content:
                note_content += f"\n---\n\n← [本章导读](../README.md)\n"
            note_path.write_text(note_content, encoding="utf-8")
            note_entries.append((title, fname, "note"))

    # build README
    readme = preamble.rstrip() + "\n\n"
    readme += "## 小节笔记\n\n"
    readme += "| 节 | 笔记 |\n|----|------|\n"
    for title, fname, _ in note_entries:
        short = title.split("·")[0].strip() if "·" in title else title[:40]
        readme += f"| {short} | [notes/{fname}](./notes/{fname}) |\n"
    readme += "\n---\n\n"
    readme += "".join(meta_parts)

    if "## 相关章节" not in readme:
        readme += "\n## 相关章节\n\n"
        readme += "- [02 总目录](../OUTLINE.md)\n"

    readme_path = chapter_dir / "README.md"
    readme_path.write_text(readme, encoding="utf-8")

    # remove old flat file after success
    path.unlink()
    print(f"OK {path.name} -> {chapter_dir.name}/ ({len(note_entries)} notes)")


def main():
    for path in sorted(ROOT.glob("chapter-*.md")):
        split_chapter(path)


if __name__ == "__main__":
    main()
