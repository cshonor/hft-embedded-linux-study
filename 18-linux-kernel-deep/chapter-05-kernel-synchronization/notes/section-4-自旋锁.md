## 4. 自旋锁 (Spin Locks)

> **忙等待 (busy wait)** — 锁被占用时在 tight loop 里轮询，**不睡眠**

---

### 一、适用场景

| 适合 | 不适合 |
|------|--------|
| 临界区 **极短** | 临界区 **长**、可能阻塞 |
| **中断上下文**、持锁者不能 sleep | 需要睡眠等待资源 |

获取失败 → **一直转**，占着 CPU（低优先级下可能饿死 — 需控制持锁时间）。

---

### 二、读/写自旋锁 (RW Spin Locks)

| 模式 | 并发度 |
|------|--------|
| **读锁** | 多个读者 **同时** 持有 |
| **写锁** | **独占** — 读写互斥 |

读多写少的数据结构（如某些路由表）适用。

---

### 三、与信号量对比（预览）

| | 自旋锁 | 信号量 |
|---|--------|--------|
| 等待方式 | 忙等 | **睡眠** |
| 上下文 | 可中断/进程上下文（短） | 可长时间等 |
| HFT | 热路径常见，**持锁时间要纳秒/微秒级** | 较慢路径、可阻塞 |

→ 信号量详 [section-6](./section-6-信号量与完成变量.md)

### 常见陷阱

1. 在持有 spinlock 时调用睡眠函数——会死锁或 panic（`BUG: scheduling while atomic`）
2. 以为 spinlock 会自动关中断——普通 `spin_lock()` 不关中断，中断上下文需用 `spin_lock_irqsave()`
3. 混淆 `spin_lock()` 和 `spin_lock_bh()`——前者只禁抢占，后者还禁 softirq

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** `spin_lock()` / `spin_lock_irq()` / `spin_lock_irqsave()` / `spin_lock_bh()` 的区别？

<details><summary>答案</summary>

`spin_lock()`：禁抢占。`spin_lock_irq()`：禁抢占 + 关本地中断（如果知道调用前中断是开的）。`spin_lock_irqsave(flags)`：禁抢占 + 保存中断状态 + 关中断（最安全，推荐）。`spin_lock_bh()`：禁抢占 + 禁 softirq（不关 hard IRQ）。选择原则：进程上下文 → `spin_lock()` 或 `spin_lock_irqsave()`（如中断也访问）。中断上下文 → `spin_lock_irqsave()`。softirq 上下文 → `spin_lock_bh()`。

</details>

**Q2.** 为什么持有 spinlock 时不能睡眠？

<details><summary>答案</summary>

Spinlock 假设等待者会忙等（spin），不释放 CPU。如果持锁者睡眠（`schedule()`），等待者会无限 spin 浪费 CPU。更严重的是：① `schedule()` 在 `preempt_count > 0` 时 panic。② 如果睡眠后切换到的进程也请求同一锁 → 死锁。RT 内核把 spinlock 变成可睡眠锁后，这个限制不成立，但吞吐下降。

</details>

**Q3.** HFT 用户态怎么模拟 spinlock 的效果？

<details><summary>答案</summary>

用户态没有真正的 spinlock（不关中断/不禁抢占），但可以用：① `std::atomic_flag` + `test_and_set` 自旋等待（短临界区，<100ns）。② `sched_yield()` + 重试（中等临界区）。③ `spinlock` 如果持有时间 >1us 应改用 `futex`/`mutex`（避免浪费 CPU）。关键是测量持有时间：`perf stat -e instructions` 在持锁前后计数。

</details>

</details>

---

← [3. 基础原语](./section-3-基础同步原语.md) · 下一节 [5. 顺序锁与 RCU](./section-5-顺序锁与RCU.md)
> ↔ [LKD Ch10 §10.2 自旋锁](../../../05-linux-kernel/chapter-10-sync-methods/notes/section-10.2-自旋锁.md)
