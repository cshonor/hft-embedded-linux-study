# 11.1 TCP 的错误恢复特性

> 本章：[chapter-summary.md](./chapter-summary.md) · 全书：[../README.md](../README.md) · 入门：[§8.1 延伸](../chapter-08-transport-layer-tcp-udp/03-tcp-reliability-flow-control.md) · Expert：[§5.8](../chapter-05-advanced-feature/08-expert-info.md)

**核心主旨**：高延迟/卡顿优先看 **重传、重复 ACK、快速重传**——TCP 自愈机制在包里的留痕。

## 核心知识点

### 11.1.1 TCP 重传（Retransmission）

| 概念 | 说明 |
|------|------|
| 用途 | 应对丢包（拥塞、队列满、链路错误、应用停读等） |
| **RTT** | 发段 → 收到对应 ACK 的时间 |
| **RTO** | 由 RTT 样本估算的重传超时阈值 |

**指数退避（超时重传）**

```text
发段并启动计时器
  → RTO 内无 ACK → 认为丢失，重传
  → RTO 翻倍，再重传…
  → 直到收到 ACK 或达最大重传次数（Windows 常 5，Linux 常 15）→ 断开
```

| Wireshark | 表现 |
|-----------|------|
| 列表 | 常 **黑底红字** `[TCP Retransmission]` |
| Details | `SEQ/ACK analysis` 中可见 RTO、与上次发送间隔 |

**过滤器**：`tcp.analysis.retransmission` · `tcp.analysis.rto`

---

### 11.1.2 重复确认与快速重传

**序号规则**：`收到的 Seq + 载荷长度 = 发出的 Ack`（期望下一字节）

| 事件 | 说明 |
|------|------|
| **Duplicate ACK** | 收到**乱序**段 → 认为中间有洞 → 重复发送**期望 Seq** 的 ACK |
| **Fast Retransmission** | 发送方连续收到 **3 个重复 ACK** → **不等 RTO** 立即重传丢失段 |
| **SACK** | TCP 选项选择性确认 → 只重传**丢失段**，不必重传洞后全部 |

| Wireshark | 过滤器 |
|-----------|--------|
| Dup ACK | `tcp.analysis.duplicate_ack` |
| 快速重传 | `tcp.analysis.fast_retransmission` |

**因果链（简）**

```text
丢包 → 接收端 Dup ACK ×3+ → 发送端 Fast Retransmit
     或 超时 → Retransmission（RTO 退避，更慢）
```

> **拓展**：SACK 在 Options 中为左右边界块；`tcp.options.sack` 过滤（视版本）。

## 抓包/实操记录

| 练习 | 操作 |
|------|------|
| 找重传 | Expert → Warning；`tcp.analysis.retransmission` |
| 对比 | 同连接是否先出现 3× Dup ACK 再 Fast Retrans |
| RTT 图 | `Statistics` → TCP Stream Graphs → Round Trip Time |

```bash
tshark -r slow.pcapng -Y "tcp.analysis.retransmission" -T fields -e frame.time_relative -e tcp.seq -e tcp.analysis.rto
```

## 疑问与总结

- **超时重传**比**快速重传**更伤延迟（RTO 指数增大）。
- 抓包点若在**接收端**，可能看不到重传包本身，但能看到 **Dup ACK**（见 §11.3）。
