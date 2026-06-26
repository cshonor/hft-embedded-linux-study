# Ch 5 线程级并行 · Thread-Level Parallelism

> **Computer Architecture 6th** · Hennessy & Patterson · **精读 🔴**

> 本章定位：**多核 MIMD** — ILP 见顶、功耗墙之后，性能靠 **多线程 + 共享内存**。HFT 必读：**false sharing、NUMA、缓存一致性、内存序** — 无锁队列与绑核策略的硬件根因。

**核心问题：** 多核如何 **正确且高效** 地共享数据？一致性（Coherence）与一致性模型（Consistency）有何不同？

```
SMP/UMA vs DSM/NUMA → 监听(MESI) vs 目录一致性
真共享 vs 伪共享 → 锁/原子原语 → SC vs TSO/释放一致性
```

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 5.1 引言与多处理器挑战 | [notes/section-5.1-引言与多处理器挑战.md](./notes/section-5.1-引言与多处理器挑战.md) |
| 5.2 缓存一致性与监听协议 | [notes/section-5.2-缓存一致性与监听协议.md](./notes/section-5.2-缓存一致性与监听协议.md) |
| 5.3 性能分析与伪共享 | [notes/section-5.3-性能分析与伪共享.md](./notes/section-5.3-性能分析与伪共享.md) |
| 5.4 目录式一致性与 DSM | [notes/section-5.4-目录式一致性与DSM.md](./notes/section-5.4-目录式一致性与DSM.md) |
| 5.5 同步基础 | [notes/section-5.5-同步基础.md](./notes/section-5.5-同步基础.md) |
| 5.6 内存一致性模型 | [notes/section-5.6-内存一致性模型.md](./notes/section-5.6-内存一致性模型.md) |
| 5.7–5.11 交叉问题、实例与展望 | [notes/section-5.7-5.11-交叉问题实例与展望.md](./notes/section-5.7-5.11-交叉问题实例与展望.md) |

---

## HFT 精读捷径

| 本节 | 带走什么 |
|------|----------|
| 5.1 | **NUMA** — `numactl`、本地内存分配；Amdahl 限制并行收益 |
| 5.2–5.3 | **MESI + false sharing** — 每核独立计数器、cache line 对齐 |
| 5.4 | 多路服务器 **目录协议** — 跨 socket 一致性流量 |
| 5.5–5.6 | **LL/SC、memory_order** — 无锁结构正确性 |
| 5.7–5.11 | 多核扩展极限 → 绑核、专用核、DSA |

→ [Ch2 false sharing](../chapter-02-memory-hierarchy-design/notes/section-2.3-缓存性能十项高级优化.md) · [01-CSAPP Ch12](../../01-CSAPP-3rd/chapter-12-concurrent-programming/) · [15-HFT ch7 无锁](../../../15-HFT-Low-Latency-Practice/chapter-07-无锁数据结构与内存布局.md)

---

## 相关章节

- 上一章：[chapter-04-vector-simd-gpu](../chapter-04-vector-simd-gpu/)
- 下一章：[chapter-06-warehouse-scale-computers](../chapter-06-warehouse-scale-computers/)
- 全书目录：[OUTLINE.md](../OUTLINE.md)
