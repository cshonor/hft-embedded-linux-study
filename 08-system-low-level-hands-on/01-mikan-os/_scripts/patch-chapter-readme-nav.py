#!/usr/bin/env python3
"""Insert ## 本章定位 · 前后章关系 into each chapter README."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

CHAPTERS = {
    1: {
        "dir": "chapter-01-hello-world",
        "does": "用 **裸 C + Makefile** 编出第一个 **`BOOTX64.EFI`**，在 UEFI 里打印 Hello World；建立 **C → PE → FAT → EfiMain** 的现代启动链直觉。",
        "role": "**全书起点** — 破除 OS 神秘感；证明「没有 OS 也能跑你的代码」；为 Ch2 的 EDK II / MikanLoader 打工具链地基。",
        "prev": "无（MikanOS 主线起点）；可选对照 [02 30 天 Day1](../../02-30days-os/day-01-boot-asm/) 的 BIOS 512B 启蒙",
        "next": "[Ch2 EDK II / 内存 map](../chapter-02-edk2-memmap/) — 从裸 C 升级到规范 Loader，并 **摸底物理 RAM**",
    },
    2: {
        "dir": "chapter-02-edk2-memmap",
        "does": "用 **EDK II** 重写 Hello → **MikanLoader**；调用 **`GetMemoryMap()`** 导出 **memmap CSV**；理解 **UEFI 内存类型** 与 C 指针。",
        "role": "**启动链第二环** — 从「能打印」到「能读物理世界账本」；Ch8 物理分配、Ch19 分页都依赖本章的 **Conventional / MMIO** 直觉。",
        "prev": "[Ch1 Hello World](../chapter-01-hello-world/) — 已理解 UEFI 七步与 `BOOTX64.EFI`",
        "next": "[Ch3 Loader 加载内核](../chapter-03-bootloader-display/) — 用 Loader 读 ELF、跳 **kernel.elf**",
    },
    3: {
        "dir": "chapter-03-bootloader-display",
        "does": "**MikanLoader 加载 `kernel.elf`**，经 **GOP** 把帧缓冲交给 **`KernelMain()`**；引入 Loader / 内核分离与 QEMU 调试。",
        "role": "**分水岭** — UEFI 应用退场、**自制内核登场**；第一次「固件帮手 → 内核办公室」交接（尚未 ExitBootServices 全套，但加载链成立）。",
        "prev": "[Ch2 memmap](../chapter-02-edk2-memmap/) — 知道哪些物理段能加载内核",
        "next": "[Ch4 像素 / make](../chapter-04-pixel-make/) — 在内核里画像素、熟悉构建系统",
    },
    4: {
        "dir": "chapter-04-pixel-make",
        "does": "在内核中 **直接写 Frame Buffer 像素**；完善 **make** 构建；理解 **ELF** 与加载器改进。",
        "role": "**图形底座** — 从「能显示」到「能绘图」；为 Ch5 文本、Ch9–10 窗口合成提供像素级 API 直觉。",
        "prev": "[Ch3 GOP / kernel.elf](../chapter-03-bootloader-display/) — 已有帧缓冲与 KernelMain",
        "next": "[Ch5 Console 文本](../chapter-05-console-text/) — 在像素之上叠加字符输出",
    },
    5: {
        "dir": "chapter-05-console-text",
        "does": "实现 **Console 类** 与 **printk**；在帧缓冲上 **格式化文本**；接入 **Newlib** 铺垫。",
        "role": "**可观测性** — 内核日志与调试输出；GUI 之前先解决「屏幕上读文字」— 贯穿后续所有章的排错手段。",
        "prev": "[Ch4 像素绘图](../chapter-04-pixel-make/)",
        "next": "[Ch6 鼠标 / PCI](../chapter-06-mouse-pci/) — 从输出扩展到输入设备",
    },
    6: {
        "dir": "chapter-06-mouse-pci",
        "does": "**PCI 枚举** · **xHCI** · **鼠标轮询**；理解 BAR 与设备寄存器映射。",
        "role": "**输入与总线** — 从「只看屏幕」到「读硬件事件」；为 Ch7 中断 + FIFO 提供「为何要异步」的动机。",
        "prev": "[Ch5 Console](../chapter-05-console-text/)",
        "next": "[Ch7 中断 / FIFO](../chapter-07-interrupt-fifo/) — 用中断替代忙等轮询",
    },
    7: {
        "dir": "chapter-07-interrupt-fifo",
        "does": "建立 **IDT / 中断处理** 与 **FIFO 队列**；键鼠等事件 **异步投递** 到主循环。",
        "role": "**内核骨架 · 事件模型**（🔴 HFT 精读）— 现代 OS 不再轮询硬件；Ch13+ 多任务、Ch11 定时器都建在本章事件链上。",
        "prev": "[Ch6 鼠标轮询](../chapter-06-mouse-pci/) — 体会轮询的局限",
        "next": "[Ch8 内存管理](../chapter-08-memory/) — OS 自持物理页分配",
    },
    8: {
        "dir": "chapter-08-memory",
        "does": "解析 **UEFI Memory Map**；迁移栈/GDT；**四级页表身份映射**；**位图页帧分配器**。",
        "role": "**内核骨架 · 物理内存**（🔴）— 脱离 UEFI 分配；Ch2「只读 map」在此变为 **Allocate/Free**；Ch19 进程页表的前置。",
        "prev": "[Ch2 memmap](../chapter-02-edk2-memmap/) + [Ch7 中断](../chapter-07-interrupt-fifo/)",
        "next": "[Ch9 图层合成](../chapter-09-layers/) — 在稳定内存之上做 GUI",
    },
    9: {
        "dir": "chapter-09-layers",
        "does": "**Layer / WindowManager**；多图层 **合成** 与 **Shadow Buffer** 加速。",
        "role": "**GUI 合成层** — 把多个绘制面叠成桌面；Ch10 窗口、Ch15 终端都依赖图层消息模型。",
        "prev": "[Ch8 内存](../chapter-08-memory/) — 可动态分配绘制缓冲",
        "next": "[Ch10 窗口](../chapter-10-window/) — 可拖动窗口与双缓冲",
    },
    10: {
        "dir": "chapter-10-window",
        "does": "**Window 类** · **局部重绘** · **后置缓冲区**；减轻闪烁与全屏刷新成本。",
        "role": "**GUI 交互壳** — 从「能合成」到「像桌面」；Ch12 键盘、Ch15 终端的焦点/活动窗建立在此。",
        "prev": "[Ch9 LayerManager](../chapter-09-layers/)",
        "next": "[Ch11 定时器 / ACPI](../chapter-11-timer-acpi/) — 为动画与调度引入时间",
    },
    11: {
        "dir": "chapter-11-timer-acpi",
        "does": "**Local APIC 定时器** · **ACPI PM Timer 校准** · **TimerManager**；毫秒级 **Sleep**。",
        "role": "**时间基准**（🔴）— 多任务时间片、光标闪烁、Ch13–14 调度都依赖「可信 tick」；衔接 Ch2 ACPI 表概念。",
        "prev": "[Ch10 窗口](../chapter-10-window/)",
        "next": "[Ch12 键盘](../chapter-12-keyboard/) — 文本输入与 GUI 文本框",
    },
    12: {
        "dir": "chapter-12-keyboard",
        "does": "**PS/2 或 USB 键盘** 扫描码 → 字符；**GUI 文本框** 与退格。",
        "role": "**文本输入** — 连接中断 FIFO 与用户可见输入；Ch15 终端、Ch16 命令行的键盘路由前置。",
        "prev": "[Ch11 定时器](../chapter-11-timer-acpi/) + [Ch7 FIFO](../chapter-07-interrupt-fifo/)",
        "next": "[Ch13 多任务（1）](../chapter-13-multitask1/) — 多个 Task 并发",
    },
    13: {
        "dir": "chapter-13-multitask1",
        "does": "**Task / TaskManager** · **协作式多任务** · 上下文切换；多个 Task **主动让出 CPU**。",
        "role": "**内核骨架 · 多任务入门**（🔴）— 从单线程主循环到「多个执行流」；对照 02 川合 `switch_task`。",
        "prev": "[Ch7 中断](../chapter-07-interrupt-fifo/) + [Ch11 定时器](../chapter-11-timer-acpi/)",
        "next": "[Ch14 多任务（2）](../chapter-14-multitask2/) — 抢占与优先级",
    },
    14: {
        "dir": "chapter-14-multitask2",
        "does": "**定时器抢占** · **任务优先级 Level** · 解决鼠标卡顿；完善 **TaskManager**。",
        "role": "**内核骨架 · 调度**（🔴）— 接近「像 OS 一样响应」；Ch15+ 终端/命令在抢占式调度上运行。",
        "prev": "[Ch13 协作多任务](../chapter-13-multitask1/)",
        "next": "[Ch15 终端](../chapter-15-terminal/) — GUI 里的终端窗口",
    },
    15: {
        "dir": "chapter-15-terminal",
        "does": "**kLayer 消息** · **ActiveLayer** · **TaskTerminal** · **DrawArea** 局部重绘。",
        "role": "**交互壳成熟** — 规范窗口焦点与终端 Task；为 Ch16 **shell 命令** 提供可输入的 UI 容器。",
        "prev": "[Ch14 抢占调度](../chapter-14-multitask2/) + [Ch9–12 GUI/键盘](../chapter-09-layers/)",
        "next": "[Ch16 命令 / CLI](../chapter-16-commands/) — 在终端里执行 ls 等",
    },
    16: {
        "dir": "chapter-16-commands",
        "does": "**命令解析** · **history** · 方向键；实现 **ls / cat / …** 等内置命令框架。",
        "role": "**用户界面层（内核态 shell）** — 把多任务 + 终端变成「能敲命令的 OS」；Ch17 文件系统命令的前置。",
        "prev": "[Ch15 Terminal](../chapter-15-terminal/)",
        "next": "[Ch17 文件系统](../chapter-17-filesystem/) — 持久化存储与 FAT",
    },
    17: {
        "dir": "chapter-17-filesystem",
        "does": "**FAT12/16/32** · **BPB** · **UEFI Block I/O** · **`ls`**；卷镜像与目录项。",
        "role": "**持久化与 VFS 雏形**（🟡）— 从内存态到磁盘文件；Ch25–26 用户态读写文件、Ch18 加载应用都依赖本章。",
        "prev": "[Ch16 命令](../chapter-16-commands/)",
        "next": "[Ch18 应用 / ELF](../chapter-18-apps/) — 加载用户程序 `cat.elf`",
    },
    18: {
        "dir": "chapter-18-apps",
        "does": "**加载独立 ELF 应用**（如 **cat**）；命令行参数；与内核 **分离地址空间** 的前奏。",
        "role": "**用户程序载体** — 内核不再包办一切逻辑；暴露 **链接基址 / 加载地址** 问题 → 直接引出 Ch19 分页。",
        "prev": "[Ch17 FAT / Block I/O](../chapter-17-filesystem/)",
        "next": "[Ch19 分页](../chapter-19-paging/) — 每应用独立虚拟地址空间",
    },
    19: {
        "dir": "chapter-19-paging",
        "does": "**进程虚拟地址** · **SetupPageMaps / CleanPageMaps** · 修复 **rpn** 链接基址问题。",
        "role": "**地址空间隔离**（🔴）— CSAPP Ch9 / Linux `mmap` 的极简实现版；Ch20 syscall、Ch24 多终端、Ch27 应用内存的基础。",
        "prev": "[Ch8 页表基础](../chapter-08-memory/) + [Ch18 应用 ELF](../chapter-18-apps/)",
        "next": "[Ch20 系统调用](../chapter-20-syscall/) — Ring 3 与内核边界",
    },
    20: {
        "dir": "chapter-20-syscall",
        "does": "**syscall 门** · **Ring 0/3** · 用户态 **异常处理**；Newlib 通过 syscall 调内核。",
        "role": "**特权级边界**（🔴）— 从「内核里跑 cat」到「真正的用户态进程」；TLPI / LKD syscall 章的对照实验。",
        "prev": "[Ch19 分页](../chapter-19-paging/)",
        "next": "[Ch21 窗口应用](../chapter-21-window-apps/) — 用户态 GUI 程序",
    },
    21: {
        "dir": "chapter-21-window-apps",
        "does": "**winhello** 等 **用户态窗口应用** · **syscall.h** 窗口 API。",
        "role": "**用户态 GUI** — 把 Ch9–10 窗口机制延伸到 Ring 3；Ch22–23 图形事件的应用侧实践。",
        "prev": "[Ch20 syscall](../chapter-20-syscall/) + [Ch10 窗口](../chapter-10-window/)",
        "next": "[Ch22 图形和事件（1）](../chapter-22-graphics-events1/)",
    },
    22: {
        "dir": "chapter-22-graphics-events1",
        "does": "**stars** · **ReadEvent** · 用户态 **事件循环** 与基础绘图 syscall。",
        "role": "**事件驱动 GUI（上）** — 从静态窗口到「等事件再画」；游戏/桌面应用编程模型入门。",
        "prev": "[Ch21 窗口应用](../chapter-21-window-apps/)",
        "next": "[Ch23 图形和事件（2）](../chapter-23-graphics-events2/)",
    },
    23: {
        "dir": "chapter-23-graphics-events2",
        "does": "**eye / blocks** 等动画示例；更复杂的 **图形 + 定时事件**。",
        "role": "**事件驱动 GUI（下）** — 巩固用户态图形；为多终端、多应用并发打体验基础。",
        "prev": "[Ch22 图形事件（1）](../chapter-22-graphics-events1/)",
        "next": "[Ch24 多终端](../chapter-24-multi-terminal/) — 多进程 + 多页表",
    },
    24: {
        "dir": "chapter-24-multi-terminal",
        "does": "**多 Terminal Task** · **每进程 PML4** · **KillApp** · 用户态异常处理。",
        "role": "**多进程壳** — 从单应用分页到「多个用户程序同时活」；Ch25–29 文件/IPC 的并发场景。",
        "prev": "[Ch19–20 分页与 syscall](../chapter-19-paging/) + [Ch15 终端](../chapter-15-terminal/)",
        "next": "[Ch25 应用读文件](../chapter-25-app-read-file/)",
    },
    25: {
        "dir": "chapter-25-app-read-file",
        "does": "**OpenFile / ReadFile** syscall · **fd** · **grep**；Newlib **read** 接入。",
        "role": "**用户态文件读**（🟡）— 把 Ch17 FAT 暴露给应用；Unix **read/open** 语义的最小实现。",
        "prev": "[Ch17 文件系统](../chapter-17-filesystem/) + [Ch24 多终端](../chapter-24-multi-terminal/)",
        "next": "[Ch26 应用写文件](../chapter-26-app-write-file/)",
    },
    26: {
        "dir": "chapter-26-app-write-file",
        "does": "**Write** · **stdio** · **cp** · **stdin 回显**；FAT **写扩展**。",
        "role": "**用户态文件写**（🟡）— 完整「读+写+复制」；Ch28 重定向、Ch29 管道需要可写 fd。",
        "prev": "[Ch25 读文件](../chapter-25-app-read-file/)",
        "next": "[Ch27 应用内存管理](../chapter-27-app-memory/)",
    },
    27: {
        "dir": "chapter-27-app-memory",
        "does": "**Demand Paging** · **sbrk** · **CoW** · **invlpg** · **memstat**。",
        "role": "**进程内存管理**（🔴）— Linux `mmap`/堆/brk 的极简版；理解 **按需分页** 与 **写时复制**。",
        "prev": "[Ch19 分页](../chapter-19-paging/) + [Ch26 stdio](../chapter-26-app-write-file/)",
        "next": "[Ch28 日文 / 重定向](../chapter-28-japanese-redirect/)",
    },
    28: {
        "dir": "chapter-28-japanese-redirect",
        "does": "**FreeType** 矢量字体 · **日文显示** · **stdout 重定向** · 栈扩容排错。",
        "role": "**i18n + shell 重定向** — 把 Ch16 命令行能力扩展到 **> file**；展示复杂用户态应用的内存压力。",
        "prev": "[Ch27 应用内存](../chapter-27-app-memory/) + [Ch5 Newlib](../chapter-05-console-text/)",
        "next": "[Ch29 IPC / 管道](../chapter-29-ipc/)",
    },
    29: {
        "dir": "chapter-29-ipc",
        "does": "**管道 pipe** · **sort / cat** 优化 · **WaitFinish** · 应用间 **数据流**。",
        "role": "**进程间通信**（🟡）— Unix 管道最小实现；多应用协作（shell 管道）的收官。",
        "prev": "[Ch26–28 文件与重定向](../chapter-26-app-write-file/)",
        "next": "[Ch30 额外应用](../chapter-30-extra-apps/)",
    },
    30: {
        "dir": "chapter-30-extra-apps",
        "does": "**tview / gview / more** · **窗口关闭按钮** · **cat 建文件** 等综合示例。",
        "role": "**综合演练** — 把 FS + GUI + IPC + 多终端串成「像小型 OS 发行版」；全书功能验收。",
        "prev": "[Ch21–29 GUI/文件/IPC](../chapter-29-ipc/)",
        "next": "[Ch31 前方的路](../chapter-31-road-ahead/) — 延伸学习方向",
    },
    31: {
        "dir": "chapter-31-road-ahead",
        "does": "总结 MikanOS 已覆盖模块；讨论 **网络 / SMP / 安全启动 / 其他架构** 等延伸方向。",
        "role": "**全书收尾与路线图**（🟡）— 从「跟完书」到「自己选方向深入」；对接 Linux 内核 / DPDK / 虚拟化等 HFT 后续模块。",
        "prev": "[Ch30 额外应用](../chapter-30-extra-apps/) — 功能闭环",
        "next": "无（主线结束）；可转 [04 LKD](../../../04-Linux-Kernel-Development/) · [07 TLPI](../../../07-The-Linux-Programming-Interface/) · [14 DPDK](../../../14-Systems-Performance-2nd/)",
    },
}

MARKER = "## 本章定位 · 前后章关系"
OLD_MARKER = "## 本章定位"


def build_block(info: dict) -> str:
    return f"""{MARKER}

