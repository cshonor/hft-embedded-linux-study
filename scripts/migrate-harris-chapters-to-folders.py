#!/usr/bin/env python3
"""Migrate 00-Trading-and-Exchanges flat chapter-*.md to CSAPP-style folders."""

from __future__ import annotations

import re
import unicodedata
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
HARRIS = ROOT / "00-Trading-and-Exchanges"

CHAPTERS: list[tuple[int, str, str]] = [
    (1, "introduction-market-microstructure", "引言与市场微观结构"),
    (2, "trading-stories", "交易故事"),
    (3, "trading-industry", "交易产业"),
    (4, "orders-and-order-types", "交易指令与订单类型"),
    (5, "market-structures", "市场结构"),
    (6, "order-driven-markets", "指令驱动市场"),
    (7, "brokers", "经纪人"),
    (8, "why-people-trade", "为什么人们要交易"),
    (9, "good-markets", "好市场"),
    (10, "informed-traders-market-efficiency", "知情交易者与市场效率"),
    (11, "order-anticipators", "指令预期者"),
    (12, "bluffers-market-manipulation", "虚张声势者与市场操纵"),
    (13, "dealers", "做市商"),
    (14, "bid-ask-spreads", "买卖价差"),
    (15, "block-traders", "大宗交易者"),
    (16, "value-traders", "价值交易者"),
    (17, "arbitrageurs", "套利者"),
    (18, "buy-side-traders", "买方交易者"),
    (19, "liquidity", "流动性"),
    (20, "volatility", "波动性"),
    (21, "transaction-cost-measurement", "流动性与交易成本衡量"),
    (22, "performance-evaluation-prediction", "绩效评估与预测"),
    (23, "index-portfolio-markets", "指数与投资组合市场"),
    (24, "specialists", "专家做市商系统"),
    (25, "internalization-preferencing-crossing", "内部化优先撮合与交叉交易"),
    (26, "competition-within-among-markets", "市场内与市场间的竞争"),
    (27, "floor-vs-automated-trading", "场内交易与自动交易系统"),
    (28, "bubbles-crashes-circuit-breakers", "泡沫崩盘与熔断机制"),
    (29, "insider-trading", "内幕交易"),
]

OLD_TO_DIR: dict[str, str] = {}
DIR_BY_NUM: dict[int, str] = {}
for num, slug, _zh in CHAPTERS:
    d = f"chapter-{num:02d}-{slug}"
    DIR_BY_NUM[num] = d
    OLD_TO_DIR[f"chapter-{num:02d}-{_zh}.md"] = d


def slugify_heading(text: str) -> str:
    text = text.strip()
    text = re.sub(r"^#+\s*", "", text)
    text = re.sub(r"\*\*", "", text)
    text = re.sub(r"\[([^\]]+)\]\([^)]+\)", r"\1", text)
    text = text.split("·")[0].split("(")[0].strip()
    text = re.sub(r"[^\w\u4e00-\u9fff]+", "-", text, flags=re.UNICODE)
    text = re.sub(r"-+", "-", text).strip("-")
    return text[:80] if text else "section"


def split_chapter(content: str) -> tuple[str, list[tuple[str, str]]]:
    """Return (intro_block, [(heading_line, section_body), ...])."""
    parts = re.split(r"\n(?=## )", content)
    intro = parts[0].rstrip() + "\n"
    sections: list[tuple[str, str]] = []
    for part in parts[1:]:
        if not part.strip():
            continue
        lines = part.split("\n", 1)
        heading = lines[0].strip()
        body = lines[1] if len(lines) > 1 else ""
        sections.append((heading, body.lstrip("\n")))
    return intro, sections


def build_readme(num: int, slug: str, intro: str, section_rows: list[tuple[str, str]]) -> str:
    prev_l = f"../{DIR_BY_NUM[num - 1]}/" if num > 1 else None
    next_l = f"../{DIR_BY_NUM[num + 1]}/" if num < 29 else None

    lines = [intro.rstrip(), "", "---", "", "## 小节笔记", "", "| 节 | 笔记 |", "|----|------|"]
    for label, fname in section_rows:
        short = label.replace("## ", "")
        lines.append(f"| {short} | [notes/{fname}](./notes/{fname}) |")

  # related chapters - extract if present in intro's tail, else generate
    lines.extend(["", "---", "", "## 相关章节", ""])
    if prev_l:
        lines.append(f"- 上一章：[{DIR_BY_NUM[num - 1]}]({prev_l})")
    if next_l:
        lines.append(f"- 下一章：[{DIR_BY_NUM[num + 1]}]({next_l})")
    lines.append(f"- 全书目录：[OUTLINE.md](../OUTLINE.md)")
    lines.append("")
    return "\n".join(lines)


def migrate_chapter(num: int, slug: str, zh: str) -> None:
    old_name = f"chapter-{num:02d}-{zh}.md"
    old_path = HARRIS / old_name
    if not old_path.exists():
        print(f"skip missing {old_name}")
        return

    content = old_path.read_text(encoding="utf-8")
    intro, sections = split_chapter(content)

    # Drop old trailing "## 相关章节" from intro if we rebuild it
    intro = re.split(r"\n## 相关章节\n", intro)[0].rstrip() + "\n"

    chap_dir = HARRIS / f"chapter-{num:02d}-{slug}"
    notes_dir = chap_dir / "notes"
    notes_dir.mkdir(parents=True, exist_ok=True)

    section_rows: list[tuple[str, str]] = []
    for i, (heading, body) in enumerate(sections, start=1):
        if heading.strip().startswith("## 相关章节"):
            continue
        hslug = slugify_heading(heading)
        fname = f"section-{i}-{hslug}.md"
        note_content = heading + "\n\n" + body.rstrip() + "\n"
        (notes_dir / fname).write_text(note_content, encoding="utf-8")
        section_rows.append((heading, fname))

    readme = build_readme(num, slug, intro, section_rows)
    (chap_dir / "README.md").write_text(readme, encoding="utf-8")
    old_path.unlink()
    print(f"migrated {old_name} -> {chap_dir.name}/")


