# 内核源码顶层目录 · 导航图（LKD 配套）

> **用途：** 读 Linux 内核源码时的 **根目录功能速查**；与 《Linux Kernel Development》3rd 配套。  
> **章节笔记：** [Ch2 §2.2](./chapter-02-getting-started/notes/section-2.2-内核源码树.md) · 获取源码：[§2.1](./chapter-02-getting-started/notes/section-2.1-获取内核源码.md)  
> **仓库 Phase：** LKD = [LEARNING-PATH-LOCKED](../../LEARNING-PATH-LOCKED.md) **Phase4**（先 Harris→C→CSAPP→TLPI/网络，再啃本图对照源码）

---

## 读代码标准路线

```
include/linux/     （结构体、API、宏 — 先建立合同）
        │
        ▼
kernel/  mm/  net/  fs/   （子系统实现）
        │
        ▼
arch/<arch>/  drivers/    （架构细节 / 某块硬件驱动）
```

**习惯：先头文件，再实现。** 不要从 `drivers/` 某一 `.c` 盲跳。

---

## 顶层目录逐项

| 目录 | 做什么 | 嵌入式 | HFT |
|------|--------|:------:|:---:|
| **`arch/`** | 架构专属：引导、底层汇编、平台初始化（x86 / **arm64**…） | 🔥 `arch/arm64/` | 绑核/原子/屏障实现常在这 |
| **`drivers/`** | 体量最大：网卡、显示、GPU、传感器、块设备… | 🔥 日常主战场 | 网卡/DMA 路径会碰到 |
| **`fs/`** | VFS + ext4 / tmpfs… | 需要时查 | 一般非热路径 |
| **`include/`** | 头文件：声明、结构体、宏 | 🔥 必经 | 🔥 必经 |
| **`kernel/`** | 调度、信号、同步、定时器核心 | 🔥 | 🔥 **抖动根源地图** |
| **`mm/`** | 页、slab、VMA、缺页… | 🔥 | 🔥 **大页/mlock/分配** |
| **`net/`** | TCP/IP 协议栈 | 网关/设备侧 | 🔥🔥 **UDP/TCP/组播** |
| **`ipc/`** | SysV/POSIX 共享内存、消息队列、信号量 | 按需 | 用户态 IPC 对照 |
| **`lib/`** | 内核内部通用库函数 | 按需 | 按需 |
| **`init/`** | 启动；入口 **`start_kernel`** | 启动链兴趣 | 了解即可 |

### 一句话版

1. **`arch/`** — 树莓派 / ARM64 上电与平台相关代码在这。  
2. **`drivers/`** — 嵌入式驱动开发最常打开。  
3. **`fs/`** — 文件系统与 VFS。  
4. **`include/`** — 读源码先看这里。  
5. **`kernel/`** — OS 最核心；调度延迟看这里。  
6. **`mm/`** — 双线重中之重。  
7. **`net/`** — HFT 权重极高。  
8. **`ipc/`** — 进程间通信。  
9. **`lib/`** — 内部工具函数。  
10. **`init/`** — `start_kernel` 启动主路径。

---

## 优先级（嵌入式 Linux + HFT）

### 必精读（两条路线通用）

| 目录 | 为何 |
|------|------|
| **`kernel/`** | 调度、锁、定时器 — 延迟与正确性共同地基 |
| **`mm/`** | 页/slab/地址空间 — 驱动 DMA、HFT 大页/`mlock` |
| **`net/`** | 协议栈 — HFT 权重最高；嵌入式网关也刚需 |
| **`include/linux/`** | 所有精读的「目录」 |

### 嵌入式驱动加重

| 目录 | 为何 |
|------|------|
| **`drivers/`** | 字符/块/平台驱动、总线 |
| **`arch/arm64/`** | 启动、异常、cache/TLB、原子实现 |

### 可后置 / 按需

`fs/`（非存储主业时）、`ipc/`、`lib/`、`init/`（搞清 `start_kernel` 即可）、其它 `arch/*`。

---

## 顶层目录 ↔ LKD 章节

