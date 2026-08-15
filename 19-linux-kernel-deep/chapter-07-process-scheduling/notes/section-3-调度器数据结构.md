## 3. 调度器核心数据结构

> Linux 2.6 **O(1) 调度器** — 按优先级索引，选下一进程为常数时间

---

### 一、运行队列 `runqueue`

| 要点 | 说明 |
|------|------|
| **每个 CPU 一个** `runqueue` | 存放该 CPU 上所有 **可运行** 进程 |
| 核心结构 | 2.6 调度器最重要的数据结构之一 |

→ `task_struct` 与 CPU 字段：[Ch 3](../chapter-03-processes/notes/section-3-进程描述符.md)

---

### 二、活动数组 vs 过期数组

`runqueue` 内两个 **`prio_array_t`**：

| 数组 | 内容 |
|------|------|
| **Active（活动）** | 时间片 **尚未耗尽** 的进程 |
| **Expired（过期）** | 时间片 **已耗尽** 的进程 |

**交换指针：** 当活动队列为空 → 交换 active/expired 指针 → 过期进程重新获得时间片，继续轮转。

**防饿死：** 低优先级进程不会永远排不上 — 一轮结束后进入新一轮。

---

### 三、防止 `fork()` 逃避调度

`copy_process()` 时：

- 父进程 **剩余时间片一分为二**
- 一半给父进程，一半给子进程

防止恶意 **不断 fork** 无限占用 CPU。

→ fork/COW：[Ch 3](../chapter-03-processes/notes/section-6-创建与销毁.md)

### 常见陷阱

1. 把 ULK 讲的 `runqueue` 结构当现代版——6.x 用 `cfs_rq`/`rt_rq`/`dl_rq` 分层结构，且 EEVDF 进一步改了 CFS 部分
2. 混淆 `sched_entity` 和 `task_struct`——CFS 操作 `sched_entity`（可代表组/进程），`task_struct` 内嵌 `sched_entity`
3. 以为运行队列是全局的——现代内核 per-CPU 运行队列（`rq`），负载均衡在 CPU 间迁移任务

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** 现代调度器的运行队列层级结构是什么？

<details><summary>答案</summary>

每 CPU 一个 `struct rq`，内含三个子队列：`cfs_rq`（CFS/EEVDF 任务）、`rt_rq`（RT 任务）、`dl_rq`（DEADLINE 任务）。调度时按优先级选：DEADLINE > RT > CFS。`rq` 还包含 `curr`（当前运行任务）、`clock`（CPU 时钟）、`nr_running`（总就绪数）。ULK 讲的全局 `runqueue` 数组已被 per-CPU `rq` 取代。

</details>

**Q2.** `sched_entity` 的作用是什么？为什么 CFS 不直接操作 `task_struct`？

<details><summary>答案</summary>

`sched_entity` 是调度实体，嵌在 `task_struct` 中。它支持组调度（cgroup）——一组进程作为一个 `sched_entity` 参与 CFS 调度，组内再按 CFS 分配。`sched_entity` 包含 `vruntime`、`load`（权重）、`rb_node`（红黑树节点）、`cfs_rq`（所属队列）。CFS 通过操作 `sched_entity` 实现进程/组/层级调度。

</details>

**Q3.** HFT 如何减少调度器对延迟的影响？

<details><summary>答案</summary>

① `isolcpus=N` 隔离 N 号核——该核上不跑普通任务，调度器几乎不触发。② `SCHED_FIFO` + 绑核——RT 线程独占该核，不会被 CFS 任务抢占。③ `nohz_full=N`——停止定时器中断，消除 `scheduler_tick()`。④ `rcu_nocbs=N`——RCU 回调迁移到其他核。⑤ `tuned profile=network-latency`——一键调优。

</details>

</details>

---

← [2. 调度策略](./section-2-调度策略与抢占.md) · 下一节 [4. 调度算法与核心函数](./section-4-调度算法与核心函数.md)
