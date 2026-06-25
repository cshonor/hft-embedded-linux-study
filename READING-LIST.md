# HFT 必读书目与章节精读清单

本清单锁定 **9 本**英文原版（含 CSAPP、TLPI，已剔除 *Programming Rust*），以及 **外部仓库书目**（PNP、UNP、TCP/IP），按 HFT 低延迟学习先后排序。

**总阅读顺序** → [HFT-READING-ROADMAP.md](./HFT-READING-ROADMAP.md) · **文件夹顺序** → [LEARNING-CHAIN.md](./LEARNING-CHAIN.md)

**按文件夹读：** `00 → 01 → 02 → 03 → 04 → 05 → 06 → 07 → 08 → 09 → 10 → 11 → 12 → 13 → 14 → 15 → 16`

| 标签 | 含义 |
|------|------|
| **精读** | 与延迟、抖动、内存、网络、撮合直接相关 |
| **选读** | 有上下文价值，时间紧可后补 |
| **跳过** | 与 HFT 热路径无关 |

---

## 1. Systems Performance: Enterprise and the Cloud 2nd — Brendan Gregg

> 笔记目录：[02-Systems-Performance-2nd/](./02-Systems-Performance-2nd/)

> **建议前置：** [01-CSAPP-3rd](./01-CSAPP-3rd/) 地基篇（Ch4–6/8–9/12）+ [04-Hennessy](./04-Computer-Architecture-6th/) Ch2。  
> 性能调优总纲：perf、NUMA、软中断、网卡调优 — **在懂 cache/进程/锁之后再读，事半功倍**。

| 章节 | 标签 | HFT 关联 |
|------|------|----------|
| Ch 1 简介 / Ch 2 方法论 | **精读** | USE/RED、延迟分解、perf 思维框架 |
| Ch 4 观测工具 | **精读** | 工具选型、采样 vs 跟踪 |
| Ch 6 CPU | **精读** | 调度、run queue、绑核、NUMA、context switch |
| Ch 7 Memory | **精读** | TLB、page fault、NUMA 本地性、THP |
| Ch 10 Network | **精读** | 软中断、NAPI、RSS、队列、网卡调优、TCP/UDP 栈 |
| Ch 13 perf / Ch 15 BPF | **精读** | 生产级 profiling 与 eBPF 观测 |
| Ch 3 OS / Ch 5 Applications | **选读** | 内核与用户态背景 |
| Ch 12 Benchmarking / Ch 14 Ftrace / Ch 16 Case Study | **选读** | 压测方法论、跟踪、案例 |
| Ch 8 File Systems / Ch 9 Disks / Ch 11 Cloud | **跳过** | 除非持久化日志/审计；云章节跳过 |
| 附录 A USE (Linux) / 附录 C bpftrace | **精读** | 速查清单与单行观测 |

**HFT 产出：** 建立「延迟从哪来 → 怎么量 → 怎么调」的总框架。

---

## 2. Linux Kernel Development 3rd — Robert Love

> 笔记目录：[05-Linux-Kernel-Development/](./05-Linux-Kernel-Development/) · 书本 [00_Book_3rd_Notes](./05-Linux-Kernel-Development/00_Book_3rd_Notes/)

> **推荐顺序：** [01 LFS](./05-Linux-Kernel-Development/01_Course_LFS/) → [02 内核编程 6 集](./05-Linux-Kernel-Development/02_Course_Kernel_7Lectures/) → [03 架构理论](./05-Linux-Kernel-Development/03_Course_Kernel_Architecture/) → **书本通读**。详见 [LEARNING-PATH.md](./05-Linux-Kernel-Development/LEARNING-PATH.md)。

> 内核调度、中断、CFS、CPU 隔离、绑核底层原理。

| 章节 | 标签 | HFT 关联 |
|------|------|----------|
| Ch 4 Process Scheduling | **精读** | CFS、RT 调度、`SCHED_FIFO`、CPU affinity |
| Ch 7 Interrupts and Bottom Halves | **精读** | 硬中断、软中断、延迟来源 |
| Ch 8 Deferred Work | **精读** | softirq、tasklet、workqueue 对抖动的影响 |
| Ch 9–10 Kernel Synchronization | **精读** | spinlock、preempt、RCU 基础 |
| Ch 10 Timers and Time Management | **精读** | hrtimer、时钟源、`CLOCK_MONOTONIC` |
| Ch 3 Process Management | **选读** | fork/线程模型背景 |
| Ch 11 Memory Management（概述） | **选读** | 为 Gorman 书铺垫，不深读 |
| Ch 12–18 VFS / Block / Page Cache | **跳过** | 交易热路径不走磁盘 |
| Ch 2 Getting Started / Ch 20 Patch | **跳过** | |

