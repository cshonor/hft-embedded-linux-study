# Ticket Spinlock 的问题与缓存行弹跳

> 原文: LWN Queued Spinlocks (2014)
> 对标旧书: ULK3 Ch5 (ticket spinlock) / LKD3 Ch9

---

## Ticket Spinlock 原理

Ticket spinlock 是 ULK3/LKD3 时代 Linux 内核的自旋锁实现，灵感来自面包店排队（取号等待）。

```c
// 传统 ticket spinlock (ULK3 时代)
typedef struct {
    union {
        struct {
            volatile unsigned int owner;    // 当前服务的号
            volatile unsigned int next;     // 下一个可取的号
        };
        volatile unsigned int lock;          // 32位打包
    };
} arch_spinlock_t;

// 获取锁 — 取号 + 等待叫号
static inline void arch_spin_lock(arch_spinlock_t *lock) {
    // 原子递增 next，获取自己的号
    int my_ticket = fetch_and_add(&lock->next, 1);

    // 等待 owner 追上自己的号
    while (lock->owner != my_ticket)
        cpu_relax();   // x86: pause; ARM64: yield
}

// 释放锁 — 叫下一个号
static inline void arch_spin_unlock(arch_spinlock_t *lock) {
    lock->owner++;   // 通知下一个等待者
}
```

### 公平性

Ticket spinlock 保证 FIFO 公平——先取号先服务。没有"饥饿"问题（早期 BSL 自旋锁有饥饿问题）。

---

## 核心问题：缓存行弹跳 (Cache Line Bouncing)

### 问题描述

```
假设 4 个 CPU 争用同一把锁:

CPU 0 (持锁)  CPU 1 (等)    CPU 2 (等)    CPU 3 (等)
     |             |             |             |
     |  owner=0    |  owner=0    |  owner=0    |  owner=0
     |  next=4     |  next=4     |  next=4     |  next=4
     |             |             |             |
  释放锁: owner++ → owner=1
     |             |             |             |
  所有 CPU 都读到 owner 变化! (缓存行失效)
     |          cache miss    cache miss    cache miss
     |          重新读取       重新读取       重新读取
     |
  CPU 1 获得锁 (owner==1==my_ticket)
```

**关键问题：** `owner++` 修改了共享变量，导致所有 N 个等待者的缓存行都失效。每个等待者都要重新从内存读取 owner 值，产生 N 次缓存行弹跳。

### 性能影响

| CPU 数量 | 每次释放的缓存失效次数 | 延迟影响 |
|---------|---------------------|---------|
| 2 | 1 | 小 |
| 4 | 3 | 中 |
| 8 | 7 | 大 |
| 16+ | 15+ | 严重 |

在高争用场景下（如网络包队列锁），缓存行弹跳成为主要瓶颈。

---

## 缓存一致性协议 (MESI) 视角

```
MESI 状态: Modified, Exclusive, Shared, Invalid

1. CPU 0 持有锁 (owner 变量在 CPU 0 的 L1 中为 Modified)
2. CPU 1,2,3 都在轮询 owner (各自 L1 中为 Shared)
3. CPU 0 执行 owner++:
   - CPU 0: Modified → 发送 Invalidate 消息
   - CPU 1,2,3: Shared → Invalid (缓存行失效!)
4. CPU 1,2,3 发现缓存失效，发送 Read 请求
5. CPU 0 响应 Read (Modified → Shared)，数据通过总线传输
6. 总共: 3 次 Invalidate + 3 次 Read = 6 次总线事务
```

### NUMA 影响

```
在 NUMA 系统上（多 socket）:
- 同 socket 内缓存弹跳: ~50ns
- 跨 socket 缓存弹跳: ~200-400ns

8 核 2 socket 系统中，一把高争用锁的释放:
- 4 次同 socket 弹跳: 4 × 50ns = 200ns
- 3 次跨 socket 弹跳: 3 × 300ns = 900ns
- 总计: ~1100ns (仅缓存开销，不含锁等待时间)
```

---

## HFT 关联

| 场景 | Ticket Spinlock 的影响 |
|------|----------------------|
| 网卡收包队列锁 | 高频争用，缓存弹跳严重 |
| 内核统计计数器 | 争用较低，影响小 |
| 交易线程在隔离核 | 无争用，不受影响 |
| 辅助线程共享数据 | 中等争用，尾延迟受影响 |

> **HFT 视角：** 交易线程在 isolcpus 隔离核上运行，不与其他线程争用锁。但辅助线程（日志、统计、行情解析）之间的锁争用会产生缓存噪音，间接干扰交易线程的 L1/L2 缓存。

---

## 自测题

<details>
<summary>Q1: Ticket spinlock 的 FIFO 公平性是如何实现的？</summary>

通过 owner/next 两个计数器。获取锁时原子递增 next 获取自己的号（ticket），然后等待 owner 等于自己的号。释放锁时递增 owner。因为号是严格递增的，等待者按号的大小顺序获得锁，保证 FIFO。这解决了早期 BSL (Backoff Spinlock) 的饥饿问题。
</details>

<details>
<summary>Q2: 为什么说"释放锁比获取锁更昂贵"在 ticket spinlock 中成立？</summary>

获取锁时只有取号者修改 next（1 次缓存行写），其他等待者不修改共享变量。释放锁时 owner++ 会导致所有等待者的缓存行失效，产生 N 次缓存弹跳。获取锁的开销是 O(1)，释放锁的开销是 O(N)（N = 等待者数量）。
</details>

<details>
<summary>Q3: cpu_relax() 在不同架构上的指令是什么？为什么需要它？</summary>

x86: `pause` 指令（降低流水线功耗，避免流水线冲突）。ARM64: `yield` 指令（提示虚拟化环境下让出 CPU）。需要 cpu_relax() 是因为紧凑的 while 循环会让 CPU 流水线满负荷运行，浪费功耗且干扰其他核的缓存访问。pause/yield 给内存控制器喘息空间。
</details>

<details>
<summary>Q4: 在 NUMA 系统上，ticket spinlock 的缓存弹跳开销为什么更严重？</summary>

跨 socket 的缓存一致性消息延迟远高于同 socket。同 socket 内 L1/L2 缓存弹跳约 50ns，跨 socket 约 200-400ns。在高争用场景下，每次释放锁都触发所有等待者的缓存失效，跨 socket 等待者贡献了大部分延迟。这也是 qspinlock 设计的动机之一。
</details>

---

## 交叉引用

- [02-qspinlock-design.md](./02-qspinlock-design.md) — qspinlock 如何解决缓存弹跳
- [chapter-10-preempt-rt](../chapter-10-preempt-rt/) — PREEMPT_RT 中 spinlock 变为可睡眠
- [05.6-kernel-debugging/chapter-08-lock-debug](../../05.6-kernel-debugging/chapter-08-lock-debug/) — 锁调试工具
