# HFT 必读书目与章节精读清单

本清单锁定 **8 本**英文原版（含 CSAPP，已剔除 *Programming Rust*），以及 **2 本外部仓库书目**（TCP/IP 卷一、UNP），按 HFT 低延迟学习先后排序。

**总阅读顺序与小节级读/跳指引** → [HFT-READING-ROADMAP.md](./HFT-READING-ROADMAP.md)

**推荐序号：** 0 Harris(LOB) → ① → ② → ③ → 外A → 外B → ④ → ⑤ → ⑥ → ⑦ → ⑧ → **⑫ DPDK**  
**补充资料：** OpenOnload / RDMA 对比见 [12-DPDK/note-openonload-rdma对比.md](./12-DPDK-Low-Latency-Network/note-openonload-rdma对比.md)；不另加实体书、不建新文件夹。

| 标签 | 含义 |
|------|------|
| **精读** | 与延迟、抖动、内存、网络、撮合直接相关 |
| **选读** | 有上下文价值，时间紧可后补 |
| **跳过** | 与 HFT 热路径无关 |

---

## 1. Systems Performance: Enterprise and the Cloud 2nd — Brendan Gregg

> 笔记目录：[01-Systems-Performance-2nd/](./01-Systems-Performance-2nd/)

> 性能调优总纲：perf、NUMA、软中断、网卡调优，HFT 排抖动第一本。

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

> 笔记目录：[02-Linux-Kernel-Development-3rd/](./02-Linux-Kernel-Development-3rd/)

> 内核调度、中断、CFS、CPU 隔离、绑核底层原理，承接 TLPI。

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

## 3. Understanding the Linux Virtual Memory Manager — Mel Gorman

> 笔记目录：[03-Linux-Virtual-Memory-Manager/](./03-Linux-Virtual-Memory-Manager/)

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

## 4. Linux Kernel Networking — Rami Rosen

> 笔记目录：[06-Linux-Kernel-Networking/](./06-Linux-Kernel-Networking/)

> 内核 TCP/UDP/IGMP/NAPI/RSS，交易所 UDP 组播内核实现，对接 UNP。

| 章节 | 标签 | HFT 关联 |
|------|------|----------|
| Ch 11 第 4 层协议（TCP/UDP/Socket/sk_buff） | **精读** | 对照 UNP 之下发生了什么 |
| Ch 14 高级主题（NAPI/softirq/RSS/RPS/XPS） | **精读** | 收包延迟、多队列绑核 |
| 组播 / IGMP | **精读** | 见 [note-组播IGMP.md](./06-Linux-Kernel-Networking/note-组播IGMP.md) |
| Ch 4–5 IPv4 / 路由 | **选读** | 托管/共置网络 |
| Ch 3 ICMP / Ch 7 邻居子系统 | **选读** | 排查网络 |
| Ch 13 InfiniBand | **选读** | RDMA/共置低延迟 |
| Ch 1 简介 / 附录 A–B | **选读** | 背景与 API 速查 |
| Ch 2 Netlink / Ch 8 IPv6 / Ch 9 Netfilter / Ch 10 IPsec / Ch 12 无线 | **跳过** | HFT 通常旁路或不用 |

**HFT 产出：** 从网卡 DMA → NAPI → 用户态 socket 的完整内核路径；与 UNP、DPDK 对照。

---

## 12. DPDK — 用户态旁路网络（官方文档 + 本仓库笔记）

> 笔记目录：[12-DPDK-Low-Latency-Network/](./12-DPDK-Low-Latency-Network/)

> **网络栈闭环最后一环：** 内核协议原理（04）→ Socket API（05）→ 内核实现（06）→ **用户态旁路（12）**。与 UNP/TCP/IP **并行互补**，不是重复。

