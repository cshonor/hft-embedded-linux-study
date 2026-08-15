# qspinlock 设计：MCS 队列与三级优化

> 原文: LWN Queued Spinlocks (2014)
> 内核版本: 4.x+ (x86), 5.x+ (ARM64)
> 对标旧书: ULK3 Ch5 (ticket spinlock 已过时)

---

## qspinlock 核心思想

qspinlock (Queued Spinlock) 通过 MCS 锁队列设计，让每个等待者在**自己的 per-CPU 变量**上自旋，而非在共享锁变量上自旋，将缓存弹跳从 O(N) 降到 O(1)。

### 原子变量编码

```c
// qspinlock 用一个 32-bit 原子变量编码三层状态
typedef struct qspinlock {
    union {
        atomic_t val;       // 整体原子操作
        struct {
            u8 locked;      // bits [0-7]:  0=空闲, 1=已锁
            u8 pending;     // bits [8-15]: pending 等待者
            u16 tail;       // bits [16-31]: MCS 队列尾索引
        };
    };
} arch_spinlock_t;
```

```
原子变量 (32-bit):
  bits [0-7]:   locked 位 (0=空闲, 1=已锁)
  bits [8-15]:  pending 位 (1 个 pending 等待者)
  bits [16-31]: queue tail (MCS 节点索引: CPU号 + 上下文)
```

---

## 三级优化路径

| 路径 | 条件 | 策略 | 缓存弹跳 |
|------|------|------|---------|
| Fast path | 锁空闲 | 原子 CAS: 0→1 | 0 次 |
| Pending | 1 个等待者 | 在 locked 位上自旋 | 1 次 |
| Queue | 2+ 等待者 | MCS 队列，各自在 per-CPU 变量自旋 | 1 次 |

### Fast Path (无争用)

```c
static __always_inline void queued_spin_lock(arch_spinlock_t *lock) {
    // 期望 locked=0, 设为 1
    if (likely(atomic_try_cmpxchg_acquire(&lock->val, &0, 1)))
        return;  // 获取成功，无缓存弹跳
    // 进入慢速路径
    queued_spin_lock_slowpath(lock, 1);
}
```

### Pending Path (1 个等待者)

```
CPU 0 持锁 (locked=1)
CPU 1 想获取锁:
  1. CAS 尝试设 pending=1 → 成功
  2. CPU 1 在 locked 位上自旋 (等 CPU 0 释放)

此时: locked=1, pending=1, tail=0
只有 1 个缓存行弹跳点 (locked)
```

### Queue Path (2+ 等待者)

```
CPU 0 持锁 → CPU 1 pending → CPU 2 入队 → CPU 3 入队

MCS 队列:
  CPU 0 (持锁) → CPU 1 (pending, 自旋在 locked) 
                → CPU 2 (自旋在 mcs_node[2].locked) 
                → CPU 3 (自旋在 mcs_node[3].locked, 等 CPU 2 通知)

释放流程:
  CPU 0 释放 → locked=0 → CPU 1 看到 locked=0, 获取锁
  CPU 1 释放 → 写 mcs_node[2].locked=0 → CPU 2 被通知
  CPU 2 释放 → 写 mcs_node[3].locked=0 → CPU 3 被通知

每次释放只通知 1 个等待者 → O(1) 缓存弹跳!
```

---

## MCS 锁节点

```c
// 每个 CPU 有 4 个 MCS 节点 (对应 4 种上下文)
// kernel/locking/qspinlock.c
struct mcs_spinlock {
    struct mcs_spinlock *next;   // 队列中下一个节点
    volatile u8 locked;          // 0=需要等待, 1=轮到我了
    // ...
};

// per-CPU 变量
static DEFINE_PER_CPU_ALIGNED(struct mcs_spinlock, mcs_nodes[4]);

// 入队: 将自己的 mcs_node 链接到队列尾部
// 修改 tail 索引指向自己 → 1 次缓存弹跳
// 然后在前一个节点的 next 上自旋 → 等前驱通知
```

### 为什么用 per-CPU 变量？

1. 每个 CPU 在自己的 mcs_node 上自旋 → 不干扰其他 CPU 的缓存
2. 不需要动态分配内存 → 无分配失败风险
3. 4 个节点对应 4 种上下文 (task, softirq, hardirq, nmi) → 不会嵌套死锁

---

## 关键代码对比

