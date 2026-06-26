# 01 · 30 天自制操作系统

> **父模块：** [09-system-low-level-hands-on](../README.md)  
> **原著：** 川合秀实（KAWAKAMI Hideaki）· **Osliver** — 《30 天自制操作系统》  
> **本仓库：** HFT 学习链裁剪笔记 + 实验记录（非全书翻译）

---

## 这本书是什么

**《30 天自制操作系统》** 是一本 **实战型** 编程指南：用 **30 天、分步实践**，从零写出一个带 **图形界面** 和 **多任务** 能力的 **32 位操作系统**。

| 维度 | 说明 |
|------|------|
| **核心理念** | **从零开始** · **试错式学习** — 不先堆理论，边写边改、边踩坑边懂 |
| **叙述风格** | 通俗、带挑战感；目标不是背概念，而是 **亲手跑起来** |
| **配套环境** | 作者提供 **专用工具集 + 模拟器**（QEMU 等），初学者 **不必** 先啃完汇编/体系结构再动手 |
| **技术跨度** | 汇编入门 → C 语言应用 → **内存管理** → **中断处理** → **应用程序接口（API）** 设计 |

**读完能带走什么：** 不只是「会改别人的内核」，而是理解 **启动、中断、页表、任务切换** 这些在 LKD / CSAPP 里当 **读者** 时容易滑过去的 **构造逻辑** — 对 HFT 里 **绑核、上下文切换、syscall 边界** 的直觉会扎实很多。

---

## 在本学习链中的定位

从 **零启动** 写出一个能跑多任务、有中断和内存管理的最小 OS — 把 LKD / CSAPP 里「进程、中断、页表」从 **读者** 变成 **作者**。

```
08 TLPI（Linux 上怎么用 syscall / mmap）
    ↓
01 自制 OS（这些机制在裸机上怎么搭出来）
    ↓
05 LKD / 07 Gorman（对照真实 Linux 内核实现）
    ↓
15 HFT（绑核、热路径、少 syscall）
```

**标签：** 🟡 选读 · 时间紧可后补，与 `05`/`06` 概念课 **并行** 也行。

