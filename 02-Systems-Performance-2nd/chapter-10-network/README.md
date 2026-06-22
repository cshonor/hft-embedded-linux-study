# Ch 10 网络 · Network

> **Systems Performance 2nd** · Brendan Gregg · **精读**

> 本章定位：**网络是分布式时代最常背锅的资源** — 问题可能在 DNS、TCP 握手、拥塞、内核队列、网卡，而非「带宽不够」。Ch 7–9 是内存/磁盘；本章是 **HFT 系统性能的主战场之一**（行情 ingress、发单 egress）。与 handbook `07`→`08`→`09`→`10-DPDK` 网络栈闭环 **并行互补**：本章偏 **内核标准栈 + 观测**；DPDK/XDP 偏旁路与早期 hook。

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 10.1–10.3 核心概念与延迟指标 | [notes/section-10.1-10.3-核心概念与延迟指标.md](./notes/section-10.1-10.3-核心概念与延迟指标.md) |
| 10.4 硬件与软件栈架构 | [notes/section-10.4-硬件与软件栈架构.md](./notes/section-10.4-硬件与软件栈架构.md) |
| 10.5 分析方法论 | [notes/section-10.5-分析方法论.md](./notes/section-10.5-分析方法论.md) |
| 10.6 观测工具 | [notes/section-10.6-观测工具.md](./notes/section-10.6-观测工具.md) |
| 10.7–10.8 实验与调优 | [notes/section-10.7-10.8-实验与调优.md](./notes/section-10.7-10.8-实验与调优.md) |

---

## 大白话 · 本章就五件事

> **网络慢，先拆延迟段，别先加带宽。**

**① 术语与 MTU：吞吐 ≠ 带宽；大包减开销。**

- **Throughput** = 实际数据传输率；**Bandwidth** = 链路上限。
- **MTU 1500** vs **Jumbo ~9000** — 减包头开销；共置/托管需全网一致。

**② 延迟是多段的 — 不只有一个 RTT。**

- DNS → Ping(ICMP) → **TCP 握手/重传** → **TTFB**（含服务器处理）→ RTT → 连接生命周期。
- HFT 发单：分解 **本地处理 + kernel + wire RTT + 对端**。

**③ 缓冲与积压：Bufferbloat + SYN 队列丢包。**

- 中间设备缓冲过大 → **bufferbloat** 增延迟。
- 监听队列/SYN backlog 满 → **丢 SYN** → 连接超时、重传风暴。

**④ 栈架构：TCP/UDP/QUIC + Linux 路径 + RSS/RPS + DPDK/XDP。**

- **CUBIC vs BBR**、Nagle、窗口、SACK、TFO；**TSO/GRO** 卸载。
- 极高 PPS：**RSS/RPS** 分核；极致：**DPDK** 旁路或 **XDP** 最早丢/改包。

**⑤ 工具与调优：`ss -tiepm`、tcplife、tcpretrans、iperf、sysctl、TCP_NODELAY。**

- 生产优先 **计数器 + BPF**；**tcpdump** 最后手段。
- Netflix 式 sysctl + 应用 **`TCP_NODELAY`** 降小包延迟。

下面按原书 10.1–10.8 展开。

---

## 本章 Checklist

- [ ] 能分解 **DNS / 连接 / TTFB / RTT** — 不只说「网络慢」
- [ ] 会用 **`ss -tiepm`** 看 RTT、重传、队列
- [ ] 会用 **`ip -s link`**、**`nstat`** 看重传与 drop
- [ ] 跑过 **`tcplife`** 或 **`tcpretrans`** 至少一次
- [ ] 理解 **RSS/RPS/IRQ affinity** 与 softirq 的关系
- [ ] 知道何时查 **标准栈** vs **DPDK/XDP**（handbook 07–10 闭环）
- [ ] 生产抓包 **仅作最后手段**

---

## HFT 精读捷径（Ch 10 在路线中的位置）

```
Ch 5  应用线程状态、epoll、TCP_NODELAY
Ch 6  softirq CPU、绑核
Ch 7  socket buffer 与内存
Ch 10 网络（本章：栈、TCP、工具、sysctl）
  → 07 TCP/IP  协议语义
  → 08 UNP     socket API
  → 09 Rosen   内核实现
  → 10-DPDK    旁路落地
  → 03-BPF     XDP/tc 工具
  → 11-HFT     ch06/ch10 工程
```

**本章最小行动集：**

1. **`ss -s`** + **`ss -tiepm`** — 连接数、重传、典型 RTT。
2. **`ip -s link`** + **`ethtool -S`** — 有无 drop/overrun。
3. **`sudo tcpretrans-bpfcc 30`** — 看重传是否对齐延迟尖刺。
4. **行情/发单各一条延迟 span** — 对照 TTFB vs wire RTT。

**Gregg 本章金句（HFT 版）：**

> 网络是 **最常见的替罪羊** — 用 **分段延迟 + ss/BPF** 找真凶，别先买带宽。  
> **tcpdump 是最后手段**；`tcplife` 和 **`ss -tiepm`** 往往够用。

---

## 相关章节

- 上一章：[../chapter-09-disks/](../chapter-09-disks/)
- 下一章：[../chapter-11-cloud-computing/](../chapter-11-cloud-computing/)
- 协议：[07-TCP-IP-Illustrated-Vol1](../../07-TCP-IP-Illustrated-Vol1/)
- Socket API：[08-UNP-Vol1](../../08-UNP-Vol1/)
- 内核网络：[09-Linux-Kernel-Networking](../../09-Linux-Kernel-Networking/)
- DPDK 旁路：[10-DPDK-Low-Latency-Network](../../10-DPDK-Low-Latency-Network/)
- XDP：[03-BPF note-XDP](../../03-BPF-Performance-Tools/note-XDP与tc-BPF.md)
- BPF：[../chapter-15-bpf/](../chapter-15-bpf/)
- 跨模块对照：[CROSS-MODULE-GUIDE.md](../CROSS-MODULE-GUIDE.md)
- HFT 工程：[11-HFT ch06/ch10](../../11-HFT-Low-Latency-Practice/)
- 全书目录：[OUTLINE.md](../OUTLINE.md)
