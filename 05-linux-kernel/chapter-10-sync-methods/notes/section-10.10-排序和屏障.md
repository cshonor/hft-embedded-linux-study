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

→ [02-CSAPP 并发与内存](../../../../02-computer-systems/chapter-12-concurrent-programming/) · [10.1 原子](./section-10.1-原子操作.md) · [10.8 seqlock](./section-10.8-顺序锁.md)

### 常见陷阱

1. 混淆 smp_mb() / smp_rmb() / smp_wmb()——全屏障/读屏障/写屏障，保证不同方向的重排
2. 以为 x86 不需要 memory barrier——x86 有 TSO 内存模型，大部分 barrier 是空操作，但 store-load 重排仍需要 mfence
3. 在 UP 上用 smp_mb()——UP 上 smp_mb() 是空操作（无 SMP 重排），应改用 barrier() 或不需要

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** smp_mb() / smp_rmb() / smp_wmb() 分别保证什么？

<details><summary>答案</summary>

smp_mb()：全屏障，之前的读写 + 之后的读写都不可跨屏障重排。smp_rmb()：读屏障，之前的读不可重排到之后的读之后。smp_wmb()：写屏障，之前的写不可重排到之后的写之后。x86 TSO 模型下：smp_rmb() = 空操作（loads 不重排），smp_wmb() = 空操作（stores 不重排），smp_mb() = `mfence`（禁止 store-load 重排）。ARM64：三者都是真实指令（`dmb ish`/`dmb ishld`/`dmb ishst`）。

</details>

**Q2.** smp_store_release() / smp_load_acquire() 相比 smp_mb() 有什么优势？

<details><summary>答案</summary>

smp_store_release(ptr, val)：等价于 smp_wmb() + WRITE_ONCE(*ptr, val)，只保证之前的读写不重排到这个 store 之后。smp_load_acquire(ptr)：等价于 READ_ONCE(*ptr) + smp_rmb()，只保证之后的读写不重排到这个 load 之前。优势：① 更精细——只关联一个操作，不影响其他操作。② 在 x86 上 release = 普通 store（无开销），acquire = 普通 load（无开销）。③ 代码更清晰。

</details>

**Q3.** HFT 中 memory barrier 误用会导致什么问题？

<details><summary>答案</summary>

① 缺少 barrier：消息传递 pattern 失败——`data = x; ready = true;` 如果 CPU 重排为 `ready = true; data = x;`，消费者看到 ready=true 但 data 还是旧值。② 过多 barrier：性能下降——每个 smp_mb() 在 x86 上是 `mfence`（~30 cycles），ARM64 上 `dmb`（~50 cycles）。HFT 无锁队列应精确用 release/acquire 替代 seq_cst。用 `std::atomic` + 正确的 memory_order 避免手动 barrier。

</details>

</details>

---
