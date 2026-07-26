# 第 17 章：TCP 保活（Keepalive）

> 按书节速记：[17.1](17.1-introduction.md) · [17.2](17.2-keepalive-description.md) · [17.3](17.3-keepalive-attacks.md) · [17.4](17.4-summary.md) · [QUICKREF §17](../QUICKREF.md)

> 《TCP/IP 详解》卷1 第 2 版（Fall, 2016）· 精细化学习笔记（同步自 [tcpip_vol1_ed2_notes](../../tcpip_vol1_ed2_notes/04_transport_layer/ch17_tcp_keepalive.md)）  
> 半开连接：[ch13 §RST](../chapter13-tcp-connection-manage/study.md#ch13-6) · 零窗口探测：[ch15 Persist](../chapter15-tcp-flow-window/study.md#ch15-5) · NAT 超时：[ch07](../chapter07-firewall-nat/study.md)

TCP 空闲时可**无限期静默** — 对端崩溃而本端仍 **ESTABLISHED** 会耗尽 **FD/TCB**。**Keepalive** 是可选补丁：空闲一段时间后发**探测段**，区分对端存活、崩溃、重启与路径故障。

---

<a id="ch17-1"></a>

## 17.1 引言

### 战略矛盾

**端到端纯粹性** vs **服务器资源可用性**：高并发下需及时清理 **半开连接（Half-open）**，避免应用阻塞在**僵尸 Socket** 上。

| 概念 | 说明 |
|------|------|
| **Keepalive Timer** | 空闲计时；**有数据/ACK 活动则重置** |
| **半开连接** | 一端已崩溃/掉线/重启，另一端仍认为连接有效 |

### 要点

| | 说明 |
|--|------|
| **规范** | **可选**，非 RFC 强制 |
| **默认** | 经典 **~2 小时** 空闲才探测（4.4BSD 传统） |
| **局限** | 只证明**对端内核**活着，**不能**发现应用死锁 |

### 探测流程

1. 发探测 → 期望 ACK  
2. 收到 ACK → 重置保活定时器  
3. 收到 **RST** → 对端已重启/无 TCB → **立即关闭**  
4. 无响应 → **固定间隔重试**（如 75s × 9–10 次）→ 超时关闭

### 易混

| 误区 | 事实 |
|------|------|
| 保活 = LB 健康检查 | LB 常测**应用**；TCP 保活只在**内核**交互 |

---

<a id="ch17-2"></a>

## 17.2 描述

探测段利用状态机：**过期序列号也必须回 ACK**。

### 探测报文构造

- 通常**无真实应用数据**（或 1 字节 **garbage** 兼容旧栈）  
- **SEQ = SND.NXT − 1**（“已确认过的旧序号”）→ 对端回 **ACK** 证明存活

| 步骤 | 动作 | SEQ | ACK |
|------|------|-----|-----|
| 1 | Probe | SND.NXT−1 | RCV.NXT |
| 2 | 对端 ACK | 其 SND.NXT | 其 RCV.NXT |

### 四种响应场景

| 场景 | 本地行为 |
|------|----------|
| **对端正常** | 正确 ACK → 重置保活计时 |
| **对端崩溃未重启** | 多次重试，状态仍 **ESTABLISHED** 直至计数耗尽 |
| **对端崩溃后重启** | **RST** → 立即关闭 |
| **路径中断** | 可能收 ICMP（Host Unreachable）；继续重试至上限 |

### vs 窗口探测（Window Probe）

| | 保活 Probe | Window Probe [ch15](../chapter15-tcp-flow-window/study.md#ch15-5) |
|--|------------|-----------------------------------------------------------|
| 原因 | **连接空闲** | 对端通告窗口 **= 0** |
| 目的 | 对端是否还活着 | 窗口何时变大 |

### 重传

保活重传常用**固定间隔**（如 **75s**），**不走**常规 RTO 指数退避。

---

<a id="ch17-2-1"></a>

## 17.2.1 保活举例

### 与 NAT/防火墙

默认 **2h** 保活常 **大于** NAT/防火墙 **空闲超时**（数分钟）→ 映射被**静默删除（Silent Drop）** → 后续数据无 ICMP → 连接“假活”。

→ [ch07 NAT 状态](../chapter07-firewall-nat/study.md#ch07-3)

### Linux 参数（典型）

| sysctl | 含义 | 默认量级 |
|--------|------|----------|
| `tcp_keepalive_time` | 空闲多久开始探测 | **7200s** |
| `tcp_keepalive_intvl` | 探测间隔 | **75s** |
| `tcp_keepalive_probes` | 探测次数上限 | **9** |

**RST 加速**：对端重启后**首包探测**即可收 RST，检测从数十分钟缩到**毫秒级**。

### 时序（示意）

```text
T+0        最后一次数据
T+2h       探测 1，无响应
T+2h+75s   探测 2 …
…          共 9 次
T+~2h11m   ETIMEDOUT / 连接销毁
```

---

<a id="ch17-3"></a>

## 17.3 相关攻击

| 威胁 | 说明 |
|------|------|
| **防火墙打洞** | 周期保活**喂养** NAT/状态防火墙表，维持非法长连接（如 C2） |
| **保活风暴** | 极短间隔 × 海量连接 → 带宽/CPU 尖峰 |
| **侦察** | 分析保活 ACK 行为 → **栈指纹** |

**状态防火墙**：可校验 SEQ 是否符合 **SND.NXT−1** 逻辑，伪造探测可被丢。

---

<a id="ch17-exam"></a>

## 17.4 总结与考点

### 架构取舍

| | TCP 保活 | 应用层心跳 |
|--|----------|------------|
| 验证 | **内核**可达 | **业务**健康 |
| 开销 | 低 | 略高 |
| 典型用途 | **喂 NAT/防火墙/SLB** 映射 | 死连接清理、业务 SLA |

**洞察**：保活主要防**中间盒映射超时**；真正可用性靠 **App Heartbeat**。

### 复盘

- 检测半开、加速对端重启后的 **RST**  
- 三参数：**空闲时间 / 间隔 / 次数**  
- 探测 **SEQ=NXT−1**；与 **Persist** 区分  

### 下一章

- [ch18 安全](../chapter18-network-security/study.md)

---

## Top-Down

- 长连接、TIME_WAIT：[ch13](../chapter13-tcp-connection-manage/study.md#ch13-5)

## Lab

- `ss -o` 看 `timer:(keepalive,...)`  
- `sysctl -a | grep keepalive`  
- Wireshark：`tcp.analysis.keep_alive`

## Go / Rust

- **Go**：`net.TCPConn.SetKeepAlive(true)`、`SetKeepAlivePeriod`  
- **Rust**：`socket2` `set_keepalive`  
- **实践**：K8s/gRPC 常 **应用 ping** + 较短 TCP keepalive 喂 LB
