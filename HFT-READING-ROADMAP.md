# HFT 系统开发 · 完整阅读路线图

> **文件夹 `00`–`18` 主线 + 嵌入式 Linux 支线 `19`–`24`；读序见 [LEARNING-CHAIN.md](./LEARNING-CHAIN.md)。** 主叙事 → **[LEARNING-CHAIN.md](./LEARNING-CHAIN.md)**

### 核心段（文件夹编号 = 读序）

| 文件夹 | 内容 | 阶段 |
|--------|------|------|
| **01** → **02** → **03** | CSAPP → **C 语言** → Hennessy | 知其所以然 + 系统级 C |
| **04** → **07** | LKD → ULK → Gorman → TLPI | 内核 + 用户态 API |
| **08/01** | MikanOS | HFT OS 动手主线 |
| **08/02** | 30 天 OS | 可选启蒙 |
| **15** → **16** | SysPerf → BPF | **后置** · 14 DPDK 之后或 17 HFT 之前 |
| **09** | [C++ 外部索引](./09-cpp-learning-notes/) | Modern C++ → 并发（PNP/HFT 前置） |
| **10–14** | PNP / UNP / TCP/IP / Rosen / DPDK | 网络纵深 |
| **17–18** | HFT Practice / Rust | 动手实现 |
| **19–24** | ARM64 · 构建 · 驱动 · DT · 实战 · **PID/飞控** | 嵌入式 Linux 退路（可选支线） |

### Gregg 双书 · 15 → 16（后置）

| 15 SysPerf | 16 BPF |
|------------|--------|
| USE/RED、延迟分解、perf/Ftrace | bpftrace/BCC 生产落地 |
| Ch 4 / 13 / 15 预览 | Part I–II + XDP note |

**执行顺序：** 先完成 **04–07 内核/TLPI** 与 **08/01 MikanOS**、**10–14 网络/DPDK**，再开 **15 → 16** — 有真实系统可 profile 后再读方法论。

### 15 为何不在 03 之后立刻读

| 过早读 SysPerf/BPF | 更合适的时机 |
|--------------------|--------------|
| 还没有 Linux 进程/内核/网络概念 | **07 TLPI + 08 MikanOS** 之后 |
| 火焰图看不懂在烧什么 | **14 DPDK** 或 **17 HFT** 压测有靶子 |
| 与 CSAPP 理论堆叠 | **02 C + 03 Hennessy 后直进 04 LKD** 更顺 |

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
00  Harris · 业务锚点

01  CSAPP · 地基篇（Ch1/4–6/8–9/12）
02  C 语言 · K&R · Pointers on C（系统级 C）
03  Hennessy · Ch2（+ 选读 Ch5）

04  Linux Kernel Development
05  Understanding the Linux Kernel
06  Linux Virtual Memory Manager
07  TLPI

08  自制 OS
    ├─ 01-mikan-os（HFT 主线）
    └─ 02-30days-os（可选启蒙）

09  C++ · cpp-learning-notes（外仓 · 08 OS 后 10 PNP 前）

10  陈硕 PNP / muduo
11  UNP Vol.1
01  CSAPP · 网络篇 Ch10–11
12  TCP/IP 卷一
13  Linux Kernel Networking
14  DPDK

15  Systems Performance 2nd      ← 后置
16  BPF Performance Tools       ← 紧接 15

17  HFT Low-Latency Practice
18  Rust Quant Trading Guide

