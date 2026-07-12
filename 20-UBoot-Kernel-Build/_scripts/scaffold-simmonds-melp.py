#!/usr/bin/env python3
"""Scaffold chapters for Mastering Embedded Linux Programming, 3rd ed (module 20)."""

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1] / "mastering-embedded-linux-programming"

CHAPTERS = [
    (1, "chapter-01-getting-started", "Getting Started", "入门", "选读", "S1"),
    (2, "chapter-02-toolchain", "Learning About Toolchains", "了解工具链", "精读", "S1"),
    (3, "chapter-03-bootloader", "All About Bootloaders", "关于引导加载程序", "精读", "S1"),
    (4, "chapter-04-configuring-building-kernel", "Configuring and Building the Kernel", "配置和编译内核", "精读", "S1"),
    (5, "chapter-05-building-root-filesystem", "Building a Root Filesystem", "构建根文件系统", "精读", "S1"),
    (6, "chapter-06-choosing-build-system", "Choosing a Build System", "选择构建系统", "精读", "S1"),
    (7, "chapter-07-developing-with-yocto", "Developing with Yocto", "使用 Yocto 进行开发", "精读", "S1"),
    (8, "chapter-08-yocto-under-the-hood", "Yocto Under the Hood", "Yocto 底层原理", "选读", "S1"),
    (9, "chapter-09-storage-strategy", "Creating a Storage Strategy", "创建存储策略", "精读", "S2"),
    (10, "chapter-10-field-software-updates", "Field Software Updates", "现场软件更新", "选读", "S2"),
    (11, "chapter-11-device-drivers-interaction", "Interacting with Device Drivers", "与设备驱动程序交互", "精读", "S2"),
    (12, "chapter-12-prototyping-dev-boards", "Prototyping with Breakout Boards", "使用分线板进行原型设计", "选读", "S2"),
    (13, "chapter-13-booting-init", "Booting Up — The init Program", "启动 — init 程序", "精读", "S2"),
    (14, "chapter-14-busybox-runit", "Starting with BusyBox runit", "从 BusyBox runit 开始", "选读", "S2"),
    (15, "chapter-15-power-management", "Power Management", "电源管理", "选读", "S2"),
    (16, "chapter-16-packaging-python", "Packaging Python", "打包 Python", "跳过", "S3"),
    (17, "chapter-17-processes-threads", "Learning About Processes and Threads", "了解进程和线程", "精读", "S3"),
    (18, "chapter-18-managing-memory", "Managing Memory", "管理内存", "精读", "S3"),
    (19, "chapter-19-gdb-debugging", "Debugging with GDB", "使用 GDB 调试", "精读", "S4"),
    (20, "chapter-20-profiling-tracing", "Profiling and Tracing", "性能分析和跟踪", "选读", "S4"),
    (21, "chapter-21-real-time-programming", "Real-time Programming", "实时编程", "精读", "S4"),
]

SECTIONS = {
    "S1": ("Section 1", "嵌入式 Linux 的基础要素", "Elements of Embedded Linux"),
    "S2": ("Section 2", "系统架构和设计决策", "System Architecture and Design Decisions"),
    "S3": ("Section 3", "编写嵌入式应用程序", "Writing Embedded Applications"),
    "S4": ("Section 4", "调试和优化性能", "Debugging and Optimizing Performance"),
}


def chapter_readme(num: int, slug: str, en: str, zh: str, tag: str, sec: str) -> str:
    sec_en, sec_zh, _ = SECTIONS[sec]
    prev_next = ""
    if num > 1:
        prev_next += f"← [Ch {num - 1}](../{CHAPTERS[num - 2][1]}/) · "
    if num < len(CHAPTERS):
        prev_next += f"下一章 [Ch {num + 1}](../{CHAPTERS[num][1]}/) · "
    prev_next += "[OUTLINE](../OUTLINE.md) · [20 README](../../README.md)"

    return f"""# Ch {num} · {zh}

> ***Mastering Embedded Linux Programming***, 3rd ed — Chris Simmonds · **{tag}**  
> **English:** {en} · **{sec_en}:** {sec_zh}

---

## 本章定位

<!-- 读完后补充：与 U-Boot/内核/Yocto/飞控支线的衔接 -->

| | |
|---|---|
| **阅读标签** | **{tag}**（见 [OUTLINE](../OUTLINE.md)） |
| **部分** | {sec_zh} |

---

## 小节笔记

| 笔记 | 说明 |
|------|------|
| [notes/section-本章待补充.md](./notes/section-本章待补充.md) | 阅读本章后填写 |

---

## 本章 Checklist

- [ ] 读完原书对应章
- [ ] 在 `notes/` 写下可复述的要点
- [ ] 对照 [04 LKD](../../../04-Linux-Kernel-Development/) / [07 TLPI](../../../07-The-Linux-Programming-Interface/) 可复用概念

---

{prev_next}
"""


def main() -> None:
    ROOT.mkdir(parents=True, exist_ok=True)
    for num, slug, en, zh, tag, _sec in CHAPTERS:
        d = ROOT / slug
        notes = d / "notes"
        notes.mkdir(parents=True, exist_ok=True)
        (d / "README.md").write_text(
            chapter_readme(num, slug, en, zh, tag, _sec), encoding="utf-8"
        )
        placeholder = notes / "section-本章待补充.md"
        if not placeholder.exists():
            placeholder.write_text(
                f"## {zh}\n\n> **Ch {num}** · [章导读](../README.md)\n\n<!-- 待补充 -->\n",
                encoding="utf-8",
            )
    print(f"Scaffolded {len(CHAPTERS)} chapters under {ROOT}")


if __name__ == "__main__":
    main()
