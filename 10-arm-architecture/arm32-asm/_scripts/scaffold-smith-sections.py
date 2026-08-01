#!/usr/bin/env python3
"""Scaffold per-section notes and update chapter READMEs for Smith ARM Assembly Language."""

from __future__ import annotations

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

# (section_num, slug, title)
CHAPTER_SECTIONS: dict[str, list[tuple[str, str, str]]] = {
    "chapter-01-overview-computing-systems": [
        ("1.1", "1-1-intro", "简介"),
        ("1.2", "1-2-risc-history", "RISC 历史 — ARM 起源 · Cortex A/R/M 系列"),
        ("1.3", "1-3-computing-devices", "计算设备"),
        ("1.4", "1-4-number-systems", "数字系统"),
        ("1.5", "1-5-representation", "数字与字符的表示 — 整数 · 浮点 · 字符"),
        ("1.6", "1-6-bits-to-commands", "将比特翻译为命令"),
        ("1.7", "1-7-tools", "工具 — 开源 · Keil · Code Composer Studio"),
        ("1.8", "1-8-exercises", "练习题"),
    ],
    "chapter-02-programmers-model": [
        ("2.1", "2-1-intro", "简介"),
        ("2.2", "2-2-data-types", "数据类型"),
        ("2.3", "2-3-arm7tdmi", "ARM7TDMI — 处理器模式 · 寄存器 · 向量表"),
        ("2.4", "2-4-cortex-m4", "Cortex-M4 — 处理器模式 · 寄存器 · 向量表"),
        ("2.5", "2-5-exercises", "练习题"),
    ],
    "chapter-03-instruction-sets-v4t-v7m": [
        ("3.1", "3-1-intro", "简介"),
        ("3.2", "3-2-arm-thumb-compare", "ARM、Thumb 和 Thumb-2 指令对比"),
        ("3.3", "3-3-example-shift", "示例程序 1 — 数据移位"),
        ("3.4", "3-4-example-factorial", "示例程序 2 — 阶乘计算"),
        ("3.5", "3-5-example-register-swap", "示例程序 3 — 寄存器交换"),
        ("3.6", "3-6-example-float", "示例程序 4 — 浮点数操作"),
        ("3.7", "3-7-example-int-float-xfer", "示例程序 5 — 整数与浮点寄存器数据传输"),
        ("3.8", "3-8-programming-guide", "编程指南"),
        ("3.9", "3-9-exercises", "练习题"),
    ],
    "chapter-04-assembler-rules-directives": [
        ("4.1", "4-1-intro", "简介"),
        ("4.2", "4-2-module-structure", "汇编语言模块结构"),
        ("4.3", "4-3-register-names", "预定义的寄存器名称"),
        ("4.4", "4-4-directives", "常用伪指令 — Keil / CCS · 代码块 · 对齐 · 文字池"),
        ("4.5", "4-5-macros", "宏 (Macros)"),
        ("4.6", "4-6-assembler-misc", "汇编器杂项特性 — 操作符 · CCS 数学函数"),
        ("4.7", "4-7-exercises", "练习题"),
    ],
    "chapter-05-loads-stores-addressing": [
        ("5.1", "5-1-intro", "简介"),
        ("5.2", "5-2-memory", "内存"),
        ("5.3", "5-3-load-store", "加载与存储指令"),
        ("5.4", "5-4-addressing", "操作数寻址 — 前变址 · 后变址"),
        ("5.5", "5-5-endianness", "字节序 (Endianness)"),
        ("5.6", "5-6-bit-banded", "位带内存 (Bit-Banded Memory) — Cortex-M"),
        ("5.7", "5-7-memory-notes", "内存注意事项"),
        ("5.8", "5-8-exercises", "练习题"),
    ],
    "chapter-06-constants-literal-pools": [
        ("6.1", "6-1-intro", "简介"),
        ("6.2", "6-2-rotate-constants", "ARM 循环移位方案 — 常数编码进指令"),
        ("6.3", "6-3-load-constants", "加载常量 — MOVW/MOVT"),
        ("6.4", "6-4-literal-pools", "文字池 (Literal Pools)"),
        ("6.5", "6-5-load-addresses", "向寄存器加载地址"),
        ("6.6", "6-6-exercises", "练习题"),
    ],
    "chapter-07-integer-logic-arithmetic": [
        ("7.1", "7-1-intro", "简介"),
        ("7.2", "7-2-flags", "标志位 — N · V · Z · C"),
        ("7.3", "7-3-compare", "比较指令"),
        ("7.4", "7-4-data-processing", "数据处理 — 布尔 · 移位 · 加减 · 饱和 · 乘除"),
        ("7.5", "7-5-dsp", "DSP 扩展"),
        ("7.6", "7-6-bit-ops", "位操作指令"),
        ("7.7", "7-7-fractional", "分数表示法 (Fractional Notation)"),
        ("7.8", "7-8-exercises", "练习题"),
    ],
    "chapter-08-branches-loops": [
        ("8.1", "8-1-intro", "简介"),
        ("8.2", "8-2-branches", "分支机制 — ARM7TDMI · v7-M"),
        ("8.3", "8-3-loops", "循环 — While · For · Do-While"),
        ("8.4", "8-4-conditional", "条件执行 — v4T 条件执行 · v7-M IT 块"),
        ("8.5", "8-5-straight-line", "直线型编码 — 循环展开"),
        ("8.6", "8-6-exercises", "练习题"),
    ],
    "chapter-09-floating-point-basics": [
        ("9.1", "9-1-intro", "简介"),
        ("9.2", "9-2-history", "浮点历史"),
        ("9.3", "9-3-overview", "浮点概述"),
        ("9.4", "9-4-data-types", "浮点数据类型"),
        ("9.5", "9-5-representable", "浮点可表示的值 — 正常 · 次正常 · 零"),
        ("9.6", "9-6-special-values", "特殊值 — 无穷大 · NaN"),
        ("9.7", "9-7-fp-registers", "Cortex-M4 浮点寄存器文件"),
        ("9.8", "9-8-fpu-control", "FPU 控制寄存器 — FPSCR · CPACR"),
        ("9.9", "9-9-fp-transfer", "浮点数据传输"),
        ("9.10", "9-10-precision-convert", "半精度与单精度转换"),
        ("9.11", "9-11-int-float-convert", "浮点与整数/定点格式相互转换"),
        ("9.12", "9-12-exercises", "练习题"),
    ],
    "chapter-10-floating-point-rounding-exceptions": [
        ("10.1", "10-1-intro", "简介"),
        ("10.2", "10-2-rounding", "舍入 — IEEE 754-2008 舍入模式"),
        ("10.3", "10-3-exceptions", "异常 — 除零 · 无效 · 溢出 · 下溢 · 不精确"),
        ("10.4", "10-4-algebra", "代数定律与浮点运算"),
        ("10.5", "10-5-normalization", "规格化与抵消"),
        ("10.6", "10-6-exercises", "练习题"),
    ],
    "chapter-11-floating-point-data-processing": [
        ("11.1", "11-1-intro", "简介"),
        ("11.2", "11-2-syntax", "浮点指令语法"),
        ("11.3", "11-3-summary", "浮点指令摘要"),
        ("11.4", "11-4-flags", "标志位 — 比较指令 · N/Z/C/V"),
        ("11.5", "11-5-special-modes", "Flush-to-Zero · 默认 NaN 模式"),
        ("11.6", "11-6-non-arithmetic", "非算术指令 — 绝对值 · 求反"),
        ("11.7", "11-7-arithmetic", "算术指令 — 加减 · 乘加 · 除法 · 平方根"),
        ("11.8", "11-8-examples", "编码示例"),
        ("11.9", "11-9-exercises", "练习题"),
    ],
    "chapter-12-tables": [
        ("12.1", "12-1-intro", "简介"),
        ("12.2", "12-2-int-lookup", "整数查找表"),
        ("12.3", "12-3-float-lookup", "浮点查找表"),
        ("12.4", "12-4-binary-search", "二分查找 (Binary Searches)"),
        ("12.5", "12-5-exercises", "练习题"),
    ],
    "chapter-13-subroutines-stacks": [
        ("13.1", "13-1-intro", "简介"),
        ("13.2", "13-2-stacks", "堆栈 — LDM/STM · PUSH/POP · 满/空 · 递增/递减"),
        ("13.3", "13-3-subroutines", "子程序"),
        ("13.4", "13-4-parameters", "向子程序传递参数 — 寄存器 · 指针 · 堆栈"),
        ("13.5", "13-5-apcs", "ARM APCS — 应用过程调用标准"),
        ("13.6", "13-6-exercises", "练习题"),
    ],
    "chapter-14-exception-handling-arm7tdmi": [
        ("14.1", "14-1-intro", "简介"),
        ("14.2", "14-2-interrupts", "中断"),
        ("14.3", "14-3-error-conditions", "错误条件"),
        ("14.4", "14-4-exception-sequence", "异常序列"),
        ("14.5", "14-5-vector-table", "向量表"),
        ("14.6", "14-6-handlers-priority", "处理程序与优先级"),
        ("14.7", "14-7-mechanism", "基础机制小结"),
        ("14.8", "14-8-handler-code", "处理异常的程序 — 复位 · 未定义 · VIC · 中止 · SVC"),
        ("14.9", "14-9-exercises", "练习题"),
    ],
    "chapter-15-exception-handling-v7m": [
        ("15.1", "15-1-intro", "简介"),
        ("15.2", "15-2-modes-privilege", "操作模式与特权级别"),
        ("15.3", "15-3-vector-table", "向量表"),
        ("15.4", "15-4-stack-pointers", "堆栈指针 — MSP/PSP"),
        ("15.5", "15-5-stack-frames", "处理器出入栈序列"),
        ("15.6", "15-6-fault-types", "异常类型 — 硬故障 · 内存管理故障等"),
        ("15.7", "15-7-nvic", "中断 — 基于 NVIC 的外部中断"),
        ("15.8", "15-8-exercises", "练习题"),
    ],
    "chapter-16-memory-mapped-peripherals": [
        ("16.1", "16-1-intro", "简介"),
        ("16.2", "16-2-lpc2104-uart", "LPC2104 — UART 通信"),
        ("16.3", "16-3-lpc2132-dac", "LPC2132 — D/A 转换器生成正弦波"),
        ("16.4", "16-4-tiva-gpio", "Tiva Launchpad — GPIO 操作"),
        ("16.5", "16-5-exercises", "练习题"),
    ],
    "chapter-17-arm-thumb-thumb2-instructions": [
        ("17.1", "17-1-intro", "简介"),
        ("17.2", "17-2-arm-vs-thumb16", "ARM 与 16 位 Thumb 指令"),
        ("17.3", "17-3-thumb2", "32 位 Thumb 指令 (Thumb-2)"),
        ("17.4", "17-4-state-switch", "ARM 与 Thumb 状态切换 — BX 等"),
        ("17.5", "17-5-interworking", "如何为 Thumb 编译代码 — Interworking"),
        ("17.6", "17-6-exercises", "练习题"),
    ],
    "chapter-18-mixing-c-and-assembly": [
        ("18.1", "18-1-intro", "简介"),
        ("18.2", "18-2-inline-asm", "内联汇编 (Inline Assembler)"),
        ("18.3", "18-3-embedded-asm", "嵌入式汇编 (Embedded Assembler)"),
        ("18.4", "18-4-c-asm-calls", "C 与汇编相互调用 — APCS"),
        ("18.5", "18-5-exercises", "练习题"),
    ],
}

