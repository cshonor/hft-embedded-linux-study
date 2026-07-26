# 12.1 引言

> 章级精读：[../study.md#ch12-1](../study.md#ch12-1) · ARQ：[#ch12-1-arq](../study.md#ch12-1-arq) · 窗口：[#ch12-1-window](../study.md#ch12-1-window)

## 本节核心目标

掌握可靠传输四大机制（**ARQ、滑动窗口、双窗、RTO**）— TCP/QUIC 共通理论。

---

## ARQ 与重传

- 发送后等待 **ACK**；超时未确认 → **重传**。
- 停-等太慢 → 见滑动窗口。

---

## 滑动窗口

- 在未 ACK 前提下可**连续发多段** → 填满管道（带宽时延积）。
- 发送/接收窗口界定**在途数据**上限。

---

## 变量窗口（两类）

| 类型 | 谁限制 | 目的 |
|------|--------|------|
| **流量控制 (rwnd)** | 接收方缓冲 | 别淹没接收端 |
| **拥塞控制 (cwnd)** | 网络容量 | 别塞爆路径 |

- 实际发送窗口 ≈ **min(rwnd, cwnd)** → [ch15](../../chapter15-tcp-flow-window/study.md)、[ch16](../../chapter16-tcp-congestion-control/study.md)

---

## RTO（重传超时）

- 不能写死固定值；基于 **RTT** 测量动态计算 → [ch14](../../chapter14-tcp-timeout-retransmit/study.md)
