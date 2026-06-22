## 10.4 硬件与软件栈架构

### 协议层性能要点

**TCP：**

| 机制 | 作用 | HFT |
|------|------|-----|
| **滑动窗口** | 流量控制 | 窗口满 → 背压 |
| **慢启动 / 拥塞控制** | 防网络 collapse | **CUBIC**（默认）vs **BBR** |
| **SACK** | 选择性重传 | 减不必要重传 |
| **TFO** | TCP Fast Open | 减一轮 RTT（需两端支持） |
| **Nagle** | 小包合并 | 增延迟 — 发单常 **`TCP_NODELAY`** |
| **TIME_WAIT** | 关闭后状态 | 大量短连接 → 端口耗尽 — `tcp_tw_reuse` 等 |

**BBR vs CUBIC（Gregg 语境）：**

- **CUBIC**：丢包作拥塞信号 — 丢包环境下吞吐可能差。
- **BBR**：基于带宽/RTT 模型 — 某些场景 **吞吐可达 CUBIC 数倍**；但也曾引发 buffer 争议 — 生产需 A/B。

**UDP：**

- 无连接、无重传 — **组播行情**主路径；应用自管丢包/乱序。

**QUIC / HTTP3：**

- 用户态、0-RTT、多路复用 — Web/API；**tick 热路径**量化系统较少用，了解即可。

→ [09-Rosen](../../../09-Linux-Kernel-Networking/) · [03-BPF note-XDP](../../../03-BPF-Performance-Tools/note-XDP与tc-BPF.md)

### Linux 网络栈路径（简化）

```
NIC RX interrupt → softirq (NAPI poll) → netif_receive_skb
    → IP → TCP/UDP → socket receive queue → read()/recv()
TX: write() → TCP 分段 → qdisc → driver → NIC
```

| 组件 | 说明 |
|------|------|
| **Socket buffer** | `tcp_rmem` / `tcp_wmem` — 读写缓冲 |
| **qdisc** | 排队规则（pfifo_fast、fq、fq_codel…） |
| **TSO/GSO/GRO/LRO** | 分段/聚合 **卸载到网卡** — 降 CPU、改 latency 特征 |

**HFT：** `kernel`/`softirq` % 高 → 查 **RSS、RPS、中断亲和性**、是否该 **DPDK 旁路**。

→ [09 Rosen NAPI/softirq](../../../09-Linux-Kernel-Networking/) · [CROSS-MODULE-GUIDE §二](../../CROSS-MODULE-GUIDE.md#二内核网络栈-vs-用户态旁路)

### CPU 扩展与内核绕过

| 技术 | 作用 |
|------|------|
| **RSS** | 网卡按 hash 把流分到多 RX 队列 |
| **RPS/RFS** | 软件把包分到不同 CPU 处理 |
| **IRQ affinity** | 中断绑核 — 与策略线程 **同 NUMA** |
| **DPDK** | 用户态 PMD 轮询 — **完全旁路**内核栈 |
| **XDP** | 驱动最早点 eBPF — 丢弃/转发/redirect |

```
标准栈：08 UNP socket → 09 Rosen 内核路径 → 本章观测
极致延迟：10-DPDK 01-Intro（组播） / 02-Advanced XDP·RDMA
```

→ [10-DPDK](../../../10-DPDK-Low-Latency-Network/) · [02-Advanced XDP note](../../../10-DPDK-Low-Latency-Network/02-Advanced-Book/notes/note-XDP与DPDK对照.md)

---


---

← [本章导读](../README.md)
