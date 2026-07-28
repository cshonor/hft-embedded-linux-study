## ⑩ 排序和屏障 · Ordering and Barriers

锁解决「互斥」；屏障解决「**可见顺序**」。编译器与 CPU 为性能会 **重排** load/store — 对 **设备寄存器**、**无锁算法**、**发布-订阅** 会出鬼。

#### 三类乱序来源

| 来源 | 例子 |
|------|------|
| **编译器** | 把两次写调换顺序 |
| **CPU 存储缓冲 / 乱序执行** | 其它核暂时看见「新 B、旧 A」 |
| **设备 / DMA** | MMIO 写合并、顺序敏感 |

#### 常用屏障

| 屏障 | 作用 |
|------|------|
| **`rmb()`** | **读屏障** — 屏障前的读不与屏障后的读乱序 |
| **`wmb()`** | **写屏障** — 写顺序 |
| **`mb()`** | **全屏障** — 读写皆不越过 |
| **`barrier()`** | **仅编译器屏障** — 不约束 CPU |
| **`smp_rmb/wmb/mb()`** | SMP 变体 — UP 上可退化为空，跨核可见性用这组 |

```
发布者 CPU0:  写数据 payload  ── wmb() ── 写 flag=1
订阅者 CPU1:  读 flag==1     ── rmb() ── 读 payload
```

缺 `wmb`/`rmb` → 订阅者可能看到 flag=1 但 payload 仍是旧值。

#### 与原子/锁的关系

| 机制 | 序 |
|------|-----|
| 普通 `spin_lock` / `mutex` | **通常自带** 足够的 acquire/release 语义 |
| 裸 `atomic_set` + 无锁结构 | **你必须自己想清楚** 屏障 |
| `READ_ONCE` / `WRITE_ONCE` | 防编译器「拆载/合并」；不替代全序屏障 |

#### MMIO

访问设备寄存器常用 **`readl`/`writel`** 等 — 内部已含架构所需的顺序约束；对裸指针乱写 MMIO 极易踩坑。

**HFT：** 用户态 `memory_order_acquire/release`、环形缓冲区的 head/tail 发布，与内核屏障 **同一类问题**。无锁队列 bug = 偶现脏数据、极难复现。

→ [01-CSAPP 并发与内存](../../../../01-CSAPP-3rd/chapter-12-concurrent-programming/) · [10.1 原子](./section-10.1-原子操作.md) · [10.8 seqlock](./section-10.8-顺序锁.md)

---
