# 第 14 章：TCP 超时与重传

> 按书节速记：[14.1](14.1-introduction.md) · [14.2](14.2-simple-timeout-retransmit.md) · [14.3](14.3-rto-estimation.md) · [14.4](14.4-timer-based-retransmit.md) · [14.5](14.5-fast-retransmit.md) · [14.6](14.6-sack-retransmit.md) · [14.7](14.7-spurious-timeout.md) · [14.8](14.8-reorder-duplicate.md) · [14.9](14.9-destination-metrics.md) · [14.10](14.10-resegment-retransmit.md) · [14.11](14.11-retransmit-attacks.md) · [14.12](14.12-summary.md) · [QUICKREF §14](../QUICKREF.md)

> 《TCP/IP 详解》卷1 第 2 版（Fall, 2016）· 精细化学习笔记（同步自 [tcpip_vol1_ed2_notes](../../tcpip_vol1_ed2_notes/04_transport_layer/ch14_tcp_timeout_retransmit.md)）  
> 前置：[ch12](../chapter12-tcp-basic/study.md#ch12-1-4) · [ch13 选项/时间戳](../chapter13-tcp-connection-manage/study.md#ch13-3) · 自顶向下：[§3.1 可靠/快速重传](../../top_down/03_transport_layer/study.md#ch3-1-tcp-reliable)

**RTO** 是可靠性的**最后一道防线**：丢包、严重失序或延迟尖峰使**快速重传**失效时，靠计时器溢出强制恢复。RTT 估计、重传路径与**伪超时**修正，直接影响拥塞控制稳定性。

---

<a id="ch14-1"></a>

## 14.1 引言

| 层次 | 机制 |
|------|------|
| 主动 | **快速重传**（重复 ACK） |
| 被动 | **计时器 RTO**（无 dup ACK 时） |

在「不确定」网络上用**反馈**建立可预期的重传行为 — 调优 `tcp_retries2`、`min_rto`、排障高延迟链路的理论基础。

---

<a id="ch14-2"></a>

## 14.2 简单超时与重传示例

段或 ACK 丢失、且无足够 **dup ACK** → 等待 **RTO 溢出** → 重传。

### 指数退避

| 次 | 典型间隔（初 RTO≈1.5s） |
|----|-------------------------|
| 1 | ~1.5s |
| 2 | ~3s（×2） |
| 3 | ~6s |
| 4 | ~12s |

```text
0.000s  seq 1:1025
1.500s  seq 1:1025   # 第1次重传
4.500s  seq 1:1025   # +3s
10.500s seq 1:1025   # +6s
22.500s seq 1:1025   # +12s
```

**本质**：拥塞时**主动降注入**（保守）；RTO **过小** → 伪重传浪费带宽；**过大** → 吞吐崩塌。

---

<a id="ch14-3"></a>

## 14.3 设置重传超时（RTO）

基于 **RTT 采样** 动态建模，非固定常数。

### 14.3.1 经典（RFC 793）

```text
SRTT ← α·SRTT + (1-α)·RTT_sample    （α 常 0.8~0.9）
RTO  ← f(SRTT)
```

对**方差/抖动**不敏感 → 抖动网络易误重传。

### 14.3.2 Jacobson / Karels（主流）

| 步骤 | 公式（概念） |
|------|----------------|
| 误差 | Err = M − SRTT（M 为本次 RTT） |
| 平滑 RTT | SRTT ← SRTT + g·Err（g≈1/8） |
| 方差 | RTTVAR ← RTTVAR + h·(‖Err‖−RTTVAR)（h≈1/4） |
| **RTO** | **SRTT + 4×RTTVAR** |

**为何 ×4？** 工程上覆盖多数 RTT 波动；延迟升高时 RTO **快速抬高**。

### 14.3.3 Linux 实践

| 参数 | 典型 |
|------|------|
| 初始 RTO | 现代常 **1s**（旧实现 3s） |
| 最小 RTO | 常 **200ms** 下限 |
| 时钟粒度 | 可达 **1ms**（旧系统 500ms 级） |

### 14.3.5 Karn 与 RTTM

| | 规则 |
|--|------|
| **Karn** | **重传段**的 RTT 样本**不**用于更新估计（ACK 可能对应原段或重传段，歧义） |
| **RTTM（时间戳选项）** | 每段（含重传）可测 RTT → **绕过 Karn 限制**；兼 **PAWS** [ch13](../chapter13-tcp-connection-manage/study.md#ch13-3) |

---

<a id="ch14-4"></a>

## 14.4 基于计时器的重传

- 有**未确认在途数据** → 启动/维持重传计时器  
- 收到**推进左边界**的新 ACK → **重启**计时器，超时 = 当前 **RTO**  
- 始终盯住**最老**未确认段

---

<a id="ch14-5"></a>

## 14.5 快速重传（Fast Retransmit）

**主动防御**：不依赖 RTO。

| 条件 | 动作 |
|------|------|
| **3 个重复 ACK**（dup ACK） | 认为该序号后数据**丢失**，**立即重传** |
| 为何是 3？ | 1–2 个 dup ACK 常由**失序**引起；3 个更可能真丢包 |

进入**快速恢复**（ch16）：在途量减半等，**不像超时**那样粗暴重置 cwnd。

→ [Top-Down §3.1](../../top_down/03_transport_layer/study.md#ch3-1-tcp-reliable)

---

<a id="ch14-6"></a>

## 14.6 带 SACK 的重传

累积 ACK 只说「连续收到 X 之前」→ 发送方不知**中间空洞**。

### SACK 选项

- **Left Edge / Right Edge** 描述已收到的**不连续块**  
- TCP 选项区常限 **~40B**（与时间戳等共存）→ 每段约 **3–4** 个 SACK 块

### Scoreboard（发送端）

用 SACK **精确定位空洞** → **只重传丢失段**，不重传已缓存段；**同窗多丢**时远优于纯快速重传。

→ 协商：[ch13 §SACK](../chapter13-tcp-connection-manage/study.md#ch13-3)

---

<a id="ch14-7"></a>

## 14.7 伪超时（Spurious Timeout）

RTO 溢出但数据**未丢**（延迟尖峰、路径切换）→ **伪重传**。

| 检测 | 思路 |
|------|------|
| **DSACK** | 接收方标明**重复**数据 → 若对应刚超时重传 → 伪超时 |
| **Eifel** | 时间戳 **TSecr** 指向**原段**非重传段 → 原段其实到了 |
| **F-RTO** | 无时间戳时，超时后先发**新数据**观 ACK，探测伪超时 |

**Eifel Response**：撤销误缩的 **cwnd / ssthresh**，避免性能雪崩。

---

<a id="ch14-8"></a>

## 14.8 包失序与包重复

| 现象 | 原因 | TCP 行为 |
|------|------|----------|
| **失序** | IP 多路径（ECMP）等 | dup ACK；**3 阈值**屏蔽轻微失序 |
| **重复** | 链路层重传等 | **SEQ 去重**；可能触发 **DSACK** 优化发送端 |

---

<a id="ch14-9"></a>

## 14.9–14.11 其他细节

| 节 | 要点 |
|----|------|
| **14.9 目的度量** | 路由缓存 **RTT/SRTT/RTTVAR**；新连接可**继承**，跳过不稳定初探 |
| **14.10 重新组包** | 重传时可**合并**小段为大段（≤ PMTU），提高效率 |
| **14.11 LDoS** | 周期性脉冲诱发连续指数退避 → 吞吐趋 **0**（利用重传弱点） |

---

<a id="ch14-exam"></a>

## 14.12 总结与考点

| 支柱 | 内容 |
|------|------|
| **等多久** | SRTT → Jacobson **+4×RTTVAR**；Karn；时间戳 RTTM |
| **怎么补** | Timer → **3 dup ACK** → **SACK 精准补洞** |
| **误报** | DSACK / Eifel / F-RTO **撤销**误降 cwnd |

### 易混

| 问题 | 要点 |
|------|------|
| 超时 vs 快速重传 | 超时**重置** cwnd 更狠；快速恢复较温和 |
| Karn | 重传段 RTT **不采样**（无时间戳时） |
| 3 dup ACK | 丢包 vs **失序** 的工程折中 |
| 伪超时 | 无线/移动网常见；需 DSACK/Eifel |

### 下一章

- [ch15 数据流与窗口](../chapter15-tcp-flow-window/study.md) — rwnd、Nagle  
- [ch16 拥塞控制](../chapter16-tcp-congestion-control/study.md) — 超时与 cwnd

---

## Top-Down

- [§3.1 可靠传输](../../top_down/03_transport_layer/study.md#ch3-1-tcp-reliable)  
- [§3.7 拥塞](../../top_down/03_transport_layer/study.md#ch3-1-tcp-cong)

## Lab

- Wireshark：`tcp.analysis.retransmission`、`tcp.analysis.duplicate_ack`  
- `ss -ti` 看 `rto`、`rtt`

## Go / Rust

- **Go**：`TCPConn.SetKeepAlive`；`SetWriteDeadline`；注意 **TLS 下**仍走 TCP 重传语义  
- **sysctl**：`net.ipv4.tcp_retries2`、`tcp_syn_retries`（与 ch13 SYN 区分）  
- **排障**：高 RTT 链路先看 **RTO 是否过小**、是否缺 **SACK/时间戳**
