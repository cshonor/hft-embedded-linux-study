# 2. 网络基础知识 (Background)

### Linux 网络栈路径

```
应用程序
  → Socket API（用户态）
  → 传输层 TCP/UDP
  → 网络层 IP
  → qdisc（排队规则）
  → 网卡驱动 / NAPI
  → NIC
```

→ 协议语义：[10-TCP-IP](../10-TCP-IP-Illustrated-Vol1/) · 内核栈：[13-Rosen](../11-Linux-Kernel-Networking/) · Socket API：[09-UNP](../09-UNP-Vol1/) · 实战：[10-PNP](../08-Practical-Network-Programming/)

### 内核绕过 (Kernel Bypass)

| 技术 | 说明 |
|------|------|
| **DPDK** | 用户态 PMD 轮询 NIC — **绕过内核栈**，避免 per-packet 复制/ syscall |
| **XDP / tc-BPF** | 仍在内核，但在 **最早** 收包点执行 BPF — 见 [note-XDP与tc-BPF](../../note-XDP与tc-BPF.md) |

**HFT 分工：**

```
热路径行情/下单  →  DPDK / 内核 bypass（14-DPDK）
共置辅助流量     →  内核栈 + 本章 BPF 观测
对照实验         →  同一机：BPF 看内核栈 vs DPDK 看用户态环
```

### TCP 机制（观测相关）

| 机制 | BPF 工具关联 |
|------|--------------|
| **SYN Backlog / Listen Backlog** | `tcpsynbl` — 队列满 → SYN 丢包 |
| **重传**（超时 / 快速重传） | **`tcpretrans`** |
| **发送/接收缓冲区** 动态调整 | `sormem`、`tcpwin` |
| **Nagle** | `tcpnagle` — 小 packet 延迟 |
| **TFO** (TCP Fast Open) | 连接延迟分析时区分 |

### 卸载与分段

| 技术 | 作用 |
|------|------|
| **TSO/GSO** | 大块 TCP 分段 offload |
| **GRO/LRO** | 接收合并 |
| **影响** | `netsize` 可见软件分段前后包大小分布 |

### 常见延迟指标

| 指标 | 含义 |
|------|------|
| DNS 解析 | `gethostlatency` |
| ICMP RTT | `superping`（比用户态 ping 少调度噪声） |
| TCP 连接建立 | `soconnlat`、`tcpconnect` 时间线 |
| 首字节 | **`so1stbyte`** |

---