```c
// ===== ULK3 时代 — ticket spinlock =====
static inline void arch_spin_lock(arch_spinlock_t *lock) {
    int ticket = fetch_and_add(&lock->tickets.next, 1);
    while (lock->tickets.owner != ticket)
        cpu_relax();
    // 释放: lock->tickets.owner++
    // 问题: 释放时所有 N 个等待者的缓存行都失效
}

// ===== 6.x — queued spinlock =====
// Fast path
static __always_inline void queued_spin_lock(arch_spinlock_t *lock) {
    if (likely(atomic_try_cmpxchg_acquire(&lock->val, &0, _Q_LOCKED_VAL)))
        return;
    queued_spin_lock_slowpath(lock, val);  // pending/queue 路径
}

// Slow path (简化)
void queued_spin_lock_slowpath(arch_spinlock_t *lock, u32 val) {
    // 1. 尝试设 pending 位
    if (val == _Q_LOCKED_VAL) {
        if (try_set_pending(lock))
            return;  // pending 路径，在 locked 上自旋
    }

    // 2. 进入 MCS 队列
    node = this_cpu_ptr(&mcs_nodes[idx]);
    prev = xchg_tail(lock, node);  // 将自己链接到队列尾

    // 3. 在前驱节点的 locked 上自旋
    if (prev) {
        WRITE_ONCE(prev->next, node);
        while (!smp_load_acquire(&node->locked))
            cpu_relax();  // 自旋在自己的 per-CPU 变量上!
    }

    // 4. 轮到自己，获取锁
    // 等待 pending/locked 清零
    smp_cond_load_acquire(&lock->val, !(VAL & _Q_LOCKED_PENDING_MASK));
}
```

---

## 性能对比

| 指标 | Ticket Spinlock | qspinlock |
|------|----------------|-----------|
| 无争用延迟 | ~15ns | ~15ns (相同) |
| 高争用缓存弹跳 | O(N) | O(1) |
| 4 CPU 争用延迟 | ~500ns | ~80ns |
| 16 CPU 争用延迟 | ~3000ns | ~120ns |
| 公平性 | FIFO | FIFO (pending 有微小不公平) |
| 内存占用 | 4 字节 | 4 字节 |
| 代码复杂度 | 简单 | 复杂 (slowpath) |

---

## HFT 关联

| 场景 | qspinlock 影响 |
|------|---------------|
| 低争用锁 (大多数) | 与 ticket 锁性能相当 |
| 高争用锁 (网络包队列) | 显著减少缓存弹跳，延迟更稳定 |
| SCHED_FIFO + isolcpus | 隔离核上无争用，锁类型不重要 |
| 辅助线程共享数据 | qspinlock 减少辅助线程锁等待尾延迟 |

> **HFT 实盘：** 交易线程在隔离核上运行，不与辅助线程争用锁。qspinlock 的优势体现在辅助线程之间的锁争用（如日志队列、统计计数器），间接减少对交易线程的缓存干扰。如果交易线程必须用锁（如与内核共享数据），考虑 per-CPU 队列或无锁设计（如 DPDK ring buffer）。

---

## 自测题

<details>
<summary>Q1: qspinlock 的 pending 位是什么？为什么不直接进队列？</summary>

pending 位是 fast-path（无争用 CAS）到 queue（MCS 队列）之间的过渡层。第二个等待者先在 pending 位上自旋，第三个才进 MCS 队列。这样在 2 个争用者时不需分配 MCS 节点，减少开销。大多数锁争用只有 1-2 个等待者，pending 优化覆盖了常见情况。
</details>

<details>
<summary>Q2: MCS 队列如何将缓存弹跳从 O(N) 降到 O(1)？</summary>

MCS 队列中每个等待者在**自己的 per-CPU mcs_node.locked 变量**上自旋，而非在共享的 lock 变量上自旋。释放锁时只写前驱节点的 next 指向的 mcs_node.locked=1，通知 1 个等待者。其他等待者不受影响（它们自旋在自己的变量上）。所以每次释放只有 1 次缓存弹跳，与等待者数量无关。
</details>

<details>
<summary>Q3: 为什么 MCS 节点是 per-CPU 而不是动态分配？</summary>

三个原因: ① 自旋在 per-CPU 变量上不会污染其他 CPU 的缓存；② 动态分配在锁争用时可能失败（内存不足），导致死锁；③ 每个 CPU 有 4 个节点对应 4 种上下文（task/softirq/hardirq/nmi），保证同一种上下文不会嵌套获取同一把锁。per-CPU 分配在编译时就确定了，零运行时开销。
</details>

<details>
<summary>Q4: qspinlock 在低争用场景下和 ticket spinlock 性能一样吗？为什么？</summary>

是的，几乎完全一样。低争用时走 fast path——一次原子 CAS 获取锁，不进 pending/queue 路径。fast path 的代码与 ticket spinlock 的获取逻辑本质相同（原子操作 + 判断），开销都是 ~15ns。qspinlock 的优势只在高争用（3+ 等待者）时才体现。
</details>

---

## 交叉引用

- [01-ticket-spinlock-problem.md](./01-ticket-spinlock-problem.md) — Ticket spinlock 与缓存弹跳问题
- [chapter-10-preempt-rt](../../chapter-10-preempt-rt/) — PREEMPT_RT 将 spinlock 转为 rt_mutex
- [chapter-05-interrupt-management](../../chapter-05-interrupt-management/) — 中断处理中的锁使用