| 主题 | 标签 | HFT 关联 |
|------|------|----------|
| EAL、大页、NUMA、lcore 绑定 | **精读** | DPDK 环境基础 |
| mbuf、mempool、ring | **精读** | 预分配、零 malloc 热路径 |
| PMD、poll mode、rx/tx burst | **精读** | 轮询收包 vs NAPI |
| 零拷贝、UIO/VFIO | **精读** | 旁路内核栈原理 |
| UDP 组播行情接入 | **精读** | 交易所行情主路径 |
| OpenOnload / RDMA 对比 | **选读** | 方案选型（见 note-openonload-rdma对比.md） |
| Crypto / Eventdev 等 | **跳过** | 非行情热路径 |

**HFT 产出：** 理解内核栈 vs DPDK 旁路取舍；组播行情最小工程见 `code/mcast-minimal/`。

**交叉阅读：** [CROSS-MODULE-GUIDE.md](./CROSS-MODULE-GUIDE.md) · 对照 [05-UNP](./05-UNP-Vol1/) · [08-CSAPP Ch6/Ch11](./08-CSAPP-3rd/)

---

## 5. Computer Architecture: A Quantitative Approach 6th — Hennessy & Patterson

> 笔记目录：[07-Computer-Architecture-6th/](./07-Computer-Architecture-6th/)

> CPU 缓存、MESI、NUMA 访存、流水线，无锁代码硬件优化依据。

| 章节 | 标签 | HFT 关联 |
|------|------|----------|
| Ch 1 Fundamentals / Roofline | **选读** | 性能上限直觉 |
| **Ch 2 Memory Hierarchy** | **精读** | Cache line、MESI、false sharing、NUMA |
| Ch 3 ILP（流水线、分支预测） | **选读** | 热循环微优化 |
| Ch 5 Multiprocessors / Memory Consistency | **精读** | 无锁、memory order、store buffer |
| Ch 4 SIMD / GPU | **跳过** | 除非做 SIMD 行情解析 |
| Ch 6 Warehouse-Scale / Domain-Specific | **跳过** | |

**HFT 产出：** 无锁队列、订单簿数组布局、cache-line padding 的硬件依据。

---

## 6. Computer Systems: A Programmer's Perspective 3rd — Bryant & O'Neill

> 笔记目录：[08-CSAPP-3rd/](./08-CSAPP-3rd/)

> 程序员视角的系统实践：缓存优化、虚拟内存、网络 I/O、并发，Hennessy 的落地配套。

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
| Ch 7 Linking / Ch 8 ECF | **跳过** | 链接与信号，非热路径核心 |

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

> 笔记目录：[09-BPF-Performance-Tools/](./09-BPF-Performance-Tools/)

> eBPF、XDP 小包过滤、内核观测，DPDK 配套调试，放在末尾。

| 章节 | 标签 | HFT 关联 |
|------|------|----------|
| Ch 1–2 BPF 基础 / 工具链 | **精读** | bcc/bpftrace 快速上手 |
| Ch 4–5 CPU / off-CPU 分析 | **精读** | 定位抖动、调度问题 |
| Ch 6 Memory | **选读** | alloc、page fault 追踪 |
| **Ch 9–10 Networking** | **精读** | 内核网络延迟、丢包、重传 |
| XDP / tc-BPF 相关节 | **精读** | 小包过滤、DPDK 前置对比 |
| Ch 7–8 File Systems / Disks | **跳过** | |
| Ch 12 Security / 泛化应用 | **跳过** | |

**HFT 产出：** 生产环境 eBPF 观测；与 DPDK 文档配合做「内核栈 vs 用户态旁路」对比。

---

## 外部书目（笔记在另一仓库 · 本仓库仅索引）

