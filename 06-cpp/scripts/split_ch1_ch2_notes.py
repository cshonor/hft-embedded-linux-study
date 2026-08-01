#!/usr/bin/env python3
"""Split long ch1/ch2 section notes into x.y.z sub-files."""
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CONFIG = Path(__file__).with_name("split_ch1_ch2_config.json")


def nav(parent_title: str, parent_file: str, prev_part: str | None, next_part: str | None) -> str:
    parts = [f"← [{parent_title}]({parent_file})"]
    if prev_part:
        parts.append(prev_part)
    if next_part:
        parts.append(next_part)
    return "\n\n---\n\n" + " | ".join(parts) + "\n"


def write(path: Path, content: str) -> None:
    path.write_text(content.rstrip() + "\n", encoding="utf-8")
    print(f"  {path.relative_to(ROOT)} ({len(content.splitlines())} lines)")


def extract_and_write(
    src: Path,
    parent_title: str,
    parent_file: str,
    subs: list,
    next_chapter_link: str | None = None,
) -> None:
    text = src.read_text(encoding="utf-8")
    section_intro = text.split("## 要点")[0].split("## 本节讲什么", 1)[-1].strip()
    import re
    points = text.split("## 要点")[1].split("\n## ")[0].strip()
    m = re.search(r"\n---\n", text)
    full = text[m.end() :] if m else text

    bodies = []
    for fn, title, sub_intro, start, end in subs:
        s = full.find(start)
        if s == -1:
            raise ValueError(f"{start!r} not in {src.name}")
        e = full.find(end) if end else len(full)
        bodies.append((fn, title, sub_intro, full[s:e].strip()))

    for i, (fn, title, sub_intro, chunk) in enumerate(bodies):
        prev_l = f"[← {bodies[i - 1][1]}](./{bodies[i - 1][0]})" if i > 0 else None
        next_l = f"[{bodies[i + 1][1]} →](./{bodies[i + 1][0]})" if i + 1 < len(bodies) else next_chapter_link
        content = f"# {title}\n\n## 本节讲什么\n\n{sub_intro}\n\n{chunk}{nav(parent_title, parent_file, prev_l, next_l)}"
        write(src.parent / fn, content)

    links = "\n".join(f"- [{t}](./{fn})" for fn, t, _, _, _ in [(b[0], b[1], b[2], "", "") for b in bodies])
    links = "\n".join(f"- [{b[1]}](./{b[0]})" for b in bodies)
    index = f"""# {parent_title}

## 本节讲什么

{section_intro}

## 要点

{points.strip()}

## 子节

{links}
"""
    write(src, index)


def run_config(key: str, cfg: dict) -> None:
    src = ROOT / cfg["dir"] / cfg["file"]
    if not src.exists():
        print(f"skip {key}: {src.name} missing")
        return
    print(f"Split {key}...")
    extract_and_write(
        src,
        cfg["title"],
        f"./{cfg['file']}",
        cfg["subs"],
        cfg.get("next"),
    )


def fix_cross_links() -> None:
    fixes = [
        ("01-C++Primer/ch02-variables-and-basic-types/2.6.5-头文件分离与保护.md",
         "[1.5 编译产物](../ch01-getting-started/1.5-类简介.md)",
         "[1.5.2 编译产物](../ch01-getting-started/1.5.2-链接编译产物与示例.md)"),
        ("01-C++Primer/ch02-variables-and-basic-types/2.6.5-头文件分离与保护.md",
         "[2.2 extern](./2.2-变量.md)",
         "[2.2.4 extern](./2.2.4-extern与头文件分工.md)"),
        ("01-C++Primer/ch02-variables-and-basic-types/2.6.4-友元与前置声明.md",
         "见 [2.6.2 类内初始值](./2.6.2-类内初始值.md) 对比代码",
         "见 [2.6.2 类内初始值](./2.6.2-类内初始值.md)"),
    ]
    for rel, old, new in fixes:
        path = ROOT / rel
        if path.exists():
            t = path.read_text(encoding="utf-8")
            if old in t and old != new:
                path.write_text(t.replace(old, new), encoding="utf-8")

    # 2.2 index: fix shadowing anchor link
    p22 = ROOT / "01-C++Primer/ch02-variables-and-basic-types/2.2-变量.md"
    if p22.exists():
        t = p22.read_text(encoding="utf-8")
        t = t.replace(
            "（详见 [变量遮蔽](#变量遮蔽variable-shadowing)）",
            "（详见 [2.2.3 变量遮蔽](./2.2.3-变量遮蔽.md)）",
        )
        p22.write_text(t, encoding="utf-8")


