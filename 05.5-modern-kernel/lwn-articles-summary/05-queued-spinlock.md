# Queued Spinlock (qspinlock) — 替代传统自旋锁

> **原文:** [Queued spinlocks](https://lwn.net/Articles/590243/) (LWN, 2014)
> **作者:** Waiman Long, Peter Zijlstra
> **内核版本:** 4.x+ (x86), 5.x+ (ARM64)
> **对标旧书:** ULK3 Ch5 (ticket spinlock) / LKD3 Ch9

---

## 核心观点

传统 ticket spinlock 在高争用场景下存在缓存行弹跳 (cache line bouncing) 问题。Queued Spinlock (qspinlock) 通过 MCS 锁队列设计大幅减少缓存行争用。

### Ticket Spinlock 的问题

```c
// 传统 ticket spinlock (ULK3 时代)
typedef struct {
    volatile unsigned int lock;  // 低16位=owner, 高16位=next_ticket
} arch_spinlock_t;

// 获取锁
inc = fetch_and_add(&lock->next_ticket, 1);  // 原子递增
while (lock->owner != my_ticket)             // 等待轮到自己
    cpu_relax();

// 释放锁
lock->owner++;  // 下一个 ticket
```

**问题：** 释放锁时 `owner++` 会导致**所有等待者**的缓存行失效，N 个等待者产生 N 次缓存行弹跳。

### Queued Spinlock 的设计

qspinlock 使用一个原子变量编码锁状态 + MCS 队列：

```
原子变量 (32-bit):
  bits [0-8]:   locked 位 (0=空闲, 1=已锁)
  bits [9-16]:  pending 位 (1 个 pending 等待者)
  bits [17-31]: queue tail (MCS 节点索引)
```

**MCS 锁队列：** 每个等待者在**自己的 per-CPU 变量**上自旋，而不是在共享锁变量上自旋：

```
CPU 0 (持锁) → CPU 1 (pending) → CPU 2 (queue) → CPU 3 (queue)
                                     ↓                ↓
                               自旋在自己的          自旋在 CPU2 的
                               locked 变量上         locked 变量上
```

### 三个优化层级

| 状态 | 优化策略 | 缓存弹跳次数 |
|------|---------|-------------|
| 无争用 (fast path) | 原子 CAS 获取，O(1) | 0 |
| 1 个等待者 (pending) | 第二个等待者在 locked 位上自旋 | 1 |
| 2+ 等待者 (queue) | MCS 队列，每个等待者在自己变量上自旋 | 1 (仅通知下一个) |

### 关键优势

- **低争用场景**：与 ticket spinlock 性能相当
- **高争用场景**：缓存行弹跳从 O(N) 降到 O(1)
- **内存效率**：锁变量仅 4 字节（32位），ticket 也是 4 字节
- **公平性**：MCS 队列保证 FIFO 公平

---

## 与旧书差异

| ULK3 / LKD3 讲的 | 6.x 现代实现 |
|-------------------|-------------|
| ticket spinlock | qspinlock (MCS 队列) |
| 所有等待者在同一变量自旋 | 每个等待者在自己 per-CPU 变量自旋 |
| `spin_lock()` 简单原子操作 | `queued_spin_lock_slowpath()` 队列操作 |
| 无 pending 优化 | pending 位作为 fast-path 到 queue 的过渡 |

### 关键代码变更

```c
// ULK3 时代 — ticket spinlock
static inline void arch_spin_lock(arch_spinlock_t *lock) {
    int ticket = fetch_and_add(&lock->tickets.next, 1);
    while (lock->tickets.owner != ticket)
        cpu_relax();
}

// 6.x — queued spinlock
typedef struct qspinlock {
    union {
        atomic_t val;  // 编码 locked | pending | tail
        struct {
            u8 locked;
            u8 pending;
            u16 tail;
        };
    };
} arch_spinlock_t;

// fast path: 无争用时直接 CAS
static __always_inline void queued_spin_lock(arch_spinlock_t *lock) {
    if (likely(atomic_try_cmpxchg_acquire(&lock->val, &0, 1)))
        return;  // 获取成功
    queued_spin_lock_slowpath(lock, 1);  // 队列路径
}
```

---

## HFT 关联

| 场景 | qspinlock 影响 |
|------|---------------|
| **低争用锁** (大多数) | 与 ticket 锁性能相当，无差别 |
| **高争用锁** (如网络包队列) | 显著减少缓存行弹跳，延迟更稳定 |
| **SCHED_FIFO + isolcpus** | 隔离核上无争用，锁类型不重要 |
| **辅助线程共享数据** | qspinlock 减少辅助线程的锁等待尾延迟 |

> **HFT 实盘：** 交易线程在隔离核上运行，不与辅助线程争用锁。qspinlock 的优势体现在辅助线程之间的锁争用（如日志队列、统计计数器），间接减少对交易线程的缓存干扰。

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** 为什么 qspinlock 在高争用下比 ticket spinlock 好？

> Ticket spinlock 释放锁时，所有 N 个等待者都会读到 `owner++` 的变化，产生 N 次缓存行弹跳。qspinlock 的 MCS 队列让每个等待者在**自己的 per-CPU 变量**上自旋，释放锁只通知队首的下一个等待者，缓存弹跳从 O(N) 降到 O(1)。

**Q2:** qspinlock 的 "pending" 位是什么？为什么不直接进队列？

> pending 位是 fast-path（无争用 CAS）到 queue（MCS 队列）之间的过渡层。第二个等待者先在 pending 位上自旋，第三个才进队列。这样在 2 个争用者时不需分配 MCS 节点，减少开销。大多数锁争用只有 1-2 个等待者，pending 优化覆盖了常见情况。

**Q3:** qspinlock 保证 FIFO 公平吗？ticket spinlock 呢？

> 两者都保证 FIFO 公平。ticket spinlock 按 ticket 顺序服务。qspinlock 的 MCS 队列也是 FIFO——每个节点按入队顺序被唤醒。但 pending 位使得第二个等待者可能"插队"到队列之前，严格来说有微小的不公平。

</details>