── 可选支线 · 嵌入式 Linux ──
19–24  …
```

**HFT 主线执行序号：** `00 → 01 → 02 C → 03 → 04 → 05 → 06 → 07 → 08/01 → 09 → 01网络 → 10 → 11 → 12 → 13 → 14 → 15 → 16 → 17 → 18`

**嵌入式支线（独立顺序 · 建议 04–07 后再开 · 24 用业余时间）：** `19 → 20 → 21 → 22 → 23 → 24`

> **C 语言模块：** [02-c-programming/](./02-c-programming/) — **01 CSAPP 之后、03 Hennessy 之前**；K&R + *Pointers on C* 打底。HFT 与嵌入式 **共用** 这套 C 基础；学体系结构时可同步用 C 做硬件侧小实验。

> **C++ 外部仓：** [09-cpp-learning-notes/](./09-cpp-learning-notes/) — **08 OS / 07 TLPI 之后、10 PNP 之前** 至少读完 *Effective Modern C++*。

> **板块封顶：** `00`–`18` 在本仓（含 **`09` C++ 索引**）+ **`19`–`24`** 嵌入式 Linux + 运动控制支线；跨模块对照 → [CROSS-MODULE-GUIDE.md](./CROSS-MODULE-GUIDE.md)

---

## 二、外部仓库书目（UNP + TCP/IP 卷一）

### 要不要「搬」到本仓库？

**结论：不建议把整本书笔记复制过来。**

| 方案 | 说明 |
|------|------|
| ✅ **推荐** | 笔记留在 [Computer-Networking](https://github.com/cshonor/Computer-Networking)；本仓库 [`12-TCP-IP-Illustrated-Vol1/`](./12-TCP-IP-Illustrated-Vol1/)、[`11-UNP-Vol1/`](./11-UNP-Vol1/) 做**索引 + HFT 裁剪清单** |
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

### ② Linux Kernel Development

> 子目录与课书关系 → [02/LEARNING-PATH.md](./04-Linux-Kernel-Development/LEARNING-PATH.md)

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

> 与 `07`/`08`/`09` **并行互补**；详见 [CROSS-MODULE-GUIDE.md](./CROSS-MODULE-GUIDE.md)

### ⑤ Computer Architecture 6th

> **阶段 1 先读 Ch2**（可与 CSAPP Ch6 交叉）；剩余章节阶段 6 补强或按需。

| 原书 | 标签 | HFT 为何读 |
|------|------|-----------|
| Ch 2 Cache line、MESI、false sharing | 🔴 | **SysPerf 之前** — 伪共享、订单簿布局硬件依据 |
| Ch 5 内存一致性、store buffer、memory order | 🔴 | 无锁队列；可与 CSAPP Ch12 交叉 |
| Ch 1 Roofline | 🟡 | 性能上限直觉 |
| Ch 3 ILP、分支预测 | 🟡 | 热循环微优化 |
| Ch 4 SIMD/GPU、Ch 6 仓储级、Ch 7 领域架构 | ⚪ | 除非 SIMD 解析行情 |
| 附录 B/C、在线 L | 🟡 | 与 Ch2/CSAPP 交叉 |

### ⑥ CSAPP 3rd

> **分两遍读：** **地基篇**（阶段 1，SysPerf 之前）与 **网络篇**（阶段 5，UNP 前后）。

| 原书 | 标签 | 何时读 | HFT 为何读 |
|------|------|--------|-----------|
| Ch 6 局部性、Cache、伪共享 | 🔴 | **阶段 1 地基** | 火焰图热点、订单簿布局 |
| Ch 9 虚拟内存、mmap、大页 | 🔴 | **阶段 1 地基** | 预分配；衔接 Gorman |
| Ch 12 线程、互斥、并发 | 🔴 | **阶段 1 地基** | 理解锁为何拖性能 |
| Ch 4–5 流水线、编译优化 | 🔴 | 阶段 1 或 6 | 热路径 `-O3` / PGO |
| Ch 8 异常控制流（进程/syscall） | 🟡→🔴 | **阶段 1 建议读** | 衔接 SysPerf off-CPU、上下文切换 |
| Ch 1 漫游 | 🟡 | 阶段 1 可选 | Amdahl、系统全景 |
| Ch 11 Socket 编程 | 🔴 | **阶段 5 网络** | 衔接 UNP |
| Ch 10 epoll、非阻塞 I/O | 🟡 | **阶段 5 网络** | 与 UNP Ch6/16 交叉 |
| Ch 3 汇编 | 🟡 | 需反汇编时 | 读 perf 火焰图汇编 |
| Ch 2 数据表示、Ch 7 链接 | ⚪ | 跳过 | 除非二进制协议 |

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

| 目录 | 文件夹 |
|------|--------|
| [01 CSAPP](./01-CSAPP-3rd/) | 01 |
| [02 C 语言](./02-c-programming/) | 02 |
| [03 Hennessy](./03-Computer-Architecture-6th/) | 03 |
| [15 SysPerf](./15-Systems-Performance-2nd/) | 15 |
| [16 BPF](./16-BPF-Performance-Tools/) | 16 |
| [04 LKD](./04-Linux-Kernel-Development/) · [06 Gorman](./06-Linux-Virtual-Memory-Manager/) · [10–14 网络](./CROSS-MODULE-GUIDE.md) | 04–07 · 10–14 |
| [17 HFT](./17-HFT-Low-Latency-Practice/) · [18 Rust](./18-Rust-Quant-Trading-Guide/) | 17 / 18 |

→ [LEARNING-CHAIN.md](./LEARNING-CHAIN.md) · [CROSS-MODULE-GUIDE.md](./CROSS-MODULE-GUIDE.md)

---

## 六、嵌入式 Linux 支线（`19`–`24`）

> **定位：** **第二职业退路** — 飞行器 / 网关 / 车载；**主线仍是 HFT**。  
> **范围：** 仅 **ARM-A + 嵌入式 Linux**；**不学** STM32 / MCU 裸机 / FreeRTOS 飞控 / PCB。  
> **C 基础：** [02 C](./02-c-programming/) 是支线 **硬前置** — 驱动写寄存器、飞控用户态环、与内核 `ioctl` 交互全是 C；主线 `01`→`02`→`03` 过关后，开 `19` 不必重学语法。读 Hennessy 时可 **用 C 做 ARM 裸机小实验**（QEMU 验证 EL 切换）同步练硬件理论。  
> **24 特别说明：** 只学 **PID / 姿态 / 电机算法 + Linux 对接**，硬件只做理论常识。

### 主次优先级（不可颠倒）

| 优先级 | 内容 | 时间 |
|--------|------|------|
| **P0 · 主线** | HFT — C++ / Rust / DPDK / `17` 引擎 | **全职学习** |
| **P1 · 支线** | 嵌入式 Linux `19`–`23` | 并行或 HFT 阶段完成后 |
| **P2 · 飞控算法** | `24` PID / IMU / 飞控环 | **仅 HFT 每日任务后的业余时间** |

### 为何必须学 24（运动控制）

1. **无人机 GitHub 项目缺这一环就飞不起来** — WiFi/图传/视觉不能替代 PID + 姿态 + 电机。  
2. **岗位：** 驱动 + DT → 适配；**+ 自控** → 飞控 / 伺服整机。  
3. **HFT 互补：** 飞控严格周期 ↔ 绑核 / PREEMPT_RT / p99 jitter 测量（15 · 16 · 17）。

### 学习边界

| ✅ 学 | ❌ 不学 |
|-------|--------|
| 位置式/增量式 PID · 离散闭环 · 抗饱和 | Cortex-M 裸机 · HAL 模板 |
| 三轴 · 矩阵 · Kalman · IMU 融合 | 纯 FreeRTOS 飞控栈 |
| Linux PWM / I2C 驱动 · 用户态飞控 | PCB / 硬件电路设计 |
| PWM · 无刷 · ESC 协议（理论） | STM32-F4/H7 整条路线 |

### 何时开这条线

| 条件 | 说明 |
|------|------|
| **建议前置** | [04 LKD](./04-Linux-Kernel-Development/) + [07 TLPI](./07-The-Linux-Programming-Interface/) + [21 驱动](./21-Linux-Device-Driver/) |
| **24 前置** | 建议 **23 或至少 21–22** 后再开算法整合 |
| **C 语言** | [02 C](./02-c-programming/) · K&R + *Pointers on C* + CSAPP/TLPI 可复用 |

### 严格顺序

```
19  ARM64 架构
 ↓