| 源码目录 | LKD 主章（3rd） | 笔记入口 |
|----------|----------------|----------|
| （获取/树概览） | **Ch 2** Getting Started | [chapter-02](./chapter-02-getting-started/) |
| **`kernel/`**（进程） | **Ch 3** Process Management | [chapter-03](./chapter-03-process-management/) |
| **`kernel/`**（调度） | **Ch 4** Process Scheduling | [chapter-04](./chapter-04-process-scheduling/) |
| **`kernel/`** + syscall 入口 | **Ch 5** System Calls | [chapter-05](./chapter-05-system-calls/) |
| **`include/`** + **`lib/`** 数据结构 | **Ch 6** Kernel Data Structures | [chapter-06](./chapter-06-kernel-data-structures/) |
| **`kernel/`** + **`arch/*/kernel/`** 中断 | **Ch 7–8** IRQ / Bottom Halves | [chapter-07](./chapter-07-interrupts/) · [chapter-08](./chapter-08-bottom-halves/) |
| **`kernel/`** 同步 | **Ch 9–10** Sync | [chapter-09](./chapter-09-kernel-sync-intro/) · [chapter-10](./chapter-10-sync-methods/) |
| **`kernel/`** 时间 | **Ch 11** Timers | [chapter-11](./chapter-11-timers/) |
| **`mm/`** | **Ch 12** Memory Management | [chapter-12](./chapter-12-memory-management/) |
| **`fs/`** | **Ch 13** VFS | [chapter-13](./chapter-13-vfs/) |
| **`block/`** · 块层 | **Ch 14** Block I/O | [chapter-14](./chapter-14-block-io/) |
| **`mm/`**（进程地址空间） | **Ch 15** Process Address Space | [chapter-15](./chapter-15-process-address-space/) |
| **`mm/`** + 回写 | **Ch 16** Page Cache | [chapter-16](./chapter-16-page-cache/) |
| **`drivers/`** · **`include/linux/`** 模块 | **Ch 17** Devices and Modules | [chapter-17](./chapter-17-devices-modules/) |
| （调试手段） | **Ch 18** Debugging | [chapter-18](./chapter-18-debugging/) |
| **`arch/`** 可移植性 | **Ch 19** Portability | [chapter-19](./chapter-19-portability/) |
| 根目录 `MAINTAINERS` 等 | **Ch 20** Patches / Community | [chapter-20](./chapter-20-patches-community/) |
| **`net/`** | LKD 几乎不讲深 | → [13-Linux-Kernel-Networking](../../13-Linux-Kernel-Networking/)（Phase5B） |
| **`mm/`** 深挖 | LKD Ch12/15 入门 | → [06-Gorman](../../06-Linux-Virtual-Memory-Manager/)（与 LKD 同步） |

HFT 精读章顺序仍见 [OUTLINE.md](./OUTLINE.md)：`4 → 7–8 → 9–10 → 11`（+ 3/12/15 选读）。

---

## 和锁定学习顺序怎么用这份图

| 阶段 | 怎么用本图 |
|------|------------|
| **现在 Phase1–2**（Harris / C / CSAPP） | **不必** 沉进 `drivers/`；最多当「地图收藏」 |
| **Phase3**（TLPI + 网络） | 用户态概念齐了，再对照 `include`/`net` 名词 |
| **Phase4**（**LKD + Gorman**） | **主用本图** — 每读一章就打开对应目录 |
| **Phase5A** 嵌入式 | 加重 `drivers/` + `arch/arm64/` |
| **Phase5B** HFT | 加重 `net/` + `kernel/`/`mm/` 热路径 |
| **Phase6 ULK** | 高频对照源码；本文件当书签首页 |

> **纠正常见说法：** 不是「先啃完 5 本嵌入式再碰 LKD」。按 [LEARNING-PATH-LOCKED](../../LEARNING-PATH-LOCKED.md)，**LKD 在 Phase4**，嵌入式专书 `19–23` 在 **Phase5A**（内核地图建立之后）。

---

## 源码放哪 · 下哪个版本

| 建议 | 原因 |
|------|------|
| 用户目录（如 `~/linux-*`、Windows `Desktop\linux-*`） | 开发不必 root |
| **不要** `/usr/src/linux` | 避免污染系统树、误链发行版头文件 |
| **书本时代** | LKD 3rd = **2.6.34**（考古对照可选） |
| **本仓库主树** | **linux-7.1.5**（已下到 Desktop并验收） |

下载顺序、踩坑、验收清单 → [§2.1 获取内核源码](./chapter-02-getting-started/notes/section-2.1-获取内核源码.md)

---

→ [OUTLINE](./OUTLINE.md) · [04 README](../README.md) · [Ch2 导读](./chapter-02-getting-started/)
