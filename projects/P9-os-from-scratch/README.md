# P9 · OS From Scratch (拓展)

> **Phase 6 拓展** · 前置：P3 (HTTP Server) + P3.5 (BusyBox Linux) + 07 (LKD 内核入门)  
> **定位：** Phase 4 内核学完后，想"从零造一个 OS"时来翻。不在主线上，不影响 HFT / 嵌入式进度。

---

## 为什么是拓展而不是主线？

| 维度 | 主线覆盖 | 本项目补充 |
|------|----------|------------|
| OS 概念 | CSAPP (02) + TLPI (04) + LKD (07) | 从零搭建的"手感" |
| 启动链 | P3.5 BusyBox (ARM64/Linux boot) | UEFI/x86-64 启动链 |
| 中断/调度 | LKD Ch7/Ch4 (Linux 实现) | 玩具 OS 的最简实现 |
| 内存管理 | Gorman (09) + P2-B malloc | 玩具 OS 的最简实现 |
| ARM64 裸机 | P5a-P5e (树莓派 bare metal) | x86-64 对照视角 |

**结论：** 如果主线走完（CSAPP → TLPI → LKD → P3.5），你已经从 Linux 内核源码学过中断/分页/调度了。这时候翻 MikanOS 是"原来从零搭长这样"的验证，不是学新知识。

---

## 两个子模块

### 1. MikanOS (内田公太《从零自制操作系统》)

| | |
|---|---|
| **架构** | UEFI + x86-64 |
| **语言** | C++ + EDK II |
| **章节** | 31 章 + 6 附录 |
| **前置** | C 扎实 + C++ 基础（EDK II 框架） |

**章节路线图：**

| 阶段 | 章节 | 主题 |
|------|------|------|
| A 启动链 | Ch1-2 | UEFI Hello World → EDK II 内存 map |
| B 显示 | Ch3-5 | bootloader → 像素 → 文字控制台 |
| C 中断 | Ch6-7 | 鼠标/PCI → 中断 + FIFO |
| D 内存 | Ch8-9 | 内存管理 → 图层 |
| E 多任务 | Ch10-14 | 窗口 → 定时器 → 键盘 → 多任务 1/2 |
| F 应用 | Ch15-18 | 终端 → 命令 → 文件系统 → 应用 |
| G 保护 | Ch19-21 | 分页 → syscall → 内存隔离 |
| H 高级 | Ch22-31 | GUI → 文件 I/O → IPC → 展望 |

详见 [mikanos/README.md](./mikanos/README.md)

### 2. 30 天 OS 精华 (川合秀实《30 天自制操作系统》)

从 30 天 OS 的 30 天中提取了 9 个有基础 OS 概念价值的章节，其余（BIOS 启动/GUI 应用/文件系统等）已删除。

| Day | 主题 | 保留理由 |
|-----|------|----------|
| 5 | GDT/IDT | 段描述符 + 中断向量表概念 |
| 6 | PIC + ISR | 中断控制器 + 汇编 stub/C 逻辑分离 |
| 7 | FIFO 环形缓冲区 | HFT SPSC ring buffer 原型 |
| 9 | 内存管理 | 位图 vs 空闲链表 |
| 15 | 多任务/TSS | 上下文切换概念 |
| 20 | OS API | 用户态/内核态边界 |
| 21 | 内存保护 | 段级内存保护 |
| 23 | malloc | 内存分配器 |
| 27 | LDT/库 | 进程隔离 + 静态库 |

详见 [thirty-days-os-essentials/](./thirty-days-os-essentials/)

---

## 与主线模块的交叉

| 本项目概念 | 主线对照 |
|------------|----------|
| UEFI 启动链 | [P3.5 BusyBox](../P3.5-busybox-minimal-linux/) (Linux boot) · [08 嵌入式 boot](../../08-embedded-boot-build/) (U-Boot) |
| GDT/IDT/中断 | [05 LKD Ch7](../../05-linux-kernel/) (Linux 中断) · [02 CSAPP Ch8](../../02-computer-systems/) (异常控制流) |
| 分页 | [06 Gorman](../../06-linux-mm/) (Linux MM) · [02 CSAPP Ch9](../../02-computer-systems/) (VM) |
| 多任务/调度 | [05 LKD Ch4](../../05-linux-kernel/) (Linux 调度) |
| syscall | [03 TLPI](../../03-linux-userspace-api/) (用户态 API) |
| FIFO ring buffer | [P2.5 C Toolkit](../P2.5-c-toolkit/) (SPSC ring) · [18 HFT](../../18-hft-engineering/) (无锁队列) |
| malloc | [P2-B](../P2-shell-malloc/Part-B-malloc.md) (malloc 实现) |

---

## 学习建议

1. **不要现在做** — 等 Phase 4 内核（05 LKD）学完再来
2. **挑着读** — Ch1-2 (启动) + Ch7 (中断) + Ch13-14 (多任务) + Ch19-20 (分页/syscall) 是核心
3. **30 天精华当字典** — 遇到概念不懂时翻一下对照，不要逐天读
4. **代码不编译也行** — 重点是理解概念，不是跑通玩具 OS
