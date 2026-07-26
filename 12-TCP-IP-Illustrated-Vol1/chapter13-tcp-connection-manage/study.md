# 第 13 章：TCP 连接管理

> 按书节速记：[13.1](13.1-introduction.md) · [13.2](13.2-tcp-connection-setup.md) · [13.3](13.3-tcp-options.md) · [13.4](13.4-tcp-pmtud.md) · [13.5](13.5-tcp-state-time-wait.md) · [13.6](13.6-tcp-rst.md) · [13.7](13.8-listen-queue-backlog.md) · [13.8](13.8-tcp-connection-attacks.md) · [13.9](13.9-summary.md) · [QUICKREF §13](../QUICKREF.md)

> 《TCP/IP 详解》卷1 第 2 版（Fall, 2016）· 精细化学习笔记（同步自 [tcpip_vol1_ed2_notes](../../tcpip_vol1_ed2_notes/04_transport_layer/ch13_tcp_connection.md)）  
> 前置：[ch12 TCP 初步](../chapter12-tcp-basic/study.md) · PMTUD：[ch08](../chapter08-icmpv4-icmpv6/study.md#ch08-3) · 自顶向下：[§3.1 连接/状态机](../../top_down/03_transport_layer/study.md#ch3-1-tcp-conn)

连接管理本质是**分布式状态一致性**：在不可靠 IP 上通过 **TCB** 维护有状态**虚拟电路**，支撑可靠重传、流控与拥塞控制。

---

<a id="ch13-1"></a>

## 13.1 引言

| 特性 | 说明 |
|------|------|
| **面向连接** | 传数据前完成 TCB 初始化与**序列号同步** |
| **全双工** | 两方向**独立**序列号与窗口 |
| **双向关闭** | 发送方向分别终止 → 状态机复杂 |

---

<a id="ch13-2"></a>

## 13.2 连接的建立与终止

握手/挥手是**资源准入**与参数协商阶段。

### 术语

**三路握手** · **四路挥手** · **ISN** · **MSS**

### 三路握手

```text
Client                          Server
  | --- SYN, Seq=ISNc -----------> |  SYN_RCVD
  | <--- SYN+ACK, Seq=ISNs, Ack=ISNc+1 --- |
  | --- ACK, Ack=ISNs+1 -----------> |  ESTABLISHED
```

**为何不是两路？** 防止**过期重复 SYN** 误建连接（RFC 793）；第三段 ACK 证明发起方**存活**。

| 考点 | 说明 |
|------|------|
| 严格时延 | 建连 **1.5 RTT**（SYN → SYN-ACK → ACK） |
| 考试简化 | 有时记 **1 RTT**（数据可随第三次捎带） |

→ 图示：[tcp_three_way_handshake.png](../../top_down/03_transport_layer/assets/tcp_three_way_handshake.png)

### 四路挥手

每方用 **FIN** 关闭**本向**发送流；全双工下对端可能仍有数据 → **半关闭**。

```text
主动方: FIN → FIN_WAIT_1 → … → TIME_WAIT → CLOSED
被动方: ACK → CLOSE_WAIT → FIN → LAST_ACK → CLOSED
```

**易混**：不必总是 4 个独立报文；被动方若同时关闭，可 **ACK+FIN 合并** → 类似 3 步。

→ 图示：[tcp_four_way_handshake.png](../../top_down/03_transport_layer/assets/tcp_four_way_handshake.png)

### 13.2.x 要点

| 主题 | 要点 |
|------|------|
| **半关闭** | `shutdown()` 只关一端；`close()` 全关。rsh/传文件后仍要读服务器尾响应 |
| **ISN (13.2.3)** | 非从 0/1 起；RFC 1948 等**时间+哈希**，防预测与旧报文干扰 |
| **SYN 超时 (13.2.5)** | 丢 SYN → 指数退避重传（首约 1s）→ 影响 fail-fast 设计 |
| **同时打开/关闭** | 竞态 → **SYN_RCVD**、**CLOSING** 等专门状态 |
| **中间盒 (13.2.6)** | NAT/防火墙改写或丢弃 SYN/选项 → [ch07](../chapter07-firewall-nat/study.md) |

---

<a id="ch13-3"></a>

## 13.3 TCP 选项

协议「进化舱」；首部最大 **60 B**，选项与载荷需权衡。

| 选项 | Code | 长 | 功能 | 何时 |
|------|------|-----|------|------|
| **MSS** | 2 | 4 | 最大段大小，避免 IP 分片 | **仅 SYN** |
| **Window Scale** | 3 | 3 | 窗口左移，>64KB 接收窗 | **仅 SYN** |
| **SACK Permitted / SACK** | 4,5 | 变 | 选择性确认 | 协商在 SYN；块在数据段 |
| **Timestamps** | 8 | 10 | RTT 采样 + **PAWS** | SYN 协商后可持续 |

### 机制

- **WSCALE**：突破 16 位窗口 **65535** → 高 BDP 吞吐关键  
- **PAWS**：高速下 SEQ **回绕**；用时间戳拒绝**过旧延迟段**  
- **易混**：MSS、WSCALE **连接建立后不可改**

---

<a id="ch13-4"></a>

## 13.4 路径 MTU 发现（PMTUD）

异构网中 **IP 分片**代价高（任一片丢 → 整报文废）。

1. 发 **DF=1** 的 IP 报文  
2. 中间 MTU 不足 → 丢弃 + **ICMPv4 Type3 Code4**（需要分片）  
3. TCP 据此**缩小 MSS**，避免再触发分片  

**IPv6**：路由器**不分片** → **ICMPv6 Packet Too Big (Type 2)** → 必须调 MSS。

→ [ch08](../chapter08-icmpv4-icmpv6/study.md#ch08-3) · 防火墙**勿拦**此类 ICMP

---

<a id="ch13-5"></a>

## 13.5 TCP 状态转换

排障地图：**CLOSE_WAIT 堆积**、**TIME_WAIT 端口耗尽** 等。

### 典型路径

| 角色 | 路径 |
|------|------|
| 主动打开 | CLOSED → **SYN_SENT** → ESTABLISHED |
| 被动打开 | CLOSED → **LISTEN** → SYN_RCVD → ESTABLISHED |
| 主动关闭 | ESTABLISHED → **FIN_WAIT_1** → FIN_WAIT_2 → **TIME_WAIT** → CLOSED |
| 被动关闭 | ESTABLISHED → **CLOSE_WAIT** → LAST_ACK → CLOSED |

### TIME_WAIT（2MSL）

| 目的 | 说明 |
|------|------|
| 可靠终止 | 确保最后 **ACK** 可达（丢则对端重 FIN） |
| 静默时间 | 旧连接**延迟报文**不致污染**同四元组**新连接 |

**风险**：**TIME_WAIT assassination** — 伪造 RST 可能提前结束 TIME_WAIT（实现相关）。

### FIN_WAIT_2

主动方等对端 FIN；对端应用不 `close()` → 卡死 → 内核 **tcp_fin_timeout**。

### 易混

**TIME_WAIT 不是 bug**：高并发用 `SO_REUSEADDR`、连接池、内核调优；慎盲目缩短 2MSL。

---

<a id="ch13-6"></a>

## 13.6 重置（RST）

异常**紧急刹车**：不四路挥手，直接拆 TCB。

| 场景 | 行为 |
|------|------|
| 端口无监听 | **RST** |
| 应用 `SO_LINGER` 异常终止 | 可能丢发送缓冲；RST **不可靠** |
| **半开连接** | 一端崩溃重启，对端仍发数据 → 无状态方 **RST** |

**RFC 5961**：仅当 RST 的 SEQ **在接收窗口内**才接受 → 减劫持难度。

---

<a id="ch13-7"></a>

## 13.7 服务器选项与连接队列

伸缩瓶颈常在**内核队列**，非带宽。

| 队列 | 内容 |
|------|------|
| **半连接（SYN 队列）** | **SYN_RCVD** 的 TCB |
| **全连接（Accept 队列）** | 握手完成、待 `accept()` |

**backlog**（Linux）：主要限制 **Accept 队列**；满时常**丢弃 ACK 诱发重传**而非 RST，给缓冲余地。

---

<a id="ch13-8"></a>

## 13.8 相关攻击

连接管理**预分配资源** → SYN Flood 温床。

| 威胁 | 防御 |
|------|------|
| **SYN Flood** | 填满半连接队列 |
| **SYN Cookies** | 握手不预分配 TCB；状态编码进 **ISN**；合法 ACK 后再建 TCB |
| **ISN 预测** | 强随机 ISN（RFC 1948） |

→ 中间盒：[ch07](../chapter07-firewall-nat/study.md#ch07-7)

---

<a id="ch13-exam"></a>

## 13.9 总结与考点

| 维度 | 要点 |
|------|------|
| 建连 | 三路握手、防旧 SYN、**1.5 RTT** |
| 拆连 | 四路挥手、半关闭、可合并 FIN/ACK |
| 选项 | MSS/WSCALE 仅 SYN；Timestamp+PAWS |
| PMTUD | DF + ICMP；v6 **PTB** |
| 状态 | **TIME_WAIT 2MSL**、CLOSE_WAIT 泄漏 |
| 服务器 | SYN 队列 vs Accept 队列、backlog |
| 安全 | SYN Cookie、RFC 5961 RST |

### 下一章

- [ch14 超时与重传](../chapter14-tcp-timeout-retransmit/study.md)  
- [ch15 数据流与窗口](../chapter15-tcp-flow-window/study.md)

---

## Top-Down

- [study.md §3.1 连接](../../top_down/03_transport_layer/study.md#ch3-1-tcp-conn)  
- [SEQ/ACK 全表](../../top_down/03_transport_layer/study.md#ch3-1-tcp-seq-full)

## Lab

- Wireshark：`tcp.flags.syn==1`、`tcp.analysis.flags`  
- `ss -tan state time-wait` / `CLOSE-WAIT` 计数  
- `sysctl net.ipv4.tcp_tw_reuse`（理解后再改）

## Go / Rust

- **Go**：`SetKeepAlive`、`SetLinger`；短连接风暴 → **连接池** + 复用  
- **Rust**：`tokio::net::TcpStream`；服务端 `listen(backlog)`  
- **排障**：大量 **TIME_WAIT** → 是否主动关闭方、是否可池化/长连接