CHAPTER_META = {
    slug: (num, en, zh, tag)
    for slug, num, en, zh, tag in [
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
}


def section_note(section_num: str, title: str, zh: str, num: int) -> str:
    return f"""## §{section_num} {title}

> **Ch {num} · {zh}** · [章导读](../README.md)

<!-- 待补充 -->
"""


def chapter_readme(slug: str, sections: list[tuple[str, str, str]]) -> str:
    num, en, zh, tag = CHAPTER_META[slug]
    rows = "\n".join(
        f"| **§{sec}** | {title} | [notes/section-{sslug}.md](./notes/section-{sslug}.md) |"
        for sec, sslug, title in sections
    )
    prev_next = ""
    if num > 1:
        prev_slug = list(CHAPTER_META.keys())[num - 2]
        prev_next += f"← [Ch {num - 1}](../{prev_slug}/) · "
    if num < 18:
        next_slug = list(CHAPTER_META.keys())[num]
        prev_next += f"下一章 [Ch {num + 1}](../{next_slug}/) · "
    prev_next += "[OUTLINE](../OUTLINE.md) · [19 README](../README.md)"

    return f"""# Ch {num} · {zh}

> ***ARM Assembly Language*** — William Sw Smith · **{tag}**  
> **English:** {en}

---

## 本章定位

<!-- 读完后补充：要点、与 20 U-Boot / 21 驱动的衔接 -->

| | |
|---|---|
| **阅读标签** | **{tag}**（见 [OUTLINE](../OUTLINE.md)） |
| **架构** | 本书 **v4T / v7-M**；AArch64 主书见 [奔跑吧 ARM64](../arm64-programming-practice/) |

---

## 小节笔记

| 小节 | 标题 | 笔记 |
|------|------|------|
{rows}

---

## 本章 Checklist

- [ ] 读完原书对应章
- [ ] 在 `notes/` 写下可复述的要点
- [ ] （若 **精读**）能对照 [02 C](../../02-c-programming/) 或内核 `.S` 举例

---

{prev_next}
"""


def main() -> None:
    total = 0
    for slug, sections in CHAPTER_SECTIONS.items():
        num, _, zh, _ = CHAPTER_META[slug]
        chapter_dir = ROOT / slug
        notes_dir = chapter_dir / "notes"
        notes_dir.mkdir(parents=True, exist_ok=True)

        for sec_num, sec_slug, title in sections:
            path = notes_dir / f"section-{sec_slug}.md"
            if not path.exists():
                path.write_text(section_note(sec_num, title, zh, num), encoding="utf-8")
                total += 1

        (chapter_dir / "README.md").write_text(chapter_readme(slug, sections), encoding="utf-8")

    print(f"Updated {len(CHAPTER_SECTIONS)} chapters; created {total} new section notes under {ROOT}")


if __name__ == "__main__":
    main()
