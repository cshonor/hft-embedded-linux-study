#!/usr/bin/env python3
"""Split ER Item README.md into hub + topics/*.md (skip Item-01)."""
from __future__ import annotations

import re
from pathlib import Path

ER_ROOT = Path(__file__).resolve().parent.parent

TOPIC_MAP = [
    (re.compile(r"^##\s*1\."), "01-core-concepts.md", "核心知识点"),
    (re.compile(r"^##\s*2\."), "02-logic-flow.md", "逻辑脉络"),
    (re.compile(r"^##\s*3\."), "03-key-takeaways.md", "重点结论"),
    (re.compile(r"^##\s*4\."), "04-examples.md", "案例与代码"),
    (re.compile(r"^##\s*5\."), "05-pitfalls.md", "易错细节"),
    (re.compile(r"^##\s*记忆"), "cheat-sheet.md", "记忆卡片"),
]
SEC6 = re.compile(r"^##\s*6\.")


def fix_topic_links(body: str) -> str:
    body = re.sub(r"\]\(\.\./\.\./00-Book/", r"](../../../../00-Book/", body)
    body = re.sub(r"\]\(\.\./\.\./ER-demos/", r"](../../../ER-demos/", body)
    body = re.sub(r"\]\(\.\./ER-", r"](../../../ER-", body)
    body = re.sub(r"\]\(\.\./Chapter-", r"](../../Chapter-", body)
    body = re.sub(r"\]\(\.\./Item-", r"](../../Item-", body)
    return body


def fix_hub_links(body: str) -> str:
    body = re.sub(r"\]\(\.\./ER-", r"](../../ER-", body)
    body = re.sub(r"\]\(\.\./\.\./00-Book/", r"](../../../00-Book/", body)
    body = re.sub(r"\]\(\.\./\.\./ER-demos/", r"](../../ER-demos/", body)
    body = re.sub(r"\]\(\.\./Chapter-", r"](../../Chapter-", body)
    return body


def parse_sections(lines: list[str]) -> dict[str, str]:
    sections: dict[str, list[str]] = {"_header": []}
    key = "_header"

    for line in lines:
        switched = False
        for pat, fname, _ in TOPIC_MAP:
            if pat.match(line):
                sections.setdefault(key, [])
                key = fname
                sections[key] = [line]
                switched = True
                break
        if not switched and SEC6.match(line):
            sections.setdefault(key, [])
            key = "06-extension"
            sections[key] = [line]
            switched = True
        if not switched:
            sections.setdefault(key, [])
            sections[key].append(line)

    return {k: "\n".join(v).rstrip() for k, v in sections.items()}


def strip_section_title(body: str) -> str:
    lines = body.splitlines()
    if lines and lines[0].startswith("## "):
        return "\n".join(lines[1:]).strip()
    return body.strip()


def item_num(name: str) -> int:
    m = re.search(r"Item-(\d{2})", name)
    return int(m.group(1)) if m else 0


def split_readme(readme: Path) -> None:
    item_dir = readme.parent
    item_name = item_dir.name
    if item_name == "Item-01-express-data-structures":
        print(f"skip {item_name} (custom split)")
        return

    raw = readme.read_text(encoding="utf-8")
    if "## 专项笔记" in raw:
        print(f"skip {item_name} (already hub)")
        return

    lines = raw.splitlines()
    sections = parse_sections(lines)
    if "01-core-concepts.md" not in sections:
        print(f"WARN: no section 1 in {item_name}")
        return

    num = item_num(item_name)
    topics_dir = item_dir / "topics"
    topics_dir.mkdir(exist_ok=True)

    table_rows: list[str] = []
    for _, fname, title in TOPIC_MAP:
        if fname not in sections:
            continue
        body = fix_topic_links(strip_section_title(sections[fname]))
        topic_title = "背诵提纲" if fname == "cheat-sheet.md" else title
        content = (
            f"# Item {num} · {title}\n\n"
            f"← [Item {num} 目录](../README.md)\n\n"
            f"{body}\n"
        )
        (topics_dir / fname).write_text(content, encoding="utf-8")
        if fname == "cheat-sheet.md":
            table_rows.append(
                f"| — | 背诵提纲 | [cheat-sheet.md](./cheat-sheet.md) |"
            )
        else:
            n = fname[:2]
            table_rows.append(
                f"| {n} | {title} | [{fname}](./{fname}) |"
            )

    header_lines: list[str] = []
    for hl in sections["_header"].splitlines():
        if re.match(r"^##\s*1\.", hl):
            break
        header_lines.append(hl)
    hub_header = fix_hub_links("\n".join(header_lines).rstrip())

    one_liner = "见 [03-key-takeaways.md](./03-key-takeaways.md)。"
    if "03-key-takeaways.md" in sections:
        t3 = sections["03-key-takeaways.md"]
        m = re.search(r"(?m)^1\.\s+\*\*(.+?)\*\*", t3) or re.search(
            r"(?m)^-\s+\*\*(.+?)\*\*", t3
        )
        if m:
            one_liner = f"**{m.group(1)}**"

    logic_block = ""
    if "02-logic-flow.md" in sections:
        t2 = sections["02-logic-flow.md"]
        m = re.search(r"```text\s*\n(.*?)```", t2, re.DOTALL)
        if m:
            logic_block = (
                "\n---\n\n## 逻辑脉络\n\n```text\n"
                + m.group(1).strip()
                + "\n```\n"
            )

    if "06-extension" in sections:
        ext = fix_hub_links(strip_section_title(sections["06-extension"]))
        if not ext.lstrip().startswith("##"):
            ext = "## 后续拓展\n\n" + ext
        ext_block = "\n---\n\n" + ext.strip() + "\n"
    else:
        ext_block = (
            f"\n---\n\n## 后续拓展\n\n"
            f"> [ER-拓展索引 § Item {num:02d}](../../ER-拓展索引.md#item-{num:02d})\n"
        )

    table = "\n".join(table_rows)
    hub = f"""{hub_header}

---

## 一句话

{one_liner}

---

## 专项笔记（按需点开）

| # | 专题 | 阅读 |
|---|------|------|
{table}
{logic_block}{ext_block}"""
    readme.write_text(hub.rstrip() + "\n", encoding="utf-8")
    print(f"split {item_name}")


def main() -> None:
    for readme in sorted(ER_ROOT.glob("Chapter-*/Item-*/README.md")):
        split_readme(readme)
    print("Done.")


if __name__ == "__main__":
    main()
