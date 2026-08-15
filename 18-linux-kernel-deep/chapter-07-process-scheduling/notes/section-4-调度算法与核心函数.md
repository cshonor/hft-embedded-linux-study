## 4. 调度算法与核心函数

---

### 一、`scheduler_tick()` — 时钟 tick 入口

在 **时钟中断** 路径中（[Ch 6](../../chapter-06-timing/)）：

1. **递减** 当前进程时间片  
2. 若耗尽 → 移出 **活动队列**  
3. 设置 **`TIF_NEED_RESCHED`**

---

### 二、`recalc_task_prio()` — 动态优先级

- 更新 **平均睡眠时间**  
- 睡眠越久 → **Bonus 越高** → 动态优先级越高  
- 保证等 I/O 的进程唤醒后 **优先被调度**

---

### 三、`try_to_wake_up()` — 唤醒

将睡眠/停止的进程：

1. 状态 → **`TASK_RUNNING`**  
2. 插入目标 CPU 的 **`runqueue`**

→ 等待队列：[Ch 3](../../chapter-03-processes/notes/section-4-组织与查找.md)

---

### 四、`schedule()` — 调度核心

| 触发方式 | 场景 |
|----------|------|
| **直接调用** | 进程等资源 **主动阻塞** |
| **延迟调用** | 检测到 **`TIF_NEED_RESCHED`**（如 tick 返回、syscall 返回） |

**做什么：**

1. 在本地 `runqueue` 的 **活动数组** 中选 **最高优先级** 进程  
2. **`context_switch()`** — 切换地址空间 + 硬件上下文  

→ 切换细节：[Ch 3](../../chapter-03-processes/notes/section-5-进程切换.md) · TLB：[Ch 2](../../chapter-02-memory-addressing/notes/section-6-内存布局与TLB.md)

### 常见陷阱

1. 把 ULK 的 `recalc_task_prio()` / `schedule()` 当现代版——CFS 的 `schedule()` 逻辑完全不同（选红黑树最左节点）
2. 以为只有时间片耗尽才调度——CFS 还有唤醒抢占（唤醒的 vruntime 更小时直接抢占）
3. 混淆 `scheduler_tick()` 和 `schedule()`——前者更新统计 + 设标记，后者执行实际切换

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** CFS 的 `schedule()` 核心流程是什么？

<details><summary>答案</summary>

① `pick_next_task()`：从 `cfs_rq` 红黑树取最左节点（vruntime 最小）→ `sched_entity` → `task_struct`。② `put_prev_task()`：当前任务更新 vruntime，放回红黑树（如仍就绪）。③ `context_switch()`：切换 `mm_struct`（`switch_mm()`）+ 寄存器（`switch_to()`）。CFS 的 `schedule()` 比 O(1) 简单——不用维护优先级数组，红黑树 O(log n) 选下一个。

</details>

**Q2.** 唤醒抢占的判定条件是什么？

<details><summary>答案</summary>

唤醒的任务 Q 的 vruntime 比 current 的 vruntime 小一定阈值时，Q 抢占 current。阈值 = `sched_wakeup_granularity`（默认 1ms，可调）。效果：交互任务（键盘输入）唤醒后 vruntime 小（睡眠时不增长），立即抢占后台任务。这是 CFS 交互响应好的核心机制。EEVDF 改用 eligibility + deadline 判定，更精确。

</details>

**Q3.** HFT 如何测量调度延迟？

<details><summary>答案</summary>

① `cyclictest -p 99 -t 1 -a [core]`：RT 线程测最大调度延迟。② `perf sched`：记录调度事件，分析延迟分布。③ `bpftrace -e 'tracepoint:sched:sched_switch { ... }'`：追踪上下文切换。④ `/proc/[pid]/sched`：查看调度统计。目标：HFT 热路径调度延迟 <1us（绑核 + SCHED_FIFO + isolcpus）。

</details>

</details>

---

← [3. 数据结构](./section-3-调度器数据结构.md) · 下一节 [5. SMP 平衡](./section-5-SMP运行队列平衡.md)
> ↔ [LKD Ch04 §4.3 Linux-调度算法](../../../05-linux-kernel/chapter-04-process-scheduling/notes/section-4.3-Linux-调度算法.md)
