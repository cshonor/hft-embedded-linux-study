# 第 12 章：TCP 传输控制协议（初步）

> 按书节速记：[12.1](12.1-introduction.md) · [12.2](12.2-tcp-service-feature.md) · [12.3](12.3-tcp-packet-header.md) · [12.4](12.4-summary.md) · [QUICKREF §12](../QUICKREF.md)

> 《TCP/IP 详解》卷1 第 2 版（Fall, 2016）· 精细化学习笔记（同步自 [tcpip_vol1_ed2_notes](../../tcpip_vol1_ed2_notes/04_transport_layer/ch12_tcp_intro.md)）  
> 对照 UDP：[ch10](../chapter10-udp-ip-fragment/study.md) · 端到端：[ch01](../chapter01-overview/study.md#ch01-e2e) · 自顶向下：[§3.1 TCP](../../top_down/03_transport_layer/study.md#ch3-1-tcp-conn)

**TCP** 在尽力而为的 IP 之上抽象出**可靠、全双工字节流** — 后续 ch13–16（连接、超时、窗口、拥塞）的理论基石。

---

<a id="ch12-1"></a>

## 12.1 引言

### 复杂性分配（端到端论点）

IP 保持核心**简单快速**；**可靠性、流控、拥塞控制**推向边缘 **TCP 协议栈**。

| 特性 | 含义 |
|------|------|
| **面向连接** | 三次握手建立并维护**同步状态** |
| **可靠性** | 确认 + 重传 → 无丢、无错、有序、无重复 |
| **字节流** | **不保留**应用报文边界；多次 `write` 可一次 `read` 读出 |

### 与 UDP

| | UDP | TCP |
|--|-----|-----|
| 保护 | 校验和（弱） | **序列号空间** + ACK 闭环 |
| 机制 | 无重传 | 定位每字节 → 去重、排序、重传 |

```text
发送方 --[SEQ 数据段]--> 接收方
接收方 --[ACK 下一期望字节]--> 发送方
```

→ 深入连接与序号：[ch13](../chapter13-tcp-connection-manage/study.md) · [§3.1 握手/SEQ/ACK](../../top_down/03_transport_layer/study.md#ch3-1-tcp-seq-full)

---

<a id="ch12-1-arq"></a>

### 12.1.1 ARQ 与重传

**ARQ（自动重传请求）**：计时器 + 确认，把丢失不确定性变为**重传**。

| 术语 | 说明 |
|------|------|
| **停止等待** | 发一包等 ACK；LFN 上带宽利用率极低 |
| **ACK** | 接收成功 → 发送方步进 |
| **超时** | 超时未 ACK → 判定丢失并重传 |

现代 TCP：**流水线** — 未确认前可发多段 → **滑动窗口**。

**易混**：**ACK 号** = 期望收到的**下一个字节**序号（累积确认）。例：ACK **1001** → 字节 **1–1000** 已全部正确接收。

---

<a id="ch12-1-window"></a>

### 12.1.2 分组窗口与滑动窗口

通过增加**在途（in-flight）**数据填满带宽时延积（BDP）。

| 术语 | 说明 |
|------|------|
| **窗口大小** | 无需等待 ACK 即可发送的字节上限 |
| **发送窗口** | 当前允许发送的逻辑字节范围 |

**Stevens 三区**（发送方）：

1. 已发送且已确认（窗口左外）  
2. 已发送未确认（在途，窗口内）  
3. 允许发送未发送（可用窗口，窗口内）

| 动作 | 含义 |
|------|------|
| **合拢** | 新 ACK → **左缘右移**，释放已确认缓冲 |
| **张开** | 对端通告窗口增大 → **右缘右移**，可发更多 |
| **收缩** | 右缘左移（对端减小通告窗口）→ 规范**强烈不建议**，易与已发数据冲突 |

---

<a id="ch12-1-cwnd"></a>

### 12.1.3 变量窗口：流量控制与拥塞控制

| 窗口 | 来源 | 目的 |
|------|------|------|
| **通告窗口 rwnd** | 接收方 TCP 首部 **Window** 字段 | **流量控制**，防接收缓冲溢出 |
| **拥塞窗口 cwnd** | 发送方据**丢包/ECN**估算 | **拥塞控制**，防路径崩溃 |

**有效发送上限**：

```text
可发送字节数 ≤ min(rwnd, cwnd)
```

**零窗口**：rwnd=0 时连接**未断**；发送方发**零窗口探测**，直至对端重新开窗。

→ 详解：[ch15](../chapter15-tcp-flow-window/study.md) · [ch16](../chapter16-tcp-congestion-control/study.md) · [QUICKREF §五条易混](../QUICKREF.md)

---

<a id="ch12-1-rto"></a>

### 12.1.4 设置重传超时（RTO）

| 原则 | 说明 |
|------|------|
| RTO 过短 | 不必要重传 → **加剧拥塞** |
| RTO 过长 | 丢包恢复慢 |

- **RTT 采样**：段发出到 ACK 返回的时间  
- **Jacobson/Karels**：RTO 由 RTT **均值 + 方差** 推导，抖动大时 RTO 增大  
- **Karn 算法**：重传段的 RTT 样本**不参与**估计，避免歧义

→ [ch14 超时与重传](../chapter14-tcp-timeout-retransmit/study.md)

---

<a id="ch12-2"></a>

## 12.2 TCP 的引入

### 12.2.1 服务模型

| 特性 | 说明 |
|------|------|
| **全双工** | 两端可同时收发；**各自**序号、窗口、计时器 |
| **无记录边界字节流** | 应用须用长度前缀/分隔符解析逻辑报文 |

### 12.2.2 可靠性机制

1. **端到端校验和**（含伪首部）  
2. **序列号**：每字节坐标 — 乱序、重复、丢包  
3. **累积 ACK**：确认连续收到的最高字节；默认无 **SACK** 时丢中间一段会阻塞确认（可选 SACK 扩展）

---

<a id="ch12-3"></a>

## 12.3 TCP 首部与封装

标准首部 **20 字节**（含选项时可更长）。图示：[tcp_header.png](../../top_down/03_transport_layer/assets/tcp_header.png)

| 字段 | 位宽 | 作用 |
|------|------|------|
| 源/目的端口 | 16+16 | **四元组**之一；多路复用 |
| **序列号 seq** | 32 | 字节流位置；防回绕 |
| **确认号 ack** | 32 | 期望下一字节（ACK 标志置位时有效） |
| 头部长度 | 4 | 以 **4 字节** 为单位 |
| **标志** | 各 1 | URG / **ACK** / **PSH** / **RST** / **SYN** / **FIN** |
| **窗口** | 16 | **rwnd** 通告 |
| 校验和 | 16 | 段 + **伪首部** |
| 紧急指针 | 16 | 与 URG 配合（少用） |

### 标志位速记

| 标志 | 典型场景 |
|------|----------|
| **SYN** | 建连、同步初始序号 |
| **ACK** | 确认有效 |
| **FIN** | 本方向无新数据，释放 |
| **RST** | 异常复位 |
| **PSH** | 尽快上交应用层（少缓冲） |

### 伪首部

计算校验和时加入：**源/目的 IP、协议号(6)、TCP 长度** — 故意跨层，确保段投递到**正确 IP 与协议**，弥补 IP 不校载荷的缺口。

→ 与 UDP 伪首部对比：[ch10 §10.3](../chapter10-udp-ip-fragment/study.md#ch10-3)

### 封装

```text
应用字节流 → TCP 段(首部+数据) → IP 载荷(Protocol=6)
```

**MSS**：TCP 单次可发最大段载荷，通常 **MTU − IP头 − TCP头**（避免 IP 分片）→ [ch03 MTU](../chapter03-link-layer/study.md#ch03-8)

---

<a id="ch12-exam"></a>

## 12.4–12.5 总结与考点

三大支柱：**序列号**、**确认**、**窗口管理** — 不依赖路径中间节点保证，靠**端系统智能**实现可靠通信。

### 易混速记

| 问题 | 要点 |
|------|------|
| seq vs ack | **seq**=本段数据起始字节；**ack**=期望对方下一字节 |
| 累积 ACK | ACK N 表示 **< N** 的字节都已收到 |
| rwnd vs cwnd | 接收方通告 vs 发送方拥塞估计；发送取 **min** |
| TCP 段 vs IP 分片 | TCP **按 MSS 分段**；避免 IP 分片 |
| 窗口收缩 | 规范不推荐；实现需谨慎 |

### 后续章节

| 章 | 主题 |
|----|------|
| [ch13](../chapter13-tcp-connection-manage/study.md) | 三次握手/四次挥手、状态机、TIME_WAIT |
| [ch14](../chapter14-tcp-timeout-retransmit/study.md) | RTO、快速重传 |
| [ch15](../chapter15-tcp-flow-window/study.md) | 滑动窗口、Nagle、延迟 ACK |
| [ch16](../chapter16-tcp-congestion-control/study.md) | 慢启动、拥塞避免、AIMD |

### RFC

| RFC | 内容 |
|-----|------|
| 793 | TCP 规范 |
| 1122 | 主机要求 |
| 5681 | 拥塞控制（与 ch16 衔接） |

---

## Top-Down

- [study.md §3.1](../../top_down/03_transport_layer/study.md#ch3-1) · [§3.2 四元组复用](../../top_down/03_transport_layer/study.md#ch3-2)

## Lab

- Wireshark：`tcp` 过滤器，观察 seq/ack/win flags  
- 对比同连接 UDP 53 与 TCP 443 首部

## Go / Rust

- **Go**：`net.TCPConn`；`SetNoDelay`（Nagle，ch15）；`TCPConn` 读写为**流**无消息边界  
- **Rust**：`tokio::net::TcpStream`；应用层用 `length-delimited` 等 framing  
- **实践**：长肥管道调大窗口/BDP；勿混淆应用缓冲与 TCP 缓冲
