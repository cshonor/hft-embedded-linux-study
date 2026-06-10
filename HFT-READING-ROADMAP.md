# HFT 系统开发 · 完整阅读路线图

> **文件夹序号即阅读顺序：** `00-` → `12-`，在资源管理器中按名称排序即可跟着读。

面向 **HFT 高频量化交易系统**（行情接入 → 订单簿 → 策略 → 发单 → 风控 → 观测），本文件给出**不漏项**的分阶段阅读顺序，以及每本书**小节级**读/跳指引。

| 标签 | HFT 含义 | 你要怎么做 |
|------|----------|-----------|
| **🔴 必读** | 直接作用于热路径、延迟、抖动、LOB、发单 | 认真读 + 在本仓库写笔记 |
| **🟡 选读** | 有上下文价值；或特定场景才需要 | 时间紧可后补；场景触发时升为必读 |
| **⚪ 跳过** | 与当前 HFT 目标无关 | 默认不读；不要内疚 |

> **有笔记文件** = 本仓库建议你读并记录；**无笔记文件** = 默认跳过。  
> 清单是裁剪后的最短路径，不是「原书不重要」。

---

## 一、总阅读顺序（含外部仓库书目）

```
阶段 0  业务锚点（可与其他阶段并行）
        Harris《Trading and Exchanges》LOB + 市场结构

阶段 1  会量、会调（排抖动方法论）
   ①   Systems Performance 2nd

阶段 2  内核时间线与 CPU 隔离
   ②   Linux Kernel Development 3rd

阶段 3  内存：NUMA / TLB / THP / 伪共享
   ③   Understanding the Linux VM Manager
   ⑤   Computer Architecture 6th（与③⑤交叉）
   ⑥   CSAPP 3rd · Ch6/9（程序员落地）

阶段 4  网络：协议 → API → 内核栈 → 用户态旁路（⚠️ 外部两本书插在这里）
   外A  TCP/IP Illustrated Vol.1（协议语义，另一仓库）
   外B  UNP Vol.1（Socket API，另一仓库）
   ⑥   CSAPP 3rd · Ch10/11
   ④   Linux Kernel Networking
   ⑫   DPDK Low-Latency Network（用户态旁路 · 网络闭环）

阶段 5  硬件 + 代码级优化 + 并发
   ⑤   Computer Architecture 6th · 剩余精读
   ⑥   CSAPP 3rd · Ch4/5/12

阶段 6  业务闭环 + 生产观测
   ⑦   Harris 剩余章节
   ⑧   BPF Performance Tools

阶段 7  本仓库实战笔记（与以上穿插 · 非网络技术板块）
        10-HFT-Low-Latency-Practice（12 章 · 交易系统工程）
        11-Rust-Quant-Trading-Guide（11 章 · Rust 量化工程）
```

**推荐序号：** 0 → ① → ② → ③ → 外A → 外B → ④ → ⑤ → ⑥ → ⑦ → ⑧ → **⑫** → 实战笔记

> **板块封顶：** `00`–`12` 覆盖全部底层/网络/观测/工程/Rust/业务；不再新增顶层编号文件夹。  
> 跨模块对照 → [CROSS-MODULE-GUIDE.md](./CROSS-MODULE-GUIDE.md)

---

## 二、外部仓库书目（UNP + TCP/IP 卷一）

### 要不要「搬」到本仓库？

**结论：不建议把整本书笔记复制过来。**

