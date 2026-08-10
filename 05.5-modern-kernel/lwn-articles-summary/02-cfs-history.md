# CFS 调度器原理与历史 — 从 O(1) 到 EEVDF

> **原文:** [CFS scheduling](https://lwn.net/Articles/230501/) (LWN, 2007)
> **作者:** Ingo Molnar (补丁) · Jonathan Corbet (文章)
> **内核版本:** 2.6.23 (2007 合入) ~ 6.5 (2023 被 EEVDF 替代)
> **对标旧书:** ULK3 Ch7 / LKD3 Ch4

---

## 核心观点

CFS (Completely Fair Scheduler) 于 2007 年合入 Linux 2.6.23，替代了之前的 O(1) 调度器。CFS 的设计哲学是**精确的公平性**：每个进程按其权重获得精确比例的 CPU 时间。

### O(1) 调度器的问题

O(1) 调度器（2.6 ~ 2.6.22）使用优先级数组 + 时间片：

- 每个优先级一个队列，查找下一个进程 O(1)
- 但时间片分配基于启发式规则，"公平性"是近似的
- 进程交互性判断 (sleep_avg) 复杂且不可预测
- 跨 CPU 负载均衡困难

### CFS 的核心设计

| 组件 | 说明 |
|------|------|
| **vruntime** | 每个进程的虚拟运行时间，按实际运行时间 × (NICE_0_LOAD / 进程权重) 计算 |
| **红黑树** | 按 vruntime 排序，最左节点 = vruntime 最小 = 下一个运行 |
| **min_vruntime** | 追踪运行队列中最小的 vruntime，用于新进程初始化 |
| **sched_entity** | 调度实体，支持进程、组调度 (cgroup) |

### CFS 调度决策

1. **选下一个进程**：取红黑树最左节点（最小 vruntime）
2. **时间片计算**：`time_slice = (sched_period × 进程权重) / 总权重`
3. **时钟中断**：更新当前进程 vruntime，若超出时间片则重新调度
4. **新进程加入**：vruntime 初始化为 `max(min_vruntime, 当前进程 vruntime)`

### CFS 的 nice 值

- nice 值 -20 ~ 19，映射到权重 88761 ~ 15
- nice 值每差 1，CPU 份额比约差 10%
- nice 0 的权重 = 1024 (NICE_0_LOAD)

---

## 与旧书差异

| ULK3 (2.6) 讲的 | CFS 实际实现 | 说明 |
|-------------------|-------------|------|
| O(1) 调度器优先级数组 | 红黑树按 vruntime 排序 | ULK3 Ch7 完全过时 |
| `recalc_task_prio()` | 已删除 | CFS 不需要重新计算优先级 |
| 交互式进程启发式 | CFS 用 vruntime 自然处理 | 不再需要 sleep_avg |
| `array_expired` / `array_active` | 单一红黑树 | 双数组设计已废弃 |
| 时间片 = 基于优先级查表 | 时间片 = 按权重比例计算 | 计算方式完全不同 |

> **注意:** LKD3 (2010) 已覆盖 CFS，但其描述基于 2.6.34，后续版本有大量更新（如 group scheduling、bandwidth control）。

---

## HFT 关联

| 场景 | CFS 的影响 |
|------|-----------|
| **SCHED_FIFO 交易线程** | 不受 CFS 管辖，但 CFS 线程的负载均衡可能抢占 RT 线程的 CPU |
| **wakeup 延迟** | CFS 的 wakeup preemption 可能让低优先级线程抢占交易线程 |
| **解决方案** | `isolcpus` + `IRQ affinity` + `SCHED_FIFO` 完全隔离 |

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** CFS 为什么用红黑树而不是堆来维护 vruntime 排序？

> 红黑树支持高效的插入 (O(log n))、删除 (O(log n)) 和查找最左节点 (O(log n))，同时支持遍历和范围查询。堆虽然找最小值 O(1)，但删除任意节点和遍历不如红黑树方便。此外红黑树在内核中已有广泛应用 (CFS、VMA、定时器)。

**Q2:** 新创建的进程 vruntime 如何初始化？为什么不是 0？

> 初始化为 `max(min_vruntime, 当前进程 vruntime)`。如果是 0，新进程会独占 CPU 直到 vruntime 追上其他进程。用 min_vruntime 确保新进程不会"欠债"太多，也不会立即抢走 CPU。

**Q3:** O(1) 调度器的"O(1)"指的是什么？CFS 是 O(1) 吗？

> O(1) 指选择下一个进程的时间复杂度恒定（优先级数组按优先级遍历，找到第一个非空队列即可）。CFS 选择最左节点是 O(log n)（红黑树），但实际上由于缓存的帮助和进程数通常不大，性能差异可忽略。CFS 的优势在公平性而非渐近复杂度。

</details>
