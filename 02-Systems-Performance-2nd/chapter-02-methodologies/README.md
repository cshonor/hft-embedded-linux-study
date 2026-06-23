# Ch 2 方法论 · Methodologies

> **Systems Performance 2nd** · Brendan Gregg · **精读**

> 本章定位：**全书方法论基石** — 工具、内核版本、硬件会换，但术语、模型、USE/RED、排队论、统计与可视化这套「持久技能」不会过时。Ch 1 说了「先看业务再看资源」；本章给出**可重复执行的套路**。

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 2.1 HFT 术语与团队对齐 | [notes/section-2.1-HFT术语与团队对齐.md](./notes/section-2.1-HFT术语与团队对齐.md) |
| 2.2 术语→命令速查 | [notes/section-2.2-术语与命令速查.md](./notes/section-2.2-术语与命令速查.md) |
| 2.3 时间尺度与排查走查 | [notes/section-2.3-时间尺度与排查走查.md](./notes/section-2.3-时间尺度与排查走查.md) |
| 2.3 性能权衡 | [notes/section-2.3-性能权衡.md](./notes/section-2.3-性能权衡.md) |
| 2.3 负载与架构 | [notes/section-2.3-负载与架构.md](./notes/section-2.3-负载与架构.md) |
| 2.4 两种分析视角 | [notes/section-2.4-两种分析视角.md](./notes/section-2.4-两种分析视角.md) |
| 2.5 性能分析方法论 | [notes/section-2.5-性能分析方法论.md](./notes/section-2.5-性能分析方法论.md) |
| 2.6 排队论概览与 Kendall 记号 | [notes/section-2.6-排队论概览与Kendall记号.md](./notes/section-2.6-排队论概览与Kendall记号.md) |
| 2.6 M/M/1 拐点与预警线 | [notes/section-2.6-M-M-1-拐点与预警线.md](./notes/section-2.6-M-M-1-拐点与预警线.md) |
| 2.6 M/D/1 · M/M/c · M/G/1 | [notes/section-2.6-M-D-1-M-M-c-M-G-1.md](./notes/section-2.6-M-D-1-M-M-c-M-G-1.md) |
| 2.6 排队论计算器 | [notes/section-2.6-排队论计算器.md](./notes/section-2.6-排队论计算器.md) |
| 2.7 阿姆达尔与 USL | [notes/section-2.7-阿姆达尔与USL.md](./notes/section-2.7-阿姆达尔与USL.md) |
| 2.7 容量规划三步法 | [notes/section-2.7-容量规划三步法.md](./notes/section-2.7-容量规划三步法.md) |
| 2.8–2.10 统计与可视化 | [notes/section-2.8-2.10-统计与可视化.md](./notes/section-2.8-2.10-统计与可视化.md) |

---

## 大白话 · 本章就四件事

> 软件会变，下面这套思维不会。

**① 先统一「说话方式」。**

- **延迟 / 响应时间**：等多久；**吞吐量**：单位时间干多少活；**使用率**：资源忙不忙；**饱和度**：排队有多长；**瓶颈**：最卡的那一环。
- 量化里：tick 处理延迟、发单 RTT、CPU 利用率、网卡队列深度 — 都是同一套词。

**② 两个视角，别混着用 — 但要接力。**

| 视角 | 问什么 | 工具优先级 | 谁常用 |
|------|--------|------------|--------|
| **工作负载分析（先）** | 哪个环节、影响多大 | 埋点 counter / histogram → Prometheus / Grafana | 开发 / 策略 |
| **资源分析（后）** | 为什么会出问题 | `perf`、`ss`、`ethtool`、`tcpdump`、`dmesg` | 运维 / SRE |

HFT：**先 Workload**（Grafana 确认 tick 掉速、P99 10→100 μs、reject 突增）→ **再 Resource**（锁定进程/链路后 `perf`、抓包）。

**③ 别用「反面套路」，用 USE / RED / 延迟分解。**

- ❌ 只盯熟悉的工具（路灯下找钥匙）、乱改参数、甩锅给网络/DB。
- ✅ **USE**：每个资源看 Utilization / Saturation / Errors → [附录 A](../appendix-A-USE方法Linux.md)
- ✅ **RED**：每个服务看 Rate / Errors / Duration（微服务/cloud 常用）
- ✅ **延迟分解**：总延迟拆成几段，找最长那段（二分法）— 量化排查主武器。

**④ 数字别只看平均值；资源别跑太满。**

- 缓存命中极快、未命中极慢 → **平均值骗人**，看 **P99、热力图、火焰图**。
- 排队论：**M/M/1 ~70%**、**M/D/1 ~80%** — 见 [2.6 概览](./notes/section-2.6-排队论概览与Kendall记号.md)、[M/M/1 预警线](./notes/section-2.6-M-M-1-拐点与预警线.md)。
- 可扩展性：**Amdahl** 防盲目堆核；**USL** 预判 tick 2× 时的核数/网卡/内存 — 容量规划 **模型→压测→实盘** 三步闭环。

下面按原书 2.1–2.10 展开。

---

## 方法论速查 · USE vs RED vs 延迟分解

```
                    ┌─────────────────────────────────┐
                    │         发现问题               │
                    └───────────────┬─────────────────┘
                                    │
            ┌───────────────────────┼───────────────────────┐
            ▼                       ▼                       ▼
     Workload 视角            Resource 视角           延迟分解
     RED / 特征分析            USE 方法              分段 + 二分
     Rate Errors Duration      U S E per 资源         找最长段
            │                       │                       │
            └───────────────────────┴───────────────────────┘
                                    │
                                    ▼
                          统计：P99 / 热力图 / 火焰图
                                    │
                                    ▼
                          根因 → 箴言优先级 → 验证
```

---

## 本章学习目标 · 自检

- [ ] 能说清 **Latency / Throughput / Utilization / Saturation / Bottleneck**
- [ ] 能区分 **Load vs Architecture**、**Workload vs Resource** 分析
- [ ] 能列举三种 **Anti-Methods** 并对应到自己的工作习惯
- [ ] 对任意资源能套用 **USE**；对服务能套用 **RED**
- [ ] 会用 **延迟分解** 设计 tick-to-trade 排查步骤
- [ ] 理解排队论：**M/M/1 ~70%**、**M/D/1 ~80%**、**M/M/c** 多核；容量 **模型→压测→实盘**
- [ ] 知道 **热力图 / 火焰图** 各解决什么问题

---

## HFT 精读捷径（Ch 2 在路线中的位置）

```
Ch 1 概念与 60 秒清单
  → Ch 2 方法论（本章：USE / RED / 延迟分解 / 统计）
  → Ch 4 观测工具选型
  → Ch 6/7/10 资源层（落实 USE）
  → Ch 13 perf + Ch 15 BPF + 附录 A/C
```

**本章最小行动集：**

1. 画一条 **tick-to-trade** 时间线，标每段 P99。
2. 对 CPU + 网卡跑一轮 **USE 心智清单**（附录 A 展开）。
3. 选一个尖刺窗口，用 **热力图或追踪** 看是 workload 还是 resource。

---

## 相关章节

- 上一章：[../chapter-01-intro/](../chapter-01-intro/)
- 下一章：[../chapter-03-operating-systems/](../chapter-03-operating-systems/)
- USE Linux 清单：[appendix-A-USE方法Linux.md](../appendix-A-USE方法Linux.md)
- 观测工具：[../chapter-04-observability-tools/](../chapter-04-observability-tools/)
- 全书目录：[OUTLINE.md](../OUTLINE.md)
