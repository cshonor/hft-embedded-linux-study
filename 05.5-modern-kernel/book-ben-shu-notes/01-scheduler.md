# 调度器 (CFS → EEVDF)

> 笨叔《奔跑吧 Linux 内核》读书笔记
> 对应旧书: ULK3 / LKD3 (Linux 2.6)
> 对应现代内核: Linux 5.x / 6.x

---

## 本节要点

Linux 6.6 开始用 **EEVDF（Earliest Eligible Virtual Deadline First）** 替代 CFS（Completely Fair Scheduler）。EEVDF 由 Peter Zijlstra 提出，解决 CFS 的几个长期问题：

- **公平性更精确**：CFS 用 vruntime 做近似公平，EEVDF 用 eligible time + virtual deadline 做精确公平
- **延迟改善**：EEVDF 引入 deadline 概念，交互式任务（如 HFT 交易线程）能更快被调度
- **权重计算简化**：CFS 的权重 → vruntime 转换有精度损失，EEVDF 用 lag（累计欠债）直接量化公平性
- **淘汰 latency-nice 的 hack**：CFS 用 nice 值粗粒度控制延迟，EEVDF 有独立的 latency 优先级

CFS 的核心问题：vruntime 是单调递增的，长时间运行的任务 vruntime 远大于新创建任务，导致新任务被优先调度（"fork 优先"问题）。EEVDF 用 lag 归零机制解决。

---

## 与旧书对比

| ULK3 / LKD3 (2.6) | 笨叔 (5.x/6.x) | 变化原因 |
|--------------------|-----------------|----------|
| O(1) 调度器（2.6 早期） | EEVDF（6.6+） | CFS 的 vruntime 近似公平有精度问题 |
| CFS 红黑树（2.6.23+） | EEVDF 红黑树 + RB tree by deadline | 增加 deadline 排序维度 |
| nice 值 = 静态优先级 | nice + latency-nice | 延迟和 CPU 占比解耦 |
| sched_entity->vruntime | sched_entity->lag + deadline | lag 更精确表达公平性 |
| `sched_min_granularity_ns` | `EEVDF_MIN_QUANTUM` | 最小时间片概念保留但语义变化 |
| load_weight 直接乘除 | avg/runtime 计算 weight | 精度提升，避免 64 位溢出 |

---

## 关键数据结构 / 函数

```
// 源码路径: kernel/sched/fair.c (CFS/EEVDF 都在此文件)
//          kernel/sched/sched.h

struct sched_entity {
    struct load_weight      load;       // 权重
    struct rb_node          run_node;   // 红黑树节点
    struct list_head        group_node;
    unsigned int            on_rq;      // 是否在运行队列

    u64                     exec_start;  // 上次开始执行时间
    u64                     sum_exec_runtime;  // 累计执行时间
    u64                     vruntime;    // CFS: 虚拟运行时间
    u64                     prev_sum_exec_runtime;

    // EEVDF 新增字段 (6.6+):
    u64                     deadline;    // 虚拟截止时间
    u64                     min_deadline; // 子树最小 deadline
    s64                     lag;         // 公平性偏差（正=欠 CPU，负=多用）
    bool                    rel_deadline; // 是否有相对 deadline
};

// EEVDF 核心调度函数
static struct sched_entity *
pick_eevdf(struct cfs_rq *cfs_rq);  // 选最早 eligible + 最早 deadline

// CFS 核心调度函数 (旧)
static struct sched_entity *
pick_next_entity(struct cfs_rq *cfs_rq);  // 选最小 vruntime

// 权重转换
static unsigned long
sched_weight(unsigned long weight);  // EEVDF: avg → weight 映射
```

**关键变化**：`pick_eevdf()` 先过滤 eligible 的 sched_entity（lag ≤ 0），再在 eligible 集合中选 deadline 最小的。比 CFS 的「选最小 vruntime」更精确。

---

## HFT 关联

HFT 对调度器的核心需求是**确定性延迟**：

1. **SCHED_FIFO 仍可用**：EEVDF 只影响 CFS（SCHED_NORMAL/SCHED_BATCH），实时策略（SCHED_FIFO/SCHED_RR）走 RT 调度器，不受 EEVDF 影响
2. **减少干扰**：EEVDF 的 deadline 机制让交互式任务更快被调度，交易线程（即使 SCHED_NORMAL）的唤醒延迟可能改善
3. **PREEMPT_RT 兼容**：EEVDF 已在 PREEMPT_RT 内核中验证，实时补丁从 6.6 开始原生支持
4. **调优变化**：CFS 时代的 `sched_min_granularity_ns` / `sched_wakeup_granularity_ns` 在 EEVDF 中语义变化，HFT 调优参数需要重新评估
5. **退避策略**：HFT 如果用 SCHED_NORMAL 跑交易线程，EEVDF 的 lag 机制确保即使被唤醒后也不会过度占用 CPU（lag 归零）

**建议**：HFT 生产环境仍推荐 SCHED_FIFO + 绑核 + CPU 隔离（isolcpus/nohz_full），不依赖 CFS/EEVDF 的公平性保证。

---

## 自测

<details>
<summary>Q1: EEVDF 相比 CFS 解决了什么核心问题？</summary>

CFS 用 vruntime（虚拟运行时间）做近似公平调度，但 vruntime 单调递增导致新创建/唤醒的任务 vruntime 较小被优先调度（"fork 优先"问题）。EEVDF 引入 lag（累计公平性偏差）+ virtual deadline：lag ≤ 0 的任务才 eligible（可调度），在 eligible 集合中选 deadline 最早的。lag 在任务入队时归零，消除了 fork 优先问题。同时 deadline 机制让交互式任务获得更低延迟。

</details>

<details>
<summary>Q2: HFT 交易线程用 SCHED_FIFO，EEVDF 的变化有影响吗？</summary>

没有直接影响。EEVDF 只管理 SCHED_NORMAL/SCHED_BATCH 策略的任务，走 CFS 调度类（fair_sched_class）。SCHED_FIFO/SCHED_RR 走 RT 调度类（rt_sched_class），优先级高于 CFS，EEVDF 的任何变化都不影响 RT 任务。HFT 交易线程用 SCHED_FIFO + 绑核 + CPU 隔离是最佳实践，不依赖 CFS/EEVDF 的行为。但如果交易线程用 SCHED_NORMAL（某些场景需要），EEVDF 的 deadline 机制可能改善唤醒延迟。

</details>

<details>
<summary>Q3: EEVDF 的 lag 是什么？它如何保证公平性？</summary>

lag = 任务应得的 CPU 时间 - 实际获得的 CPU 时间。lag > 0 表示任务被欠了 CPU 时间（应该优先调度）；lag ≤ 0 表示任务已用够或超额（不 eligible）。任务从运行队列移除时 lag 归零（不累积历史偏差）。这比 CFS 的 vruntime 更精确：vruntime 只反映相对公平性，lag 直接量化欠债。新 fork 的任务 lag = 0（不优先也不落后），解决了 CFS 的 fork 优先问题。

</details>
