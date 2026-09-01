## ③ 并发的原因 · Causes of Concurrency

内核既要处理 **真并发**，也要处理 **伪并发**：

| 类型 | 说明 |
|------|------|
| **真并发（true concurrency）** | 多 CPU **同时** 执行 |
| **伪并发（pseudo-concurrency）** | 单 CPU 上 **交替** 执行 — 仍可能竞态 |

#### Linux 内核五类并发源

| # | 来源 | 说明 | 章节 |
|---|------|------|------|
| 1 | **中断** | 随时异步插入 | **Ch 7** |
| 2 | **软中断 / tasklet** | 中断返回后异步 | **Ch 8** |
| 3 | **内核抢占** | 内核态可被换下 | **Ch 4** |
| 4 | **睡眠** | 显式或缺页等导致 **进程切换** | **Ch 3–5** |
| 5 | **SMP** | 多处理器真并行 | **Ch 1** |

```
        ┌─ IRQ 上半部 ─────────────┐
        ├─ softirq / tasklet ─────┤
同一数据 ◄├─ 另一 CPU 上的进程 ────┼──► 必须同步
        ├─ 本 CPU 抢占后的另一任务 ─┤
        └─ sleep 唤醒后的竞争者 ────┘
```

**内核开发者：** 写每一行访问共享数据时，问：**五种来源里谁会同时碰到这里？**

#### 真伪并发教学例

```
真并发（SMP）：
  CPU0: 中断处理程序改 dev->stats        CPU1: 进程上下文读 dev->stats
  ───────────── 同一纳秒 ─────────────►  两者物理同时进行

伪并发（单 CPU）：
  进程 A: stats.count++（已读入寄存器，还没写回）
      ▲ 被中断打断
      └──► IRQ handler: stats.count++（完整读-改-写）
  中断返回，A 写回寄存器旧值 + 1      ←── handler 的更新被覆盖丢失
```

**关键**：`count++` 在 CPU 眼里是 load/add/store 三条指令——**任何两条指令之间都可能切走**。伪并发丢更新与真并发丢更新的**结果一模一样**，所以单核机器同样需要同步（这是"单核不需要锁"这个流传甚广误解的来源——错在忘了中断）。

#### 历史视角：并发源是逐个冒出来的

| 时期 | 存在的并发源 | 当时的同步压力 |
|------|--------------|----------------|
| 早期单核 Linux（1.x） | 只有 **中断** | 主要防 IRQ：`cli/sti`（关中断即万事大吉） |
| 2.x 加入 **SMP** | + 真并发 | 自旋锁登场（关中断不够了——别的 CPU 照跑） |
| 2.6 加入**内核抢占** | + 抢占 | 单核上进程上下文之间也会交错 |
| 现代 + threaded IRQ / softirq 繁荣 | 五源齐备 | 分层锁、per-CPU、无锁结构 |

> LKD3rd 强调：**"two tasks can be concurrently executing in this manner"（伪并发）这句话定义了为什么单处理器系统也需要同步**——不是历史遗迹，是每一代新人必踩的坑。

#### 「谁防谁」速查矩阵（Ch 10 预习）

| 同步手段 | 防 SMP | 防硬中断 | 防 softirq | 防抢占 |
|----------|:---:|:---:|:---:|:---:|
| `preempt_disable()` | ✗ | ✗ | ✗ | ✓ |
| `local_irq_disable()` | ✗ | ✓（本 CPU） | ✓（本 CPU） | ✓（顺带，关中断期间不会调度） |
| `spin_lock()` | ✓ | ✗ | ✗ | ✓（spinlock 隐含禁抢占） |
| `spin_lock_irqsave()` | ✓ | ✓ | ✓ | ✓ |
| `local_bh_disable()` | ✗ | ✗ | ✓（本 CPU） | ✓ |

> 记忆锚点：**`spin_lock` 只防"人"，不防"异步"**——中断/软中断不属于任何进程，抢占计数对它们无效。所以中断处理程序会碰的数据必须 `spin_lock_irqsave`。

→ 01 Day 14 临界区 · [Ch 10 各锁详解](../../chapter-10-sync-methods/)

### 常见陷阱

1. 以为只有多 CPU 才有并发——中断、抢占、softirq 都是并发来源
2. 忽略中断引起的并发——进程在修改数据时被中断，中断处理函数也修改同一数据
3. 以为 preempt_disable() 能防止所有并发——只防抢占，不防中断

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** 内核中并发的来源有哪些？

<details><summary>答案</summary>

① SMP：多 CPU 同时执行。② 中断：hard IRQ 打断进程/softirq。③ softirq：softirq 打断进程上下文。④ 抢占：CONFIG_PREEMPT 时进程可被另一进程抢占。⑤ 信号：某些操作可被信号中断。⑥ 线程化：同一进程的多个内核线程。分析并发：问「这段代码是否可能被另一个执行路径同时进入？」如果是 → 需要同步。

</details>

**Q2.** preempt_disable() 能防止所有并发吗？

<details><summary>答案</summary>

不能。preempt_disable() 只防止本 CPU 上的抢占（内核态），不防：① 其他 CPU 上的并行执行（SMP）。② 中断（hard IRQ 仍可触发）。③ softirq（仍可执行）。全面防止：preempt_disable() + local_irq_disable() + spinlock（防 SMP）。或直接 spin_lock_irqsave()（一步到位）。

</details>

**Q3.** HFT 用户态的并发来源和内核有什么不同？

<details><summary>答案</summary>

用户态并发来源：① 多线程（pthread）。② 信号处理函数。③ atexit/fork handlers。④ 多进程共享内存。用户态不能禁中断/禁抢占（需要 root + 内核模块），所以用：① `std::atomic` 无锁同步。② `std::mutex`（futex）。③ `pthread_cancel` 屏蔽。④ 共享内存 + 原子操作。HFT 热路径只用无锁设计。

</details>

**Q4.** 数据只被本 CPU 的进程上下文和硬中断共享（无 SMP 情况、softirq 不碰它），最小开销的正确保护是什么？

<details><summary>答案</summary>

`spin_lock_irqsave()`（或更精细：仅在进程上下文与 IRQ 都会碰的那一小段用 `local_irq_disable()` + 数据自身无需锁——单 CPU 上关中断后中断与抢占都进不来）。错误答案 `spin_lock()`：spinlock 防不了硬中断——持锁进程被中断打断，IRQ handler 再去 `spin_lock()` 同一把锁 = **在持锁者暂停期间自旋等自己**，单核死锁。这正是"spin_lock 只防人不防异步"的典型翻车现场。

</details>

</details>

---
