# Ch 6 利用请求级和数据级并行的仓储级计算机 · Warehouse-Scale Computers

> **Computer Architecture 6th** · Hennessy & Patterson · **跳过 ⚪**（HFT 实盘非 WSC 场景）

> 本章定位：**从单机到数据中心** — 数万服务器组成的 **仓储级计算机（WSC）** 如何靠 **请求级并行（RLP）+ 数据级并行** 支撑搜索、云、批处理。与 colo **单机低延迟** 目标相反，但回测基础设施、风控批算、云研究环境仍值得建立对照。

**核心问题：** 当「一台机器」变成「一整仓机器」，架构假设如何变？**故障是常态、长尾是产品、一致性可放宽、电费是 OPEX 大头。**

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 6.1 引言 | [notes/section-6.1-引言.md](./notes/section-6.1-引言.md) |
| 6.2 编程模型与工作负载 | [notes/section-6.2-编程模型与工作负载.md](./notes/section-6.2-编程模型与工作负载.md) |
| 6.3 WSC 体系结构 | [notes/section-6.3-WSC体系结构.md](./notes/section-6.3-WSC体系结构.md) |
| 6.4 效率与成本 | [notes/section-6.4-效率与成本.md](./notes/section-6.4-效率与成本.md) |
| 6.5 云计算 | [notes/section-6.5-云计算.md](./notes/section-6.5-云计算.md) |
| 6.6 交叉领域问题 | [notes/section-6.6-交叉领域问题.md](./notes/section-6.6-交叉领域问题.md) |
| 6.7 谷歌 WSC 案例 | [notes/section-6.7-谷歌WSC案例.md](./notes/section-6.7-谷歌WSC案例.md) |
| 6.8 谬误与陷阱 | [notes/section-6.8-谬误与陷阱.md](./notes/section-6.8-谬误与陷阱.md) |

---

## HFT 精读捷径（对照阅读）

| WSC 思维 | HFT 对照 |
|----------|----------|
| **容忍长尾**（备份任务） | **消灭 P99** — 慢请求 = 滑点/拒单 |
| **最终一致性** | 交易所/清算 **强一致**、确定性撮合 |
| **故障常态** | 实盘 **failover + 双活** 但单 tick 路径要确定性 |
| **跨机架延迟 300×** | colo **同机架/同主机** 优化 |
| **云规模经济** | 回测/研究上云；**生产 tick 路径裸金属** |

→ [Ch1 RLP/WSC 类别](../chapter-01-quantitative-design-fundamentals/notes/section-1.1-1.2-引言与计算机分类.md) · [Ch7 DSA](../chapter-07-domain-specific-architectures/)

---

## 相关章节

- 上一章：[chapter-05-thread-level-parallelism](../chapter-05-thread-level-parallelism/)
- 下一章：[chapter-07-domain-specific-architectures](../chapter-07-domain-specific-architectures/)
- 全书目录：[OUTLINE.md](../OUTLINE.md)
