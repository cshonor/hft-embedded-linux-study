# Ch 16 案例研究 · Case Study

> **Systems Performance 2nd** · Brendan Gregg · **选读**（🟡 全书收官 · 新手可先读作「预告片」）

> 本章定位：**全书大阅兵** — 无新基础理论，用 Netflix 生产案例 **「An Unexplained Win（无法解释的性能提升）」** 串起 Ch 2 方法论 + Ch 6–11 资源视角 + Ch 13–15 工具链。  
> **Gregg 建议：** 新手 **可先读本章** 当实战预览，读完理论后再 **重温** — 第二次读会认出每一步对应的章。  
> **HFT：** 「为什么突然变快了」与「为什么变慢」 **同样值得查** — 暴露隐藏瓶颈、错误 baseline、或邻居/配置变化。

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 案例背景：An Unexplained Win | [notes/section-案例背景An-Unexplained-Win.md](./notes/section-案例背景An-Unexplained-Win.md) |
| 16.1.1–16.1.2 问题陈述与分析策略 | [notes/section-16.1.1-16.1.2-问题陈述与分析策略.md](./notes/section-16.1.1-16.1.2-问题陈述与分析策略.md) |
| 16.1.3–16.1.4 统计数据与静态配置 | [notes/section-16.1.3-16.1.4-统计数据与静态配置.md](./notes/section-16.1.3-16.1.4-统计数据与静态配置.md) |
| 16.1.5–16.1.6 PMC 与软件事件 | [notes/section-16.1.5-16.1.6-PMC-与软件事件.md](./notes/section-16.1.5-16.1.6-PMC-与软件事件.md) |
| 16.1.7–16.1.8 动态追踪与结论 | [notes/section-16.1.7-16.1.8-动态追踪与结论.md](./notes/section-16.1.7-16.1.8-动态追踪与结论.md) |
| HFT 版「Unexplained Win」演练模板 | [notes/section-HFT-版Unexplained-Win演练模板.md](./notes/section-HFT-版Unexplained-Win演练模板.md) |

---

## 大白话 · 本章就一件事

> **一个云生产谜题：系统莫名变快了 — 用全书套路把它讲清楚。**

不是新工具教程，而是一条 **可复制的排查叙事**：

```
问题陈述 → 策略（别乱试）
  → 统计 + 静态配置（浅层）
  → PMC + 软件事件（硬件/内核计数）
  → 动态追踪 + 栈（细粒度）
  → 结论（Drill-Down 拼拼图）
```

下面按原书 **16.1.1–16.1.8** 展开，并映射到本 handbook 各章。

---

## 全书知识地图（本章如何串书）

```
┌─────────────────────────────────────────────────────────┐
│  Ch 16 Case Study · An Unexplained Win                  │
└─────────────────────────────────────────────────────────┘
         │
    Ch 2 方法论 ─── 问题陈述、假设、Drill-Down、反反面模式
         │
    Ch 4 工具地图 ── 先 stat/配置，再 perf/BPF
         │
    ┌────┴────┬────────┬────────┬────────┐
  Ch 6 CPU  Ch 7 Mem  Ch 8–9 I/O  Ch 10 Net  Ch 11 Cloud
    └────┬────┴────────┴────────┴────────┘
         │
    Ch 12 基准 ─── 对比是否公平、WSS、Sanity Check
         │
    Ch 13 perf ─── stat PMC + record 火焰图
    Ch 14 Ftrace ─ graph / hwlat（若需要）
    Ch 15 BPF ─── runqlat、offcputime、tcpretrans…
         │
    附录 A USE ─── 各资源 U/S/E 核对
    附录 C bpftrace ─ ad hoc 收尾
```

---

## 推荐阅读顺序（Gregg + 本仓库）

| 读者 | 顺序 |
|------|------|
| **新手** | **先 Ch 16 预览** → Ch 1–2 → Ch 4 → 资源章 → Ch 13–15 → **再读 Ch 16** |
| **HFT 已有基础** | Ch 1–15 按 OUTLINE → **Ch 16 作总复盘** → 附录 A/C |
| **之后** | [03-BPF 专书](../../03-BPF-Performance-Tools/) · [11-HFT 工程](../../11-HFT-Low-Latency-Practice/) |

---

## 本章 Checklist

- [ ] 理解 **Unexplained Win** 为何值得查
- [ ] 能写出 **问题陈述 + 2–3 假设 + 验证顺序**
- [ ] 排查顺序：**统计 → 配置 → PMC → trace**
- [ ] 能把案例每一步 **映射到本仓库章节号**
- [ ] （可选）新手 **先读本章** 再读全书；二刷对照工具名

---

## Gregg 本章金句（HFT 版）

> 第十六章是 **全书演习** — 不是教新公式，是教 **在真实云生产里如何把公式用一遍**。  
> **「莫名变快」和「莫名变慢」一样，都是性能工程师的功课** — 不懂赢，就不懂输。

---

## 相关章节

- 上一章：[../chapter-15-bpf/](../chapter-15-bpf/)
- 下一章：[appendix-A-USE方法Linux.md](../appendix-A-USE方法Linux.md)
- 方法论：[../chapter-02-methodologies/](../chapter-02-methodologies/)
- 观测入门：[../chapter-01-intro/](../chapter-01-intro/) · [../chapter-04-observability-tools/](../chapter-04-observability-tools/)
- 资源专章：Ch [6](../chapter-06-cpus/)–[11](../chapter-11-cloud-computing/)
- 工具专章：Ch [13](../chapter-13-perf/)–[15](../chapter-15-bpf/)
- BPF 专书：[03-BPF-Performance-Tools](../../03-BPF-Performance-Tools/)
- HFT 压测：[11-HFT ch10](../../11-HFT-Low-Latency-Practice/chapter-10-延迟测量与基准压测/)
- 全书目录：[OUTLINE.md](../OUTLINE.md)
