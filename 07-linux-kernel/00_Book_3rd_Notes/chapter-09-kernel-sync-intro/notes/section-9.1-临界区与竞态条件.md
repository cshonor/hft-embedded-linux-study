## ① 临界区与竞态条件

**临界区（critical region / critical section）** = 访问或修改 **共享数据** 的代码段。内核并发源众多（Ch 9.3），任何共享变量都可能成为 **竞态** 靶点。

| 概念 | 含义 |
|------|------|
| **临界区** | 读写 **共享资源** 的代码范围 — 必须 **互斥** |
| **互斥（mutual exclusion）** | 同一时刻 **至多一个** 执行流进入 |
| **同步（synchronization）** | 协调并发访问、保证数据 **一致性** 的机制与过程 |
| **原子性（atomicity）** | 临界区 **如同一条不可分割指令** — 执行期间不被并发打断 |

**竞态条件（race condition）** = 两个或更多执行线程 **可能同时** 进入 **同一临界区** 操作共享数据，结果依赖 **碰巧的交错顺序** — 不可预测、可能损坏数据。

```
初始 count = 5

线程 A:  读 count=5 ──► 计算 6 ──► 写 count=6
线程 B:       读 count=5 ──► 计算 6 ──► 写 count=6
                                    ↑
                              正确应为 7 — 「丢失更新」
```

#### 竞态的表现形式

| 类型 | 例子 |
|------|------|
| **丢失更新** | 两个 `++` 只生效一次 |
| ** torn read** | 64 位值在 32 位 CPU 上读一半被换 |
| **NULL 解引用** | A 判非空 → B 删除 → A 仍用指针 |
| **双重释放** | 两路径同时 `kfree` 同一对象 |

#### 内核为何特别容易竞态

| 原因 | 说明 |
|------|------|
| **中断随时插入** | ISR 与进程 **交错** — Ch 7 |
| **SMP** | 真并行 — 两 CPU 同时进临界区 |
| **抢占** | 内核态也可被 **更高优先级** 任务换下 — Ch 4 |
| **下半部异步** | softirq/tasklet 与进程 **并发** — Ch 8 |

#### 正确性 vs 性能

| 阶段 | 优先级 |
|------|--------|
| **第一版** | **正确** — 宁可粗锁 |
| **优化** | profiling 见争用后再 **细化**（Ch 9.6） |

| 错误思路 | 问题 |
|----------|------|
| 「单 CPU 不会竞态」 | 仍有 **中断/下半部** |
| 「只读不用锁」 | 读可能与写 **交错** — 需 RCU/seqlock（Ch 10） |
| 「汇编里 inc 一条指令」 | 复合操作（链表插入）仍非原子 |

#### 与用户态 HFT 的对照

| 内核 | 用户态 HFT |
|------|------------|
| 临界区 + spinlock/mutex | **SPSC/MPSC 无锁队列** |
| `local_irq_save` 防 ISR | **isolcpus** 上无 IRQ 干扰 |
| per-CPU 变量 | **thread-local / 每核 order book 分片** |
| RCU 读侧无锁 | **atomic load + memory_order_acquire** |

**HFT 对照：** 用户态 **无锁队列、原子计数** 与内核 **「临界区必须互斥」** 是同一问题的两层。Tail 延迟常来自 **锁争用** 或 **false sharing（伪共享）** — 两核改同一 cache line 的不同变量。

→ [Ch 9.3](section-9.3-并发的原因.md) 五类并发源 · [Ch 9.2](section-9.2-加锁.md) 加锁 · [Ch 7](../../chapter-07-interrupts/) 中断并发 · [03 SysPerf §5.2 mutex/spin](../../../../19-systems-performance/chapter-05-applications/notes/section-5.2-应用程序性能提升技术.md)

### 常见陷阱

1. 以为单核不需要同步——UP 上仍需禁抢占/中断防止竞态
2. 混淆「竞态条件」和「数据竞争」——竞态是逻辑层（结果依赖时序），数据竞争是内存层（并发读写同一地址）
3. 以为原子操作能解决所有竞态——原子操作只保证单操作原子性，多操作组合仍需锁

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** 什么是临界区？为什么需要保护？

<details><summary>答案</summary>

临界区是访问共享资源的代码段。如果不保护：多 CPU/中断/抢占并发执行 → 数据竞争 → 数据损坏/崩溃。例：`counter++` 实际是 load → add → store 三步，两 CPU 同时执行可能丢失一次更新。保护方式：spinlock/mutex/atomic/RCU，确保同一时间只有一个执行者进入临界区。

</details>

**Q2.** UP（单处理器）上为什么还需要同步？

<details><summary>答案</summary>

UP 上没有多 CPU 并行，但有：① 抢占：进程在临界区中被抢占，另一进程进入临界区。② 中断：进程在临界区中被中断，中断处理函数访问同一数据。解决：进程上下文 → preempt_disable() 或 spinlock（UP 上退化为 preempt_disable）。中断也访问 → spin_lock_irqsave()。

</details>

**Q3.** HFT 用户态的竞态条件和内核有什么不同？

<details><summary>答案</summary>

用户态竞态来源：多线程 + 信号 + atexit handlers。用户态不能用内核锁，用：① `std::atomic`（无锁，适合简单操作）。② `std::mutex`/`pthread_mutex`（基于 futex，适合复杂临界区）。③ 无锁数据结构（SPSC 队列等）。HFT 优先无锁设计避免 futex 的 syscall 开销。ThreadSanitizer 可检测数据竞争。

</details>

</details>


> ↔ [ULK Ch5 §1 本章定位](../../../../08-linux-kernel-deep/chapter-05-kernel-synchronization/notes/section-1-本章定位.md)
---
