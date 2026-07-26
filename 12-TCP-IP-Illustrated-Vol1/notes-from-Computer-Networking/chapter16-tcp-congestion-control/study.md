# 第 16 章：TCP 拥塞控制

> 按书节速记：[16.1](16.1-introduction.md) · [16.2](16.2-classic-algorithms.md) · [16.3](16.3-algorithm-improvements.md) · [16.4](16.4-eifel-response.md) · [16.5](16.5-extended-example.md) · [16.6](16.6-shared-congestion-state.md) · [16.7](16.7-tcp-friendliness.md) · [16.8](16.8-high-speed-cubic.md) · [16.9](16.9-delay-based-cc.md) · [16.10](16.10-bufferbloat.md) · [16.11](16.11-aqm-ecn.md) · [16.12](16.12-congestion-attacks.md) · [16.13](16.13-summary.md) · [QUICKREF §16](../QUICKREF.md)

> 《TCP/IP 详解》卷1 第 2 版（Fall, 2016）· 精细化学习笔记（同步自 [tcpip_vol1_ed2_notes](../../tcpip_vol1_ed2_notes/04_transport_layer/ch16_tcp_congestion.md)）  
> 流控：[ch15 rwnd](../chapter15-tcp-flow-window/study.md#ch15-5) · 重传：[ch14](../chapter14-tcp-timeout-retransmit/study.md) · 自顶向下：[§3.1 拥塞](../../top_down/03_transport_layer/study.md#ch3-1-tcp-cong)

**流量控制** = 收发双方**私约**（不压垮接收缓冲）；**拥塞控制** = 维护**公共资源**的「最高法律」。TCP 在无路由器直接指令下，靠**端到端观测**推断路径容量。

---

<a id="ch16-1"></a>

## 16.1 引言

| 术语 | 说明 |
|------|------|
| **拥塞** | 入网流量逼近/超过链路或缓冲能力 → 时延↑、丢包 |
| **cwnd** | 发送方据网络状况限制**在途数据** |
| **发送窗口 W** | **W = min(cwnd, awnd)**（awnd = 通告窗口） |

| | 流量控制 | 拥塞控制 |
|--|----------|----------|
| 反馈 | **显式**（rwnd） | **隐式**（丢包、时延、ECN） |
| 范围 | 端到端接收能力 | **全网**路径 |

**工作点**：吞吐–负载曲线的 **膝部（Knee）**（高吞吐、低时延）vs **崖部（Cliff）**（拥塞崩溃）。

### 易混

- 拥塞控制**不是**路由器主动算速率的标准职责 → **端到端**；路由器**丢包/ECN** 参与反馈  
- 即使网络很快，**awnd 小**仍限制吞吐

---

<a id="ch16-2"></a>

## 16.2 经典算法

哲学：**AIMD** — 无拥塞时**加性增**，检测到拥塞时**乘性减**。

→ 图示：[tcp_congestion_control.png](../../top_down/03_transport_layer/assets/tcp_congestion_control.png)

### 16.2.1 慢启动（Slow Start）

- 初值：**cwnd = 1×SMSS**  
- **每收到一个 ACK → cwnd += 1** → 每个 RTT 约**翻倍**（指数）  
- 直至 **cwnd ≥ ssthresh** 进入拥塞避免  

「慢」是相对旧协议一次发满 awnd 而言。

### 16.2.2 拥塞避免（Congestion Avoidance）

- **cwnd ≥ ssthresh**：每 RTT **cwnd += 1**（每 ACK **+1/cwnd**）→ **线性**增长

### 16.2.3 超时后的行为

**RTO 超时**（严重拥塞信号）：

- **ssthresh ← cwnd/2**（或基于在途量，实现相关）  
- **cwnd ← 1** → 重新**慢启动**

### 16.2.4 Tahoe / Reno / 快速恢复

| 算法 | 丢包信号 | 反应 |
|------|----------|------|
| **Tahoe** | 3 dup ACK 或超时 | **cwnd=1**，慢启动 |
| **Reno** | 3 dup ACK | **快速恢复**：ssthresh=cwnd/2，cwnd=ssthresh+3×SMSS；恢复期每多 1 dup ACK → cwnd+1 |

### 16.2.5 标准 TCP（RFC 5681）

整合 Reno 族 + [ch14](../chapter14-tcp-timeout-retransmit/study.md) 快速重传；快速恢复中 **dup ACK 与允许新发** 的「守恒」关系。

---

<a id="ch16-3"></a>

## 16.3 对标准算法的改进

### NewReno

**多包同窗丢失**：Reno 可能多次减半；NewReno 用**部分 ACK（Partial ACK）** 在未确认完所有重传前数据前**留在快速恢复**，连续补洞。

### SACK + 拥塞控制

**Scoreboard** 精确定位空洞 → 少无效重传，丢包期维持较高有效利用率 → [ch14 §SACK](../chapter14-tcp-timeout-retransmit/study.md#ch14-6)

### FACK / Rate Halving

- **FACK**：用 SACK 更激进推断丢包  
- **Rate Halving**：拥塞后**每 2 ACK 发 1 新包**，平滑替代 Reno 瞬时 cwnd 跳变

### Limited Transmit

窗口小、dup ACK **1–2 个**时也允许**发 1 新段** → 凑够 3 dup ACK 触发快速重传，**避免 RTO**

### CWV（Congestion Window Validation）

长闲置连接 **缩小陈旧 cwnd**，避免闲置后突发冲击。

---

<a id="ch16-4"></a>

## 16.4 伪 RTO：Eifel 响应

无线等导致 **RTT 尖峰** → **伪超时** → cwnd 无谓减半。

- 用 **时间戳（TSOPT）** 判断 ACK 对应**原传**还是重传  
- 若伪超时 → **撤销** ssthresh/cwnd 削减 → [ch14 §14.7](../chapter14-tcp-timeout-retransmit/study.md#ch14-7)

---

<a id="ch16-5"></a>

## 16.5–16.7 扩展、共享状态、TCP 友好性

| 节 | 要点 |
|----|------|
| **16.5 扩展例** | 多丢包 + SACK 下状态机在重传与新发间切换更平滑 |
| **16.6 共享拥塞状态** | 同路径多连接（同站多图）可**共享**统计，减重复探测 |
| **16.7 TCP 友好性** | 非 TCP（UDP）若不降速 → **饿死** TCP；需模拟 **AIMD** 自律 |

---

<a id="ch16-8"></a>

## 16.8–16.9 高速与基于延迟的算法

### LFN 问题

标准 TCP **线性增 cwnd** → 填满高 **BDP** 需**数千 RTT**。

### BIC / CUBIC

**CUBIC**（Linux 默认）：`W(t) = C(t-K)³ + W_max`  
- 远离上次丢包点：**快增**  
- 接近推测容量：**缓增**  
- 再探测上限  

### Vegas / FAST（基于延迟）

| | 丢包型（Reno/CUBIC） | 延迟型（Vegas） |
|--|----------------------|-----------------|
| 信号 | **事后**丢包 | **RTT 微增**预测 |
| 竞争 | — | 常对 CUBIC **过早谦让** |

---

<a id="ch16-10"></a>

## 16.10–16.12 Bufferbloat、AQM/ECN、攻击

### Bufferbloat

路由器**过大缓冲** → 排队时延秒级 → TCP 反馈**迟钝**。

### AQM 与 ECN

| 机制 | 作用 |
|------|------|
| **RED** | 队列介于 min_th–max_th 时**概率**丢/标；减**全局同步** |
| **ECN** | IP 标 **CE**；ACK **ECE** 回传 → 发送方降 cwnd **不必等丢包** |

→ [ch05 DS/ECN](../chapter05-ip-protocol/study.md#ch05-2)

### 拥塞相关攻击

| 攻击 | 机制 |
|------|------|
| **ACK 分割** | 拆 ACK 诱导按**个数**增 cwnd |
| **Opt-ACK** | 数据未到先发 ACK |

防御：按 ACK 确认的**字节数**而非 ACK **个数** 更新 cwnd。

---

<a id="ch16-exam"></a>

## 结语与考点

| 维度 | 要点 |
|------|------|
| 公式 | **W = min(cwnd, awnd)** |
| 经典 | 慢启动、拥塞避免、AIMD、Tahoe vs Reno |
| 改进 | NewReno、SACK、Limited Transmit、CWV |
| 高速 | **CUBIC**；BBR（扩展阅读，非本书重点） |
| 网络侧 | Bufferbloat、RED、**ECN** |
| 伪超时 | Eifel + 时间戳 |

### 易混（QUICKREF 对齐）

**rwnd**（接收方） vs **cwnd**（网络）→ 发送 **min(两者)**

### 下一章

- [ch17 Keepalive](../chapter17-tcp-keepalive/study.md)  
- [ch18 安全](../chapter18-network-security/study.md)

---

## Top-Down

- [§3.1 拥塞](../../top_down/03_transport_layer/study.md#ch3-1-tcp-cong) · [§3.6–3.7](../../top_down/03_transport_layer/study.md#ch3-6)

## Lab

- `ss -ti` 看 `cwnd`、`ssthresh`  
- `sysctl net.ipv4.tcp_congestion_control`（cubic/bbr）  
- `tc qdisc` + RED/ECN 实验（进阶）

## Go / Rust

- 高 BDP：调大 socket 缓冲、确保 **WSCALE**；理解默认 **cubic**  
- 实时 UDP 流：自行 **AIMD** 或 GCC，避免饿死同路径 TCP