| | |
|---|---|
| **本章干什么** | {info["does"]} |
| **全书作用** | {info["role"]} |
| **← 前置** | {info["prev"]} |
| **→ 后续** | {info["next"]} |

---
"""


def patch_readme(path: Path, block: str) -> bool:
    text = path.read_text(encoding="utf-8")
    if MARKER in text:
        # replace existing block up to next ---
        start = text.index(MARKER)
        rest = text[start + len(MARKER) :]
        end_rel = rest.find("\n---\n")
        if end_rel == -1:
            return False
        end = start + len(MARKER) + end_rel + len("\n---\n")
        text = text[:start] + block + text[end:]
    else:
        # insert after first --- following title
        parts = text.split("---", 1)
        if len(parts) < 2:
            return False
        text = parts[0] + "---\n\n" + block + parts[1].lstrip("\n")
    path.write_text(text, encoding="utf-8")
    return True


def main() -> None:
    for num, info in sorted(CHAPTERS.items()):
        readme = ROOT / info["dir"] / "README.md"
        if not readme.exists():
            print(f"SKIP missing: {readme}")
            continue
        block = build_block(info)
        if patch_readme(readme, block):
            print(f"OK Ch{num}: {readme.name}")
        else:
            print(f"FAIL Ch{num}")


if __name__ == "__main__":
    main()
