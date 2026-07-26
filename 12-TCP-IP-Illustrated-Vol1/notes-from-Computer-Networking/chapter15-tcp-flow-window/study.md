# 第 15 章：TCP 数据流与窗口管理

> 按书节速记：[15.1](15.1-introduction.md) · [15.2](15.2-interactive-communication.md) · [15.3](15.3-delayed-ack.md) · [15.4](15.4-nagle-algorithm.md) · [15.5](15.5-flow-control-window.md) · [15.6](15.6-urgent-pointer.md) · [15.7](15.7-window-attacks.md) · [15.8](15.8-summary.md) · [QUICKREF §15](../QUICKREF.md)

> 《TCP/IP 详解》卷1 第 2 版（Fall, 2016）· 精细化学习笔记（同步自 [tcpip_vol1_ed2_notes](../../tcpip_vol1_ed2_notes/04_transport_layer/ch15_tcp_dataflow_window.md)）  
> 前置：[ch12 滑动窗口](../chapter12-tcp-basic/study.md#ch12-1-2) · [ch13 WSCALE](../chapter13-tcp-connection-manage/study.md#ch13-3) · 自顶向下：[§3.1 流控](../../top_down/03_transport_layer/study.md#ch3-1-tcp-flow)

TCP 是**闭环反馈控制系统**：在交互式低延迟与成块高吞吐之间，用**窗口**、**Nagle**、**延迟 ACK** 等启发式算法找平衡。

---

<a id="ch15-1"></a>

## 15.1 引言

| 类型 | 目标 | 典型 |
|------|------|------|
| **交互式数据流** | 低延迟 | SSH 按键、远程 shell |
| **成块数据流** | 高吞吐（填满 **BDP**） | FTP、大文件 |

**矛盾**：交互产生大量 **tinygram**（如 1B 数据 + 40B 头 → 开销极高）；需抑制小包又不致用户可感延迟。

**发送决策**：综合 **Flight Size（在途）** 与 **Advertised Window（对端通告）** 判断是否可发新数据。

---

<a id="ch15-2"></a>

## 15.2 交互式通信

### 模式与开销

- **交互模式**：每键一字节 → 一包（Rlogin/SSH）  
- **有效载荷占比**：1B 数据 + ~40B 头（20 IP + 20 TCP）→ **PPS** 压力巨大

### Rlogin 回显四步（高 RTT 下卡顿）

1. Client → Server：按键 1B  
2. Server → Client：**ACK**  
3. Server → Client：回显数据  
4. Client → Server：**ACK**  

高 RTT 下每键可见延迟；10 键/s ≈ **40 报文/s**（未优化时）。

### 易混

| | 应用层回显 | 传输层 ACK |
|--|------------|------------|
| 谁 | 远程 Shell | 内核 TCP **自动** |
| 作用 | 显示什么 | 确认收到 |

---

<a id="ch15-3"></a>

## 15.3 延时确认（Delayed ACKs）

接收端**暂缓纯 ACK**，提高带宽利用率、支持**捎带**。

| 概念 | 说明 |
|------|------|
| **Delayed ACK 计时器** | 常 **200ms** 量级（实现相关） |
| **捎带（Piggybacking）** | ACK 嵌入本端反向数据段 |

### 规则

1. 收到数据后**不立即** ACK，短时等待本端响应数据  
2. **必须发 ACK**：计时器到点；或收到**第二个全尺寸（MSS）**段（促发送方滑动窗口）

### 易混

**200ms** 多为协议栈**周期性 tick**（下一 tick 发 ACK），而非「收到后精确等 200ms」的单一计时器语义（实现细节因 OS 而异）。

---

<a id="ch15-4"></a>

## 15.4 Nagle 算法

发送端整形：**自时钟** — 「链路上最多一个未确认的小分组」。

**规则**：若待发 < **MSS** 且**仍有在途未确认数据** → **缓存**，不立即发。

### 15.4.1 Nagle + 延迟 ACK：经典死锁

```text
1. 发送方：发 1B → 等 ACK（Nagle 阻塞后续小包）
2. 接收方：收 1B → 启动延迟 ACK，应用暂无数据可捎带
3. 发送方：应用再产生小数据 → Nagle 仍禁止发
4. 直至接收方 ~200ms 后强发 ACK → 发送方才能继续
```

→ 交互应用可出现 **~200ms 卡顿**。

### 15.4.2 禁用 Nagle

金融、游戏等：**`TCP_NODELAY`**（Go `SetNoDelay(true)`）— 立即发，**PPS/开销上升**。

| | Nagle 开 | NODELAY |
|--|----------|---------|
| 带宽 | 省小包 | 开销大 |
| 延迟 | 可能 +200ms | 更低 |

---

<a id="ch15-5"></a>

## 15.5 流量控制与窗口管理

**Advertised Window**：接收方反向约束发送方，解决**收端慢于发端**。

### 15.5.1 滑动窗口

| 术语 | 含义 |
|------|------|
| **Offered / Advertised Window** | 接收方通告的接收能力 |
| **Usable Window** | 通告窗口 − 已发未确认 |
| **左沿** | 已确认边界；随 ACK 右移 |
| **右沿** | 允许发送的最大序号 |
| **SND.NXT** | 下一待发送字节 |

```text
| 已确认 | 已发未确认(在途) | 可发(可用窗口) | 不可发 |
  ^左沿      ^SND.NXT          ^右沿
```

→ 图示：[tcp_sliding_window.png](../../top_down/03_transport_layer/assets/tcp_sliding_window.png) · [ch12](../chapter12-tcp-basic/study.md#ch12-1-2)

### 15.5.2 零窗口与 Persist Timer

通告窗口 **= 0** → 发送方停发。若**窗口更新**丢失 → 可能永久死锁。

**Persist Timer**：周期发 **Window Probe**（1 字节探测），迫使对端 ACK 中带**新窗口**。

### 15.5.3 糊涂窗口综合征（SWS）

接收方通告**极小窗口** + 发送方发**极小段** → 链路被头部占满。

| 方 | 对策 |
|----|------|
| **接收方（Clark）** | 除非可增 **≥1 MSS** 或 **≥缓冲区一半**，否则不增大通告窗 |
| **发送方** | 仅当数据达 **MSS**、收到 ACK、或满足 Clark 阈值才发 |

### 15.5.4 大缓存与 Autotuning

**LFN / 大 BDP** 需 [Window Scale](../chapter13-tcp-connection-manage/study.md#ch13-3)（突破 64KB）。

Linux 等 **Autotuning**：动态调 **SO_RCVBUF/SO_SNDBUF** 相关内核缓冲，填满管道。

### 易混（必背）

| | Advertised Window (rwnd) | Congestion Window (cwnd) |
|--|--------------------------|---------------------------|
| 解决 | **接收方**慢 | **网络**堵 |
| 谁通告 | 接收方 TCP 首部 | 发送方**自行估算** |
| 有效发送 | **min(rwnd, cwnd)** | 同左 → [ch12](../chapter12-tcp-basic/study.md#ch12-1-3) |

---

<a id="ch15-6"></a>

## 15.6 紧急机制（Urgent）

模拟**带外**控制（如 Telnet 中断）。

| 字段 | 说明 |
|------|------|
| **URG 标志** | 存在紧急数据 |
| **Urgent Pointer** | 指向紧急数据**末字节之后**的偏移（加在 SEQ 上） |

接收方可 **SIGURG** 等异步通知；与现代 `send(MSG_OOB)` 语义因 OS 而异，新设计少用。

---

<a id="ch15-7"></a>

## 15.7 窗口相关攻击

**窗口缩减攻击**：伪造 ACK 使**通告窗口异常缩小**（右沿左移）。

若发送方已发数据超出收缩后窗口 → 状态紊乱 → **RST** 或截断 → **DoS**。

现代栈对窗口收缩有多项限制（RFC 7323 等上下文）。

---

<a id="ch15-exam"></a>

## 15.8 总结与考点

### 核心算法矩阵

| 机制 | 责任方 | 目标 | 影响 |
|------|--------|------|------|
| **Nagle** | 发送方 | 抑制小包 | 交互延迟；与延迟 ACK **死锁** |
| **延迟 ACK** | 接收方 | 少纯 ACK、捎带 | 可能拖慢窗口滑动 |
| **滑动窗口** | 双方 | 端到端流控 | 理论吞吐上界（与 cwnd 取 min） |
| **Persist** | 发送方 | 破零窗口死锁 | 极端负载存活 |
| **Clark / SWS** | 接收方 | 防糊涂窗口 | 成块传输效率 |

### 工程清单

- 交互延迟：**NODELAY** + 考虑对端延迟 ACK  
- 大文件：**勿** NODELAY；开 **WSCALE**、Autotuning  
- 零窗口：查 Persist / 应用读太慢  

### 下一章

- [ch16 拥塞控制](../chapter16-tcp-congestion-control/study.md) — cwnd、慢启动

---

## Top-Down

- [§3.1 流量控制](../../top_down/03_transport_layer/study.md#ch3-1-tcp-flow)

## Lab

- `ss -ti` 看 `rcv_space`、`snd_wnd`  
- 对比 SSH `TCP_NODELAY` 开/关的报文间隔

## Go / Rust

- **Go**：`tcpConn.SetNoDelay(true)`；`SetReadBuffer`/`SetWriteBuffer`  
- **Rust**：`socket2` `set_nodelay(true)`  
- **排障**：游戏/交易卡顿先查 **Nagle + 延迟 ACK** 组合
