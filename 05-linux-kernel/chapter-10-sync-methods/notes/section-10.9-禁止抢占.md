## ⑨ 禁止抢占 · Disabling Preemption

有时并不需要「锁住其他 CPU」，只需保证 **当前任务在本 CPU 上不被调度走** — 典型是保护 **per-CPU 数据**。

| API | 作用 |
|-----|------|
| **`preempt_disable()`** | 禁止内核抢占（可嵌套计数） |
| **`preempt_enable()`** | 恢复；可能立即调度 |
| **`get_cpu()` / `put_cpu()`** | 禁用抢占并返回 CPU 编号（防迁移） |

#### 为何 per-CPU 还要禁抢占

```
CPU0 上任务 A 操作 per_cpu(var, 0)
  │
  若被抢占迁移到 CPU1 再继续
  │
  ▼
  可能改到错误的 per-CPU 槽 / 假设被打破
```

| 手段 | 防什么 |
|------|--------|
| `preempt_disable` | 被调度到别的 CPU / 中间插入同 CPU 其他内核路径（视场景） |
| 再加 `local_irq_disable` | 连中断也不要打断（更重） |

#### 与锁的关系

| 场景 | 做法 |
|------|------|
| 数据真·每 CPU 一份、无跨 CPU 共享 | 常 **只需禁抢占**（或 `local_bh_disable`） |
| 跨 CPU 共享 | **自旋锁 / 原子** 等 |
| 既 per-CPU 又要防中断 | `local_irq_save` 或 `*_irq` 锁变体 |

**规则：** `preempt_disable` 区间必须 **短**；里面 **禁止睡眠**。

**HFT：** 用户态「绑核 + 不主动阻塞」近似减少迁移；内核驱动里统计计数用 `this_cpu_*` / per-CPU + 禁抢占是常规手法。乱禁抢占 = 调度延迟 ↑。

→ [Ch 4.5 抢占](../../chapter-04-process-scheduling/notes/section-4.5-抢占与上下文切换.md) · [12.10 per-CPU 分配](../../chapter-12-memory-management/notes/section-12.10-每个-CPU-的分配.md)

### 常见陷阱

1. 混淆 preempt_disable() 和 local_irq_disable()——前者只禁抢占，后者还禁中断
2. 以为 preempt_disable() 后不能被中断——可以被中断，但不能被调度
3. 在 preempt_disable() 区域做耗时操作——会延迟调度器，增加系统延迟

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** preempt_disable() 的精确效果？

<details><summary>答案</summary>

① 递增 preempt_count 的 preempt 位。② 当前 CPU 上的内核代码不会被抢占（schedule() 检查 preempt_count == 0 才调度）。③ 中断仍可触发（hard IRQ）。④ softirq 仍可执行。⑤ 其他 CPU 不受影响。用于保护 per-CPU 数据（防止被另一进程在同 CPU 上访问）。对应 preempt_enable() 递减并检查 need_resched。

</details>

**Q2.** preempt_enable() 时如果 need_resched 被设置会怎样？

<details><summary>答案</summary>

`preempt_enable()` 递减 preempt_count，如果 preempt_count 归零且 `need_resched` 被设置 → `preempt_schedule()` → `schedule()` 切换到更高优先级任务。这就是内核抢占点。`preempt_enable_no_resched()` 不检查 need_resched（延迟到下一个抢占点），用于明确不需要立即调度的场景。

</details>

**Q3.** HFT 如何利用抢占控制降低延迟？

<details><summary>答案</summary>

① `SCHED_FIFO`：RT 线程不可被 CFS 抢占（只有更高 RT 或中断能抢占）。② `isolcpus`：隔离核上无其他任务，调度器几乎不触发。③ `nohz_full`：停止定时器中断，减少 `scheduler_tick()`。④ `preempt=full`：让非 RT 任务的内核路径也可被抢占（减少长尾延迟）。⑤ 内核模块中 `preempt_disable()` 临界区 <1us。

</details>

</details>

---