**HFT 产出：** 理解「绑核、隔离、中断」在内核里怎么实现。

---

## 2b. Understanding the Linux Kernel 3rd — Bovet & Cesati

> 笔记目录：[08-Understanding-Linux-Kernel/](./08-Understanding-Linux-Kernel/)

> **定位：** LKD **功能** ↔ 源码 **实现** 的桥梁（Linux 2.6 基线；概念对照 modern 5.x/6.x）。  
> **顺序：** `05` LKD 后 **选章精读**；VM 深潜仍交给 `06` Gorman。

| 章节 | 标签 | HFT 关联 |
|------|------|----------|
| Ch 2 内存寻址 / Ch 8–9 MM 与地址空间 | **精读** | 分页、页表 — 接 Gorman / CSAPP |
| Ch 4 中断 / Ch 5 同步 / Ch 7 调度 | **精读** | 延迟、绑核、runqueue 数据结构 |
| Ch 10 系统调用 | **精读** | 接 TLPI · 08 MikanOS Ch 20 |
| Ch 19 IPC / Ch 20 程序执行 | **选读** | 远期 IPC 模块铺垫 |
| Ch 12–18 VFS/块/Ext2 | **跳过** | 非热路径 |

**HFT 产出：** 读 `perf`/BPF 栈时能对应到内核真实数据结构与路径。

---

## 3. Understanding the Linux Virtual Memory Manager — Mel Gorman

> 笔记目录：[06-Linux-Virtual-Memory-Manager/](./06-Linux-Virtual-Memory-Manager/)

> Linux 虚拟内存、slab、THP、NUMA 内存、伪共享，内存池/订单簿优化。

| 章节 | 标签 | HFT 关联 |
|------|------|----------|
| Ch 2 描述物理内存 / Zones / NUMA | **精读** | 本地内存分配、跨 NUMA 代价 |
| Ch 3 页表管理 / TLB / 大页 | **精读** | TLB miss 对热路径影响；见 note-透明大页THP |
| Ch 8 Slab / Slub 分配器 | **精读** | 内核/用户态内存池设计参照 |
| Ch 4 进程地址空间 | **选读** | mmap、缺页 fault |
| Ch 6 物理页分配 / Ch 10 页框回收 | **选读** | 预分配策略；避免运行时回收抖动 |
| Ch 12 共享内存 | **选读** | 跨进程共享订单簿场景 |
| Ch 1 简介 | **选读** | VM 子系统总览 |
| Ch 5/7/9/11/13/14 | **跳过** | 启动分配器、vmalloc、高端内存、Swap、OOM |
| 附录 B/C/D/H | **选读** | 对应章节的内核代码走读 |

**HFT 产出：** 订单簿/内存池布局、NUMA 绑内存、伪共享（配合 Hennessy Ch2）的理论依据。

---

## 7. The Linux Programming Interface — Michael Kerrisk

> 笔记目录：[07-The-Linux-Programming-Interface/](./07-The-Linux-Programming-Interface/) · 章节裁剪 → [OUTLINE.md](./07-The-Linux-Programming-Interface/OUTLINE.md)

> **定位：** Linux **用户态** syscall 全书 — 衔接 `05` LKD（内核里怎么实现）与 `09` PNP / `10` UNP（网络实战）。  
> **推荐顺序：** `06` Gorman 之后 → **TLPI** → `09` 自制系统 → 网络段。

