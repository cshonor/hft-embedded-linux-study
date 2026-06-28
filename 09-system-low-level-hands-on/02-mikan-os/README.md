# 02 · MikanOS · 从零自制操作系统

**09-system-low-level-hands-on** 子模块 · [返回 09 总览](../README.md)

> **父模块：** [09-system-low-level-hands-on](../README.md)  
> **原著：** 内田公太（uchan）· **《从零自制操作系统》**（日版 *ゼロからの OS 自作入門*）  
> **本仓库：** HFT 学习链裁剪笔记 + 实验记录（非全书翻译）  
> **官方：** [zero.osdev.jp](http://zero.osdev.jp/) · 源码 [uchan-nos/os-from-zero](https://github.com/uchan-nos/os-from-zero)

---

## 这本书是什么

**《从零自制操作系统》** 是一本指导读者从头构建 **MikanOS** 的 **实战教程**，以 **「做中学」** 为核心理念。

| 维度 | 说明 |
|------|------|
| **技术路线** | 从 **UEFI BIOS 启动** 到 **64 位多任务** 系统的完整流程 |
| **核心模块** | **内存管理** · **窗口系统** · **文件系统** · **USB 驱动** 等底层技术 |
| **章节结构** | 约 **三十章** 循序渐进，把对程序员而言如同「黑箱」的 **计算机内部结构** 透明化 |
| **目标读者** | 具备 **基础编程经验** 的开发者 — 鼓励 **亲手写代码** 理解 OS **运行原理** |
| **语言栈** | **C++** + **EDK II**（与 [01 30 天](../01-30days-os/) 的 32 位 BIOS/C 路线互补） |

**读完能带走什么：** 现代 PC 上的 **UEFI 启动链、长模式、页表、APIC、系统调用** — 与 [01-CSAPP](../../01-CSAPP-3rd/chapter-09-virtual-memory/) · [05-LKD](../../05-Linux-Kernel-Development/) · [08-TLPI](../../08-The-Linux-Programming-Interface/) 对照时，不再只是「读源码」，而是知道 **这些机制从零怎么搭**。

---

## 在本学习链中的定位

> **定位：** **现代 64 位 UEFI OS** — 与 [01 30 天 OS](../01-30days-os/)（实模式 BIOS 软盘）**互补**，不替代 Linux 主线。

```
08 TLPI（Linux 上怎么用 syscall / mmap）
    ↓
01 30 天 OS（BIOS 软盘 · 实模式体感）
    ↓
02 MikanOS（UEFI · 64 位 · 分页 · syscall）← 本书
    ↓
05 LKD / 07 Gorman（对照真实 Linux 内核）
```

**标签：** 🟡 选读 · 建议 **01 通读或 Day 1–15 后** 再开。

---

## 与 01 的分工

| | **01 川合 30 天** | **02 MikanOS** |
|---|---------------------|------------------|
| 启动 | BIOS · 软盘 · 实模式 → 保护模式 | **UEFI** · GPT · **长模式** |
| 语言 | C + 汇编（nask） | **C++** + EDK II |
| 内存 | 分段/GDT · 后期分页 | **内存 map** · **页表** · 进程地址空间 |
| 中断 | PIC · IDT | **APIC** · ACPI |
| 价值 | 「上电后第一条指令」体感 | **现代 PC 启动链** + 规范分层内核 |

**推荐顺序：** 至少完成 **01 Day 1–15**（引导、GDT/IDT、中断、多任务雏形）→ 再开 MikanOS；或 **01 通读后** 整本 MikanOS 作「现代版重制」。

**交叉：** [01-CSAPP](../01-CSAPP-3rd/) Ch 9 虚拟内存 · [02-Hennessy](../02-Computer-Architecture-6th/) · [08-TLPI](../08-The-Linux-Programming-Interface/) 进程/内存 API 对照。

---

## 目录结构

与 [01 30 天 OS](../01-30days-os/) · [03-SysPerf](../../03-Systems-Performance-2nd/) 一致：每章独立目录，导读 + 分段笔记。

```
chapter-XX-slug/
├── README.md              ← 本章结构、小结、检查单、上下章导航
├── notes/
│   └── section-*.md
└── code/                  ← 对照官方 osbook_dayXX 快照（可选）
```

**动手前必读：** [LEARNING_PLAN.md](./LEARNING_PLAN.md) · 环境 [SETUP.md](./SETUP.md)（附录 A）

---

## 章节笔记

| 章 | 主题 | 笔记 |
|----|------|------|
| **0** | 个人可以制作操作系统吗 | [chapter-00-intro](./chapter-00-intro/) |
| **1** | 计算机工作原理和 Hello World | [chapter-01-hello-world](./chapter-01-hello-world/) |
| **2** | EDK II 和内存映射 | [chapter-02-edk2-memmap](./chapter-02-edk2-memmap/) |
| **3** | 屏幕显示实践和引导加载器 | [chapter-03-bootloader-display](./chapter-03-bootloader-display/) |
| **4** | 像素绘图和 make 入门 | [chapter-04-pixel-make](./chapter-04-pixel-make/) |
| **5** | 文本显示和控制台类 | [chapter-05-console-text](./chapter-05-console-text/) |
| **6** | 鼠标输入和 PCI | [chapter-06-mouse-pci](./chapter-06-mouse-pci/) |
| **7** | 中断和 FIFO | [chapter-07-interrupt-fifo](./chapter-07-interrupt-fifo/) |
| **8** | 内存管理 | [chapter-08-memory](./chapter-08-memory/) |
| **9** | 叠加过程 | [chapter-09-layers](./chapter-09-layers/) |
| **10** | 窗口 | [chapter-10-window](./chapter-10-window/) |
| **11** | 定时器和 ACPI | [chapter-11-timer-acpi](./chapter-11-timer-acpi/) |
| **12** | 键盘输入 | [chapter-12-keyboard](./chapter-12-keyboard/) |
| **13** | 多任务处理（1） | [chapter-13-multitask1](./chapter-13-multitask1/) |
| **14** | 多任务处理（2） | [chapter-14-multitask2](./chapter-14-multitask2/) |
| **15** | 终端 | [chapter-15-terminal](./chapter-15-terminal/) |
| **16** | 命令 | [chapter-16-commands](./chapter-16-commands/) |
| **17** | 文件系统 | [chapter-17-filesystem](./chapter-17-filesystem/) |
| **18** | 应用 | [chapter-18-apps](./chapter-18-apps/) |
| **19** | 分页 | [chapter-19-paging](./chapter-19-paging/) |
| **20** | 系统调用 | [chapter-20-syscall](./chapter-20-syscall/) |
| **21** | 窗口应用 | [chapter-21-window-apps](./chapter-21-window-apps/) |
| **22** | 图形和事件（1） | [chapter-22-graphics-events1](./chapter-22-graphics-events1/) |
| **23** | 图形和事件（2） | [chapter-23-graphics-events2](./chapter-23-graphics-events2/) |
| **24** | 多终端 | [chapter-24-multi-terminal](./chapter-24-multi-terminal/) |
| **25** | 使用应用读取文件 | [chapter-25-app-read-file](./chapter-25-app-read-file/) |
| **26** | 使用应用写入文件 | [chapter-26-app-write-file](./chapter-26-app-write-file/) |
| **27** | 应用的内存管理 | [chapter-27-app-memory](./chapter-27-app-memory/) |
| **28** | 日文显示和重定向 | [chapter-28-japanese-redirect](./chapter-28-japanese-redirect/) |
| **29** | 应用间通信 | [chapter-29-ipc](./chapter-29-ipc/) |
| **30** | 额外应用 | [chapter-30-extra-apps](./chapter-30-extra-apps/) |
| **31** | 前方的路 | [chapter-31-road-ahead](./chapter-31-road-ahead/) |

### 附录

| | 主题 | 笔记 |
|---|------|------|
| A | 配置开发环境 | [appendix-A-dev-env](./appendix-A-dev-env/) |
| B | 获取 MikanOS | [appendix-B-get-mikanos](./appendix-B-get-mikanos/) |
| C | EDK II 文件说明 | [appendix-C-edk2-files](./appendix-C-edk2-files/) |
| D | C++ 中的模板 | [appendix-D-cpp-templates](./appendix-D-cpp-templates/) |
| E | iPXE | [appendix-E-ipxe](./appendix-E-ipxe/) |
| F | ASCII 码表 | [appendix-F-ascii-table](./appendix-F-ascii-table/) |

完整章表与要点速览 → [OUTLINE.md](./OUTLINE.md)。**目录骨架 Ch 0–31 + 附录 A–F 已建。**

---

## 建议阶段（与 LEARNING_PLAN 一致）

| 阶段 | 章 | 主题 | HFT 关联 |
|------|-----|------|----------|
| **启动链** | 0–2 | UEFI · EDK II · 内存 map | 现代 PC 启动 |
| **内核骨架** | 7–8, 11, 13–14 | 中断 · 内存 · 定时器 · 多任务 | 上下文切换直觉 |
| **分页 syscall** | 19–20 | 页表 · 系统调用 | CSAPP / LKD / TLPI |
| **生态** | 17, 25–29 | FS · 文件 I/O · IPC | 用户态边界 |

---

## 产出清单

- [x] `OUTLINE.md` — 第 0–31 章 + 附录 A–F
- [x] `LEARNING_PLAN.md` · `SETUP.md`
- [x] `chapter-XX-slug/` — Ch 0–31 导读骨架
- [x] `appendix-A` … `appendix-F` 导读骨架
- [ ] 各章 `notes/` 正文（随学习增量填写）
- [x] [Ch 1 Hello World](./chapter-01-hello-world/) — 6 小节笔记
- [x] [Ch 2 EDK II / memmap](./chapter-02-edk2-memmap/) — 6 小节笔记
- [x] [Ch 3 Loader / GOP / kernel.elf](./chapter-03-bootloader-display/) — 6 小节笔记
- [x] [Ch 4 像素 / make / ELF](./chapter-04-pixel-make/) — 6 小节笔记
- [x] [Ch 5 文本 / Console / printk](./chapter-05-console-text/) — 6 小节笔记
- [x] [Ch 6 鼠标 / PCI / xHCI](./chapter-06-mouse-pci/) — 6 小节笔记
- [x] [Ch 7 中断 / FIFO](./chapter-07-interrupt-fifo/) — 6 小节笔记
- [x] [Ch 8 内存管理](./chapter-08-memory/) — 6 小节笔记
- [x] [Ch 9 图层 / Shadow Buffer](./chapter-09-layers/) — 6 小节笔记
- [x] [Ch 10 窗口 / Back Buffer](./chapter-10-window/) — 6 小节笔记
- [x] [Ch 11 定时器 / ACPI](./chapter-11-timer-acpi/) — 6 小节笔记
- [x] [Ch 12 键盘输入](./chapter-12-keyboard/) — 6 小节笔记
- [x] [Ch 13 多任务（1）](./chapter-13-multitask1/) — 6 小节笔记

---

## 进度（学习自检）

- [ ] 环境 [SETUP.md](./SETUP.md) / [appendix-A-dev-env](./appendix-A-dev-env/)
- [ ] [Ch 0–2](./chapter-00-intro/) UEFI + 内存 map
- [ ] [Ch 7–8](./chapter-07-interrupt-fifo/) 中断 + 内存管理
- [ ] [Ch 13–14](./chapter-13-multitask1/) 多任务
- [ ] [Ch 19–20](./chapter-19-paging/) 分页 + 系统调用
- [ ] [Ch 29](./chapter-29-ipc/) 应用间通信

---

## 交叉阅读

| 仓库 | 对照点 |
|------|--------|
| [01 30 天 OS](../01-30days-os/) | BIOS 实模式 → 保护模式（建议先读 Day 1–15） |
| [01-CSAPP Ch9](../../01-CSAPP-3rd/chapter-09-virtual-memory/) | 虚拟内存 |
| [05-LKD](../../05-Linux-Kernel-Development/) | 中断、调度、syscall 真实实现 |
| [08-TLPI](../../08-The-Linux-Programming-Interface/) | 用户态 API 边界 |