20  U-Boot · 内核裁剪 · Buildroot
 ↓
21  Linux 设备驱动（LDD3 + 现代驱动）
 ↓
22  设备树 Device Tree
 ↓
23  无人机 / 网关项目实战
 ↓
24  PID · 电机 · 姿态解算 · 飞控调度（PREEMPT_RT）
```

### 文件夹 ↔ 必读书（每模块 1–2 本 · 不冗余）

| 文件夹 | 必读书 | 索引 |
|--------|--------|------|
| **19** | 《ARMv8-A Programmer's Guide》· 《ARM64 汇编编程实战》 | [19-ARM64-Architecture/](./19-ARM64-Architecture/) |
| **20** | 《嵌入式 Linux 开发实战：U-Boot、内核、根文件系统》· 《Buildroot 实战指南》 | [20-UBoot-Kernel-Build/](./20-UBoot-Kernel-Build/) |
| **21** | LDD3 · 《Linux 内核驱动深度开发》 | [21-Linux-Device-Driver/](./21-Linux-Device-Driver/) |
| **22** | 《Device Tree for Embedded Linux》 | [22-Device-Tree-Study/](./22-Device-Tree-Study/) |
| **23** | 《嵌入式 Linux 无人机开发实战》 | [23-Embedded-Linux-Practice/](./23-Embedded-Linux-Practice/) |
| **24** | 《自动控制原理》胡寿松 · 《卡尔曼滤波与组合导航原理》秦永元 · （整合复用 23） | [24-Motion-Control-Motor/](./24-Motion-Control-Motor/) |

**24 子目录：** [Ch1 PID](./24-Motion-Control-Motor/chapter-01-pid-discrete-control/) · [Ch2 姿态/Kalman](./24-Motion-Control-Motor/chapter-02-attitude-kalman-imu/) · [Ch3 电机/ESC](./24-Motion-Control-Motor/chapter-03-motor-pwm-esc/) · [Ch4 Linux 对接](./24-Motion-Control-Motor/chapter-04-linux-drivers-integration/) · [Ch5 飞控调度](./24-Motion-Control-Motor/chapter-05-flight-control-scheduling/)

### 可直接复用（HFT 链 · 不用重学）

| 类别 | 来源模块 |
|------|----------|
| C / GNU-C / 指针 / 结构体 | [02 C](./02-c-programming/) · CSAPP · TLPI · 《嵌入式 C 自我修养》 |
| 进程 / VM / 中断 / 同步 | 04 LKD · 05 ULK · 06 Gorman |
| 性能 / 绑核 / BPF / **周期 jitter** | 15 SysPerf · 16 BPF · **17 HFT 测量** |
| 网络 / 零拷贝思想 | 11 UNP · 13 内核网 · 14 DPDK → 传感器链路 |
| 低延迟工程思维 | 17 HFT — 绑核、无锁、异步日志、T2T |

### 岗位定位（支线完成后）

嵌入式 Linux · 车载 Linux · 工业网关 · **无人机飞控 / 运动控制**

### GitHub 简介表述

**English**

> My primary research interest lies in HFT quantitative-trading backend development. As a long-term secondary path, I also learn embedded Linux on the ARM-A platform. I implement self-coded PID control algorithms, motor-driver programming, IMU-sensor communication and flight-control scheduling logic, avoiding STM32-M4 bare-metal development, to build a self-developed drone project as an alternative-career track.

**中文**

> 核心主攻方向为高频量化（HFT）后端开发；同时拓展 ARM-A 平台下的嵌入式 Linux，自研实现 PID 控制算法、电机驱动、IMU 传感器通信与飞控调度逻辑，绕开 STM32-M4 单片机裸机开发，自研无人机项目，作为职业备选路线。