| 章节 | 标签 | HFT 关联 |
|------|------|----------|
| Ch 2 系统调用 API | **精读** | errno、用户/内核边界 |
| Ch 6 进程 | **精读** | `fork`、进程模型 |
| Ch 20–21 信号 | **精读** | `sigaction`、热路径屏蔽 |
| Ch 34–37 优先级 / 调度 | **精读** | `SCHED_FIFO`、nice |
| Ch 49 内存映射 | **精读** | `mmap`、共享内存、大页 |
| Ch 29–30 线程 | **精读** | pthread、同步 baseline |
| Ch 58–61 Socket | **精读** | 非阻塞、socket 选项 |
| **Ch 63 备选 I/O** | **精读** | **epoll** — 多路行情 |
| Ch 64 高级 I/O | **选读** | `eventfd`、`timerfd` |
| Ch 50 虚拟内存操作 | **选读** | `mlock` 防 swap |
| Ch 44–48 / 51–55 IPC | **选读** | 管道、POSIX shm |
| 文件属性、凭证、大部分 IPC 细节 | **跳过** | 非热路径 |

**HFT 产出：** 读懂 `epoll_wait`、`mmap`、`mlock`、RT 调度 — PNP/UNP 实验的 Linux 底座。

---

## 4. Linux Kernel Networking — Rami Rosen

> 笔记目录：[13-Linux-Kernel-Networking/](./13-Linux-Kernel-Networking/)

> 内核 TCP/UDP/IGMP/NAPI/RSS，交易所 UDP 组播内核实现，对接 UNP。

| 章节 | 标签 | HFT 关联 |
|------|------|----------|
| Ch 11 第 4 层协议（TCP/UDP/Socket/sk_buff） | **精读** | 对照 UNP 之下发生了什么 |
| Ch 14 高级主题（NAPI/softirq/RSS/RPS/XPS） | **精读** | 收包延迟、多队列绑核 |
| 组播 / IGMP | **精读** | 见 [note-组播IGMP.md](./13-Linux-Kernel-Networking/note-组播IGMP.md) |
| Ch 4–5 IPv4 / 路由 | **选读** | 托管/共置网络 |
| Ch 3 ICMP / Ch 7 邻居子系统 | **选读** | 排查网络 |
| Ch 13 InfiniBand | **选读** | RDMA/共置低延迟 |
| Ch 1 简介 / 附录 A–B | **选读** | 背景与 API 速查 |
| Ch 2 Netlink / Ch 8 IPv6 / Ch 9 Netfilter / Ch 10 IPsec / Ch 12 无线 | **跳过** | HFT 通常旁路或不用 |

**HFT 产出：** 从网卡 DMA → NAPI → 用户态 socket 的完整内核路径；与 UNP、DPDK 对照。

---

## 12. DPDK — 用户态旁路网络（官方文档 + 本仓库笔记）

> 笔记目录：[14-DPDK-Low-Latency-Network/](./14-DPDK-Low-Latency-Network/)

> **网络栈闭环最后一环：** 内核协议原理（04）→ Socket API（05）→ 内核实现（06）→ **用户态旁路（12）**。与 UNP/TCP/IP **并行互补**，不是重复。

| 主题 | 标签 | HFT 关联 |
|------|------|----------|
| EAL、大页、NUMA、lcore 绑定 | **精读** | DPDK 环境基础 |
| mbuf、mempool、ring | **精读** | 预分配、零 malloc 热路径 |
| PMD、poll mode、rx/tx burst | **精读** | 轮询收包 vs NAPI |
| 零拷贝、UIO/VFIO | **精读** | 旁路内核栈原理 |
| UDP 组播行情接入 | **精读** | 交易所行情主路径 |
| OpenOnload / RDMA 对比 | **选读** | 方案选型（见 [02-Advanced/note-openonload-rdma对比](./14-DPDK-Low-Latency-Network/02-Advanced-Book/notes/note-openonload-rdma对比.md)） |
| Crypto / Eventdev 等 | **跳过** | 非行情热路径 |

**HFT 产出：** 理解内核栈 vs DPDK 旁路取舍；组播行情最小工程见 `01-Intro-Book/code/mcast-minimal/`。

**交叉阅读：** [CROSS-MODULE-GUIDE.md](./CROSS-MODULE-GUIDE.md) · 对照 [08-UNP](./11-UNP-Vol1/) · [01-CSAPP Ch6/Ch11](./01-CSAPP-3rd/)

---

## 5. Computer Architecture: A Quantitative Approach 6th — Hennessy & Patterson

> 笔记目录：[04-Computer-Architecture-6th/](./04-Computer-Architecture-6th/)

