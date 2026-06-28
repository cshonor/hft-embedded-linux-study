# 第10章 延迟测量与基准压测

> **Tick-to-Trade · Tail Latency · 先量后优**

← [chapter-01 实战建议](./chapter-01-高频交易基础与生态.md#实战启动建议)

---

## 1. 测什么

| 指标 | 含义 |
|------|------|
| **T2T** | 行情 **NIC 时间戳** → 订单 **离开发送队列** |
| **p50 / p99 / p999** | 平均不够 — **尾延迟** 决定风控 |
| **Jitter** | Turbo/HT/调度 **关** 后的稳定性 |

---

## 2. 怎么测

- **硬件时间戳**（NIC PHC / Switch mirror）
- **内联 probe**（`rdtsc` / `CLOCK_MONOTONIC_RAW` — 知悉开销）
- **JMH**（Java 预热与微基准）
- **Replay** — 生产 capture **离线压 Book+Strategy**

---

## 3. 原则

> 任何优化必须建立在 **精确端到端测量** 上；HFT 拼的不只是平均延迟，更是 **最大延迟的稳定性**。

→ [chapter-05](./chapter-05-操作系统内核极致调优.md) · [03-Systems-Performance](../03-Systems-Performance-2nd/)
