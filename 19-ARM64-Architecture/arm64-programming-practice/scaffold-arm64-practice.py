#!/usr/bin/env python3
"""Scaffold 《ARM64体系结构编程与实践》 chapter folders under module 19."""

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1] / "arm64-programming-practice"

CHAPTERS = [
    ("chapter-01-arm64-fundamentals", 1, "ARM64体系结构基础知识", "精读"),
    ("chapter-02-raspberry-pi-lab", 2, "搭建树莓派实验环境", "精读"),
    ("chapter-03-a64-load-store", 3, "A64指令集1——加载与存储指令", "精读"),
    ("chapter-04-a64-arithmetic-shift", 4, "A64指令集2——算术与移位指令", "精读"),
    ("chapter-05-a64-compare-branch", 5, "A64指令集3——比较指令与跳转指令", "精读"),
    ("chapter-06-a64-other-instructions", 6, "A64指令集4——其他重要指令", "精读"),
    ("chapter-07-a64-traps", 7, "A64指令集的陷阱", "精读"),
    ("chapter-08-gnu-assembler", 8, "GNU汇编器", "精读"),
    ("chapter-09-linker-scripts", 9, "链接器与链接脚本", "精读"),
    ("chapter-10-gcc-inline-asm", 10, "GCC内嵌汇编代码", "精读"),
    ("chapter-11-exception-handling", 11, "异常处理", "精读"),
    ("chapter-12-interrupt-handling", 12, "中断处理", "精读"),
    ("chapter-13-gic-v2", 13, "GIC-V2", "精读"),
    ("chapter-14-memory-management", 14, "内存管理", "精读"),
    ("chapter-15-cache-basics", 15, "高速缓存基础知识", "选读"),
    ("chapter-16-cache-coherency", 16, "缓存一致性", "选读"),
    ("chapter-17-tlb-management", 17, "TLB管理", "选读"),
    ("chapter-18-memory-barriers", 18, "内存屏障指令", "精读"),
    ("chapter-19-barrier-usage", 19, "合理使用内存屏障指令", "精读"),
    ("chapter-20-atomic-operations", 20, "原子操作", "精读"),
    ("chapter-21-os-topics", 21, "操作系统相关话题", "精读"),
    ("chapter-22-fp-neon", 22, "浮点运算与NEON指令", "选读"),
    ("chapter-23-sve-optimization", 23, "可伸缩矢量计算与优化", "跳过"),
]


def readme(num: int, title: str, tag: str, slug: str) -> str:
    nav = f"← [Ch {num - 1}](../{CHAPTERS[num - 2][0]}/) · " if num > 1 else ""
    if num < len(CHAPTERS):
        nav += f"下一章 [Ch {num + 1}](../{CHAPTERS[num][0]}/) · "
    nav += "[OUTLINE](../OUTLINE.md) · [本书导读](../README.md) · [19 模块](../../README.md)"
    return f"""# 第 {num} 章 · {title}

> **《ARM64体系结构编程与实践》** · 奔跑吧Linux社区 · 人民邮电出版社 · **{tag}**

---

## 本章定位

<!-- 读完后补充：要点、BenOS/QEMU 实验、与飞控/驱动的衔接 -->

| | |
|---|---|
| **阅读标签** | **{tag}**（见 [OUTLINE](../OUTLINE.md)） |
| **实验** | 树莓派 4B / **QEMU ARM64**（官方仓库 [arm64_programming_practice](https://github.com/runninglinuxkernel/arm64_programming_practice)） |

---

## 小节笔记

| 笔记 | 说明 |
|------|------|
| [notes/section-本章待补充.md](./notes/section-本章待补充.md) | 阅读本章后填写 |

---

## 本章 Checklist

- [ ] 读完原书对应章
- [ ] 完成书中实验（若有）
- [ ] 在 `notes/` 记录可复述要点

---

{nav}
"""


def main() -> None:
    ROOT.mkdir(parents=True, exist_ok=True)
    (ROOT / "code").mkdir(exist_ok=True)
    readme_root = ROOT / "README.md"
    if not readme_root.exists():
        readme_root.write_text(
            """# 《ARM64体系结构编程与实践》

> **奔跑吧Linux社区** · 人民邮电出版社 · **模块 19 · AArch64 实战主书**  
> **实验代码：** [github.com/runninglinuxkernel/arm64_programming_practice](https://github.com/runninglinuxkernel/arm64_programming_practice)  
> **平台：** 树莓派 4B · **QEMU ARM64**（类无人机应用处理器环境）

---

## 定位

| | |
|---|---|
| **补什么** | **ARMv8/v9 · A64 64 位指令** · 异常/中断 · **GIC** · **内存管理** |
| **与 Smith 关系** | [Smith v4T/v7-M 汇编](../chapter-02-programmers-model/) = 汇编思维入门；**本书 = AArch64 主战场** |
| **飞控/无人机** | 异常 · GIC · MM · 屏障/原子 — 对接 [21 驱动](../../21-Linux-Device-Driver/) · [24 飞控](../../24-Motion-Control-Motor/) |
| **下一步** | [20 U-Boot/构建](../../20-UBoot-Kernel-Build/) |

📋 **章节目录与裁剪** → [OUTLINE.md](./OUTLINE.md)

---

## 章节目录（Ch 1–23）

| 章 | 文件夹 | 标签 |
|----|--------|------|
| 1 | [chapter-01-arm64-fundamentals](./chapter-01-arm64-fundamentals/) | 精读 |
| 2 | [chapter-02-raspberry-pi-lab](./chapter-02-raspberry-pi-lab/) | 精读 |
| 3–7 | [ch03](./chapter-03-a64-load-store/) … [ch07](./chapter-07-a64-traps/) | **A64 指令集** · 精读 |
| 8–10 | [ch08](./chapter-08-gnu-assembler/) … [ch10](./chapter-10-gcc-inline-asm/) | 工具链 · 精读 |
| 11–14 | [ch11](./chapter-11-exception-handling/) … [ch14](./chapter-14-memory-management/) | **异常/GIC/MM** · 精读 |
| 15–17 | [ch15](./chapter-15-cache-basics/) … [ch17](./chapter-17-tlb-management/) | 缓存/TLB · 选读（可对照 [03 Hennessy](../../03-Computer-Architecture-6th/)） |
| 18–20 | [ch18](./chapter-18-memory-barriers/) … [ch20](./chapter-20-atomic-operations/) | 屏障/原子 · 精读 |
| 21 | [chapter-21-os-topics](./chapter-21-os-topics/) | OS 话题 · 精读 |
| 22–23 | [ch22](./chapter-22-fp-neon/) · [ch23](./chapter-23-sve-optimization/) | NEON/SVE · 选读/跳过 |

← [19 模块总览](../README.md)
""",
            encoding="utf-8",
        )

    for slug, num, title, tag in CHAPTERS:
        d = ROOT / slug
        notes = d / "notes"
        notes.mkdir(parents=True, exist_ok=True)
        (d / "README.md").write_text(readme(num, title, tag, slug), encoding="utf-8")
        ph = notes / "section-本章待补充.md"
        if not ph.exists():
            ph.write_text(
                f"## 第 {num} 章 · {title}\n\n> [章导读](../README.md)\n\n<!-- 待补充 -->\n",
                encoding="utf-8",
            )

    print(f"Scaffolded {len(CHAPTERS)} chapters under {ROOT}")


if __name__ == "__main__":
    main()
