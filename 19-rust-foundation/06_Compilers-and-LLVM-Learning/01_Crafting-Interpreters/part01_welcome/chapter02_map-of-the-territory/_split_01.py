import re
from pathlib import Path

base = Path(__file__).parent
src = (base / "01-the-parts-of-a-language.md").read_text(encoding="utf-8")
lines = src.splitlines(keepends=True)

hub_end = next(i for i, l in enumerate(lines) if l.startswith("### §2.1.1"))
hub_lines = lines[:hub_end]

sections = []
pattern = re.compile(r"^### §2\.1\.(\d+)")
i = hub_end
self_test = []
while i < len(lines):
    m = pattern.match(lines[i])
    if m:
        num = int(m.group(1))
        start = i
        i += 1
        while i < len(lines) and not (
            lines[i].startswith("### §2.1.") or lines[i].startswith("## §2.1 自测")
        ):
            i += 1
        sections.append((num, lines[start:i]))
    elif lines[i].startswith("## §2.1 自测"):
        self_test = lines[i:]
        break
    else:
        i += 1

files = {
    1: "01-1-scanning-lexing.md",
    2: "01-2-parsing.md",
    3: "01-3-intermediate-representations.md",
    4: "01-4-static-analysis.md",
    5: "01-5-optimization.md",
    6: "01-6-code-generation.md",
    7: "01-7-virtual-machine.md",
    8: "01-8-runtime.md",
}

link_map = {
    "#241-静态分析static-analysis": "./01-4-static-analysis.md",
    "#251-优化optimization": "./01-5-optimization.md",
    "#261-代码生成code-generation": "./01-6-code-generation.md",
    "#217-虚拟机virtual-machine": "./01-7-virtual-machine.md",
    "#218-运行时runtime": "./01-8-runtime.md",
    "#231-中间表示intermediate-representations-ir": "./01-3-intermediate-representations.md",
    "#例子-4--常量折叠优化发生在-ir-上": "./01-3-intermediate-representations.md",
}


def nav(n: int) -> str:
    hub = "[§2.1 hub](./01-the-parts-of-a-language.md)"
    prev_f = "./00-overview.md" if n == 1 else f"./{files[n - 1]}"
    next_f = "./02-shortcuts-and-alternate-routes.md" if n == 8 else f"./{files[n + 1]}"
    prev_label = "00-overview" if n == 1 else f"§2.1.{n - 1}"
    next_label = "§2.2 捷径" if n == 8 else f"§2.1.{n + 1}"
    return (
        f"← {hub} · 上一节 · [{prev_label}]({prev_f}) · "
        f"下一节 · [{next_label}]({next_f})\n\n---\n\n"
    )


def fix_links(content: str) -> str:
    for old, new in link_map.items():
        content = content.replace(f"]({old})", f"]({new})")
    content = re.sub(
        r"→ 下一节：\[§2\.1\.(\d+) [^\]]+\]\([^)]+\)",
        lambda m: f"→ 下一节：[§2.1.{m.group(1)}](./{files[int(m.group(1))]})",
        content,
    )
    return content


for num, body in sections:
    title_line = body[0].replace("### ", "# ", 1)
    content = fix_links(title_line + nav(num) + "".join(body[1:]))
    (base / files[num]).write_text(content.rstrip() + "\n", encoding="utf-8")
    print("wrote", files[num])

if self_test:
    cs = (
        "# 第 2 章 · §2.1 语言的组成部分 · 速记与自测\n\n"
        "← [§2.1 hub](./01-the-parts-of-a-language.md) · [本章目录](./README.md)\n\n"
        "---\n\n"
        + "".join(self_test[1:])
    )
    (base / "01-cheat-sheet.md").write_text(cs.rstrip() + "\n", encoding="utf-8")
    print("wrote 01-cheat-sheet.md")

print("done", len(sections), "sections")
