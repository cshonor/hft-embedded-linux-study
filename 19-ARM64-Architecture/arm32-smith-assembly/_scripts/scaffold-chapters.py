#!/usr/bin/env python3
"""Scaffold chapter/appendix folders for ARM Assembly Language (module 19)."""

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

CHAPTERS = [
    ("chapter-01-overview-computing-systems", 1, "An Overview of Computing Systems", "计算机系统概述", "选读"),
    ("chapter-02-programmers-model", 2, "The Programmer's Model", "程序员模型", "精读"),
    ("chapter-03-instruction-sets-v4t-v7m", 3, "Introduction to Instruction Sets: v4T and v7-M", "指令集简介：v4T 和 v7-M", "精读"),
    ("chapter-04-assembler-rules-directives", 4, "Assembler Rules and Directives", "汇编器规则与伪指令", "精读"),
    ("chapter-05-loads-stores-addressing", 5, "Loads, Stores, and Addressing", "加载、存储与寻址", "精读"),
    ("chapter-06-constants-literal-pools", 6, "Constants and Literal Pools", "常量与文字池", "选读"),
    ("chapter-07-integer-logic-arithmetic", 7, "Integer Logic and Arithmetic", "整数逻辑与算术", "精读"),
    ("chapter-08-branches-loops", 8, "Branches and Loops", "分支与循环", "精读"),
    ("chapter-09-floating-point-basics", 9, "Introduction to Floating-Point: Basics, Data Types, and Data Transfer", "浮点简介：基础、类型与传输", "跳过"),
    ("chapter-10-floating-point-rounding-exceptions", 10, "Introduction to Floating-Point: Rounding and Exceptions", "浮点简介：舍入与异常", "跳过"),
    ("chapter-11-floating-point-data-processing", 11, "Floating-Point Data-Processing Instructions", "浮点数据处理指令", "跳过"),
    ("chapter-12-tables", 12, "Tables", "表", "选读"),
    ("chapter-13-subroutines-stacks", 13, "Subroutines and Stacks", "子程序与堆栈", "精读"),
    ("chapter-14-exception-handling-arm7tdmi", 14, "Exception Handling: ARM7TDMI", "异常处理：ARM7TDMI", "选读"),
    ("chapter-15-exception-handling-v7m", 15, "Exception Handling: v7-M", "异常处理：v7-M", "选读"),
    ("chapter-16-memory-mapped-peripherals", 16, "Memory-Mapped Peripherals", "内存映射外设", "精读"),
    ("chapter-17-arm-thumb-thumb2-instructions", 17, "ARM, Thumb and Thumb-2 Instructions", "ARM、Thumb 和 Thumb-2 指令", "选读"),
    ("chapter-18-mixing-c-and-assembly", 18, "Mixing C and Assembly", "C 与汇编混合编程", "精读"),
]

APPENDICES = [
    ("appendix-A-code-composer-studio", "A", "Running Code Composer Studio", "运行 Code Composer Studio", "跳过"),
    ("appendix-B-keil-tools", "B", "Running Keil Tools", "运行 Keil 工具", "跳过"),
    ("appendix-C-ascii-character-codes", "C", "ASCII Character Codes", "ASCII 字符代码", "选读"),
    ("appendix-D", "D", "Appendix D", "附录 D", "选读"),
]

MISC = [
    ("glossary", "Glossary", "术语表", "选读"),
    ("references", "References", "参考文献", "选读"),
]


def chapter_readme(slug: str, num: int, en: str, zh: str, tag: str, kind: str = "chapter") -> str:
    label = f"Ch {num}" if kind == "chapter" else f"附录 {num}"
    prev_next = ""
    if kind == "chapter" and num > 1:
        prev_slug = CHAPTERS[num - 2][0]
        prev_next += f"← [Ch {num - 1}](../{prev_slug}/) · "
    if kind == "chapter" and num < len(CHAPTERS):
        next_slug = CHAPTERS[num][0]
        prev_next += f"下一章 [Ch {num + 1}](../{next_slug}/) · "
    prev_next += "[OUTLINE](../OUTLINE.md) · [19 README](../README.md)"

    return f"""# {label} · {zh}

> ***ARM Assembly Language*** — William Sw Smith · **{tag}**  
> **English:** {en}

---

## 本章定位

<!-- 读完后补充：要点、与 20 U-Boot / 21 驱动的衔接 -->

| | |
|---|---|
| **阅读标签** | **{tag}**（见 [OUTLINE](../OUTLINE.md)） |
| **架构** | 本书 **v4T / v7-M**；AArch64 语法见 [ARMv8-A Guide](https://developer.arm.com/documentation/den0024/latest) |

---

## 小节笔记

| 笔记 | 说明 |
|------|------|
| [notes/section-本章待补充.md](./notes/section-本章待补充.md) | 阅读本章后填写 |

---

## 本章 Checklist

- [ ] 读完原书对应章
- [ ] 在 `notes/` 写下可复述的要点
- [ ] （若 **精读**）能对照 [02 C](../../02-c-programming/) 或内核 `.S` 举例

---

{prev_next}
"""


def misc_readme(slug: str, en: str, zh: str, tag: str) -> str:
    return f"""# {zh} · {en}

> ***ARM Assembly Language*** — William Sw Smith · **{tag}**

---

## 说明

<!-- 阅读时按需速查 -->

→ [OUTLINE](../OUTLINE.md) · [19 README](../README.md)
"""


def main() -> None:
    (ROOT / "code").mkdir(exist_ok=True)
    (ROOT / "code" / ".gitkeep").touch(exist_ok=True)

    for slug, num, en, zh, tag in CHAPTERS:
        d = ROOT / slug
        notes = d / "notes"
        notes.mkdir(parents=True, exist_ok=True)
        (d / "README.md").write_text(
            chapter_readme(slug, num, en, zh, tag), encoding="utf-8"
        )
        placeholder = notes / "section-本章待补充.md"
        if not placeholder.exists():
            placeholder.write_text(
                f"## {zh}\n\n> **Ch {num}** · [章导读](../README.md)\n\n<!-- 待补充 -->\n",
                encoding="utf-8",
            )

    for slug, letter, en, zh, tag in APPENDICES:
        d = ROOT / slug
        notes = d / "notes"
        notes.mkdir(parents=True, exist_ok=True)
        (d / "README.md").write_text(
            f"""# 附录 {letter} · {zh}

> ***ARM Assembly Language*** — William Sw Smith · **{tag}**  
> **English:** {en}

---

## 说明

| | |
|---|---|
| **阅读标签** | **{tag}**（见 [OUTLINE](../OUTLINE.md)） |

<!-- 本路线用 WSL + GCC，附录 A/B 工具链可跳过 -->

→ [OUTLINE](../OUTLINE.md) · [19 README](../README.md)
""",
            encoding="utf-8",
        )
        placeholder = notes / "section-本章待补充.md"
        if not placeholder.exists():
            placeholder.write_text(
                f"## {zh}\n\n> **附录 {letter}** · [导读](../README.md)\n\n<!-- 待补充 -->\n",
                encoding="utf-8",
            )

    for slug, en, zh, tag in MISC:
        d = ROOT / slug
        notes = d / "notes"
        notes.mkdir(parents=True, exist_ok=True)
        (d / "README.md").write_text(misc_readme(slug, en, zh, tag), encoding="utf-8")
        placeholder = notes / "section-待补充.md"
        if not placeholder.exists():
            placeholder.write_text(f"## {zh}\n\n<!-- 待补充 -->\n", encoding="utf-8")

    print(f"Scaffolded under {ROOT}")


if __name__ == "__main__":
    main()