def update_readmes() -> None:
    write(ROOT / "01-C++Primer/ch02-variables-and-basic-types/README.md", """# 第 2 章 变量和基本类型

数据类型是程序的基础。本章讲述 C++ 的基本内置类型、复合类型、常量限定符、类型推断机制，并初步介绍如何自定义数据结构。

## 小节

- [2.1 基本内置类型](./2.1-基本内置类型.md)
- [2.2 变量](./2.2-变量.md)
  - [2.2.1 标识符与未初始化](./2.2.1-标识符与未初始化.md)
  - [2.2.2 变量、常量与 static](./2.2.2-变量常量与static.md)
  - [2.2.3 变量遮蔽](./2.2.3-变量遮蔽.md)
  - [2.2.4 extern 与头文件分工](./2.2.4-extern与头文件分工.md)
- [2.3 复合类型](./2.3-复合类型.md)
  - [2.3.1 引用](./2.3.1-引用.md)
  - [2.3.2 指针与 nullptr](./2.3.2-指针与nullptr.md)
- [2.4 const 限定符](./2.4-const限定符.md)
  - [2.4.1 const 基础与 constexpr](./2.4.1-const基础与constexpr.md)
  - [2.4.2 const 口诀与声明语法](./2.4.2-const口诀与声明语法.md)
- [2.5 处理类型](./2.5-处理类型.md)
- [2.6 自定义数据结构](./2.6-自定义数据结构.md)
  - [2.6.1 Sales_data 入门与聚合](./2.6.1-Sales_data入门与聚合.md)
  - [2.6.2 类内初始值](./2.6.2-类内初始值.md)
  - [2.6.3 Sales_data 完整版](./2.6.3-Sales_data完整版.md)
  - [2.6.4 友元与前置声明](./2.6.4-友元与前置声明.md)
  - [2.6.5 头文件分离与保护](./2.6.5-头文件分离与保护.md)
- [小结](./2.7-小结.md)
""")
    write(ROOT / "01-C++Primer/ch01-getting-started/README.md", """# 第 1 章 开始

本章介绍 C++ 的大部分基础内容，包括类型、变量、表达式、语句及函数，帮助读者具备编写、编译及运行简单程序的能力。

## 小节

- [1.1 编写一个简单的 C++ 程序](./1.1-编写一个简单的C++程序.md)
- [1.2 初识输入输出](./1.2-初识输入输出.md)
- [1.3 注释简介](./1.3-注释简介.md)
- [1.4 控制流](./1.4-控制流.md)
- [1.5 类简介](./1.5-类简介.md)
  - [1.5.1 类的使用与头文件分离](./1.5.1-类的使用与头文件分离.md)
  - [1.5.2 链接、编译产物与示例](./1.5.2-链接编译产物与示例.md)
- [1.6 书店程序](./1.6-书店程序.md)
- [小结与术语表](./1.7-小结与术语表.md)
""")


if __name__ == "__main__":
    config = json.loads(CONFIG.read_text(encoding="utf-8"))
    for key in ("2.2", "2.3", "2.4", "1.5"):
        run_config(key, config[key])
    fix_cross_links()
    update_readmes()
    print("Done.")