> CPU 缓存、MESI、NUMA 访存、流水线，无锁代码硬件优化依据。

| 章节 | 标签 | HFT 关联 |
|------|------|----------|
| Ch 2 存储器层次结构设计 | **精读** | Cache line、MESI、false sharing、NUMA |
| Ch 5 线程级并行 / 内存一致性 | **精读** | 无锁、memory order、store buffer |
| Ch 1 量化设计与分析基础 / Roofline | **选读** | 性能上限直觉 |
| Ch 3 指令级并行（分支预测等） | **选读** | 热循环微优化 |
| 附录 B/C、在线附录 L | **选读** | 存储层次/流水线/地址转换复习 |
| Ch 4 SIMD/GPU / Ch 6–7 仓储·领域架构 | **跳过** | 除非 SIMD 行情解析 |
| 在线附录 D/E/G–K/M | **跳过** | 非 HFT 热路径 |

**HFT 产出：** 无锁队列、订单簿数组布局、cache-line padding 的硬件依据。

---

## 6. Computer Systems: A Programmer's Perspective 3rd — Bryant & O'Neill

> 笔记目录：[01-CSAPP-3rd/](./01-CSAPP-3rd/)

> **HFT 分两遍读：** **① 地基篇（SysPerf 之前）** — 程序如何在硬件上跑；**② 网络篇（阶段 5）** — Ch10–11 衔接 UNP。Hennessy 理论 → CSAPP 程序员落地。

| 章节 | 标签 | HFT 关联 |
|------|------|----------|
| Ch 1 A Tour of Computer Systems | **选读** | 程序生命周期、Amdahl 定律 |
| Ch 3 Machine-Level Programs | **选读** | 热路径反汇编、栈帧 |
| Ch 4 Processor Architecture | **精读** | 流水线、冒险、分支预测 |
| Ch 5 Optimizing Program Performance | **精读** | 编译优化、循环、消除内存引用 |
| **Ch 6 The Memory Hierarchy** | **精读** | 局部性、Cache 行、伪共享 |
| **Ch 9 Virtual Memory** | **精读** | 页表、TLB、mmap、大页 |
| Ch 10 System-Level I/O | **选读** | epoll、非阻塞 I/O |
| **Ch 11 Network Programming** | **精读** | Socket、TCP/UDP、并发服务器 |
| **Ch 12 Concurrent Programming** | **精读** | 线程、互斥、无锁铺垫 |
| Ch 2 Information Representation | **跳过** | 除非做二进制协议解析 |
| Ch 7 Linking / Ch 8 ECF | **选读→地基** | Ch8 进程/syscall/信号 — **SysPerf 之前建议读**；Ch7 链接非热路径 |

**HFT 产出：** 从程序员角度落地 Hennessy 的缓存/一致性理论；并发与网络编程直接对接引擎开发。

---

## 7. Trading and Exchanges — Larry Harris

> 笔记目录：[00-Trading-and-Exchanges/](./00-Trading-and-Exchanges/)

> 市场微观、LOB、交易所撮合、机房托管，HFT 业务基石。

| 章节 | 标签 | HFT 关联 |
|------|------|----------|
| 市场结构 / 交易功能 / 参与者 | **精读** | HFT 在生态中的位置 |
| **订单 / 订单类型 / LOB** | **精读** | 订单簿、撮合逻辑业务基础 |
| 交易规则 / 透明度 / 监管（概述） | **选读** | 合规边界 |
| 经纪 / 清算 / 结算 | **选读** | 实盘对接需要时再读 |
| 估值 / 组合管理 | **跳过** | 偏 buy-side PM，非 HFT 核心 |

**HFT 产出：** 读 LOB 代码、设计撮合/做市策略时的业务语言。

---

## 8. BPF Performance Tools — Brendan Gregg

> 笔记目录：[03-BPF-Performance-Tools/](./03-BPF-Performance-Tools/)

> eBPF、XDP 小包过滤、内核观测。**紧接 [02-SysPerf](../02-Systems-Performance-2nd/) 阅读**（Gregg 性能双书第二本；不必等内核/网络全书）。