| 方案 | 说明 |
|------|------|
| ✅ **推荐** | 笔记留在 [Computer-Networking](https://github.com/cshonor/Computer-Networking)；本仓库 [`04-TCP-IP-Illustrated-Vol1/`](./04-TCP-IP-Illustrated-Vol1/)、[`05-UNP-Vol1/`](./05-UNP-Vol1/) 做**索引 + HFT 裁剪清单** |
| ⚠️ 可选 | 只把「HFT 必读章节」的笔记摘要链过来，不要 duplicate 全书 |
| ❌ 不推荐 | 整本迁移 — 与 Rosen / CSAPP Ch11 重叠，且双倍维护 |

**为什么不漏：** Rosen 讲**内核怎么收发包**；UNP 讲**用户态怎么调 Socket**；TCP/IP 卷一讲**线上字节长什么样**。HFT 三条都要，但分属不同层，各读裁剪章节即可。

> 笔记仓库：[cshonor/Computer-Networking](https://github.com/cshonor/Computer-Networking) · TCP/IP → [`TCP-IP-Volume1-Protocols/`](https://github.com/cshonor/Computer-Networking/tree/main/TCP-IP-Volume1-Protocols) · UNP → [`UNP_Vol1/`](https://github.com/cshonor/Computer-Networking/tree/main/UNP_Vol1)

---

## 三、分书小节级指引

### ① Systems Performance 2nd

| 原书 | 小节/主题 | 标签 | HFT 为何读 |
|------|-----------|------|-----------|
| Ch 1–2 | USE 方法、延迟分解、perf 思维 | 🔴 | 所有调优的前置语言 |
| Ch 4 | 观测工具：采样、跟踪、计数器 | 🔴 | 选型与排障工具链 |
| Ch 6 | run queue、context switch、绑核、NUMA | 🔴 | 策略线程隔离 |
| Ch 7 | TLB、page fault、THP、NUMA 访存 | 🔴 | 订单簿内存布局 |
| Ch 10 | 软中断、NAPI、RSS、网卡队列、TCP/UDP | 🔴 | 行情 burst 排抖动；协议栈对照 UNP |
| Ch 13 | perf 采样与火焰图 | 🔴 | 生产 profiling |
| Ch 15 | BPF/eBPF 动态跟踪 | 🔴 | 低开销内核观测 |
| 附录 A/C | USE 清单、bpftrace 单行 | 🔴 | 现场速查 |
| Ch 3 / Ch 5 | 操作系统、应用程序 | 🟡 | 背景 |
| Ch 12 / Ch 14 / Ch 16 | 基准测试、Ftrace、案例 | 🟡 | 方法论与跟踪 |
| Ch 8–9 | 文件系统、磁盘 | ⚪ | 除非审计落盘 |
| Ch 11 | 云计算 | ⚪ | 托管用 bare metal |

### ② Linux Kernel Development 3rd

| 原书 | 标签 | HFT 为何读 |
|------|------|-----------|
| Ch 4 调度：CFS、RT、`SCHED_FIFO`、affinity | 🔴 | 绑核、隔离策略核 |
| Ch 7 中断：硬中断、软中断 | 🔴 | 网卡 interrupt 延迟 |
| Ch 8 softirq、tasklet、workqueue | 🔴 | 收包路径抖动来源 |
| Ch 9 spinlock、RCU | 🔴 | 理解内核锁 vs 用户态无锁 |
| Ch 10 hrtimer、`CLOCK_MONOTONIC` | 🔴 | 延迟测量、定时发单 |
| Ch 3 进程/线程 | 🟡 | 背景 |
| Ch 11 内存概述 | 🟡 | 衔接 Gorman |
| Ch 12–18 VFS/Block | ⚪ | 热路径不经磁盘 |

### ③ Linux Virtual Memory Manager

| 原书 | 标签 | HFT 为何读 |
|------|------|-----------|
| Ch 2 Zones、NUMA、物理内存布局 | 🔴 | `numactl --membind` |
| Ch 3 页表、TLB、大页 | 🔴 | 减少 TLB miss；THP 见 note |
| Ch 8 Slab/Slub | 🔴 | 内存池设计参照 |
| Ch 4 进程地址空间、mmap、fault | 🟡 | 预分配订单簿 |
| Ch 6 物理页分配、Ch 10 页框回收 | 🟡 | 避免运行时 fault/回收 |
| Ch 12 共享内存 | 🟡 | 跨进程场景 |
| Ch 1 简介 | 🟡 | 背景 |
| 附录 B/C/H | 🟡 | 代码走读 |
| Ch 5/7/9/11/13 | ⚪ | Swap、高端内存、OOM — HFT 通常禁用 |

### 外A TCP/IP Illustrated Vol.1（外部仓库）

| 原书 | 标签 | HFT 为何读 |
|------|------|-----------|
| Ch 7 广播与 **多播**、IGMP | 🔴 | 交易所行情组播 |
| Ch 8 **UDP** 首部、校验、长度 | 🔴 | 主流行情封装 |
| Ch 3 IP：分片、DF、TTL | 🟡 | 避免 IP 分片增延迟 |
| Ch 9–11 **TCP**：握手、窗口、重传、拥塞 | 🟡 | **订单走 TCP 时升为 🔴** |
| Ch 6 ICMP | 🟡 | 排查网络 |
| Ch 17–18 路由表、选路 | 🟡 | 托管/共置网络 |
| Ch 2 链路层、ARP | ⚪ | 除非 raw socket / DPDK L2 |
| Ch 14 DNS、Ch 15–16 SNMP/HTTP | ⚪ | 非热路径 |

### 外B UNP Vol.1（外部仓库 · 伯克利网络编程）

| 原书 | 标签 | HFT 为何读 |
|------|------|-----------|
| Ch 3 Socket 简介、`sockaddr` | 🔴 | 一切网络代码起点 |
| Ch 6 **I/O 多路复用**：`select`/`poll`/`epoll` | 🔴 | 单线程收多路行情 |
| Ch 7 **Socket 选项**：`TCP_NODELAY`、buffer、reuseport | 🔴 | 低延迟发单必知 |
| Ch 8 **UDP** socket、`recvfrom` | 🔴 | 组播行情 |
| Ch 16 **非阻塞** I/O | 🔴 | busy-poll 前置 |
| Ch 4–5 TCP/UDP 入门 | 🟡 | 与 TCP/IP 卷一对照 |
| Ch 9–10 TCP 客户端/服务端 | 🟡 | 订单 TCP 时细读 |
| Ch 11 名字与时间 | 🟡 | `getaddrinfo` 等 |
| SCTP、RPC、复杂服务器模型 | ⚪ | HFT 不用 |

### ④ Linux Kernel Networking

| 原书 | 标签 | HFT 为何读 |
|------|------|-----------|
| Ch 11 传输层：Socket、sk_buff、TCP/UDP | 🔴 | 内核收发路径 |
| Ch 14 高级主题：NAPI、RSS/RPS/XPS | 🔴 | 收包延迟、绑核 |
| 组播/IGMP（note） | 🔴 | 行情内核路径 |
| Ch 4–5 IPv4、路由 | 🟡 | 托管网络 |
| Ch 3 ICMP、Ch 7 邻居 | 🟡 | 排查 |
| Ch 13 InfiniBand | 🟡 | RDMA 场景 |
| Ch 1、附录 A/B | 🟡 | 背景 |
| Ch 2/8/9/10/12 | ⚪ | Netlink、IPv6、Netfilter、无线 |

### ⑫ DPDK Low-Latency Network

| 主题 | 标签 | HFT 为何读 |
|------|------|-----------|
| EAL、大页、NUMA | 🔴 | DPDK 环境与绑核 |
| mbuf、mempool | 🔴 | 预分配热路径 |
| PMD、poll mode、burst | 🔴 | vs NAPI 收包模型 |
| 零拷贝、UIO/VFIO | 🔴 | 旁路内核栈 |
| UDP 组播行情 | 🔴 | 交易所行情主路径 |
| OpenOnload / RDMA 对比 | 🟡 | 方案选型 |

> 与 04/05/06 **并行互补**；详见 [CROSS-MODULE-GUIDE.md](./CROSS-MODULE-GUIDE.md)

### ⑤ Computer Architecture 6th

| 原书 | 标签 | HFT 为何读 |
|------|------|-----------|
| Ch 2 Cache line、MESI、false sharing | 🔴 | 订单簿伪共享 |
| Ch 5 内存一致性、store buffer、memory order | 🔴 | 无锁队列硬件依据 |
| Ch 1 Roofline | 🟡 | 性能上限直觉 |
| Ch 3 ILP、分支预测 | 🟡 | 热循环微优化 |
| Ch 4 SIMD/GPU、Ch 6 仓储级、Ch 7 领域架构 | ⚪ | 除非 SIMD 解析行情 |
| 附录 B/C、在线 L | 🟡 | 与 Ch2/CSAPP 交叉 |

### ⑥ CSAPP 3rd

| 原书 | 标签 | HFT 为何读 |
|------|------|-----------|
| Ch 6 局部性、Cache、伪共享 | 🔴 | 与 Hennessy Ch2 配套实操 |
| Ch 9 虚拟内存、mmap、大页 | 🔴 | 预分配订单簿 |
| Ch 12 线程、互斥、并发 | 🔴 | 引擎多线程模型 |
| Ch 4–5 流水线、编译优化 | 🔴 | 热路径 `-O3` / PGO 理解 |
| Ch 11 Socket 编程 | 🔴 | 衔接 UNP |
| Ch 10 epoll、非阻塞 I/O | 🟡 | 与 UNP Ch6/16 交叉 |
| Ch 1 漫游、Ch 3 汇编 | 🟡 | 反汇编热函数 |
| Ch 2 数据表示、Ch 7–8 链接/ECF | ⚪ | 除非二进制协议 |

### ⑦ Trading and Exchanges

| 主题 | 标签 | HFT 为何读 |
|------|------|-----------|
| 市场结构、参与者、HFT 角色 | 🔴 | **建议阶段 0 先读** |
| 订单类型、**LOB**、撮合规则 | 🔴 | 写订单簿/策略的语言 |
| 监管、透明度 | 🟡 | 上实盘前 |
| 清算、结算 | 🟡 | 对接券商时 |
| 估值、组合管理 | ⚪ | buy-side，非 HFT 核心 |

### ⑧ BPF Performance Tools

| 原书 | 标签 | HFT 为何读 |
|------|------|-----------|
| Part I Ch 1–2 BPF/eBPF 技术背景 | 🔴 | 观测基础 |
| Part I Ch 4–5 BCC/bpftrace | 🔴 | 工具链上手 |
| Part II Ch 6 CPU、off-CPU | 🔴 | 查抖动 |
| Part II Ch 10 网络 | 🔴 | 行情/订单链路 |
| note XDP/tc-BPF | 🔴 | vs DPDK 决策 |
| 附录 A/B bpftrace 速查 | 🔴 | 现场单行命令 |
| Part II Ch 7 内存、Ch 13–14 | 🟡 | fault、内核子系统 |
| Part III Ch 17–18 | 🟡 | 面板集成、排障 |
| Part II Ch 8–9/11–12/15–16 | ⚪ | 磁盘、安全、容器 |

---

## 四、HFT 不漏项检查清单

读完以下各项，可认为**主线没有明显缺口**：

- [ ] 会用 perf/bcc 分解延迟，知道 CPU/内存/网络各贡献多少
- [ ] 能解释：绑核、`SCHED_FIFO`、isolcpus、中断亲和
- [ ] 能解释：NUMA、大页、THP、TLB miss、伪共享
- [ ] 能画：网卡 → NAPI → sk_buff → socket → 用户态 收包路径
- [ ] 读过 UDP/组播协议（TCP/IP 卷一）+ epoll/非阻塞（UNP）
- [ ] 理解 LOB、限价单/市价单、撮合与 queue priority
- [ ] 能读无锁结构并知道 memory order 硬件原因（Hennessy + CSAPP）
- [ ] 会用 eBPF 查生产抖动；知道 DPDK 旁路与内核栈取舍（⑫ + CROSS-MODULE-GUIDE）

---

## 五、与本仓库其他目录的关系

| 目录 | 何时读 |
|------|--------|
| [04/05/06/12](./CROSS-MODULE-GUIDE.md#一仓库板块总览) | 网络技术栈（协议 → Socket → 内核 → DPDK） |
| [10-HFT-Low-Latency-Practice/](./10-HFT-Low-Latency-Practice/) | 阶段 3–7 穿插 · **交易系统工程**（≠ 网络书） |
| [11-Rust-Quant-Trading-Guide/](./11-Rust-Quant-Trading-Guide/) | 需要 Rust 全栈量化工程时并行 |
| [CROSS-MODULE-GUIDE.md](./CROSS-MODULE-GUIDE.md) | DPDK↔UNP、DPDK↔CSAPP 对照 |
| 各书文件夹 [README](./01-Systems-Performance-2nd/README.md) | 进入单本书时的速查 |

完整书目表格 → [READING-LIST.md](./READING-LIST.md)