| 外 | 书目 | 索引 | 插入顺序 |
|----|------|------|----------|
| 外A | TCP/IP Illustrated Vol.1 — Stevens | [04-TCP-IP-Illustrated-Vol1/](./04-TCP-IP-Illustrated-Vol1/) · [笔记](https://github.com/cshonor/Computer-Networking/tree/main/TCP-IP-Volume1-Protocols) | Rosen / UNP **之前** |
| 外B | UNIX Network Programming Vol.1 — Stevens | [05-UNP-Vol1/](./05-UNP-Vol1/) · [笔记](https://github.com/cshonor/Computer-Networking/tree/main/UNP_Vol1) | TCP/IP 卷一 **之后**，Rosen **并行** |

> **不要整本迁入本仓库。** 三本网络书分工：TCP/IP = 协议；UNP = API；Rosen = 内核实现。详见 [HFT-READING-ROADMAP.md](./HFT-READING-ROADMAP.md#二外部仓库书目unp--tcpip-卷一)。

---

## 补充：官方文档（无实体书 · 已纳入 12 号文件夹）

| 资料 | 本仓库入口 | 用途 |
|------|-----------|------|
| **DPDK Programmer's Guide** | [12-DPDK-Low-Latency-Network/](./12-DPDK-Low-Latency-Network/) | 用户态轮询、PMD、mbuf、零拷贝旁路 |
| **DPDK Sample Applications** | [12-DPDK/code/mcast-minimal/](./12-DPDK-Low-Latency-Network/code/mcast-minimal/) | 组播最小工程参考 |
| **OpenOnload / RDMA** | [note-openonload-rdma对比.md](./12-DPDK-Low-Latency-Network/note-openonload-rdma对比.md) | 方案对比，不建新文件夹 |
| **RDMA 规范** | https://www.infinibandta.org/ | RoCE 部署背景 |
| **Linux RDMA 文档** | https://www.kernel.org/doc/html/latest/infiniband/ | ibverbs、rdma_cm |

**建议阅读顺序：** 先 Rosen（06 内核栈）→ 再 12 DPDK（旁路对比）→ OpenOnload/RDMA 对比笔记（选型）。

---

## 与 `10-HFT-Low-Latency-Practice` 章节映射

| 仓库章节 | 主要参考书 | 补充资料 |
|----------|------------|----------|
| ch01 高频交易基础与生态 | Harris | — |
| ch02 交易所架构与撮合原理 | Harris | Rosen（组播行情） |
| ch03 订单簿深度与行情解析 | Harris | Gorman、CSAPP Ch6 |
| ch04 硬件选型与服务器配置 | Hennessy Ch2/Ch5 | Gregg SysPerf Ch6、CSAPP Ch4/Ch6 |
| ch05 操作系统内核极致调优 | Love Ch4/7–10 | Gregg SysPerf Ch6–7 |
| ch06 低延迟网络与协议优化 | Rosen + **12 DPDK** | CSAPP Ch11、Gregg BPF Ch9–10 |
| ch07 无锁数据结构与内存布局 | Hennessy Ch2/Ch5 | Gorman、CSAPP Ch6/Ch12 |
| ch08 超低延迟核心引擎开发 | Love + Gorman + Hennessy | CSAPP Ch5/Ch12、**12 DPDK** |
| ch09 高频做市与套利策略 | Harris | — |
| ch10 延迟测量与基准压测 | Gregg SysPerf + Gregg BPF | **12 DPDK** testpmd |
| ch11 风控合规与滑点控制 | Harris（监管/规则） | — |
| ch12 实盘上线与运维进阶 | Gregg BPF | **12 DPDK** / OpenOnload / RDMA 对比笔记 |

---

## 阅读节奏总览

```
Harris LOB（阶段 0，可并行）
    ↓
Gregg SysPerf (方法论/观测)
    ↓
Love (调度/中断/定时器)
    ↓
Gorman (虚拟内存/NUMA/THP)
    ↓
TCP/IP 卷一 (协议：UDP/组播/TCP)     ← 外部仓库
    ↓
UNP Vol.1 (Socket API：epoll/非阻塞)  ← 外部仓库
    ↓
Rosen (内核网络栈/组播)
    ↓
Hennessy (缓存/MESI/一致性)
    ↓
CSAPP (程序员视角：缓存/VM/网络/并发落地)
    ↓
Harris 剩余 (监管/清算)
    ↓
Gregg BPF (eBPF/XDP 观测闭环)
    ↓
⑫ DPDK (用户态旁路 · 网络栈闭环)
    ↓
10/11 实战笔记 (交易系统工程落地)
```
