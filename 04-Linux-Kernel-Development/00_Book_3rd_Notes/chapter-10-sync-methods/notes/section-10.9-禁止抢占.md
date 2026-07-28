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

→ [Ch 4.5 抢占](../chapter-04-process-scheduling/notes/section-4.5-抢占与上下文切换.md) · [12.10 per-CPU 分配](../chapter-12-memory-management/notes/section-12.10-每个-CPU-的分配.md)

---
