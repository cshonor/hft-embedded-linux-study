## ② 内核源码树 · The Kernel Source Tree

按 **功能子系统** 划分的顶层目录 — 读 LKD / 对照源码时的 **根导航**（原独立「目录地图」已并入本节）。

#### 读代码标准路线

```
include/linux/     ← 结构体 / API 合同
       │
       ▼
kernel/  mm/  net/  fs/   ← 子系统实现
       │
       ▼
arch/<arch>/  ·  drivers/  ← 架构细节 / 某硬件
```

**习惯：先头文件，再实现。**

#### 顶层速查

| 目录 | 内容 | 你常问的 |
|------|------|----------|
| **`arch/`** | 架构相关：x86、**arm64**、引导、平台初始化 | 树莓派 ARM64 相关 |
| **`drivers/`** | 设备驱动（体量最大） | 嵌入式驱动主战场 |
| **`fs/`** | **VFS** + ext4、tmpfs… | 文件系统 |
| **`include/`** | 头文件：声明、结构体、宏 | **先看头、再看 .c** |
| **`kernel/`** | 核心：调度、信号、锁、定时器… | **HFT 抖动地图** |
| **`mm/`** | 页、slab、VMA、缺页… | 双线重中之重 |
| **`net/`** | TCP/IP 协议栈 | **HFT 极高权重** |
| **`ipc/`** | 共享内存、消息队列、信号量 | IPC |
| **`lib/`** | 内核自用库函数 | 工具 |
| **`init/`** | 启动；**`start_kernel`** | 上电路线 |

#### 双线优先级

| 优先级 | 目录 |
|--------|------|
| **必精读（通用）** | `kernel/` · `mm/` · `net/` · `include/linux/` |
| **嵌入式加重** | `drivers/` · `arch/arm64/` |
| **按需** | `fs/` · `ipc/` · `lib/` · `init/` |

#### 顶层目录 ↔ LKD 章节

| 源码目录 | LKD 主章 | 笔记入口 |
|----------|----------|----------|
| 获取/树概览 | **Ch 2** | 本节 · [§2.1](./section-2.1-获取内核源码.md) |
| **`kernel/`** 进程 | **Ch 3** | [../chapter-03-process-management/](../chapter-03-process-management/) |
| **`kernel/`** 调度 | **Ch 4** | [../chapter-04-process-scheduling/](../chapter-04-process-scheduling/) |
| syscall | **Ch 5** | [../chapter-05-system-calls/](../chapter-05-system-calls/) |
| 数据结构 | **Ch 6** | [../chapter-06-kernel-data-structures/](../chapter-06-kernel-data-structures/) |
| 中断 / 下半部 | **Ch 7–8** | [../chapter-07-interrupts/](../chapter-07-interrupts/) · [../chapter-08-bottom-halves/](../chapter-08-bottom-halves/) |
| 同步 | **Ch 9–10** | [../chapter-09-kernel-sync-intro/](../chapter-09-kernel-sync-intro/) · [../chapter-10-sync-methods/](../chapter-10-sync-methods/) |
| 定时器 | **Ch 11** | [../chapter-11-timers/](../chapter-11-timers/) |
| **`mm/`** | **Ch 12 · 15** | [../chapter-12-memory-management/](../chapter-12-memory-management/) · [../chapter-15-process-address-space/](../chapter-15-process-address-space/) |
| **`fs/`** | **Ch 13** | [../chapter-13-vfs/](../chapter-13-vfs/) |
| 块 I/O | **Ch 14** | [../chapter-14-block-io/](../chapter-14-block-io/) |
| 页缓存 | **Ch 16** | [../chapter-16-page-cache/](../chapter-16-page-cache/) |
| 设备/模块 | **Ch 17** | [../chapter-17-devices-modules/](../chapter-17-devices-modules/) |
| **`net/`** | 书内浅 | → [13 Rosen](../../../12-kernel-networking/) |
| **`mm/`** 深挖 | Gorman | → [06 Gorman](../../../../06-linux-mm/) |

#### 源码版本与放置

| 项 | 定论 |
|----|------|
| 书本对照 | **2.6.34**（可选考古树） |
| 本仓库主树 | **linux-7.1.5**（Desktop，见 [§2.1](./section-2.1-获取内核源码.md)） |
| 放置 | 用户目录；**勿** `/usr/src/linux` |

**Phase：** 主用于 **Phase4 读 LKD**；Phase1–2 收藏即可。→ [README](../../../../README.md)

镜像如何被 UEFI 加载 → [§2.5](./section-2.5-ELF与UEFI启动链路.md)



<details>
<summary>自测题（点击展开）</summary>

**Q1.** 内核源码树中 kernel/、mm/、fs/、net/ 分别对应什么子系统？

<details><summary>答案</summary>

kernel/ = 进程调度/信号/时间/线程；mm/ = 内存管理（buddy/slab/page fault）；fs/ = VFS 和各文件系统（ext4/sysfs/proc）；net/ = 网络协议栈（socket/TCP/IP/netfilter）。HFT 工程师最常读 net/ 和 kernel/sched/。

</details>

**Q2.** arch/ 目录为什么有这么多子目录？它解决什么问题？

<details><summary>答案</summary>

Linux 支持几十种 CPU 架构（x86/arm64/riscv/...），arch/ 下每个子目录放架构相关代码：中断入口、TLB 刷新、原子操作、页表格式。可移植代码在顶层目录，通过 `#include <asm/xxx.h>` 间接调用 arch 实现。这就是 Linux 能跑在从手表到超算的原因。

</details>

</details>
---