def replace_links_in_text(text: str) -> str:
    for old_file, new_dir in OLD_TO_DIR.items():
        # ./chapter-XX.md or ../chapter-XX.md or bare chapter-XX.md
        patterns = [
            (rf"\]\(\./{re.escape(old_file)}\)", f"](../{new_dir}/)"),
            (rf"\]\({re.escape(old_file)}\)", f"]({new_dir}/)"),
            (rf"\]\(\.\./{re.escape(old_file)}\)", f"](../{new_dir}/)"),
            (rf"00-Trading-and-Exchanges/{re.escape(old_file)}", f"00-Trading-and-Exchanges/{new_dir}/"),
            (rf"chapter-{old_file.replace('.md', '')}", new_dir),  # unlikely
        ]
        for pat, repl in patterns:
            text = re.sub(pat, repl, text)

    # Fix double notes paths if any
    text = text.replace("/notes/../", "/")
    return text


def update_all_links() -> None:
    targets: list[Path] = []
    targets.extend(HARRIS.rglob("*.md"))
    targets.extend(ROOT.glob("*.md"))
    for p in [
        ROOT / "README.md",
        ROOT / "LEARNING-CHAIN.md",
        ROOT / "READING-LIST.md",
        ROOT / "HFT-READING-ROADMAP.md",
        ROOT / "CROSS-MODULE-GUIDE.md",
    ]:
        if p.exists() and p not in targets:
            targets.append(p)

    for path in targets:
        if "migrate-harris" in path.name:
            continue
        original = path.read_text(encoding="utf-8")
        updated = replace_links_in_text(original)
        if updated != original:
            path.write_text(updated, encoding="utf-8")
            print(f"updated links: {path.relative_to(ROOT)}")


def rewrite_harris_readme() -> None:
    readme = HARRIS / "README.md"
    parts = [
        "# Trading and Exchanges — Larry Harris",
        "",
        "**文件夹 00 · 阶段 0（建议最先读）** · 全书 **29 章 / 7 部分**",
        "",
        "📋 **完整目录与 HFT 读/跳标注** → [OUTLINE.md](./OUTLINE.md)",
        "",
        "**配套练手（Go DEX · 与理论绑定、代码独立）：** [00-practice-go-dex/](./00-practice-go-dex/) — 理论读各章 `chapter-XX-*/`，实现见 `00-practice-go-dex/notes/` + `code/`。",
        "",
        "---",
        "",
        "## 全书结构",
        "",
        "```",
        "chapter-XX-english-slug/   ← 全书 29 章均已采用",
        "├── README.md",
        "└── notes/section-*.md",
        "```",
        "",
    ]

    def part_block(title: str, nums: list[int]) -> None:
        parts.append(f"### {title}")
        parts.append("| 章 | 笔记 |")
        parts.append("|----|------|")
        for n in nums:
            slug = DIR_BY_NUM[n].split("-", 2)[2] if False else None
            for num, s, zh in CHAPTERS:
                if num == n:
                    parts.append(f"| {n} | [{DIR_BY_NUM[n]}](./{DIR_BY_NUM[n]}/) |")
                    break
        parts.append("")

    part_block("引言", [1, 2])
    part_block("Part I · 交易的结构", list(range(3, 8)))
    part_block("Part II · 交易的益处", [8, 9])
    part_block("Part III · 投机者", [10, 11, 12])
    part_block("Part IV · 流动性提供者", list(range(13, 19)))
    part_block("Part V · 流动性与波动性", [19, 20])
    part_block("Part VI · 评估与预测", [21, 22])
    part_block("Part VII · 市场结构", list(range(23, 30)))

    parts.extend([
        "---",
        "",
        "## HFT 精读捷径",
        "",
        "```",
        "Ch 1–6   结构 + 指令驱动市场（LOB / 撮合规则）",
        "Ch 10–11 知情 · 指令预期（adverse selection）",
        "Ch 13–14 做市商 · 价差",
        "Ch 17    套利 · 多市场连接",
        "Ch 19    流动性四维",
        "Ch 21    TCA / markout",
        "Ch 25–27 PFOF · 碎片化 · 电子化",
        "```",
        "",
        "完整路线 → [LEARNING-CHAIN.md](../LEARNING-CHAIN.md) · [HFT-READING-ROADMAP.md](../HFT-READING-ROADMAP.md)",
        "",
    ])
    readme.write_text("\n".join(parts), encoding="utf-8")


def rewrite_outline() -> None:
    outline_path = HARRIS / "OUTLINE.md"
    text = outline_path.read_text(encoding="utf-8")

    for num, slug, zh in CHAPTERS:
        old = f"[chapter-{num:02d}](./chapter-{num:02d}-{zh}.md)"
        new = f"[chapter-{num:02d}](./chapter-{num:02d}-{slug}/)"
        text = text.replace(old, new)

    text = text.replace(
        "理论笔记 **仍在本目录 `chapter-*.md`**",
        "理论笔记 **在各章 `chapter-XX-*/notes/`**",
    )
    text = text.replace("Ch 1–6   结构基础（已完成 1–5，6 待补）", "Ch 1–6   结构基础（LOB / 撮合）")
    outline_path.write_text(text, encoding="utf-8")


def main() -> None:
    for num, slug, zh in CHAPTERS:
        migrate_chapter(num, slug, zh)
    rewrite_harris_readme()
    rewrite_outline()
    update_all_links()
    print("done.")


if __name__ == "__main__":
    main()
