# 13.3 TCP 选项

> 章级精读：[../study.md#ch13-3](../study.md#ch13-3)

## 本节核心目标

掌握握手期协商的 **MSS、Window Scale、Timestamps/SACK**。

---

## 必知选项

| 选项 | 作用 |
|------|------|
| **MSS** | 本端愿收的**最大 TCP 载荷**；避免 IP 分片 |
| **Window Scale** | 将 rwnd 左移，突破 64KB |
| **Timestamps** | RTTM 测 RTT；**PAWS** 防高速下序号回绕 |
| **SACK Permitted** | 允许选择性确认 → [ch14](../../chapter14-tcp-timeout-retransmit/study.md) |

---

## 协商时机

- 主要在 **SYN / SYN-ACK** 中携带；连接期内能力确定。

---

## 考点

- 无 WSCALE → 高 BDP 链路 rwnd 成瓶颈。
- 无 Timestamp → 部分栈 RTT 估计变差；PAWS 依赖时间戳。
