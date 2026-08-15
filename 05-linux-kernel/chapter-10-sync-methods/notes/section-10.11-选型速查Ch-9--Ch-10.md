## 选型速查（Ch 9 + Ch 10）

一页做完「该用哪把锁」— 先问 **上下文** 与 **持有时间**，再问读写比。

#### 决策树

```
能在中断 / softirq / 原子上下文？
  │是
  ├─ 只需改一个计数/标志 ──► atomic
  ├─ 短临界区 ──► spinlock（共享给 ISR 则 irqsave / bh）
  ├─ 读极多写极少且写要及时 ──► seqlock
  └─ 读多写少且写可饿 ──► rwlock（慎）
  │否（仅进程上下文）
  ├─ 短且绝不睡眠 ──► 仍可用 spinlock
  ├─ 互斥且可睡眠 ──► mutex（首选）
  ├─ 资源计数 >1 ──► semaphore
  └─ 等「做完了」事件 ──► completion

只要防本 CPU 调度迁移、数据 per-CPU ──► preempt_disable
跨核/设备看见顺序 ──► mb / rmb / wmb（或依赖锁自带语义）
```

#### 表格式速查

| 场景 | 首选 | 禁止 |
|------|------|------|
| 单变量计数/标志 | **atomic_t** | 无保护 `++` |
| 短临界区、中断可重入路径 | **spinlock + irqsave/bh** | 持锁睡眠 |
| 读多写少、写可延迟 | **rwlock** | 读锁里长时间工作 |
| 读极多、写很少、写不能饿 | **seqlock** | 读侧有副作用 |
| 仅当前 CPU 私有 | **`preempt_disable` / this_cpu** | 当跨 CPU 锁用 |
| 进程上下文、互斥、可睡 | **`mutex`** | 在 ISR 用 |
| 计数资源池 | **semaphore** | ISR 里 `down` |
| 等另一上下文完成 | **completion** | 在 ISR 里 wait |
| 跨核/MMIO 顺序 | **barrier 家族** | 假设「赋值即全局可见」 |
| 新代码大锁省事 | — | **BKL** |

#### 持有时间经验

| 量级 | 倾向 |
|------|------|
| 几十周期～几微秒 | spinlock / atomic |
| 可能阻塞、等 I/O、拷用户态 | mutex |
| 「几乎只读配置」 | seqlock / RCU（书后续主题） |

#### HFT / 驱动一句话

| 路径 | 原则 |
|------|------|
| **热路径** | 原子 > 短自旋 > 无锁结构；避免睡眠锁 |
| **慢路径 / ioctl / probe** | mutex、completion |
| **硬中断** | 只做最短工作 + 调度 softirq/tasklet/work；锁用 irqsave |
| **观测** | `perf lock`、锁持有时间直方图、`%soft` |

→ [Ch 9](../../chapter-09-kernel-sync-intro/) · [Ch 7–8](../../chapter-07-interrupts/) · 本章 README 小结表

### 常见陷阱

1. 在所有场景都用 spinlock——短临界区用 spinlock，长临界区用 mutex
2. 忽略 RCU——读极多写极少时 RCU 是最优解（读端零开销）
3. 忘记 lockdep——开发阶段开 lockdep 检测死锁/锁顺序问题

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** 给定场景，如何快速选择同步原语？

<details><summary>答案</summary>

中断上下文 → spin_lock_irqsave()。softirq → spin_lock_bh()。进程上下文 + 短临界区（<1us） → spin_lock()。进程上下文 + 长临界区 → mutex。读极多写极少 + 简单数据 → seqlock。读极多写极少 + 复杂数据 → RCU。一次性等待 → completion。引用计数 → refcount_t。不确定 → spin_lock_irqsave()（最安全）。

</details>

**Q2.** 同步原语的性能排序？

<details><summary>答案</summary>

最快 → 最慢：① atomic 操作（~20ns）。② RCU 读端（~0ns，只禁抢占）。③ seqlock 读端（~10ns）。④ spinlock 无争用（~20ns）。⑤ rwlock 读端无争用（~20ns）。⑥ mutex 无争用（~20ns）。⑦ spinlock 有争用（~100ns-spin）。⑧ mutex 有争用（~1-5us，schedule）。⑨ RCU 写端（~ms，等 grace period）。选择：热路径用 ①-⑤，冷路径可用 ⑥-⑨。

</details>

**Q3.** HFT 同步原语选型决策树？

<details><summary>答案</summary>

```
热路径？
├─ 是 → 数据可 per-thread？
│       ├─ 是 → 无锁（per-thread 变量）
│       └─ 否 → SPSC 队列？
│               ├─ 是 → atomic<head,tail> + release/acquire
│               └─ 否 → 分片锁 / 无锁哈希表
└─ 否 → 临界区 <1us？
        ├─ 是 → spinlock / atomic
        └─ 否 → mutex（+ rt_mutex 优先级继承）
```

</details>

</details>


> ↔ [ULK Ch5 §7 选型与实例](../../../18-linux-kernel-deep/chapter-05-kernel-synchronization/notes/section-7-选型与实例.md)
---