| 章节 | 标签 | HFT 关联 |
|------|------|----------|
| Part I Ch 1–2 简介 / 技术背景 | **精读** | BPF/eBPF、kprobes/uprobes/火焰图 |
| Part I Ch 4–5 BCC / bpftrace | **精读** | 工具链快速上手 |
| Part II Ch 6 CPU | **精读** | off-CPU、run queue、抖动定位 |
| Part II Ch 10 网络 | **精读** | 套接字延迟、TCP/UDP 重传丢包 |
| XDP / tc-BPF | **精读** | 见 [note-XDP与tc-BPF.md](./03-BPF-Performance-Tools/note-XDP与tc-BPF.md)；vs DPDK |
| Part II Ch 7 内存 / Ch 13–14 应用·内核 | **选读** | alloc、fault、调度唤醒 |
| Part I Ch 3 / Part III Ch 17–18 | **选读** | 方法论、排障技巧 |
| 附录 A/B bpftrace 单行/备忘单 | **精读** | 现场速查 |
| Part II Ch 8–9 文件系统/磁盘 / Ch 11–12 安全/语言 / Ch 15–16 容器/虚拟化 | **跳过** | HFT 热路径无关 |

**HFT 产出：** 生产环境 eBPF 观测；与 DPDK 文档配合做「内核栈 vs 用户态旁路」对比。

---

## 外部书目（笔记在另一仓库 · 本仓库仅索引）

