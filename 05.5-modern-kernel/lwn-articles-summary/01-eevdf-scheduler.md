# EEVDF 调度器 — Linux 6.6+ CFS 替代方案

> **原文:** [The earliest eligible virtual deadline first scheduler](https://lwn.net/Articles/925371/) (LWN, 2023)
> **作者:** Peter Zijlstra (补丁) · Jonathan Corbet (文章)
> **内核版本:** 6.6+ (2023 年合入)
> **对标旧书:** ULK3 Ch7 / LKD3 Ch4 的 O(1) 调度器和 CFS 章节

---

## 核心观点

EEVDF (Earliest Eligible Virtual Deadline First) 是 2023 年合入 Linux 6.6 的全新调度器，**完全替代了自 2007 年起使用的 CFS (Completely Fair Scheduler)**。

### CFS 的缺陷

CFS 的核心理念是**公平性**——通过 vruntime 追踪每个进程已获 CPU 时间，优先调度 vruntime 最小的进程。但 CFS 无法让进程表达**延迟需求**：

- nice 值只调整 CPU 时间分配比例，不等于延迟保障
- 实时调度类 (SCHED_FIFO/RR) 可用于延迟敏感任务，但属于特权操作
- CFS 内部大量"脆弱的启发式规则"(heuristics) 来处理延迟问题，维护困难

### EEVDF 的三个核心概念

| 概念 | 含义 | 作用 |
|------|------|------|
| **Lag** | 进程应得时间 - 实际获得时间 | 正 lag = 未获公平份额，应优先调度；负 lag = 已超额 |
| **Eligible Time** | lag ≥ 0 的时刻 | 只有 eligible 的进程才能被调度运行 |
| **Virtual Deadline** | time_slice + eligible_time | 调度器选择 virtual deadline 最早的进程运行 |

### 如何解决延迟问题

EEVDF 引入 **latency-nice** 值：

- 低 latency-nice（严格延迟要求）→ 更短时间片 → 更近的 virtual deadline → 更快被调度
- 高 latency-nice（无延迟要求）→ 更长时间片 → 更远 deadline → 但获得相同总量 CPU
- **关键：两个相同 nice 值的进程获得相同总量 CPU 时间**，只是分片方式不同

这比 CFS 的启发式规则**更一致、更可预测**。

---

## 与旧书差异

| ULK3 / LKD3 讲的 | 6.x 现代实现 | 差异 |
|-------------------|-------------|------|
| O(1) 调度器（优先级数组 + 时间片） | 已删除，2.6.23 起 CFS 取代 | ULK3 Ch7 完全过时 |
| CFS: vruntime + 红黑树 | 6.6 起 EEVDF 取代 CFS | LKD3 Ch4 的 CFS 描述也已过时 |
| `recalc_task_prio()` | 已删除 | 不要在源码中查找 |
| `runqueue` 结构 | 改为 `cfs_rq` → `eevdf_rq` | 结构体重命名 |
| nice 值 = CPU 份额 | nice = 份额 + latency-nice = 延迟需求 | 新增延迟维度 |

### 关键代码变更

```
// CFS 时代 (2.6.23 ~ 6.5)
struct cfs_rq {
    struct rb_root_cached tasks_timeline;  // 红黑树
    struct sched_entity *curr;
    u64 min_vruntime;
    // ...
};

// EEVDF 时代 (6.6+)
struct cfs_rq {  // 结构体名暂未改，但内部已重构
    // 红黑树保留用于按 deadline 排序
    // 新增 lag 计算、eligible 判断、deadline 计算
    s64 avg_vruntime;
    u64 avg_load;
    u64 min_vruntime;
    // ...
};
```

EEVDF 仍使用虚拟时间和红黑树，但**选择逻辑从"最小 vruntime"变为"最早 eligible virtual deadline"**。

---

## HFT 关联

| 场景 | EEVDF 影响 |
|------|-----------|
| **SCHED_FIFO 交易线程** | 不受影响——EEVDF 只管 CFS 调度类 (SCHED_NORMAL)，实时类仍用 RT 调度器 |
| **非关键线程延迟** | latency-nice 可让日志/监控线程不抢占交易线程的时间片 |
| **延迟一致性** | EEVDF 比 CFS 调度更一致，减少尾延迟毛刺 |
| **核绑定 + isolation** | 仍需 `isolcpus` + `irq affinity` 隔离，EEVDF 不改变这一层 |

> **HFT 实盘：** 交易线程用 SCHED_FIFO + CPU isolation，EEVDF 不影响。但辅助线程的延迟毛刺会减少，系统整体更稳定。

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** EEVDF 的 lag 值为负意味着什么？该进程还能被调度吗？

> lag < 0 意味着进程已获得超过公平份额的 CPU 时间。此时进程**不合格** (ineligible)，不能被调度。直到虚拟时间推进使其 lag 回到 ≥ 0 时才重新变为合格。

**Q2:** latency-nice 和 nice 的区别是什么？

> nice 控制 CPU 时间**份额**（总量），latency-nice 控制时间片**长度**（分片粒度）。两个相同 nice 的进程获得相同总量 CPU，但不同 latency-nice 导致不同分片频率。低 latency-nice = 短时间片 = 更频繁被调度。

**Q3:** 为什么说 EEVDF 比 CFS "更一致"？

> CFS 用大量启发式规则处理延迟敏感场景（如 wakeup preemption、idle balance），这些规则在不同负载下表现不一致。EEVDF 通过 lag + deadline 将公平性和延迟统一在一个数学框架内，消除了大部分启发式规则，调度决策更可预测。

</details>
