# CFS 运行时机制 — 一次调度的完整链条

> **定位:** `02-cfs-history.md` 讲设计与历史，本篇讲**运行时怎么转**：tick → vruntime → 选任务 → 抢占判定。
> **对标旧书:** LKD3 Ch4 描述基于 2.6.34，机制主体沿用至今（6.6 被 EEVDF 替代）。

---

## 数据结构定位

```c
struct rq {              // 每 CPU 一个
    struct cfs_rq cfs;   // CFS 运行队列（另有 .rt 给实时类）
};
struct cfs_rq {
    struct rb_root_cached tasks_timeline;  // 红黑树 + 最左节点缓存
    struct sched_entity *curr;             // 当前在 CPU 上跑的实体
    u64 min_vruntime;                      // 单调递增基准线
};
```

树上挂的是 **`sched_entity`（调度实体）不是 `task_struct`**——组调度（cgroup）时一个实体代表一整组任务。

> **修正（对照 02 笔记 Q3）:** 取最左节点是 **O(1)** 不是 O(log n)——`rb_root_cached` 把 `rb_leftmost` 指针单独缓存，pick 只读一个指针。插入/删除才是 O(log n)。

---

## ① 时钟 tick → `update_curr()`

```c
delta_exec = now - curr->exec_start;                      /* 这次上 CPU 跑了多久（真实 ns） */
curr->vruntime += delta_exec * NICE_0_LOAD / se->weight;  /* 折算成虚拟时间 */
cfs_rq->min_vruntime = max(cfs_rq->min_vruntime, curr->vruntime); /* 基准线只涨不降 */
```

**权重决定折算率：**

| 进程 | 权重 | 跑 1ms 后 vruntime 涨 |
|------|------|----------------------|
| nice 0 | 1024 | 1ms |
| nice -10 | 8192 | 0.125ms |
| nice +10 | 110 | ~9.3ms |

> **CFS 最核心一句：公平不是把时间片分得匀，是让所有任务的 vruntime 涨得一样快。**
> 高权重任务涨得慢 → 在树里"落后"更久 → 占更多真实 CPU。

---

## ② 选下一个任务

```
schedule() → pick_next_task_fair() → 读 rb_leftmost 指针（O(1)）
           → prev 从树上摘下/插回，next 设为 curr
```

- 最左节点 = vruntime 最小 = **最"久没跑"的任务**
- 不遍历树、更不碰全局任务链表（对照 [2.5 任务列表 vs 运行队列](./05-task-list-vs-runqueue.md)）

---

## ③ 抢占判定 → `check_preempt_tick()`

两个条件任一成立 → 置 `TIF_NEED_RESCHED`，等内核返回用户态/中断返回时 `schedule()`：

| 条件 | 公式 | 含义 |
|------|------|------|
| **配额用完** | `delta_exec > ideal_runtime` | `ideal_runtime = sched_period × 自身权重 / 总权重` |
| **有人追上** | 最左节点 vruntime 领先 curr 超过粒度阈值 | 新唤醒者已明显"更饿" |

**sched_period 是动态的：**

- runnable ≤ 8：周期 = sched_latency（默认 6ms），每任务保 `min_granularity`（0.75ms）
- runnable > 8：周期按 `nr_running` 扩展——进程再多也不把切换切得太碎

---

## 休眠唤醒：`place_entity()` 与 min_vruntime 锚定

休眠进程不在树上，vruntime 冻结在睡前的值。醒来直接插树会有问题：

| 若唤醒时 | 后果 |
|----------|------|
| vruntime 严重落后（长期睡眠） | 醒来独占 CPU"还债"，别的任务全部停摆 |
| 无任何补偿 | 频繁短睡的任务（如等待网络）被系统性惩罚 |

解法：唤醒时把 vruntime **拉到 `cfs_rq->min_vruntime` 附近**（`place_entity()`，sleep 奖励默认只给一半——GENTLE_FAIR_SLEEPERS）。`min_vruntime` 只增不减，是整个队列的"当前公平时刻"锚点，新进程/唤醒进程/负载均衡迁移都用它对表。

> sleep 奖励的尺度争议（奖励多少才不鼓励"睡一下骗补偿"）是 CFS 后期无解的难题之一，也是 EEVDF 用 deadline 显式建模延迟的动机。

---

## 与旧书差异

| LKD3 (2.6.34) | 现代内核 |
|----------------|----------|
| `rb_root` + `__pick_next_entity` 沿左链下走 | `rb_root_cached` 缓存最左，pick O(1) |
| sched_latency / min_granularity 是固定 sysctl | 动态周期；6.6 后随 EEVDF 重构 |
| place_entity 的 sleeper bonus 讲得较细 | 机制仍在，参数语义多次调整 |

---

## HFT 关联

| 点 | 影响 |
|----|------|
| **wakeup preemption 粒度** | 决定 SCHED_OTHER 辅助线程被唤醒后多久真正上 CPU——唤醒到运行的毛刺来源 |
| **SCHED_FIFO 不进这条链** | 交易线程在 `rq->rt`，CFS 的 vruntime/tick 与它无关 |
| **同核 CFS 抖动** | CFS 线程的 wakeup 抢占会污染 RT 线程的 cache/TLB → isolcpus + IRQ affinity 隔离的机制根源 |
| **tick 频率** | 调度判定的最小粒度受 HZ / tickless（NO_HZ_full）影响——隔离核常配 nohz_full 减少无用 tick |

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** 两个任务权重 1024 和 2048，CFS 下它们实际获得的 CPU 时间比是多少？为什么？

> 1:2。vruntime 涨速 = NICE_0_LOAD/weight，权重 2048 的涨速是一半；要让两者 vruntime 同步增长，真实时间必须按权重反比分配。

**Q2:** CFS 取下一个任务的真实复杂度是多少？为什么不是 O(log n)？

> O(1)。`rb_root_cached` 缓存了最左节点指针，pick 直接读缓存。O(log n) 的是插入和删除（红黑树再平衡）。

**Q3:** 一个任务睡了 10 秒后唤醒，它的 vruntime 远落后于 min_vruntime，会发生什么？如果没有 place_entity 会怎样？

> place_entity 把它的 vruntime 拉到 min_vruntime 附近（给一半 sleep 奖励），插入红黑树。若没有这个锚定，它 vruntime 最小成为最左节点，会连续霸占 CPU 直到 vruntime 追上其他任务——"还债"期间其他任务全部停摆。

**Q4:** 为什么 sched_period 要随 runnable 数量动态扩展而不是固定 6ms？

> 固定 6ms 在任务多时每任务只能分到极小的时间片，上下文切换开销占比失控。扩展周期保证每任务至少有 min_granularity（0.75ms）的连续运行时间。

</details>
---