**动手前必读：** [LEARNING_PLAN.md](./LEARNING_PLAN.md) · Day 1 汇编时跟 [§1.3 装 NASM](./day-01-boot-asm/notes/section-1.3-初次体验汇编程序.md#安装-nasm替代-nask) · [SETUP.md](./SETUP.md)（QEMU / GCC / Make）

---

## 目录结构

与 [03-SysPerf](../../03-Systems-Performance-2nd/) / [05-LKD](../../05-Linux-Kernel-Development/00_Book_3rd_Notes/) 一致：每个 Day 独立目录，导读 + 分段笔记。

```
day-XX-slug/
├── README.md              ← 本日结构、小结、检查单、上下日导航
└── notes/
    └── section-N.M-标题.md
```

---

## 小节笔记

| Day | 主题 | 笔记 |
|-----|------|------|
| **1** | 从计算机结构到汇编程序入门 | [day-01-boot-asm/](./day-01-boot-asm/) |
| **2** | 汇编语言学习与 Makefile 入门 | [day-02-asm-makefile/](./day-02-asm-makefile/) |
| **3** | 进入 32 位模式并导入 C 语言 | [day-03-32bit-c/](./day-03-32bit-c/) |
| **4** | C 语言与画面显示的练习 | [day-04-c-graphics/](./day-04-c-graphics/) |
| **5** | 结构体、文字显示与 GDT/IDT 初始化 | [day-05-gdt-idt/](./day-05-gdt-idt/) |
| **6** | 分割编译与中断处理 | [day-06-split-compile-irq/](./day-06-split-compile-irq/) |
| **7** | FIFO 与鼠标控制 | [day-07-fifo-mouse/](./day-07-fifo-mouse/) |
| **8** | 鼠标控制与 32 位模式切换 | [day-08-mouse-32bit/](./day-08-mouse-32bit/) |
| **9** | 内存管理 | [day-09-memory/](./day-09-memory/) |
| **10** | 叠加处理 | [day-10-layers/](./day-10-layers/) |
| **11** | 制作窗口 | [day-11-window/](./day-11-window/) |
| **12** | 定时器（1） | [day-12-timer1/](./day-12-timer1/) |
| **13** | 定时器（2） | [day-13-timer2/](./day-13-timer2/) |
| **14** | 高分辨率及键盘输入 | [day-14-keyboard/](./day-14-keyboard/) |
| **15** | 多任务（1） | [day-15-multitask1/](./day-15-multitask1/) |
| **08** | 多任务（2） | [day-16-multitask2/](./day-16-multitask2/) |
| **17** | 命令行窗口 | [day-17-console/](./day-17-console/) |
| **18** | dir 命令 | [day-18-dir/](./day-18-dir/) |
| **19** | 应用程序 | [day-19-apps/](./day-19-apps/) |
| **20** | API | [day-20-api/](./day-20-api/) |
| **21** | 保护操作系统 | [day-21-protection/](./day-21-protection/) |
| **22** | 用 C 语言编写应用程序 | [day-22-c-apps/](./day-22-c-apps/) |
| **23** | 图形处理相关 | [day-23-graphics/](./day-23-graphics/) |
| **24** | 窗口操作 | [day-24-window-ops/](./day-24-window-ops/) |
| **25** | 增加命令行窗口 | [day-25-multi-console/](./day-25-multi-console/) |
| **26** | 为窗口移动提速 | [day-26-window-speed/](./day-26-window-speed/) |
| **27** | LDT 与库 | [day-27-ldt-lib/](./day-27-ldt-lib/) |
| **28** | 文件操作与文字显示 | [day-28-files/](./day-28-files/) |
| **29** | 压缩与简单的应用程序 | [day-29-compression/](./day-29-compression/) |
| **30** | 高级的应用程序 | [day-30-advanced-apps/](./day-30-advanced-apps/) |

完整 Day 列表见 [OUTLINE.md](./OUTLINE.md)。**全书 Day 1–30 笔记已齐。**

---

## 内容维度 · 与原书 Day 对应

| 维度 | 原书大致覆盖 | HFT 关联 |
|------|--------------|----------|
| **汇编 / 启动** | 引导扇区、实模式→保护模式 | 理解 **冷启动** 与裸机环境 |
| **C 与工具链** | 与汇编协作、链接、调试 | 热路径 **C++** 与底层边界 |
| **内存管理** | 分配器、页表、地址空间 | **TLB / 缺页 / 绑内存** 直觉 |
| **中断 / 多任务** | GDT/IDT、时钟、任务切换 | **上下文切换** 从哪来 |
| **GUI / API** | 显存、窗口、系统调用接口 | 与 **Linux syscall**、用户态 API 对照 |

### 建议阶段（三周期 · 详见 LEARNING_PLAN）

| 周期 | Day | 主题 | HFT 关联 |
|------|-----|------|----------|
| **地基** | 0–7 | NASM、引导扇区、保护模式、GDT/IDT、键鼠 FIFO | 启动链、中断 vs 轮询 |
| **完善** | 8–14 | 内存管理、图层/窗口、PIT、键盘输入 | 定时、ISR 延迟 |
| **生态** | 15–30 | 多任务、FAT/Shell、API、用户态程序 | syscall 边界、上下文切换 |

---

## 工具与环境

- **汇编：** **NASM**（全程核心；原书 nask 为作者魔改，本仓库用原版 NASM）— [TOOLCHAIN.md](./TOOLCHAIN.md)
- **C / 构建：** **GCC** + **GNU Make**；运行 **QEMU** 加载 `.img`（见 [SETUP.md](./SETUP.md)）
- **路径规范：** 工程目录 **禁止中文与空格**
- **本仓库：** 实验代码与映像放各 Day 目录下 `code/`（如 [day-01-boot-asm/code/](./day-01-boot-asm/code/)），笔记在 `day-XX-slug/notes/`
- **不必先备：** 完整 OS 理论课；**需要：** 基本 C + 简易汇编（与 [01-CSAPP Ch3](../../01-CSAPP-3rd/chapter-03-machine-level-programs/) 互补）

---

## 产出清单

- [x] `LEARNING_PLAN.md` — 三阶段标准学习方案 + 避坑
- [x] `SETUP.md` — Windows + NASM/GCC/QEMU Day 0 部署
- [x] `TOOLCHAIN.md` — NASM + GCC + Make 选型（替代 nask/bcc）
- [x] `OUTLINE.md` — 按原书 Day 裁剪 🔴/🟡/⚪（**Day 1–30**）
- [x] `day-XX-slug/` — 每日导读 + `notes/section-*.md`（**Day 1–30 ✓**）
- [x] `day-01-boot-asm/code/` — helloos 映像与十六进制对照（**Day 1 ✓**）
- [ ] 与 [05-LKD](../../05-Linux-Kernel-Development/) Ch4/7/8 对照表

---

## 交叉阅读

| 仓库 | 对照点 |
|------|--------|
| [01-CSAPP Ch8/9](../../01-CSAPP-3rd/chapter-08-exceptional-control-flow/) | 异常、进程、虚拟内存 |
| [05-LKD](../../05-Linux-Kernel-Development/) | 中断、调度、内存管理 **真实实现** |
| [08-TLPI](../../08-The-Linux-Programming-Interface/) | 用户态 API 与内核边界 |
| [02-mikan-os](../02-mikan-os/) | 现代 UEFI/64 位 OS — **01 通读后的推荐下一站** |
| [03-30days-cpu](../03-30days-cpu/) | 指令怎么被 CPU **执行**（硬件侧，可与 02 并行或后补） |