| 外 | 书目 | 索引 | 插入顺序 |
|----|------|------|----------|
| 外P | 陈硕 PNP / muduo 实战 | [10-Practical-Network-Programming/](./10-Practical-Network-Programming/) · [PNP 笔记](https://github.com/cshonor/Computer-Networking/tree/main/PNP) | `07` 自制系统 **之后**，UNP **之前** |
| 外B | UNIX Network Programming Vol.1 — Stevens | [11-UNP-Vol1/](./11-UNP-Vol1/) · [笔记](https://github.com/cshonor/Computer-Networking/tree/main/UNP_Vol1) | PNP **之后** |
| 外A | TCP/IP Illustrated Vol.1 — Stevens | [12-TCP-IP-Illustrated-Vol1/](./12-TCP-IP-Illustrated-Vol1/) · [笔记](https://github.com/cshonor/Computer-Networking/tree/main/TCP-IP-Volume1-Protocols) | UNP **之后**，Rosen **之前** |

> **不要整本迁入本仓库。** 分工：**PNP** = 动手实验；**UNP** = API；**TCP/IP** = 协议；**Rosen** = 内核实现。详见 [HFT-READING-ROADMAP.md](./HFT-READING-ROADMAP.md)。

---

## 补充：陈硕 PNP / muduo 实战（`09` 文件夹）

> 笔记目录：[10-Practical-Network-Programming/](./10-Practical-Network-Programming/) · 外部 [PNP/](https://github.com/cshonor/Computer-Networking/tree/main/PNP)

> **插入位置：** `07` TLPI 之后、`10` UNP 之前

| 主题 | 标签 | HFT 关联 |
|------|------|----------|
| epoll / 多路复用实验 | **精读** | 多路行情接入骨架 |
| 粘包 / 缓冲区 | **精读** | 二进制协议解析 |
| muduo Reactor | **选读** | 线程 + event loop |
| TTCP / Netcat | **选读** | RTT 粗测、排查工具 |

---

## 补充：DPDK 官方文档 + 实体书（`12` 文件夹）

| 资料 | 本仓库入口 | 用途 |
|------|-----------|------|
| **DPDK Programmer's Guide** | [14-DPDK-Low-Latency-Network/](./14-DPDK-Low-Latency-Network/) | 用户态轮询、PMD、mbuf、零拷贝旁路 |
| **① 《深入浅出 DPDK》** | [01-Intro-Book/notes/](./14-DPDK-Low-Latency-Network/01-Intro-Book/notes/) · [note-DPDK实体书递进](./14-DPDK-Low-Latency-Network/01-Intro-Book/notes/note-DPDK实体书递进.md) | **先读** — 建立旁路认知（配 chapter-01–04） |
| **② 《Linux 高性能网络详解》** | [02-Advanced-Book/notes/](./14-DPDK-Low-Latency-Network/02-Advanced-Book/notes/) | **后读** — DPDK/RDMA/XDP 深度与选型 |
| **DPDK Sample Applications** | [01-Intro/code/mcast-minimal/](./14-DPDK-Low-Latency-Network/01-Intro-Book/code/mcast-minimal/) | 组播最小工程参考 |
| **OpenOnload / RDMA** | [02-Advanced/note-openonload-rdma对比](./14-DPDK-Low-Latency-Network/02-Advanced-Book/notes/note-openonload-rdma对比.md) | 方案对比 |
| **RDMA 规范** | https://www.infinibandta.org/ | RoCE 部署背景 |
| **Linux RDMA 文档** | https://www.kernel.org/doc/html/latest/infiniband/ | ibverbs、rdma_cm |

**建议顺序：** `11` Rosen → **① 深入浅出 DPDK** ∥ 官方 doc → **② Linux 高性能网络详解** ∥ RDMA/XDP 笔记。  
**触发条件：** `01`/`02` 打底 + perf 确认网络是瓶颈后再开实体书（见 note）。

---

## 补充：自制系统动手（`09` 文件夹）

> 笔记目录：[09-system-low-level-hands-on/](./09-system-low-level-hands-on/)

> **插入位置：** `07` TLPI 之后、`10` PNP 之前。

| 子模块 | 参考 | 标签 | HFT 关联 |
|--------|------|------|----------|
| [01-30days-os](./09-system-low-level-hands-on/01-30days-os/) | 《30 天自制操作系统》 | 🟡 选读 | BIOS/实模式启蒙 — 对照 LKD |
| [02-mikan-os](./09-system-low-level-hands-on/02-mikan-os/) | 《ゼロからの OS 自作入門》MikanOS | 🟡 选读 | UEFI/64位/分页/syscall — 现代 OS 互补 |
| [03-30days-cpu](./09-system-low-level-hands-on/03-30days-cpu/) | 《30 天自制 CPU》 | 🟡 选读 | 指令执行、数据通路 — 对照 Hennessy / CSAPP Ch4 |

**HFT 产出：** 把「绑核、syscall、cache 类比」从读者变成作者；时间紧可后补，与 `05`/`06` 概念课并行。

---

## 与 `15-HFT-Low-Latency-Practice` 章节映射

| 仓库章节 | 主要参考书 | 补充资料 |
|----------|------------|----------|
| ch01 高频交易基础与生态 | Harris | — |
| ch02 交易所架构与撮合原理 | Harris | Rosen（组播行情） |
| ch03 订单簿深度与行情解析 | Harris | Gorman、CSAPP Ch6 |
| ch04 硬件选型与服务器配置 | Hennessy Ch2/Ch5 | Gregg SysPerf Ch6、CSAPP Ch4/Ch6 |
| ch05 操作系统内核极致调优 | Love Ch4/7–10 | Gregg SysPerf Ch6–7 |
| ch06 低延迟网络与协议优化 | Rosen + **12 DPDK** | [10 PNP](./10-Practical-Network-Programming/)、CSAPP Ch11、**03 BPF Ch10** |
| ch07 无锁数据结构与内存布局 | Hennessy Ch2/Ch5 | Gorman、CSAPP Ch6/Ch12、[03 自制 CPU](../09-system-low-level-hands-on/03-30days-cpu/) |
| ch08 超低延迟核心引擎开发 | Love + Gorman + Hennessy | CSAPP Ch5/Ch12、**12 DPDK** |
| ch09 高频做市与套利策略 | Harris | — |
| ch10 延迟测量与基准压测 | Gregg SysPerf + **03 BPF** | **12 DPDK** testpmd |
| ch11 风控合规与滑点控制 | Harris（监管/规则） | — |
| ch12 实盘上线与运维进阶 | Gregg BPF | **12 DPDK** / OpenOnload / RDMA 对比笔记 |

---

## 阅读节奏总览

> 与 [LEARNING-CHAIN.md](./LEARNING-CHAIN.md) 对齐。

```
L0  Harris LOB（业务锚点）
    ↓
L1  Hennessy Ch2 + CSAPP 地基     ← 知其所以然
    ↓
L2  Gregg SysPerf                 ← 知其然
    ↓
L3  Gregg BPF Tools               ← 工具落地
    ↓
L4  Love → Gorman → 07 自制 → 08 PNP → 09–12 网络栈  ← 系统纵深
    ↓
L5  13-HFT + 14-Rust              ← 动手实现（C++ / Rust）
```
